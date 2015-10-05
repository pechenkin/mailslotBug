/*
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       drivers/filesystems/msfs/msfssup.h
 * PURPOSE:    Mailslot filesystem
 * PROGRAMMER: nikita pechenkin (n.pechenkin@mail.ru)
 */

#ifndef __DRIVERS_FS_MS_MSFSSUP_H
#define __DRIVERS_FS_MS_MSFSSUP_H

VOID
MsfsInsertIrp (PIO_CSQ Csq, PIRP Irp);

VOID
MsfsRemoveIrp(PIO_CSQ Csq, PIRP Irp);

PIRP
MsfsPeekNextIrp(PIO_CSQ Csq, PIRP Irp, PVOID PeekContext);

VOID
MsfsAcquireLock(PIO_CSQ Csq, PKIRQL Irql);

VOID
MsfsReleaseLock(PIO_CSQ Csq, KIRQL Irql);

VOID
MsfsCompleteCanceledIrp(PIO_CSQ pCsq, PIRP Irp);

VOID
MsfsTimeout(struct _KDPC *Dpc,
            PVOID DeferredContext,
            PVOID SystemArgument1,
            PVOID SystemArgument2);

#endif /* __DRIVERS_FS_MS_MSFSSUP_H */
