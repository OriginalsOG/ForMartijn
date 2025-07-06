#pragma once

#include <windows.h>
#include <cstdint>
#include <TlHelp32.h>
#include <cstring>
#include <type_traits>

#define MAX_BUFFER_SIZE 256
#define IO_GET_CLIENTADDRESS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x777, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IO_READ_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x778, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IO_WRITE_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x779, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _IO_READ_REQUEST {
    ULONG ProcessId;
    UINT64 Address;
    UCHAR Buffer[MAX_BUFFER_SIZE];  // Using the defined constant
    ULONG Size;
} KERNEL_READ_REQUEST, *PKERNEL_READ_REQUEST;

typedef struct _IO_WRITE_REQUEST {
    ULONG ProcessId;
    UINT64 Address;
    UCHAR Buffer[MAX_BUFFER_SIZE];  // Using the defined constant
    ULONG Size;
} KERNEL_WRITE_REQUEST, *PKERNEL_WRITE_REQUEST;

class kernelinterface {
public:
    HANDLE hDriver;

    kernelinterface(LPCSTR RegistryPath) {
        hDriver = CreateFileA(RegistryPath, GENERIC_READ | GENERIC_WRITE, 
            FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
    }

    uint64_t GetClientAddress(HANDLE processID) {
        if (hDriver == INVALID_HANDLE_VALUE) return 0;
        uint64_t baseAddress = 0;
        DWORD bytesReturned = 0;
        if (DeviceIoControl(hDriver, IO_GET_CLIENTADDRESS, &processID, sizeof(processID),
            &baseAddress, sizeof(baseAddress), &bytesReturned, NULL)) {
            return baseAddress;
        }
        return 0;
    }

    uint32_t get_process_id(const char* proc) {
        auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        auto pe = PROCESSENTRY32{ sizeof(PROCESSENTRY32) };

        if (Process32First(snapshot, &pe)) {
            do {
                if (!std::strcmp(proc, pe.szExeFile)) {
                    CloseHandle(snapshot);
                    return pe.th32ProcessID;
                }
            } while (Process32Next(snapshot, &pe));
        }
        CloseHandle(snapshot);
        return 0;
    }

    template<typename type>
    bool ReadVirtualMemory(ULONG ProcessId, uint64_t ReadAddress, type& Buffer) {
        // Static assertions to validate type requirements at compile time
        static_assert(sizeof(type) <= MAX_BUFFER_SIZE, 
            "Type size exceeds maximum buffer size");
        static_assert(std::is_trivially_copyable<type>::value,
            "Type must be trivially copyable");
        
        if (hDriver == INVALID_HANDLE_VALUE) return false;

        KERNEL_READ_REQUEST ReadRequest = {0};
        DWORD bytesReturned = 0;

        ReadRequest.ProcessId = ProcessId;
        ReadRequest.Address = ReadAddress;
        ReadRequest.Size = sizeof(type);

        if (DeviceIoControl(hDriver, IO_READ_REQUEST, &ReadRequest, sizeof(ReadRequest),
            &ReadRequest, sizeof(ReadRequest), &bytesReturned, NULL)) {
            memcpy(&Buffer, ReadRequest.Buffer, sizeof(type));
            return true;
        }
        return false;
    }

    template<typename type>
    bool WriteVirtualMemory(ULONG ProcessId, uint64_t WriteAddress, const type& WriteValue) {
        // Static assertions to validate type requirements at compile time
        static_assert(sizeof(type) <= MAX_BUFFER_SIZE,
            "Type size exceeds maximum buffer size");
        static_assert(std::is_trivially_copyable<type>::value,
            "Type must be trivially copyable");
        
        if (hDriver == INVALID_HANDLE_VALUE) return false;

        KERNEL_WRITE_REQUEST WriteRequest = {0};
        DWORD bytesReturned = 0;

        WriteRequest.ProcessId = ProcessId;
        WriteRequest.Address = WriteAddress;
        WriteRequest.Size = sizeof(type);
        memcpy(WriteRequest.Buffer, &WriteValue, sizeof(type));

        return DeviceIoControl(hDriver, IO_WRITE_REQUEST, &WriteRequest, sizeof(WriteRequest),
            NULL, 0, &bytesReturned, NULL) != 0;
    }
};