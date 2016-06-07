//---------------------------------------------------------------------------//
//
// Main.cpp
//  Copyright (C) tapetums 2016
//
//---------------------------------------------------------------------------//

#include <windows.h>

#include "Plugin.hpp"
#include "MessageDef.hpp"
#include "Utility.hpp"

#include "MainWnd.hpp"

#include "Main.hpp"

//---------------------------------------------------------------------------//
//
// グローバル変数
//
//---------------------------------------------------------------------------//

HINSTANCE g_hInst     { nullptr };
HWND      g_hwnd      { nullptr };
HWND      g_hwnd_next { nullptr };

bool  g_enabled  { true };
INT32 g_max_item { 10 };

//---------------------------------------------------------------------------//

// プラグインの名前
LPCTSTR PLUGIN_NAME { TEXT("クリップボード履歴 Light") };

// コマンドの数
DWORD COMMAND_COUNT { CMD_CUONT };

//---------------------------------------------------------------------------//

// コマンドの情報
PLUGIN_COMMAND_INFO g_cmd_info[] =
{
    {
        TEXT("Toggle ClipBoardViewer"),                        // コマンド名（英名）
        TEXT("クリップボードを監視"),                          // コマンド説明（日本語）
        CMD_SWITCH,                                            // コマンドID
        0,                                                     // Attr（未使用）
        -1,                                                    // ResTd(未使用）
        DISPMENU(dmSystemMenu | dmHotKeyMenu | dmMenuChecked), // DispMenu
        0,                                                     // TimerInterval[msec] 0で使用しない
        0                                                      // TimerCounter（未使用）
    },
    {
        TEXT("Show ClipBoard History"),        // コマンド名（英名）
        TEXT("クリップボード履歴を表示"),      // コマンド説明（日本語）
        CMD_MENU,                              // コマンドID
        0,                                     // Attr（未使用）
        -1,                                    // ResTd(未使用）
        DISPMENU(dmSystemMenu | dmHotKeyMenu), // DispMenu
        0,                                     // TimerInterval[msec] 0で使用しない
        0                                      // TimerCounter（未使用）
    },
};

//---------------------------------------------------------------------------//

// プラグインの情報
PLUGIN_INFO g_info =
{
    0,                   // プラグインI/F要求バージョン
    (LPTSTR)PLUGIN_NAME, // プラグインの名前（任意の文字が使用可能）
    nullptr,             // プラグインのファイル名（相対パス）
    ptAlwaysLoad,        // プラグインのタイプ
    0,                   // バージョン
    0,                   // バージョン
    COMMAND_COUNT,       // コマンド個数
    &g_cmd_info[0],      // コマンド
    0,                   // ロードにかかった時間（msec）
};

//---------------------------------------------------------------------------//
//
// CRT を使わないため new/delete を自前で実装
//
//---------------------------------------------------------------------------//

#if defined(_NODEFLIB)

void* __cdecl operator new(size_t size)
{
    return ::HeapAlloc(::GetProcessHeap(), 0, size);
}

void __cdecl operator delete(void* p)
{
    if ( p != nullptr ) ::HeapFree(::GetProcessHeap(), 0, p);
}

void __cdecl operator delete(void* p, size_t) // C++14
{
    if ( p != nullptr ) ::HeapFree(::GetProcessHeap(), 0, p);
}

void* __cdecl operator new[](size_t size)
{
    return ::HeapAlloc(::GetProcessHeap(), 0, size);
}

void __cdecl operator delete[](void* p)
{
    if ( p != nullptr ) ::HeapFree(::GetProcessHeap(), 0, p);
}

void __cdecl operator delete[](void* p, size_t) // C++14
{
    if ( p != nullptr ) ::HeapFree(::GetProcessHeap(), 0, p);
}

// プログラムサイズを小さくするためにCRTを除外
#pragma comment(linker, "/nodefaultlib:libcmt.lib")
#pragma comment(linker, "/entry:DllMain")

#endif

//---------------------------------------------------------------------------//

// DLL エントリポイント
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID)
{
    if ( fdwReason == DLL_PROCESS_ATTACH )
    {
        g_hInst = hInstance;
    }
    return TRUE;
}

//---------------------------------------------------------------------------//
//
// エクスポート関数の内部実装
//
//---------------------------------------------------------------------------//

// TTBEvent_Init() の内部実装
BOOL WINAPI Init()
{
    if ( g_hwnd ) { return TRUE; }

    constexpr auto class_name = L"ClipBoardHistoryLight";

    // ウィンドウクラスの登録
    WNDCLASSEXW wcex;
    wcex.cbSize        = sizeof(WNDCLASSEX);
    wcex.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcex.lpfnWndProc   = WndProc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hInstance     = g_hInst;
    wcex.hIcon         = nullptr;
    wcex.hCursor       = nullptr;
    wcex.hbrBackground = nullptr;
    wcex.lpszMenuName  = nullptr;
    wcex.lpszClassName = class_name;
    wcex.hIconSm       = nullptr;

    if( ::RegisterClassExW(&wcex) == 0 )
    {
        return FALSE;
    }

    // ウィンドウの生成
    g_hwnd = ::CreateWindowExW
    (
        0, class_name, class_name, 0,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr, nullptr, g_hInst, nullptr
    );

    return g_hwnd ? TRUE : FALSE;
}

//---------------------------------------------------------------------------//

// TTBEvent_Unload() の内部実装
void WINAPI Unload()
{
    if ( g_hwnd )
    {
        ::DestroyWindow(g_hwnd);
        g_hwnd = nullptr;
    }
}

//---------------------------------------------------------------------------//

// TTBEvent_Execute() の内部実装
BOOL WINAPI Execute(INT32 CmdID, HWND)
{
    switch ( CmdID )
    {
        case CMD_SWITCH:
        case CMD_MENU:
        {
            ::PostMessage(g_hwnd, WM_COMMAND, MAKEWPARAM(CmdID, 0), 0);
            return TRUE;
        }
        default:
        {
            return FALSE;
        }
    }
}

//---------------------------------------------------------------------------//

// TTBEvent_WindowsHook() の内部実装
void WINAPI Hook(UINT, WPARAM, LPARAM)
{
}

//---------------------------------------------------------------------------//

// Main.cpp