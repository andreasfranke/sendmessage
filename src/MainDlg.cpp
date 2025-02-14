﻿// SendMessage - a tool to send custom messages

// Copyright (C) 2010, 2012-2015, 2018-2019, 2021 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//

#include "stdafx.h"
#include "resource.h"
#include "MainDlg.h"

#include <shlwapi.h>

#include "AboutDlg.h"
#include "WindowTreeDlg.h"
#include "WinMessage.h"
#include "AccessibleName.h"
#include "StringUtils.h"

CMainDlg::CMainDlg(HWND hParent)
    : m_hParent(hParent)
    , m_bStartSearchWindow(false)
    , m_hwndFoundWindow(nullptr)
    , m_hRectanglePen(nullptr)
{
}

CMainDlg::~CMainDlg()
{
}

LRESULT CMainDlg::DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (uMsg)
    {
        case WM_INITDIALOG: {
            InitDialog(hwndDlg, IDI_SENDMESSAGE);

            // add an "About" entry to the system menu
            HMENU hSysMenu = GetSystemMenu(hwndDlg, FALSE);
            if (hSysMenu)
            {
                int menuItemsCount = GetMenuItemCount(hSysMenu);
                if (menuItemsCount > 2)
                {
                    InsertMenu(hSysMenu, menuItemsCount - 2, MF_STRING | MF_BYPOSITION, ID_ABOUTBOX, _T("&About SendMessage..."));
                    InsertMenu(hSysMenu, menuItemsCount - 2, MF_SEPARATOR | MF_BYPOSITION, NULL, nullptr);
                }
                else
                {
                    AppendMenu(hSysMenu, MF_SEPARATOR, NULL, nullptr);
                    AppendMenu(hSysMenu, MF_STRING, ID_ABOUTBOX, _T("&About SendMessage..."));
                }

                MENUITEMINFO stItem;
                stItem.cbSize     = sizeof(MENUITEMINFO);
                stItem.fMask      = MIIM_STRING;
                stItem.dwTypeData = (LPTSTR)TEXT("Always On Top");
                SetMenuItemInfo(hSysMenu, SC_MAXIMIZE, FALSE, &stItem);
            }
            m_link.ConvertStaticToHyperlink(hwndDlg, IDC_ABOUTLINK, _T(""));

            m_hRectanglePen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));

            // now fill the window message combo box
            for (size_t i = 0; i < WinMessages::Instance().GetCount(); ++i)
            {
                WinMessage msg   = WinMessages::Instance().At(i);
                LRESULT    index = SendDlgItemMessage(*this, IDC_MESSAGE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(msg.messagename.c_str()));
                SendDlgItemMessage(*this, IDC_MESSAGE, CB_SETITEMDATA, index, msg.message);
            }

            WINDOWPLACEMENT wpl  = {0};
            DWORD           size = sizeof(wpl);
            if (SHGetValue(HKEY_CURRENT_USER, _T("Software\\SendMessage"), _T("windowpos"), REG_NONE, &wpl, &size) == ERROR_SUCCESS)
                SetWindowPlacement(*this, &wpl);
            else
                ShowWindow(*this, SW_SHOW);

            ExtendFrameIntoClientArea(IDC_SENDGROUP, IDC_SENDGROUP, IDC_SENDGROUP, IDC_SENDGROUP);
            m_aerocontrols.SubclassControl(GetDlgItem(*this, IDC_SEARCHW));
            m_aerocontrols.SubclassControl(GetDlgItem(*this, IDC_ABOUTLINK));
            m_aerocontrols.SubclassControl(GetDlgItem(*this, IDC_POS));
            m_aerocontrols.SubclassControl(GetDlgItem(*this, IDC_STATIC_X_POS));
            m_aerocontrols.SubclassControl(GetDlgItem(*this, IDC_STATIC_Y_POS));
            m_aerocontrols.SubclassControl(GetDlgItem(*this, IDC_EDIT_STATUS));
            m_aerocontrols.SubclassControl(GetDlgItem(*this, IDC_WINDOWTREE));
            m_aerocontrols.SubclassControl(GetDlgItem(*this, IDOK));

            if (HWND hCmdShow = GetDlgItem(*this, IDC_CMDSHOW))
            {
                ComboBox_AddString(hCmdShow, TEXT("SW_HIDE"));
                ComboBox_AddString(hCmdShow, TEXT("SW_SHOWNORMAL"));
                ComboBox_AddString(hCmdShow, TEXT("SW_SHOWMINIMIZED"));
                ComboBox_AddString(hCmdShow, TEXT("SW_SHOWMAXIMIZED"));
                ComboBox_AddString(hCmdShow, TEXT("SW_SHOWNOACTIVATE"));
                ComboBox_AddString(hCmdShow, TEXT("SW_SHOW"));
                ComboBox_AddString(hCmdShow, TEXT("SW_MINIMIZE"));
                ComboBox_AddString(hCmdShow, TEXT("SW_SHOWMINNOACTIVE"));
                ComboBox_AddString(hCmdShow, TEXT("SW_SHOWNA"));
                ComboBox_AddString(hCmdShow, TEXT("SW_RESTORE"));
                ComboBox_AddString(hCmdShow, TEXT("SW_SHOWDEFAULT"));
                ComboBox_AddString(hCmdShow, TEXT("SW_FORCEMINIMIZE"));
                ComboBox_SetCurSel(hCmdShow, 0);
            }
        }
            return FALSE;
        case WM_COMMAND:
            return DoCommand(LOWORD(wParam), HIWORD(wParam));
        case WM_SYSCOMMAND:
            switch (wParam & 0xFFF0)
            {
                case ID_ABOUTBOX:
                    CAboutDlg(*this).DoModal(hResource, IDD_ABOUTBOX, *this);
                    break;
                case SC_MINIMIZE:
                    SetWindowLongPtr(hwndDlg, GWL_STYLE, GetWindowLongPtr(hwndDlg, GWL_STYLE) & ~WS_MAXIMIZE);
                    break;
                case SC_MAXIMIZE:
                    SetWindowPos(hwndDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                    return TRUE;
                case SC_RESTORE:
                    if (!IsIconic(hwndDlg))
                    {
                        SetWindowPos(hwndDlg, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                        return TRUE;
                    }
                    break;
            }
            break;
        case WM_WINDOWPOSCHANGED:
            if (GetWindowLongPtr(hwndDlg, GWL_EXSTYLE) & WS_EX_TOPMOST)
                SetWindowLongPtr(hwndDlg, GWL_STYLE, GetWindowLongPtr(hwndDlg, GWL_STYLE) | WS_MAXIMIZE);
            else
                SetWindowLongPtr(hwndDlg, GWL_STYLE, GetWindowLongPtr(hwndDlg, GWL_STYLE) & ~WS_MAXIMIZE);
            break;
        case WM_MOUSEMOVE:
            if (m_bStartSearchWindow)
                DoMouseMove(uMsg, wParam, lParam);
            break;
        case WM_LBUTTONUP:
            if (m_bStartSearchWindow)
                DoMouseUp(uMsg, wParam, lParam);
            break;
        case WM_SETCURSOR:
            if (m_bStartSearchWindow)
            {
                SetCursor(LoadCursor(hResource, MAKEINTRESOURCE(IDC_SEARCHW)));
                return TRUE;
            }
            break;
        default:
            return FALSE;
    }
    return FALSE;
}

