//---------------------------------------------------------------------------//
//
// MainWnd.cpp
//  メインウィンドウ
//   Copyright (C) tapetums 2016
//
//---------------------------------------------------------------------------//

#include <array>

#include <windows.h>
#include <windowsx.h>
#include <strsafe.h>

#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#include "deque.hpp"

#include "Plugin.hpp"
#include "Utility.hpp"
#include "Main.hpp"

#include "MainWnd.hpp"

//---------------------------------------------------------------------------//
//
// 文字列を保持するクラス (RAII)
//
//---------------------------------------------------------------------------//

namespace tapetums { class wstring; }

class tapetums::wstring
{
private:
    wchar_t* str { nullptr };

public:
    explicit wstring(const wchar_t* const s)
    {
        const auto len = lstrlenW(s) + 1;
        str = new wchar_t[len];

        ::StringCchCopyW(str, len, s);
    }

    ~wstring() { delete[] str; str = nullptr; }

    wstring(const wstring& lhs) : wstring(lhs.str) { }
    wstring(wstring&& rhs) noexcept { std::swap(str, rhs.str); }

public:
    const wchar_t* c_str() const noexcept { return str; }
    wchar_t*       c_str()       noexcept { return str; }
};

//---------------------------------------------------------------------------//
//
// クリップボード履歴を保持するデータ構造
//
//---------------------------------------------------------------------------//

tapetums::deque<tapetums::wstring>* clipboard_log { nullptr };

//---------------------------------------------------------------------------//
//
// ウィンドウプロシージャ
//
//---------------------------------------------------------------------------//

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch ( msg )
    {
        case WM_CREATE:        { return OnCreate(hwnd); }
        case WM_DESTROY:       { return OnDestroy(hwnd); }
        case WM_CLOSE:         { return 0; }
        case WM_ENDSESSION:    { SaveClipLog(); return 0; }
        case WM_COMMAND:       { return OnCommand(hwnd, LOWORD(wp)); }
        case WM_DRAWCLIPBOARD: { return OnDrawClipboard(hwnd); }
        case WM_CHANGECBCHAIN: { return OnChangeCBChain(hwnd, HWND(wp), HWND(lp)); }
        default:               { return ::DefWindowProc(hwnd, msg, wp, lp); }
    }
}

//---------------------------------------------------------------------------//
//
// イベントハンドラ
//
//---------------------------------------------------------------------------//

LRESULT CALLBACK OnCreate(HWND hwnd)
{
    TCHAR ininame[MAX_PATH];

    // iniファイル名取得
    const auto len = ::GetModuleFileName(g_hInst, ininame, MAX_PATH);
    ininame[len - 3] = 'i';
    ininame[len - 2] = 'n';
    ininame[len - 1] = 'i';

    // パラメータの読み込み
    g_enabled = ::GetPrivateProfileInt
    (
        TEXT("Setting"), TEXT("enable"), 1, ininame
    )
    ? true : false;

    g_max_item = ::GetPrivateProfileInt
    (
        TEXT("Setting"), TEXT("max_item"), 10, ininame
    );
    if ( g_max_item > 50 )
    {
        g_max_item = 50;
    }

    // クリップボード監視の開始
    CheckMenu(CMD_SWITCH, g_enabled);
    SwitchClipboardViewer(hwnd, g_enabled);

    // クリップボード履歴を作成
    if ( nullptr == clipboard_log )
    {
        clipboard_log = new tapetums::deque<tapetums::wstring>();
    }

    // クリップボード履歴の読み込み
    LoadClipLog();

    return 0;
}

//---------------------------------------------------------------------------//

LRESULT CALLBACK OnDestroy(HWND hwnd)
{
    // クリップボード履歴の書き出し
    SaveClipLog();

    // クリップボード履歴を破棄
    if ( clipboard_log )
    {
        delete clipboard_log; clipboard_log = nullptr;
    }

    // クリップボード監視の停止
    SwitchClipboardViewer(hwnd, false);

    return 0;
}

//---------------------------------------------------------------------------//

