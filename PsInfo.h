#pragma once
#include <tlhelp32.h>
#include "ModInfo.h"
#include "mysuspend.h"

class PsInfo
{
    char m_name[MAX_PATH] = {0};
public:
    const HANDLE sym;
    const DWORD pid;
    const char *name = m_name;
    std::vector <ModInfo*> psMods;

    PsInfo(DWORD new_pid, HANDLE new_sym, bool enumMods) : sym(new_sym), pid(new_pid)
    {
        HMODULE mods[PSLIST_SIZE] = { 0 };
        DWORD needed, procNum;
        HANDLE proc = 0;
        // Get handle to process
        proc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                           FALSE,
                           pid);
        DWORD readLen = MAX_PATH;
        QueryFullProcessImageNameA(proc, 0, m_name, &readLen);
        // Get a list of all the modules in this process.
        if (enumMods)
        {
            if (EnumProcessModules(proc, mods, sizeof(mods), &needed))
            {
                size_t mod_num = (needed / sizeof(HMODULE));
//                psMods.reserve(mod_num);

                for (int i = 0; i < mod_num; ++i)
                {
                    MODULEINFO info = { 0 };
                    // Get more module information.
                    if (!GetModuleInformation(proc,
                        mods[i],
                        &info,
                        sizeof(MODULEINFO)))
                    {
                        std::cerr << "ERROR: cannot get module info for process: "
                            << std::hex << mods[i] << std::endl;
                        return;
                    }

                    ModInfo* mod = new ModInfo(proc,
                                               mods[i],
                                               sym,
                                               (DWORD64)info.lpBaseOfDll,
                                               info.SizeOfImage);
                    psMods.push_back(mod);
                }
            }
        }
        CloseHandle(proc);
    }
    template<typename T> friend T& operator << (T& os, PsInfo* ps)
    {
        // print pid and name; leave cr3 blank because it's unknown
        os << "P : " << ps->pid << " : " << ps->name << std::endl;;

        // Print modules
        for (auto i = ps->psMods.begin(); i != ps->psMods.end(); ++i)
        {
            os << *i;
        }
        return os;
    }
};