LRESULT CMainDlg::DoCommand(int id, int msg)
{
    switch (id)
    {
        case IDOK:
        case IDCANCEL:
            DeleteObject(m_hRectanglePen);
            EndDialog(*this, id);
            break;
        case IDC_SEARCHW:
            SearchWindow();
            break;
        case IDC_WINDOWTREE: {
            CWindowTreeDlg treeDlg(*this, GetSelectedHandle());
            if (treeDlg.DoModal(hResource, IDD_WINDOWSTREE, *this) == IDOK)
            {
                m_hwndFoundWindow = treeDlg.GetSelectedWindow();
                DisplayInfoOnFoundWindow(m_hwndFoundWindow);
                TCHAR szText[256];
                _stprintf_s(szText, _countof(szText), _T("0x%p"), m_hwndFoundWindow);
                SetDlgItemText(*this, IDC_WINDOW, szText);
            }
        }
        break;
        case IDC_SENDMESSAGE:
        case IDC_POSTMESSAGE:
            if (!SendPostMessage(id))
            {
                SetDlgItemText(*this, IDC_ERROR, _T("Message not recognized"));
            }
            break;
        case IDC_PINWINDOW:
        case IDC_UNPINWINDOW:
            if (HWND hWnd = GetSelectedHandle())
            {
                HWND hWndInsertAfter = id == IDC_PINWINDOW ? HWND_TOPMOST : HWND_NOTOPMOST;
                int res = SetWindowPos(hWnd, hWndInsertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                TCHAR buf[MAX_PATH]{};
                _stprintf_s(buf, _countof(buf), _T("0x%08x (%d)"), res, res);
                SetDlgItemText(*this, IDC_RETVALUE, buf);
                SetDlgItemText(*this, IDC_ERROR, _T(""));
            }
            else
            {
                SetDlgItemText(*this, IDC_RETVALUE, _T(""));
                SetDlgItemText(*this, IDC_ERROR, _T("Window not found"));
            }
            break;
        case IDC_SHOWWINDOW:
            if (HWND hWnd = GetSelectedHandle())
            {
                int nCmdShow = ComboBox_GetCurSel(GetDlgItem(*this, IDC_CMDSHOW));
                int res = ShowWindow(hWnd, nCmdShow);
                TCHAR buf[MAX_PATH]{};
                _stprintf_s(buf, _countof(buf), _T("0x%08x (%d)"), res, res);
                SetDlgItemText(*this, IDC_RETVALUE, buf);
                SetDlgItemText(*this, IDC_ERROR, _T(""));
            }
            else
            {
                SetDlgItemText(*this, IDC_RETVALUE, _T(""));
                SetDlgItemText(*this, IDC_ERROR, _T("Window not found"));
            }
            break;
        case IDC_ABOUTLINK: {
            CAboutDlg dlgAbout(*this);
            dlgAbout.DoModal(hResource, IDD_ABOUTBOX, *this);
        }
        break;
        case IDC_MESSAGE: {
            switch (msg)
            {
                case CBN_EDITCHANGE:
                case CBN_SELCHANGE: {
                    // the selected window message has changed, adjust the wparam and lparam comboboxes
                    LRESULT selIndex = SendDlgItemMessage(*this, IDC_MESSAGE, CB_GETCURSEL, 0, 0);
                    SendDlgItemMessage(*this, IDC_WPARAM, CB_RESETCONTENT, 0, 0);
                    SendDlgItemMessage(*this, IDC_LPARAM, CB_RESETCONTENT, 0, 0);
                    if (selIndex != CB_ERR)
                    {
                        LRESULT textlen = SendDlgItemMessage(*this, IDC_MESSAGE, CB_GETLBTEXTLEN, selIndex, 0);
                        auto    textbuf = std::make_unique<TCHAR[]>(textlen + 1);
                        SendDlgItemMessage(*this, IDC_MESSAGE, CB_GETLBTEXT, selIndex, reinterpret_cast<LPARAM>(textbuf.get()));
                        for (size_t i = 0; i < WinMessages::Instance().GetCount(); ++i)
                        {
                            WinMessage wmsg = WinMessages::Instance().At(i);
                            if (wmsg.messagename.compare(textbuf.get()) == 0)
                            {
                                for (size_t j = 0; j < wmsg.wparams.size(); ++j)
                                {
                                    std::wstring desc  = std::get<0>(wmsg.wparams[j]);
                                    LRESULT      index = SendDlgItemMessage(*this, IDC_WPARAM, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(desc.c_str()));
                                    SendDlgItemMessage(*this, IDC_WPARAM, CB_SETITEMDATA, index, std::get<1>(wmsg.wparams[j]));
                                }
                                for (size_t j = 0; j < wmsg.lparams.size(); ++j)
                                {
                                    std::wstring desc  = std::get<0>(wmsg.lparams[j]);
                                    LRESULT      index = SendDlgItemMessage(*this, IDC_LPARAM, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(desc.c_str()));
                                    SendDlgItemMessage(*this, IDC_LPARAM, CB_SETITEMDATA, index, std::get<1>(wmsg.lparams[j]));
                                }
                            }
                        }
                    }
                }
                break;
            }
        }
        break;
    }
    return 1;
}

void CMainDlg::SaveWndPosition()
{
    WINDOWPLACEMENT wpl = {0};
    wpl.length          = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(*this, &wpl);
    SHSetValue(HKEY_CURRENT_USER, _T("Software\\SendMessage"), _T("windowpos"), REG_NONE, &wpl, sizeof(wpl));
}

bool CMainDlg::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN)
    {
    }
    return false;
}

