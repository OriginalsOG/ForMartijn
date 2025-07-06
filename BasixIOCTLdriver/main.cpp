#include <ntifs.h>
#include <ntddk.h>

extern "C" {
	NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
	PVOID NTAPI PsGetProcessSectionBaseAddress(PEPROCESS Process);
	NTKERNELAPI PPEB NTAPI PsGetProcessPeb(PEPROCESS Process);
	NTKERNELAPI NTSTATUS NTAPI MmCopyVirtualMemory(PEPROCESS SourceProcess,PVOID SourceAddress,PEPROCESS TargetProcess,PVOID TargetAddress,SIZE_T BufferSize,KPROCESSOR_MODE PreviousMode,PSIZE_T ReturnSize);
}

NTSTATUS KernelReadVirtualMemory(PEPROCESS Process, PVOID SourceAddress, PVOID TargetAddress, SIZE_T Size)
{
	SIZE_T Bytes;

	return MmCopyVirtualMemory(Process, SourceAddress, PsGetCurrentProcess(), TargetAddress, Size, KernelMode, &Bytes);
}

NTSTATUS KernelWriteVirtualMemory(PEPROCESS Process, PVOID SourceAddress, PVOID TargetAddress, SIZE_T Size)
{
	SIZE_T Bytes;

	return MmCopyVirtualMemory(PsGetCurrentProcess(), SourceAddress, Process, TargetAddress, Size, KernelMode, &Bytes);
}



#define IO_GET_CLIENTADDRESS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x777, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IO_READ_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x778, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IO_WRITE_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x779, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define DebugMessage(x, ...) DbgPrintEx(0, 0, x, __VA_ARGS__)

#define MAX_BUFFER_SIZE 256

PDEVICE_OBJECT pDeviceObject;
UNICODE_STRING dev, dos;

typedef struct _IO_READ_REQUEST {

	ULONG ProcessId;
	UINT64 Address;
	UCHAR Buffer[256];  // Fixed size buffer instead of PVOID
	ULONG Size;         // Changed to ULONG for better alignment

} KERNEL_READ_REQUEST, * PKERNEL_READ_REQUEST;

typedef struct _IO_WRITE_REQUEST {

	ULONG ProcessId;
	UINT64 Address;
	UCHAR Buffer[256];  // Fixed size buffer instead of PVOID
	ULONG Size;         // Changed to ULONG for better alignment

} KERNEL_WRITE_REQUEST, * PKERNEL_WRITE_REQUEST;

NTSTATUS CreateCall(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}
NTSTATUS CloseCall(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS IoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	NTSTATUS Status = STATUS_SUCCESS;
	PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
	ULONG controlCode = Stack->Parameters.DeviceIoControl.IoControlCode;
	ULONG inputBufferLength = Stack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG outputBufferLength = Stack->Parameters.DeviceIoControl.OutputBufferLength;

	switch (controlCode) {
	case IO_GET_CLIENTADDRESS: {
		if (inputBufferLength < sizeof(HANDLE) || outputBufferLength < sizeof(UINT64)) {
			Status = STATUS_BUFFER_TOO_SMALL;
			break;
		}
		DebugMessage("Processbase Requested\n");
		HANDLE processId = *(HANDLE*)Irp->AssociatedIrp.SystemBuffer;
		PEPROCESS Process;

		if (!NT_SUCCESS(PsLookupProcessByProcessId(processId, &Process))) {
			DebugMessage("Invalid Process ID\n");
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		UINT64 processBase = (UINT64)PsGetProcessSectionBaseAddress(Process);
		if (processBase == NULL) {
			DebugMessage("Invalid ProcessBase\n");
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		*(UINT64*)Irp->AssociatedIrp.SystemBuffer = processBase;
		Irp->IoStatus.Information = sizeof(UINT64);
		Status = STATUS_SUCCESS;
		break;
	}
	case IO_READ_REQUEST: {
		if (inputBufferLength < sizeof(KERNEL_READ_REQUEST) || 
			outputBufferLength < sizeof(KERNEL_READ_REQUEST)) {
			Status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		PKERNEL_READ_REQUEST readRequest = (PKERNEL_READ_REQUEST)Irp->AssociatedIrp.SystemBuffer;
		
		// Validate buffer size
		if (readRequest->Size > MAX_BUFFER_SIZE || readRequest->Size == 0) {
			DebugMessage("Invalid buffer size in read request\n");
			Status = STATUS_INVALID_PARAMETER;
			break;
		}

		PEPROCESS Process;
		if (!NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)readRequest->ProcessId, &Process))) {
			DebugMessage("Invalid Process ID\n");
			Status = STATUS_UNSUCCESSFUL;
			break;
		}
		DebugMessage("Read Request for Process ID: %d, Address: %p, Size: %d\n",
			readRequest->ProcessId, (PVOID)readRequest->Address, readRequest->Size);

		Status = KernelReadVirtualMemory(Process, (PVOID)readRequest->Address, readRequest->Buffer, readRequest->Size);
		if (!NT_SUCCESS(Status)) {
			DebugMessage("Read Failed\n");
			break;
		}
		Irp->IoStatus.Information = sizeof(KERNEL_READ_REQUEST);
		break;
	}
	case IO_WRITE_REQUEST: {
		if (inputBufferLength < sizeof(KERNEL_WRITE_REQUEST)) {
			Status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		PKERNEL_WRITE_REQUEST writeRequest = (PKERNEL_WRITE_REQUEST)Irp->AssociatedIrp.SystemBuffer;
		
		// Validate buffer size
		if (writeRequest->Size > MAX_BUFFER_SIZE || writeRequest->Size == 0) {
			DebugMessage("Invalid buffer size in write request\n");
			Status = STATUS_INVALID_PARAMETER;
			break;
		}

		PEPROCESS Process;
		if (!NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)writeRequest->ProcessId, &Process))) {
			DebugMessage("Invalid Process ID\n");
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		Status = KernelWriteVirtualMemory(Process, writeRequest->Buffer, (PVOID)writeRequest->Address, writeRequest->Size);
		if (!NT_SUCCESS(Status)) {
			DebugMessage("Write Failed\n");
			break;
		}
		Irp->IoStatus.Information = sizeof(KERNEL_WRITE_REQUEST);
		break;
	}
	default:
		Status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}


VOID DriverUnload(PDRIVER_OBJECT DriverObject) {
	UNREFERENCED_PARAMETER(DriverObject);

	DebugMessage("Driver Unloaded\n");

	NTSTATUS status = IoDeleteSymbolicLink(&dos);
	if (!NT_SUCCESS(status)) {
		DebugMessage("Failed to delete symbolic link: 0x%X\n", status);
	}

	IoDeleteDevice(pDeviceObject);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);
	DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateCall;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = CloseCall;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IoControl;
	DriverObject->DriverUnload = DriverUnload;  

	RtlInitUnicodeString(&dev, L"\\Device\\Test");
	RtlInitUnicodeString(&dos, L"\\DosDevices\\Test");

	NTSTATUS status = IoCreateDevice(DriverObject, 0, &dev, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &pDeviceObject);
	if (!NT_SUCCESS(status)) return status;

	status = IoCreateSymbolicLink(&dos, &dev);
	if (!NT_SUCCESS(status)) {
		IoDeleteDevice(pDeviceObject);
		return status;
	}

	pDeviceObject->Flags |= DO_BUFFERED_IO;
	pDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

	DebugMessage("Driver Loaded\n");
	return STATUS_SUCCESS;
}


