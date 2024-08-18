#include "../include/memDump.h"

#include <iostream>
#include <string>
#include <vector>
#include <ostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <span>

using namespace MemoryUtils;

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

/**
 * @brief 
 * 
 * @param ptr 
 * @param size 
 * @param type_name 
 * @param os 
 */
void Memory::printMemory(const void* ptr, std::size_t size, std::string_view type_name, std::ostream& os)
{
	using byte_t = unsigned char;
	constexpr std::size_t bytesPerLine = 16;

	std::span<const byte_t> data(static_cast<const byte_t*>(ptr), size);

	os << "---------------------------------------------------------------------------------\n"
		<< "Type: " << (type_name.empty() ? "Unknown" : type_name)
		<< " | Size: " << size << " bytes | Start Address: "
		<< std::hex << std::uppercase << static_cast<const void*>(ptr) << "\n\n";

	os << "Address          ";
	for (std::size_t i = 0; i < bytesPerLine; ++i)
	{
		if (i % 4 == 0) os << ' ';
		os << std::setw(2) << i;
	}
	os << "	  | ASCII\n";
	os << "---------------------------------------------------------------------------------\n";

	std::size_t offset = 0;
	while (offset < size)
	{
		std::size_t lineLength = MIN(bytesPerLine, size - offset);

		os << "0x" << std::setfill('0') << std::setw(16) << offset << ": ";

		std::string asciiView;
		for (std::size_t i = 0; i < bytesPerLine; ++i)
		{
			if (i < lineLength)
			{
				if (i % 4 == 0) os << ' ';
				os << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[offset + i]);
				asciiView += (data[offset + i] >= 32 && data[offset + i] <= 126) ? static_cast<char>(data[offset + i]) : '.';
			}
			else
			{
				os << "   ";
				if (i % 4 == 0) os << ' ';
			}
		}

		os << "  | " << asciiView << '\n';

		offset += bytesPerLine;
	}

	os << "---------------------------------------------------------------------------------" << std::endl;
}

/**
 * @brief 
 * 
 * @param process 
 * @param mbi 
 * @param os 
 */
void Memory::dumpMemoryRegion(HANDLE process, const MEMORY_BASIC_INFORMATION& mbi, std::ostream& os)
{
    std::vector<unsigned char> buffer(mbi.RegionSize);

    SIZE_T bytesRead;
    if (ReadProcessMemory(process, mbi.BaseAddress, buffer.data(), mbi.RegionSize, &bytesRead))
    {
        os << "Memory Region Dump:\n";
        Memory::dumpPtr(buffer.data(), os);
        os << "\nRaw Memory Data:\n";
        Memory::printMemory(buffer.data(), static_cast<std::size_t>(bytesRead), "Memory Region", os);
    }
    else
    {
        os << "Failed to read memory at " << mbi.BaseAddress << " with size " << mbi.RegionSize << "\n";
    }
}

/**
 * @brief 
 * 
 * @param pid 
 * @param os 
 */
void Memory::scanAndDumpMemory(DWORD pid, std::ostream& os)
{
    HANDLE process = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!process)
    {
        os << "Failed to open process with PID " << pid << "\n";
        Memory::dumpVar(pid, os);
        return;
    }

    MEMORY_BASIC_INFORMATION mbi;
    std::uintptr_t address = 0;

    while (VirtualQueryEx(process, reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi)))
    {
        if (mbi.State == MEM_COMMIT && (mbi.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_READONLY)))
        {
            os << "Dumping memory at " << mbi.BaseAddress << ", size: " << mbi.RegionSize << "\n";
            Memory::dumpVar(mbi, os);
            Memory::dumpMemoryRegion(process, mbi, os);
        }
        address += mbi.RegionSize;
    }

    CloseHandle(process);
}

/**
 * @brief 
 * 
 * @param fileName 
 * @param words 
 */
void Memory::searchInDump(const std::string& fileName, const std::vector<std::string>& words)
{
	std::ifstream file(fileName);
	if (!file.is_open())
	{
		std::cerr << "Failed to open dump file: " << fileName << "\n";
		return;
	}

	std::string line;
	size_t lineNumber = 0;

	while (std::getline(file, line))
	{
		lineNumber++;
		for (const auto& word : words)
		{
			if (line.find(word) != std::string::npos)
			{
				std::cout << "Found '" << word << "' on line " << lineNumber << ": " << line << "\n";
			}
		}
	}

	file.close();
}

/**
 * @brief 
 * 
 * @param dumpStream 
 * @param words 
 * @param searchResults 
 */
void Memory::searchInDumpStream(std::stringstream& dumpStream, const std::vector<std::string>& words, std::vector<std::string>& searchResults)
{
	std::string line;
	size_t lineNumber = 0;

	searchResults.clear();

	while (std::getline(dumpStream, line))
	{
		lineNumber++;
		for (const auto& word : words)
		{
			if (line.find(word) != std::string::npos)
			{
				std::ostringstream resultStream;
				resultStream << "Found '" << word << "' on line " << lineNumber << ": " << line;
				searchResults.push_back(resultStream.str());
			}
		}
	}
}