bool CMainDlg::SearchWindow()
{
    m_bStartSearchWindow = true;
    SetCapture(*this);

    return true;
}

bool CMainDlg::DoMouseMove(UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    POINT screenpoint{};
    HWND  hwndFoundWindow = nullptr;
    TCHAR szText[256]{};

    // Must use GetCursorPos() instead of calculating from "lParam".
    GetCursorPos(&screenpoint);

    // Display global positioning in the dialog box.
    _stprintf_s(szText, _countof(szText), _T("%d"), screenpoint.x);
    SetDlgItemText(*this, IDC_STATIC_X_POS, szText);

    _stprintf_s(szText, _countof(szText), _T("%d"), screenpoint.y);
    SetDlgItemText(*this, IDC_STATIC_Y_POS, szText);

    // Determine the window that lies underneath the mouse cursor.
    hwndFoundWindow = WindowFromPoint(screenpoint);

    if (CheckWindowValidity(hwndFoundWindow))
    {
        // We have just found a new window.

        // If there was a previously found window, we must instruct it to refresh itself.
        // This is done to remove any highlighting effects drawn by us.
        if (m_hwndFoundWindow)
        {
            RefreshWindow(m_hwndFoundWindow);
        }

        m_hwndFoundWindow = hwndFoundWindow;

        // Display some information on this found window.
        DisplayInfoOnFoundWindow(hwndFoundWindow);

        // We now highlight the found window.
        HighlightFoundWindow(m_hwndFoundWindow);
    }
    SetCursor(LoadCursor(hResource, MAKEINTRESOURCE(IDC_SEARCHW)));

    return true;
}

