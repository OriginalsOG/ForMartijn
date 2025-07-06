#include "Driver.hpp"
#include <iostream>
#include "offsets.hpp"

int main() {
    kernelinterface Driver = kernelinterface("\\\\.\\Test");

    uint32_t procid = Driver.get_process_id("SquadGame.exe");
    uint64_t Address = Driver.GetClientAddress((HANDLE)procid);

    std::cout << "Process ID: " << procid << std::endl;
    std::cout << "Client Address: 0x" << std::hex << Address << std::dec << std::endl;

    // Example of reading memory using the new interface
    uint64_t UWorld;
	uint64_t PersistentLevel;
    Driver.ReadVirtualMemory<uint64_t>(procid, Address + GlobalOffsets::OFFSET_GWORLD, UWorld);
	Driver.ReadVirtualMemory<uint64_t>(procid, UWorld + offsets::UWorld::PersistentLevel, PersistentLevel);

	std::cout << "UWorld Address: 0x" << std::hex << UWorld << std::dec << std::endl;
	std::cout << "PersistentLevel Address: 0x" << std::hex << PersistentLevel << std::dec << std::endl;
    



    std::cin.get();
    return 0;
}
