/*
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       drivers/filesystems/msfs/msfssup.c
 * PURPOSE:    Mailslot filesystem
 * PROGRAMMER: nikita pechenkin (n.pechenkin@mail.ru)
 */

/* INCLUDES ******************************************************************/
#include "msfs.h"
#include "msfssup.h"

#include <ndk/iotypes.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

VOID MsfsInsertIrp (PIO_CSQ Csq, PIRP Irp)
{
    PMSFS_FCB   fsb;

    fsb = CONTAINING_RECORD(Csq, MSFS_FCB, CancelSafeQueue);
    InsertTailList(&fsb->PendingIrpQueue,&Irp->Tail.Overlay.ListEntry);
}

VOID MsfsRemoveIrp(PIO_CSQ Csq, PIRP Irp)
{
    UNREFERENCED_PARAMETER(Csq);

    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
}

PIRP MsfsPeekNextIrp(PIO_CSQ Csq, PIRP Irp, PVOID PeekContext)
{
    PMSFS_FCB      fsb;
    PIRP                    nextIrp = NULL;
    PLIST_ENTRY             nextEntry;
    PLIST_ENTRY             listHead;

    UNREFERENCED_PARAMETER(PeekContext);

    fsb = CONTAINING_RECORD(Csq, MSFS_FCB, CancelSafeQueue);

    listHead = &fsb->PendingIrpQueue;

    if (Irp == NULL)
    {
        nextEntry = listHead->Flink;
    }
    else
    {
        nextEntry = Irp->Tail.Overlay.ListEntry.Flink;
    }

    if (nextEntry != listHead)
    {
        nextIrp = CONTAINING_RECORD(nextEntry, IRP, Tail.Overlay.ListEntry);
    }

    return nextIrp;
}

VOID MsfsAcquireLock(PIO_CSQ Csq, PKIRQL Irql)
{
    PMSFS_FCB   fsb;

    fsb = CONTAINING_RECORD(Csq,MSFS_FCB, CancelSafeQueue);
    KeAcquireSpinLock(&fsb->QueueLock, Irql);
}


VOID MsfsReleaseLock(PIO_CSQ Csq, KIRQL Irql)
{
    PMSFS_FCB   fsb;

    fsb = CONTAINING_RECORD(Csq,MSFS_FCB, CancelSafeQueue);
    KeReleaseSpinLock(&fsb->QueueLock, Irql);
}

VOID MsfsCompleteCanceledIrp(PIO_CSQ pCsq, PIRP Irp)
{

    UNREFERENCED_PARAMETER(pCsq);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

VOID MsfsTimeout(struct _KDPC *Dpc,
            PVOID DeferredContext,
            PVOID SystemArgument1,
            PVOID SystemArgument2)
{
   PMSFS_DPC_CTX Context;
   PIRP wIrp;
   Context = (PMSFS_DPC_CTX)DeferredContext;
   wIrp = IoCsqRemoveIrp(Context->Csq,&Context->Csq_context);
   if (wIrp)
   {
        wIrp->IoStatus.Status = STATUS_IO_TIMEOUT;
        IoCompleteRequest( wIrp, IO_NO_INCREMENT );
   }
   ExFreePoolWithTag(Context,'tFsM');
}

/* EOF */
