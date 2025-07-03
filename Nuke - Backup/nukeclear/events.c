#pragma warning (disable : 4047 )

#include "events.h"
#include "messages.h"
#include "data.h"

PLOAD_IMAGE_NOTIFY_ROUTINE ImageLoadCallback(PUNICODE_STRING FullImageName, HANDLE ProcessId, PIMAGE_INFO ImageInfo)
{
	if (wcsstr(FullImageName->Buffer, L"\\SquadGame\\Binaries\\Win64\\SquadGame.exe"))
	{
		// Log the image load event
		Debugmessage("Image loaded: %wZ, Process ID: %d\n", FullImageName, ProcessId);
		SquadGameBaseAddress = ImageInfo->ImageBase;
	}
	return STATUS_SUCCESS;
}