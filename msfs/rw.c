/*
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       drivers/filesystems/msfs/rw.c
 * PURPOSE:    Mailslot filesystem
 * PROGRAMMER: Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include "msfs.h"
#include "msfssup.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS DEFAULTAPI
MsfsRead(PDEVICE_OBJECT DeviceObject,
         PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PFILE_OBJECT FileObject;
    PMSFS_FCB Fcb;
    PMSFS_CCB Ccb;
    PMSFS_MESSAGE Message;
    KIRQL oldIrql;
    ULONG Length;
    ULONG LengthRead = 0;
    PVOID Buffer;
    NTSTATUS Status;
    LARGE_INTEGER Timeout;
    PKTIMER Timer;
    PMSFS_DPC_CTX Context;
    PKDPC Dpc;

    DPRINT("MsfsRead(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

    IoStack = IoGetCurrentIrpStackLocation (Irp);
    FileObject = IoStack->FileObject;
    Fcb = (PMSFS_FCB)FileObject->FsContext;
    Ccb = (PMSFS_CCB)FileObject->FsContext2;

    DPRINT("MailslotName: %wZ\n", &Fcb->Name);

    /* reading is not permitted on client side */
    if (Fcb->ServerCcb != Ccb)
    {
        Irp->IoStatus.Status = STATUS_ACCESS_DENIED;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_ACCESS_DENIED;
    }

    Length = IoStack->Parameters.Read.Length;
    if (Irp->MdlAddress)
        Buffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
    else
        Buffer = Irp->UserBuffer;


    if (Fcb->MessageCount > 0)
    {
        /* copy current message into buffer */
        Message = CONTAINING_RECORD(Fcb->MessageListHead.Flink,
                                    MSFS_MESSAGE,
                                    MessageListEntry);

        memcpy(Buffer, &Message->Buffer, min(Message->Size,Length));
        LengthRead = Message->Size;

        KeAcquireSpinLock(&Fcb->MessageListLock, &oldIrql);
        RemoveHeadList(&Fcb->MessageListHead);
        KeReleaseSpinLock(&Fcb->MessageListLock, oldIrql);

        ExFreePoolWithTag(Message, 'rFsM');
        Fcb->MessageCount--;
        Irp->IoStatus.Status = Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = LengthRead;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        Context = (PMSFS_DPC_CTX)Irp->Tail.Overlay.DriverContext[0];
        if (Context)
        {
	    if (Context->Timer)
                KeCancelTimer(&Context->Timer);
            ExFreePool(Context);
        }
    }
    else
    {
        Timeout = Fcb->TimeOut;
        if (Timeout.HighPart == 0 && Timeout.LowPart == 0)
        {
            Irp->IoStatus.Status = Status = STATUS_IO_TIMEOUT;
            Irp->IoStatus.Information = LengthRead;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }
        else
        {

            Context = ExAllocatePoolWithTag( NonPagedPool, sizeof(MSFS_DPC_CTX), 'tFsM');
            Irp->Tail.Overlay.DriverContext[0] = Context;

            IoCsqInsertIrp(&Fcb->CancelSafeQueue, Irp, &Context->Csq_context);
            Timer = &Context->Timer;
            Dpc = &Context->Dpc;
            Context->Csq = &Fcb->CancelSafeQueue;

            if ( Timeout.QuadPart != -1 ) //no timer for INFINITY_WAIT
            {
                KeInitializeTimer( Timer );
                KeInitializeDpc( Dpc, (PKDEFERRED_ROUTINE)MsfsTimeout, (PVOID)Context );
                KeSetTimer( Timer, Timeout, Dpc );
            }

            Fcb->WaitCount++;
            Irp->IoStatus.Status = Status = STATUS_PENDING;
            Irp->IoStatus.Information = LengthRead;
            IoMarkIrpPending( Irp );
        }
    }

    return Status;
}


NTSTATUS DEFAULTAPI
MsfsWrite(PDEVICE_OBJECT DeviceObject,
          PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PFILE_OBJECT FileObject;
    PMSFS_FCB Fcb;
    PMSFS_CCB Ccb;
    PMSFS_MESSAGE Message;
    KIRQL oldIrql;
    ULONG Length;
    PVOID Buffer;
    PIRP IrpW;

    DPRINT("MsfsWrite(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

    IoStack = IoGetCurrentIrpStackLocation (Irp);
    FileObject = IoStack->FileObject;
    Fcb = (PMSFS_FCB)FileObject->FsContext;
    Ccb = (PMSFS_CCB)FileObject->FsContext2;

    DPRINT("MailslotName: %wZ\n", &Fcb->Name);

    /* writing is not permitted on server side */
    if (Fcb->ServerCcb == Ccb)
    {
        Irp->IoStatus.Status = STATUS_ACCESS_DENIED;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_ACCESS_DENIED;
    }

    Length = IoStack->Parameters.Write.Length;
    if (Irp->MdlAddress)
        Buffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
    else
        Buffer = Irp->UserBuffer;

    DPRINT("Length: %lu Message: %s\n", Length, (PUCHAR)Buffer);

    /* Allocate new message */
    Message = ExAllocatePoolWithTag(NonPagedPool,
                                    sizeof(MSFS_MESSAGE) + Length,
                                    'rFsM');
    if (Message == NULL)
    {
        Irp->IoStatus.Status = STATUS_NO_MEMORY;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_NO_MEMORY;
    }

    Message->Size = Length;
    memcpy(&Message->Buffer, Buffer, Length);

    KeAcquireSpinLock(&Fcb->MessageListLock, &oldIrql);
    InsertTailList(&Fcb->MessageListHead, &Message->MessageListEntry);
    KeReleaseSpinLock(&Fcb->MessageListLock, oldIrql);

    Fcb->MessageCount++;
    if (Fcb->WaitCount > 0)
    {
        IrpW = IoCsqRemoveNextIrp(&Fcb->CancelSafeQueue, NULL);
        /* FIXME: It is necessary to reset the timers. */
        MsfsRead(DeviceObject, IrpW);
        Fcb->WaitCount--;
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = Length;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

/* EOF */
