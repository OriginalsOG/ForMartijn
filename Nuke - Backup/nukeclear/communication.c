#pragma warning (disable : 4022 4013)
#include "communication.h"
#include "messages.h"
#include "memory.h"
#include "data.h"

NTSTATUS CreateCall(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	Debugmessage("CreateCall was called, connection enstablished!\n");

	return STATUS_SUCCESS;
}
NTSTATUS CloseCall(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	Debugmessage("CloseCall was called, connection closed!\n");

	return STATUS_SUCCESS;
}
NTSTATUS IoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	ULONG ByteIo = 0;

	PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);

	ULONG ControlCode = Stack->Parameters.DeviceIoControl.IoControlCode;

	if (ControlCode == IO_GET_CLIENTADDRESS)
	{
		PULONG Output = (PULONG)Irp->AssociatedIrp.SystemBuffer;
		*Output = SquadGameBaseAddress;

		Debugmessage("ClientAddress requested\n");
		Status = STATUS_SUCCESS;
		ByteIo = sizeof(*Output);
	}
	else if (ControlCode == IO_READ_REQUEST)
	{
		PKERNEL_READ_REQUEST ReadInput = (PKERNEL_READ_REQUEST)Irp->AssociatedIrp.SystemBuffer;
		PEPROCESS Process;
		
		if (NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)ReadInput->ProcessId, &Process)))
		{
			KernelReadVirtualMemory(Process, (PVOID)ReadInput->Address, ReadInput->pBuff, ReadInput->Size);
			Status = STATUS_SUCCESS;
			ByteIo = sizeof(KERNEL_READ_REQUEST);
		}
	}
	else if (ControlCode == IO_WRITE_REQUEST)
	{
		PKERNEL_WRITE_REQUEST WriteInput = (PKERNEL_WRITE_REQUEST)Irp->AssociatedIrp.SystemBuffer;
		PEPROCESS Process;

		if (NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)WriteInput->ProcessId, &Process)))
		{
			KernelReadVirtualMemory(Process, (PVOID)WriteInput->pBuff, WriteInput->Address, WriteInput->Size);
			Status = STATUS_SUCCESS;
			ByteIo = sizeof(KERNEL_READ_REQUEST);
		}
	}
	else
	{
		ByteIo = 0;
	}

	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = ByteIo;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return Status;
}