LRESULT CALLBACK OnCommand(HWND hwnd, INT32 CmdID)
{
    if ( CmdID == CMD_SWITCH )
    {
        g_enabled = !g_enabled;
        CheckMenu(CMD_SWITCH, g_enabled);
        SwitchClipboardViewer(hwnd, g_enabled);
    }
    else if ( CmdID == CMD_MENU )
    {
        const auto cThId = ::GetCurrentThreadId();
        const auto tThId = ::GetWindowThreadProcessId
        (
            ::GetForegroundWindow(), nullptr
        );

        ::AttachThreadInput(tThId, cThId, TRUE);
        const auto hwnd_target = ::GetForegroundWindow();
        const auto hwnd_focus  = ::GetFocus();
        ::AttachThreadInput(tThId, cThId, FALSE);
        //WriteLog(elDebug, TEXT("%s: hwnd_target = %p"), PLUGIN_NAME, hwnd_target);
        //WriteLog(elDebug, TEXT("%s: hwnd_focus  = %p"), PLUGIN_NAME, hwnd_focus);

        const auto index = PopupMenu(hwnd);

        ::AttachThreadInput(cThId, tThId, TRUE);
        ::SetForegroundWindow(hwnd_target);
        ::SetFocus(hwnd_focus);

        if ( index > 0 ) // index == 0 のとき選択なし
        {
            // SHIFT が押されていたら 貼り付けは行わない
            if ( !(::GetAsyncKeyState(VK_SHIFT) & 0x80000000) )
            {
                PasteText();
            }
        }

        ::AttachThreadInput(cThId, tThId, FALSE);
    }

    return 0;
}

//---------------------------------------------------------------------------//

LRESULT CALLBACK OnDrawClipboard(HWND hwnd)
{
    if ( nullptr == clipboard_log ) { return 0; }

    if ( ::OpenClipboard(hwnd) )
    {
        const auto hText = ::GetClipboardData(CF_UNICODETEXT);
        if ( hText )
        {
            auto p = (LPCWSTR)::GlobalLock(hText);

            // 重複分を削除
            const auto end = clipboard_log->end();
            for ( auto it = clipboard_log->begin(); it != end; ++it )
            {
                //WriteLog(elDebug, TEXT("%s, %s"), p, it->c_str());
                if ( 0 == lstrcmpW(p, it->c_str()) )
                {
                    //WriteLog(elDebug, TEXT("erase"));
                    clipboard_log->erase(it);
                    break;
                }
            }

            // 先頭に追加
            clipboard_log->push_front(tapetums::wstring(p));
            if ( clipboard_log->size() > size_t(g_max_item) )
            {
                clipboard_log->pop_back(); // 一番古い履歴を消去
            }
            //WriteLog(elDebug, TEXT("%u"), clipboard_log->size());

            ::GlobalUnlock(hText);
        }

        ::CloseClipboard();
    }

    if ( g_hwnd_next ) 
    {
        FORWARD_WM_DRAWCLIPBOARD(g_hwnd_next, SendMessage);
    }

    return 0;
}

//---------------------------------------------------------------------------//

LRESULT CALLBACK OnChangeCBChain(HWND hwnd, HWND hwndRemove, HWND hwndNext)
{
    if ( hwndRemove == g_hwnd_next && hwnd != hwndNext )
    {
        g_hwnd_next = hwndNext;
    }
    else if ( g_hwnd_next && hwnd != g_hwnd_next ) 
    {
        FORWARD_WM_CHANGECBCHAIN(g_hwnd_next, hwndRemove, hwndNext, SendMessage);
    }

    return 0;
}

//---------------------------------------------------------------------------//
//
// ユーティリティ関数
//
//---------------------------------------------------------------------------//

// システムメニューにチェックマークを付ける
void CheckMenu(INT32 CmdID, bool checked)
{
    TTBPlugin_SetMenuProperty
    (
        g_hPlugin, CmdID, DISPMENU_CHECKED,
        checked ? dmMenuChecked : dmUnchecked
    );
}

//---------------------------------------------------------------------------//

// クリップボードの監視を ON/OFF する
void SwitchClipboardViewer(HWND hwnd, bool enable)
{
    if ( enable )
    {
        g_hwnd_next = ::SetClipboardViewer(hwnd);
    }
    else
    {
        ::ChangeClipboardChain(hwnd, g_hwnd_next);
        g_hwnd_next = nullptr;
    }
}

//---------------------------------------------------------------------------//

// メニューの表示項目を生成する
void MakeMenuString
(
    wchar_t* buf, size_t size, UINT index, const wchar_t* const str
)
{
    if ( index < 10 )
    {
        ::wsprintfW
        (
            buf, /*size,*/ L"&%u. %ls", index, str
        );
    }
    else if ( index - 10 <= L'z' - L'a' )
    {
        ::wsprintfW
        (
            buf, /*size,*/ L"&%lc. %ls", L'a' + index - 10, str
        );
    }
    else
    {
        ::wsprintfW
        (
            buf, /*size,*/ L"& . %ls", str
        );
    }

    const auto len = lstrlenW(buf);
    if ( len > 50 )
    {
        ::StringCchCopyW(buf + 50 + 4, size, L"...");
    }
}