bool CMainDlg::DoMouseUp(UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    // If there was a found window, refresh it so that its highlighting is erased.
    if (m_hwndFoundWindow)
    {
        RefreshWindow(m_hwndFoundWindow);
    }

    ReleaseCapture();

    m_bStartSearchWindow = false;

    if (m_hwndFoundWindow == nullptr)
        return false;
    if (IsWindow(m_hwndFoundWindow) == FALSE)
        return false;
    if (m_hwndFoundWindow == *this)
        return false;

    HWND hwndTemp = GetParent(m_hwndFoundWindow);
    if (hwndTemp == *this)
        return false;

    hwndTemp = GetParent(hwndTemp);
    if (hwndTemp == *this)
        return false;

    TCHAR szText[256]{};
    _stprintf_s(szText, _countof(szText), _T("0x%p"), m_hwndFoundWindow);
    SetDlgItemText(*this, IDC_WINDOW, szText);

    return true;
}

bool CMainDlg::CheckWindowValidity(HWND hwndToCheck)
{
    HWND hwndTemp = nullptr;

    // The window must not be NULL.
    if (hwndToCheck == nullptr)
    {
        return false;
    }

    // Ensure that the window is not the current one which has already been found.
    if (hwndToCheck == m_hwndFoundWindow)
    {
        return false;
    }

    // It must also be a valid window as far as the OS is concerned.
    if (IsWindow(hwndToCheck) == FALSE)
    {
        return false;
    }

    // It must also not be the main window itself.
    if (hwndToCheck == *this)
    {
        return false;
    }

    // It also must not be one of the dialog box's children...
    hwndTemp = GetParent(hwndToCheck);
    if (hwndTemp == *this)
    {
        return false;
    }
    hwndTemp = GetParent(hwndTemp);
    if (hwndTemp == *this)
    {
        return false;
    }

    return true;
}

