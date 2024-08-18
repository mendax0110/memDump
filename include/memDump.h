#pragma once

#include <iostream>
#include <string_view>
#include <type_traits>
#include <memory>
#include <vector>
#include <string>
#include <Windows.h>

namespace MemoryUtils
{
    class Memory
    {
    public:
        template <typename T>
        static constexpr std::string_view type_name()
        {
            #if defined(_MSC_VER)
            constexpr auto function = std::string_view{ __FUNCSIG__ };
            constexpr auto prefix = "type_name<";
            constexpr auto suffix = ">(void)";
            const auto start = function.find(prefix) + std::strlen(prefix);
            const auto end = function.rfind(suffix);
            return function.substr(start, end - start);
            #else
            return "Unsupported compiler";
            #endif
        }

        static void dumpMemoryRegion(HANDLE process, const MEMORY_BASIC_INFORMATION& mbi, std::ostream& os);
        static void scanAndDumpMemory(DWORD pid, std::ostream& os);
        static void printMemory(const void* ptr, std::size_t size, std::string_view type_name, std::ostream& os);
        void searchInDump(const std::string& fileName, const std::vector<std::string>& words);
        void searchInDumpStream(std::stringstream& dumpStream, const std::vector<std::string>& words, std::vector<std::string>& searchResults);

        template <typename T>
        static void printMemory(T* ptr, std::size_t size, std::ostream& os)
        {
            printMemory(static_cast<const void*>(ptr), size, type_name<std::remove_pointer_t<T>>(), os);
        }

        template <typename T>
        static void printMemory(T&& var, std::ostream& os)
        {
            printMemory(std::addressof(var), sizeof(var), type_name<std::remove_reference_t<T>>(), os);
        }

        template <typename T>
        static void dumpVar(const T& var, std::ostream& os)
        {
            printMemory(std::addressof(var), sizeof(var), type_name<T>(), os);
        }

        template <typename T>
        static void dumpPtr(const T* ptr, std::ostream& os)
        {
            if (ptr)
            {
                printMemory(ptr, sizeof(*ptr), type_name<std::remove_pointer_t<T>>(), os);
            }
            else
            {
                os << "Null ptr of type " << type_name<std::remove_pointer_t<T>>() << "\n";
            }
        }
    };
}