//---------------------------------------------------------------------------//

// コンテクストメニューを生成する
HMENU MakeMenu()
{
    if ( nullptr == clipboard_log ) { return nullptr; }

    std::array<wchar_t, MAX_PATH> buf;

    auto hMenu = ::LoadMenu(g_hInst, MAKEINTRESOURCE(101));
    auto hSubMenu = ::GetSubMenu(hMenu, 0);

    MENUITEMINFOW mii;
    mii.cbSize        = sizeof(mii);
    mii.fMask         = MIIM_ID | MIIM_TYPE;
    mii.fType         = MFT_STRING;
    mii.fState        = MFS_ENABLED;
    mii.hSubMenu      = nullptr;
    mii.hbmpChecked   = nullptr;
    mii.hbmpUnchecked = nullptr;
    mii.dwItemData    = 0;
    mii.cch           = 0;
    mii.hbmpItem      = nullptr;

    UINT index = 1; // 選択なしを 0 として扱うため 先頭を 1 にする
    for ( auto&& str: *clipboard_log )
    {
        MakeMenuString(buf.data(), buf.size(), index, str.c_str());

        mii.wID        = index;
        mii.dwTypeData = buf.data();
        ::InsertMenuItemW(hSubMenu, index, TRUE, &mii);

        ++index;
    }

    // セパレータの位置を移動
    //  メニューリソースでは項目が予め最低一つないと
    //  メニューが表示されないバグがあるため、
    //  最初にダミーとして一つセパレータを入れてある
    DeleteMenu(hSubMenu, 0, MF_BYPOSITION);

    return hSubMenu;
}

//---------------------------------------------------------------------------//

// クリップボードの履歴順を入れ替える
void SelectText(HWND hwnd, UINT index)
{
    if ( nullptr == clipboard_log ) { return; }

    UINT i = 1; // 選択なしを 0 として扱うため 先頭を 1 にする

    for ( auto && str: *clipboard_log )
    {
        if ( i == index )
        {
            // 文字列をコピー
            const auto size = sizeof(wchar_t) * (lstrlenW(str.c_str()) + 1);
            const auto tmp = ::GlobalAlloc(GMEM_FIXED, size);
            auto p = (wchar_t*)::GlobalLock(tmp);
            ::StringCbCopyW(p, size, str.c_str());
            ::GlobalUnlock(tmp);

            // クリップボードにデータを送る
            if ( ::OpenClipboard(hwnd) )
            {
                ::EmptyClipboard();
                ::SetClipboardData(CF_UNICODETEXT, tmp);
                ::CloseClipboard();
            }

            break;
        }

        ++i;
    }
}

//---------------------------------------------------------------------------//

// Ctrl + V を送る
void PasteText()
{
    ::Sleep(100);

    INPUT input[4];

    input[0].type           = INPUT_KEYBOARD;
    input[0].ki.wVk         = VK_CONTROL;
    input[0].ki.wScan       = (WORD)MapVirtualKey(VK_CONTROL, 0);
    input[0].ki.dwFlags     = 0;
    input[0].ki.time        = 0;
    input[0].ki.dwExtraInfo = 0;

    input[1].type           = INPUT_KEYBOARD;
    input[1].ki.wVk         = 'V';
    input[1].ki.wScan       = (WORD)MapVirtualKey('V', 0);
    input[1].ki.dwFlags     = 0;
    input[1].ki.time        = 0;
    input[1].ki.dwExtraInfo = 0;

    input[2].type           = INPUT_KEYBOARD;
    input[2].ki.wVk         = 'V';
    input[2].ki.wScan       = (WORD)MapVirtualKey('V', 0);
    input[2].ki.dwFlags     = KEYEVENTF_KEYUP;
    input[2].ki.time        = 0;
    input[2].ki.dwExtraInfo = 0;

    input[3].type           = INPUT_KEYBOARD;
    input[3].ki.wVk         = VK_CONTROL;
    input[3].ki.wScan       = (WORD)MapVirtualKey(VK_CONTROL, 0);
    input[3].ki.dwFlags     = KEYEVENTF_KEYUP;
    input[3].ki.time        = 0;
    input[3].ki.dwExtraInfo = 0;

    ::SendInput(4, input, sizeof(INPUT));
}

