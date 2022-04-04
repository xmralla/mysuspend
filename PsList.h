#pragma once
#include "mysuspend.h"
#include "PsInfo.h"
#include "DrvInfo.h"

class PsList
{
	DWORD pid;
	HANDLE sym;
    HANDLE token;
    DWORD unnamed;

    void Insert(DWORD new_pid, bool enumMods)
    {
        PsInfo* ps = new PsInfo(new_pid, sym, enumMods);
        pidMap.insert(std::pair<DWORD, PsInfo*>(new_pid, ps));
        if (ps->name[0] == 0)
        {
            ++unnamed;
        }
    }

    void insert(LPVOID new_addr)
    {
        DrvInfo* drvInfo = new DrvInfo(new_addr, sym);
        drvMap.insert(std::pair<LPVOID, DrvInfo*>(new_addr, drvInfo));
    }

    bool SetPrivilege(LPCTSTR privilege, bool enable)
    {
        TOKEN_PRIVILEGES tp = { 0 };

        // Initialize everything to zero
        LUID luid;
        DWORD tpSize = sizeof(TOKEN_PRIVILEGES);
        if (!LookupPrivilegeValue(NULL, privilege, &luid))
        {
            std::cerr << "ERROR: LookupPrivilegeValue(): "
                << GetLastError() << std::endl;
            return false;
        }
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;

        if (!AdjustTokenPrivileges(token, FALSE, &tp, tpSize, NULL, NULL))
        {
            std::cerr << "ERROR: AdjustTokenPrivileges(): "
                << GetLastError() << std::endl;
            return false;
        }
        return true;
    }
public:
    Pid2PsInfoMap      pidMap; // maps pids to more info
    Addr2DriverInfoMap drvMap;

    PsList(): unnamed(0)
    {
        pid = GetCurrentProcessId();
        DWORD options = SymGetOptions();

        options |= SYMOPT_DEFERRED_LOADS |
            SYMOPT_FAIL_CRITICAL_ERRORS |
            SYMOPT_INCLUDE_32BIT_MODULES;

        SymSetOptions(options);
        sym = (HANDLE)(pid + 100000ULL);
        if (SymInitialize(sym, NULL, FALSE) != TRUE)
        {
            std::cerr << "ERROR: Cannot SymInitialize" << std::endl;
            sym = NULL;
        }

        if (!OpenThreadToken(GetCurrentThread(),
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
            FALSE,
            &token))
        {
            if (GetLastError() == ERROR_NO_TOKEN)
            {
                if (!ImpersonateSelf(SecurityDelegation))
                {
                    std::cerr << "ERROR: ImpersonateSelf(): "
                        << GetLastError() << std::endl;
                }
                if (!OpenThreadToken(GetCurrentThread(),
                    TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                    FALSE,
                    &token))
                {
                    std::cerr << "ERROR: OpenThreadToken2(): "
                        << GetLastError() << std::endl;
                }
            }
            else
            {
                std::cerr << "ERROR: OpenThreadToken1(): "
                    << GetLastError() << std::endl;
            }
        }

        // enable SeDebugPrivilege
        if (!SetPrivilege(SE_DEBUG_NAME, true))
        {
            std::cerr << "ERROR: SeDebugPrivilege() failed." << std::endl;
        }
        if (!SetPrivilege(SE_SYSTEM_PROFILE_NAME, true))
        {
            std::cerr << "ERROR: SeRestorePrivilege() failed." << std::endl;
        }
    }
    ~PsList()
    {

        CloseHandle(token);
        if (sym)
        {
            SymCleanup(sym);
        }
        sym = NULL;
    }
    bool lsTree(bool enumMods)
    {
        pidMap.clear();
        unnamed = 0;
        // Get the list of process identifiers.
        DWORD procs[PSLIST_SIZE], needed, procNum;
        unsigned int i;

        if (!EnumProcesses(procs, sizeof(procs), &needed))
        {
            std::cerr << "ERROR: Cannot EnumProcesses" << std::endl;
            return false;
        }
        // Calculate how many process identifiers were returned.
        procNum = needed / sizeof(DWORD);
        // Add each process and its modules.
        for (i = 0; i < procNum; ++i)
        {
            Insert(procs[i], enumMods);
        }
        if (enumMods)
        {
            LPVOID imageBase[PSLIST_SIZE];
            DWORD driverNum;
            if (!EnumDeviceDrivers(imageBase, sizeof(imageBase), &needed))
            {
                std::cerr << "ERROR: Cannot EnumDeviceDrivers" << std::endl;
                return false;
            }
            driverNum = needed / sizeof(LPVOID);
            // Add the modules for each process.
            for (i = 0; i < driverNum; ++i)
            {
                insert(imageBase[i]);
            }
        }

        std::cerr << "total: " << procNum
                  << " unnamed: " << unnamed << std::endl;
        return true;
    }

    DWORD find(std::string& name)
    {
        lsTree(false);
        DWORD found = -1;
        for (auto i = pidMap.begin(); i != pidMap.end(); ++i)
        {
            PsInfo* p = (i->second);
            if (strlen(p->name) != 0)
            {
                if (strstr(p->name, name.c_str()) != NULL)
                {
                    found = i->first;
                    std::cout << p->name << " pid = " << found;
                    std::cout << " - FOUND";
                    std::cout << std::endl;
                }
            }
        }
        return found;
    }

    template<typename T> friend T& operator << (T& os, PsList &pl)
    {
        // Header
        os << "# Key:" << std::endl
            << "# P : PID, FilePath" << std::endl
            << "# M : Base, Size, FileName, FilePath" << std::endl
            << "# D : Base, FileName, FilePath" << std::endl
            << "# S : Base, Size, Symbol" << std::endl;

        for (auto i = pl.pidMap.begin(); i != pl.pidMap.end(); ++i)
        {
            os << (i->second);
        }
        for (auto i = pl.drvMap.begin(); i != pl.drvMap.end(); ++i)
        {
            os << (i->second);
        }

        return os;
    }
};

