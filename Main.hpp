#pragma once

//---------------------------------------------------------------------------//
//
// Main.hpp
//  TTB Plugin Template (C++11)
//
//---------------------------------------------------------------------------//

#include <windows.h>

extern HINSTANCE g_hInst;
extern HWND      g_hwnd;
extern HWND      g_hwnd_next;

extern bool  g_enabled;
extern INT32 g_max_item;

//---------------------------------------------------------------------------//

// コマンドID
enum CMD : INT32
{
    CMD_SWITCH, CMD_MENU, CMD_CUONT,
};

//---------------------------------------------------------------------------//

// Main.hpp