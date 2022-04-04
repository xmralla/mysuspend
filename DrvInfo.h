#pragma once
#include "mysuspend.h"

class DrvInfo
{
    char m_name[MAX_PATH] = { 0 };
    char m_nameEx[MAX_PATH] = { 0 };

public:
    const LPVOID addr;
    const HANDLE sym;
    const char* name = m_name;
    const char* nameEx = m_nameEx;

    DrvInfo(LPVOID new_addr, HANDLE new_sym) : sym(new_sym), addr(new_addr)
    {
        GetDeviceDriverBaseNameA(addr, (LPSTR)name, MAX_PATH);
        GetDeviceDriverFileNameA(addr, (LPSTR)nameEx, MAX_PATH);
    }

    template<typename T> friend T& operator << (T& os, DrvInfo* drv)
    {
        os << "D : " << std::setw(16) << std::hex << drv->addr << " : "
            << drv->name << " : " << drv->nameEx << std::endl;

        return os;
    }
};

