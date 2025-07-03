#pragma warning (disable: 4100 4047 4024)

#include "nukin.h"
#include "messages.h"
#include "events.h"
#include "data.h"
#include "communication.h"

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	UNREFERENCED_PARAMETER(pRegistryPath);
	pDriverObject->DriverUnload = UnloadDriver;
	Debugmessage("Driver loaded successfully.\n");

	PsSetLoadImageNotifyRoutine(ImageLoadCallback);

	RtlInitUnicodeString(&dev, L"\\Device\\Nukin");
	RtlInitUnicodeString(&dos, L"\\DosDevice\\Nukin");

	IoCreateDevice(pDriverObject, 0, &dev, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &pDeviceObject);
	IoCreateSymbolicLink(&dos, &dev);

	pDriverObject->MajorFunction[IRP_MJ_CREATE] = CreateCall;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = CloseCall;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IoControl;

	pDeviceObject->Flags |= DO_BUFFERED_IO;
	pDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

	return STATUS_SUCCESS;
}
NTSTATUS UnloadDriver(PDRIVER_OBJECT pDriveObject)
{
	UNREFERENCED_PARAMETER(pDriveObject);
	Debugmessage("Driver unloaded successfully.");

	PsRemoveLoadImageNotifyRoutine(ImageLoadCallback);

	IoDeleteSymbolicLink(&dos);
	IoDeleteDevice(pDeviceObject);

	return STATUS_SUCCESS;
}

