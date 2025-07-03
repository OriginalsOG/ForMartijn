
#include <iostream>
#include "kernelinterface.hpp"
int main()
{
	kernelinterface Driver = kernelinterface("\\\\.\\Nukin");

	ULONG Address = Driver.GetClientAddress();

	std::cout << "Client Address: " << std::hex << Address << std::dec << std::endl;

	while (true)
	{
		
	}
}
	