bool CMainDlg::DisplayInfoOnFoundWindow(HWND hwndFoundWindow)
{
    RECT  rect{}; // Rectangle area of the found window.
    TCHAR szClassName[100]{};

    // Get the screen coordinates of the rectangle of the found window.
    GetWindowRect(hwndFoundWindow, &rect);

    // Get the class name of the found window.
    GetClassName(hwndFoundWindow, szClassName, _countof(szClassName));

    // Display some information on the found window.
    auto sText = CStringUtils::Format(L"Window Handle == 0x%p\r\nClass Name : %s\r\nAccessible Name : %s\r\nRECT == { Left: %d, Top: %d, Right: %d, Bottom: %d } [ %d x %d ]\r\n",
                                      hwndFoundWindow,
                                      szClassName,
                                      CAccessibleName(hwndFoundWindow).c_str(),
                                      rect.left,
                                      rect.top,
                                      rect.right,
                                      rect.bottom,
                                      rect.right - rect.left,
                                      rect.bottom - rect.top);

    SetDlgItemText(*this, IDC_EDIT_STATUS, sText.c_str());

    return true;
}

bool CMainDlg::RefreshWindow(HWND hwndWindowToBeRefreshed)
{
    InvalidateRect(hwndWindowToBeRefreshed, nullptr, TRUE);
    UpdateWindow(hwndWindowToBeRefreshed);
    RedrawWindow(hwndWindowToBeRefreshed, nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);

    return true;
}

bool CMainDlg::HighlightFoundWindow(HWND hwndFoundWindow) const
{
    HDC  hWindowDC = nullptr; // The DC of the found window.
    RECT rect{};              // Rectangle area of the found window.

    GetWindowRect(hwndFoundWindow, &rect);
    hWindowDC = GetWindowDC(hwndFoundWindow);

    if (hWindowDC)
    {
        HGDIOBJ hPrevPen   = SelectObject(hWindowDC, m_hRectanglePen);              // Handle of the existing pen in the DC of the found window.
        HGDIOBJ hPrevBrush = SelectObject(hWindowDC, GetStockObject(HOLLOW_BRUSH)); // Handle of the existing brush in the DC of the found window.

        // Draw a rectangle in the DC covering the entire window area of the found window.
        Rectangle(hWindowDC, 1, 1, rect.right - rect.left - 2, rect.bottom - rect.top - 2);

        SelectObject(hWindowDC, hPrevPen);
        SelectObject(hWindowDC, hPrevBrush);
        ReleaseDC(hwndFoundWindow, hWindowDC);
    }

    return true;
}

HWND CMainDlg::GetSelectedHandle()
{
    TCHAR buf[MAX_PATH]{};

    ::GetDlgItemText(*this, IDC_WINDOW, buf, _countof(buf));

    TCHAR* endptr = nullptr;
    return reinterpret_cast<HWND>(_tcstoui64(buf, &endptr, 0));
}