//---------------------------------------------------------------------------//

// クリップボードの履歴を表示する
UINT PopupMenu(HWND hwnd)
{
    const auto hMenu = MakeMenu();

    POINT pt;
    ::GetCursorPos(&pt);

    // Article ID: Q135788
    // ポップアップメニューから処理を戻すために必要
    ::SetForegroundWindow(hwnd);

    // ポップアップメニューを表示
    // index == 0 の時は選択なし
    const auto index = (UINT)::TrackPopupMenu
    (
        hMenu, TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RETURNCMD,
        pt.x, pt.y, 0, hwnd, nullptr
    );

    // 表示したメニューを破棄
    ::DestroyMenu(hMenu);

    // Article ID: Q135788
    // ポップアップメニューから処理を戻すために必要
    ::PostMessage(hwnd, WM_NULL, 0, 0);

    //WriteLog(elDebug, TEXT("%s: index = %u"), PLUGIN_NAME, index);

    if ( index > 1 ) // index == 1 の時は入れ替え作業は必要なし
    {
        SelectText(hwnd, index);
    }

    return index;
}

//---------------------------------------------------------------------------//

// クリップボードの履歴を読み込む
void LoadClipLog()
{
    if ( nullptr == clipboard_log ) { return; }

    TCHAR path   [MAX_PATH];
    TCHAR dirpath[MAX_PATH];
    TCHAR logname[MAX_PATH];

    const auto len = ::GetModuleFileName(g_hInst, path, MAX_PATH);
    for ( auto i = len - 1; i > 0; --i )
    {
        if ( path[i] == '\\' )
        {
            path[i] = '\0'; break;
        }
    }
    wsprintf(dirpath, TEXT(R"(%s\history)"), path);

    for ( UINT idx = 1; idx <= 50; ++idx )
    {
        wsprintf(logname, TEXT(R"(%s\%03u.txt)"), dirpath, idx);
        const auto file = ::CreateFile
        (
            logname, GENERIC_READ, 0, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr
        );
        if ( file == INVALID_HANDLE_VALUE )
        {
            break;
        }

        const auto size = ::GetFileSize(file, nullptr); // 4GiB 以上は無視
        auto str = new wchar_t[size / sizeof(wchar_t) + 1];

        DWORD cb;
        ::ReadFile(file, str, size, &cb, nullptr);
        str[cb / sizeof(wchar_t)] = L'\0';

        clipboard_log->push_back(tapetums::wstring(str));

        ::CloseHandle(file);
    }
}

//---------------------------------------------------------------------------//

// クリップボードの履歴を書き出す
void SaveClipLog()
{
    if ( nullptr == clipboard_log ) { return; }

    TCHAR path   [MAX_PATH];
    TCHAR dirpath[MAX_PATH];
    TCHAR logname[MAX_PATH];

    const auto len = ::GetModuleFileName(g_hInst, path, MAX_PATH);
    for ( auto i = len - 1; i > 0; --i )
    {
        if ( path[i] == '\\' )
        {
            path[i] = '\0';
            break;
        }
    }
    wsprintf(dirpath, TEXT(R"(%s\history)"), path);

    if ( !::PathIsDirectory(dirpath) )
    {
        ::CreateDirectory(dirpath, nullptr);
    }
    else
    {
        // 既存の履歴ファイルを消去
        for ( UINT idx = 1; ; ++idx )
        {
            wsprintf(logname, TEXT(R"(%s\%03u.txt)"), dirpath, idx);
            if ( ! ::DeleteFile(logname) )
            {
                break;
            }
        }
    }

    UINT idx = 1;
    for ( auto&& str: *clipboard_log )
    {
        wsprintf(logname, TEXT(R"(%s\%03u.txt)"), dirpath, idx);
        const auto file = ::CreateFile
        (
            logname, GENERIC_WRITE, 0, nullptr,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr
        );
        if ( file == INVALID_HANDLE_VALUE )
        {
            break;
        }

        DWORD cb;
        const DWORD size = sizeof(wchar_t) * lstrlenW(str.c_str());
        ::WriteFile(file, str.c_str(), size, &cb, nullptr);
        ::CloseHandle(file);

        ++idx;
    }
}

//---------------------------------------------------------------------------//

// MainWnd.cpp