#pragma once
#include "mysuspend.h"

BOOL SsPsymEnumSymbolsCallback(PSYMBOL_INFO symInfo,
    ULONG symbolSize,
    PVOID userContext)
{
    std::stringstream* ss = reinterpret_cast<std::stringstream*>(userContext);
    // Print info.
    if (symbolSize > 0)
    {
        *ss << "S : " << std::setw(16) << std::hex << symInfo->Address << " : "
            << std::setw(8) << std::dec << symbolSize << " : "
            << symInfo->Name
            << std::endl;
    }
    return TRUE;
}

BOOL FsPsymEnumSymbolsCallback(PSYMBOL_INFO symInfo,
    ULONG symbolSize,
    PVOID userContext)
{
    std::fstream* fs = reinterpret_cast<std::fstream*>(userContext);
    // Print info.
    if (symbolSize > 0)
    {
        *fs << "S : " << std::setw(16) << std::hex << symInfo->Address << " : "
            << std::setw(8) << std::dec << symbolSize << " : "
            << symInfo->Name
            << std::endl;
    }
    return TRUE;
}

template <typename T>
BOOL PsymEnumSymbolsCallback(PSYMBOL_INFO symInfo,
    ULONG symbolSize,
    PVOID userContext)
{
    T* fs = reinterpret_cast<T*>(userContext);
    // Print info.
    if (symbolSize > 0)
    {
        *fs << "S : " << std::setw(16) << std::hex << symInfo->Address << " : "
            << std::setw(8) << std::dec << symbolSize << " : "
            << symInfo->Name
            << std::endl;
    }
    return TRUE;
}


class ModInfo
{
    char m_name[MAX_PATH] = { 0 };
    char m_nameEx[MAX_PATH] = { 0 };
public:
    const HANDLE sym;
    const DWORD64 base;
    const DWORD size;
    const char* name = m_name;
    const char* nameEx = m_nameEx;

    ModInfo(HANDLE proc, HMODULE mod, HANDLE new_sym, DWORD64 new_base, DWORD new_size):
        sym(new_sym), base(new_base), size(new_size)
    {
        if (!GetModuleBaseNameA(proc, mod, m_name, MAX_PATH))
        {
            std::cerr << "ERROR: cannot get name for module: "
                << std::hex << mod << std::endl;
            return;
        }

        if (!GetModuleFileNameExA(proc, mod, m_nameEx, MAX_PATH))
        {
            std::cerr << "ERROR:: cannot get extended name for module: "
                << std::hex << mod << std::endl;
            return;
        }
    }
    template<typename T> friend T& operator << (T& os, ModInfo* mod)
    {
        os << "M : " << std::setw(16) << std::hex << mod->base << " : "
            << std::setw(8) << std::dec << mod->size << " : "
            << mod->name << " : " << mod->nameEx << std::endl;

        if (0 != SymLoadModuleEx(mod->sym,
            NULL,
            mod->nameEx,
            mod->name,
            mod->base,
            mod->size,
            NULL,
            0))
        {
            IMAGEHLP_MODULE64 modInfo;
            modInfo.SizeOfStruct = sizeof(modInfo);
            if (!SymGetModuleInfo64(mod->sym,
                mod->base,
                &modInfo))
            {
                std::cerr << "ERROR: cannot get info for "
                    << mod->name
                    << ": " << GetLastError() << std::endl;
                return os;
            }
            if (modInfo.BaseOfImage != mod->base)
            {
                std::cerr << "ERROR: info mismatch for "
                    << mod->name
                    << ": " << std::endl;

                return os;
            }
        }
        else
        {
            std::cerr << "ERROR: cannot get load module "
                << mod->name
                << ": " << GetLastError() << std::endl;
            return os;
        }
        if (FALSE == SymEnumSymbolsEx(mod->sym,
                                      mod->base,
                                      "!",
                                      SsPsymEnumSymbolsCallback,
                                      &os,
                                      0))
        {
            char buf[256];
            FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                buf,
                sizeof(buf),
                NULL);
            std::cerr << "ERROR: cannot enumerates symbols in a process: "
                << buf << std::endl;
        }

        SymUnloadModule64(mod->sym, mod->base);
        return os;
    }
};