bool CMainDlg::SendPostMessage(UINT id)
{
    HWND hTargetWnd = GetSelectedHandle();
    if (hTargetWnd == nullptr)
        return false;
    SetDlgItemText(*this, IDC_RETVALUE, _T(""));

    UINT   msg    = 0;
    WPARAM wparam = 0;
    LPARAM lparam = 0;
    TCHAR  buf[MAX_PATH]{};
    TCHAR* endptr = nullptr;

    ::GetDlgItemText(*this, IDC_MESSAGE, buf, _countof(buf));
    msg = _tcstol(buf, &endptr, 0);
    if (msg == 0)
    {
        LRESULT selIndex = SendDlgItemMessage(*this, IDC_MESSAGE, CB_GETCURSEL, 0, 0);
        if (selIndex != CB_ERR)
            msg = static_cast<UINT>(SendDlgItemMessage(*this, IDC_MESSAGE, CB_GETITEMDATA, selIndex, 0));
    }
    if (msg == 0)
    {
        msg = WinMessages::Instance().ParseMsg(buf);
    }
    DWORD err = 0;
    if (msg == 0)
    {
        err = ::GetLastError();
    }

    if (!err)
    {
        ::GetDlgItemText(*this, IDC_WPARAM, buf, _countof(buf));
        wparam = static_cast<WPARAM>(_tcstol(buf, &endptr, 0));
        if (wparam == 0)
        {
            LRESULT selIndex = SendDlgItemMessage(*this, IDC_WPARAM, CB_GETCURSEL, 0, 0);
            if (selIndex != CB_ERR)
                wparam = SendDlgItemMessage(*this, IDC_WPARAM, CB_GETITEMDATA, selIndex, 0);
        }

        ::GetDlgItemText(*this, IDC_LPARAM, buf, _countof(buf));
        lparam = static_cast<LPARAM>(_tcstol(buf, &endptr, 0));
        if (lparam == 0)
        {
            LRESULT selIndex = SendDlgItemMessage(*this, IDC_LPARAM, CB_GETCURSEL, 0, 0);
            if (selIndex != CB_ERR)
                lparam = SendDlgItemMessage(*this, IDC_LPARAM, CB_GETITEMDATA, selIndex, 0);
        }

        struct // copy of POWERBROADCAST_SETTING with a DWORD 4-byte data.
        {
            GUID PowerSetting;
            DWORD DataLength;
            DWORD Data;
        } actualPowerSetting = { GUID_SESSION_DISPLAY_STATUS, 0, 0};
        if (wparam == PBT_POWERSETTINGCHANGE)
        {
            constexpr DWORD MONITOR_STATE_FLAG = 0x00010000;
            constexpr DWORD MONITOR_STATE_MASK = 0x00000003;

            if ((lparam & MONITOR_STATE_FLAG) == MONITOR_STATE_FLAG)
            {
                const auto desiredMonitorState = static_cast<MONITOR_DISPLAY_STATE>(lparam & MONITOR_STATE_MASK);
                actualPowerSetting.Data = static_cast<DWORD>(desiredMonitorState);
                actualPowerSetting.DataLength = sizeof(DWORD);
                lparam = reinterpret_cast<LPARAM>(&actualPowerSetting);
            }
            else
            {
                err = ERROR_INVALID_PARAMETER;
            }
        }

        if (err == ERROR_SUCCESS)
        {
            LRESULT res = 0;
            if (id == IDC_SENDMESSAGE)
            {
                res = SendMessage(hTargetWnd, msg, wparam, lparam);
            }
            else
            {
                res = PostMessage(hTargetWnd, msg, wparam, lparam);
            }
        _stprintf_s(buf, _countof(buf), _T("0x%08Ix (%Id)"), res, res);
            SetDlgItemText(*this, IDC_RETVALUE, buf);
        }

        err = GetLastError();
    }

    if (err)
    {
        LPVOID lpMsgBuf     = nullptr;
        LPVOID lpDisplayBuf = nullptr;

        if (FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr,
                err,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                reinterpret_cast<LPTSTR>(&lpMsgBuf),
                0,
                nullptr))
        {
            lpDisplayBuf = LocalAlloc(LMEM_ZEROINIT, (lstrlen(static_cast<LPCTSTR>(lpMsgBuf)) + 40LL) * sizeof(TCHAR));
            if (lpDisplayBuf)
            {
                _stprintf_s(static_cast<LPTSTR>(lpDisplayBuf), LocalSize(lpDisplayBuf) / sizeof(TCHAR), _T("error %lu: %s"), err, (LPTSTR)lpMsgBuf);
                SetDlgItemText(*this, IDC_ERROR, static_cast<LPCTSTR>(lpDisplayBuf));
                LocalFree(lpDisplayBuf);
            }

            LocalFree(lpMsgBuf);
        }
    }
    else
        SetDlgItemText(*this, IDC_ERROR, _T(""));

    return true;
}
