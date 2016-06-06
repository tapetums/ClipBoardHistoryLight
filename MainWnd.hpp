#pragma once

//---------------------------------------------------------------------------//
//
// MainWnd.hpp
//  メインウィンドウ
//   Copyright (C) tapetums 2016
//
//---------------------------------------------------------------------------//

#include <windows.h>

//---------------------------------------------------------------------------//

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

LRESULT CALLBACK OnCreate       (HWND hwnd);
LRESULT CALLBACK OnDestroy      (HWND hwnd);
LRESULT CALLBACK OnCommand      (HWND hwnd, INT32 CmdID);
LRESULT CALLBACK OnDrawClipboard(HWND hwnd);
LRESULT CALLBACK OnChangeCBChain(HWND hwnd, HWND hwndRemove, HWND hwndNext);

//---------------------------------------------------------------------------//

void  CheckMenu            (INT32 CmdID, bool checked);
void  SwitchClipboardViewer(HWND hwnd, bool enable);
void  MakeMenuString       (wchar_t* buf, size_t size, UINT index, const wchar_t* const str);
HMENU MakeMenu             ();
void  SelectText           (HWND hwnd, UINT index);
void  PasteText            ();
UINT  PopupMenu            (HWND hwnd);

//---------------------------------------------------------------------------//

// MainWnd.hpp