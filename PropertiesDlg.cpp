#include "framework.h"
#include "PropertiesDlg.h"
#include "resource.h"

#include <windows.h>
#include <commctrl.h>
#include <cwchar>

#pragma comment(lib, "comctl32.lib")

namespace
{
    constexpr int kColorMin = 0;
    constexpr int kColorMax = 255;

    bool g_internalUpdate = false;

    struct ScopedInternalUpdate
    {
        ScopedInternalUpdate() { g_internalUpdate = true; }
        ~ScopedInternalUpdate() { g_internalUpdate = false; }
    };

    void SetDlgItemIntValue(HWND hwnd, int id, int value)
    {
        SetDlgItemInt(hwnd, id, static_cast<UINT>(value), TRUE);
    }

    void SetDlgItemDoubleValue(HWND hwnd, int id, double value)
    {
        wchar_t buffer[64]{};
        swprintf_s(buffer, L"%.17g", value);
        SetDlgItemTextW(hwnd, id, buffer);
    }

    bool TryGetDlgItemIntValue(HWND hwnd, int id, int& value, int minValue, int maxValue)
    {
        BOOL translated = FALSE;
        const UINT rawValue = GetDlgItemInt(hwnd, id, &translated, TRUE);
        if (!translated)
        {
            return false;
        }

        const int parsedValue = static_cast<int>(rawValue);
        if (parsedValue < minValue || parsedValue > maxValue)
        {
            return false;
        }

        value = parsedValue;
        return true;
    }

    bool TryGetDlgItemDoubleValue(HWND hwnd, int id, double& value)
    {
        wchar_t buffer[128]{};
        if (GetDlgItemTextW(hwnd, id, buffer, static_cast<int>(sizeof(buffer) / sizeof(buffer[0]))) <= 0)
        {
            return false;
        }

        wchar_t* end = nullptr;
        const double parsedValue = wcstod(buffer, &end);
        if (end == buffer)
        {
            return false;
        }

        while (*end != L'\0')
        {
            if (*end != L' ' && *end != L'\t' && *end != L'\r' && *end != L'\n')
            {
                return false;
            }
            ++end;
        }

        value = parsedValue;
        return true;
    }

    int ClampInt(int value, int minValue, int maxValue)
    {
        if (value < minValue) return minValue;
        if (value > maxValue) return maxValue;
        return value;
    }

    int GetDlgItemIntOrDefault(HWND hwnd, int id, int defaultValue)
    {
        BOOL translated = FALSE;
        const UINT rawValue = GetDlgItemInt(hwnd, id, &translated, TRUE);
        if (!translated)
        {
            return defaultValue;
        }
        return static_cast<int>(rawValue);
    }

    void InitSpinControl(HWND hwnd, int spinId, int buddyEditId, int minValue, int maxValue)
    {
        HWND spin = GetDlgItem(hwnd, spinId);
        HWND buddy = GetDlgItem(hwnd, buddyEditId);
        if (spin != nullptr)
        {
            if (buddy != nullptr)
            {
                SendMessageW(spin, UDM_SETBUDDY, reinterpret_cast<WPARAM>(buddy), 0);
            }
            SendMessageW(spin, UDM_SETRANGE32, static_cast<WPARAM>(minValue), static_cast<LPARAM>(maxValue));
        }
    }

    void SyncMinMaxPair(HWND hwnd, int minEditId, int maxEditId)
    {
        int minValue = ClampInt(GetDlgItemIntOrDefault(hwnd, minEditId, kColorMin), kColorMin, kColorMax);
        int maxValue = ClampInt(GetDlgItemIntOrDefault(hwnd, maxEditId, kColorMax), kColorMin, kColorMax);

        if (minValue > maxValue)
        {
            maxValue = minValue;
        }

        ScopedInternalUpdate guard;
        SetDlgItemIntValue(hwnd, minEditId, minValue);
        SetDlgItemIntValue(hwnd, maxEditId, maxValue);
    }

    void SyncMaxMinPair(HWND hwnd, int minEditId, int maxEditId)
    {
        int minValue = ClampInt(GetDlgItemIntOrDefault(hwnd, minEditId, kColorMin), kColorMin, kColorMax);
        int maxValue = ClampInt(GetDlgItemIntOrDefault(hwnd, maxEditId, kColorMax), kColorMin, kColorMax);

        if (maxValue < minValue)
        {
            minValue = maxValue;
        }

        ScopedInternalUpdate guard;
        SetDlgItemIntValue(hwnd, minEditId, minValue);
        SetDlgItemIntValue(hwnd, maxEditId, maxValue);
    }

    bool HandleSpinDelta(HWND hwnd, NMUPDOWN* updown)
    {
        int buddyId = 0;
        int pairedId = 0;
        bool isMinControl = false;

        switch (updown->hdr.idFrom)
        {
        case IDC_SPIN_RMIN:
            buddyId = IDC_EDIT_RMIN;
            pairedId = IDC_EDIT_RMAX;
            isMinControl = true;
            break;
        case IDC_SPIN_RMAX:
            buddyId = IDC_EDIT_RMAX;
            pairedId = IDC_EDIT_RMIN;
            isMinControl = false;
            break;
        case IDC_SPIN_GMIN:
            buddyId = IDC_EDIT_GMIN;
            pairedId = IDC_EDIT_GMAX;
            isMinControl = true;
            break;
        case IDC_SPIN_GMAX:
            buddyId = IDC_EDIT_GMAX;
            pairedId = IDC_EDIT_GMIN;
            isMinControl = false;
            break;
        case IDC_SPIN_BMIN:
            buddyId = IDC_EDIT_BMIN;
            pairedId = IDC_EDIT_BMAX;
            isMinControl = true;
            break;
        case IDC_SPIN_BMAX:
            buddyId = IDC_EDIT_BMAX;
            pairedId = IDC_EDIT_BMIN;
            isMinControl = false;
            break;
        default:
            return false;
        }

        int newValue = updown->iPos + updown->iDelta;
        newValue = ClampInt(newValue, kColorMin, kColorMax);

        {
            ScopedInternalUpdate guard;
            SetDlgItemIntValue(hwnd, buddyId, newValue);

            if (isMinControl)
            {
                int maxValue = ClampInt(GetDlgItemIntOrDefault(hwnd, pairedId, kColorMax), kColorMin, kColorMax);
                if (newValue > maxValue)
                {
                    SetDlgItemIntValue(hwnd, pairedId, newValue);
                }
            }
            else
            {
                int minValue = ClampInt(GetDlgItemIntOrDefault(hwnd, pairedId, kColorMin), kColorMin, kColorMax);
                if (newValue < minValue)
                {
                    SetDlgItemIntValue(hwnd, pairedId, newValue);
                }
            }
        }

        return true;
    }

    void LoadPropertiesIntoDialog(HWND hwnd)
    {
        ScopedInternalUpdate guard;

        SetDlgItemIntValue(hwnd, IDC_EDIT_MAX_ITER, g_props.maxIter);

        SetDlgItemDoubleValue(hwnd, IDC_EDIT_CENTER_REAL, g_props.centerReal);
        SetDlgItemDoubleValue(hwnd, IDC_EDIT_CENTER_IMAG, g_props.centerImag);
        SetDlgItemDoubleValue(hwnd, IDC_EDIT_HEIGHT, g_props.height);

        SetDlgItemIntValue(hwnd, IDC_EDIT_RMIN, ClampInt(g_props.rmin, kColorMin, kColorMax));
        SetDlgItemIntValue(hwnd, IDC_EDIT_RMAX, ClampInt(g_props.rmax, kColorMin, kColorMax));
        SetDlgItemIntValue(hwnd, IDC_EDIT_GMIN, ClampInt(g_props.gmin, kColorMin, kColorMax));
        SetDlgItemIntValue(hwnd, IDC_EDIT_GMAX, ClampInt(g_props.gmax, kColorMin, kColorMax));
        SetDlgItemIntValue(hwnd, IDC_EDIT_BMIN, ClampInt(g_props.bmin, kColorMin, kColorMax));
        SetDlgItemIntValue(hwnd, IDC_EDIT_BMAX, ClampInt(g_props.bmax, kColorMin, kColorMax));
    }

    bool SaveDialogIntoProperties(HWND hwnd)
    {
        Properties updated{};

        if (!TryGetDlgItemIntValue(hwnd, IDC_EDIT_MAX_ITER, updated.maxIter, 1, 100000))
        {
            MessageBoxW(hwnd,
                L"Maximum Iteration must be an integer between 1 and 100000.",
                L"Invalid Input",
                MB_OK | MB_ICONERROR);
            SetFocus(GetDlgItem(hwnd, IDC_EDIT_MAX_ITER));
            return false;
        }

        if (!TryGetDlgItemDoubleValue(hwnd, IDC_EDIT_CENTER_REAL, updated.centerReal))
        {
            MessageBoxW(hwnd,
                L"Center real value is invalid.",
                L"Invalid Input",
                MB_OK | MB_ICONERROR);
            SetFocus(GetDlgItem(hwnd, IDC_EDIT_CENTER_REAL));
            return false;
        }

        if (!TryGetDlgItemDoubleValue(hwnd, IDC_EDIT_CENTER_IMAG, updated.centerImag))
        {
            MessageBoxW(hwnd,
                L"Center imaginary value is invalid.",
                L"Invalid Input",
                MB_OK | MB_ICONERROR);
            SetFocus(GetDlgItem(hwnd, IDC_EDIT_CENTER_IMAG));
            return false;
        }

        if (!TryGetDlgItemDoubleValue(hwnd, IDC_EDIT_HEIGHT, updated.height) || updated.height <= 0.0)
        {
            MessageBoxW(hwnd,
                L"Height must be a positive number.",
                L"Invalid Input",
                MB_OK | MB_ICONERROR);
            SetFocus(GetDlgItem(hwnd, IDC_EDIT_HEIGHT));
            return false;
        }

        SyncMinMaxPair(hwnd, IDC_EDIT_RMIN, IDC_EDIT_RMAX);
        SyncMinMaxPair(hwnd, IDC_EDIT_GMIN, IDC_EDIT_GMAX);
        SyncMinMaxPair(hwnd, IDC_EDIT_BMIN, IDC_EDIT_BMAX);

        if (!TryGetDlgItemIntValue(hwnd, IDC_EDIT_RMIN, updated.rmin, kColorMin, kColorMax) ||
            !TryGetDlgItemIntValue(hwnd, IDC_EDIT_RMAX, updated.rmax, kColorMin, kColorMax) ||
            !TryGetDlgItemIntValue(hwnd, IDC_EDIT_GMIN, updated.gmin, kColorMin, kColorMax) ||
            !TryGetDlgItemIntValue(hwnd, IDC_EDIT_GMAX, updated.gmax, kColorMin, kColorMax) ||
            !TryGetDlgItemIntValue(hwnd, IDC_EDIT_BMIN, updated.bmin, kColorMin, kColorMax) ||
            !TryGetDlgItemIntValue(hwnd, IDC_EDIT_BMAX, updated.bmax, kColorMin, kColorMax))
        {
            MessageBoxW(hwnd,
                L"Color values must be integers between 0 and 255.",
                L"Invalid Input",
                MB_OK | MB_ICONERROR);
            return false;
        }

        g_props = updated;
        return true;
    }
}

INT_PTR CALLBACK PropertiesDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (msg)
    {
    case WM_INITDIALOG:
        InitSpinControl(hwnd, IDC_SPIN_RMIN, IDC_EDIT_RMIN, kColorMin, kColorMax);
        InitSpinControl(hwnd, IDC_SPIN_RMAX, IDC_EDIT_RMAX, kColorMin, kColorMax);
        InitSpinControl(hwnd, IDC_SPIN_GMIN, IDC_EDIT_GMIN, kColorMin, kColorMax);
        InitSpinControl(hwnd, IDC_SPIN_GMAX, IDC_EDIT_GMAX, kColorMin, kColorMax);
        InitSpinControl(hwnd, IDC_SPIN_BMIN, IDC_EDIT_BMIN, kColorMin, kColorMax);
        InitSpinControl(hwnd, IDC_SPIN_BMAX, IDC_EDIT_BMAX, kColorMin, kColorMax);

        LoadPropertiesIntoDialog(hwnd);

        SyncMinMaxPair(hwnd, IDC_EDIT_RMIN, IDC_EDIT_RMAX);
        SyncMinMaxPair(hwnd, IDC_EDIT_GMIN, IDC_EDIT_GMAX);
        SyncMinMaxPair(hwnd, IDC_EDIT_BMIN, IDC_EDIT_BMAX);
        return TRUE;

    case WM_NOTIFY:
    {
        LPNMHDR hdr = reinterpret_cast<LPNMHDR>(lParam);
        if (hdr != nullptr && hdr->code == UDN_DELTAPOS)
        {
            NMUPDOWN* updown = reinterpret_cast<NMUPDOWN*>(lParam);
            if (HandleSpinDelta(hwnd, updown))
            {
                return TRUE;
            }
        }
        break;
    }

    case WM_COMMAND:
        if (g_internalUpdate)
        {
            return TRUE;
        }

        switch (LOWORD(wParam))
        {
        case IDC_EDIT_RMIN:
        case IDC_EDIT_GMIN:
        case IDC_EDIT_BMIN:
            if (HIWORD(wParam) == EN_CHANGE && lParam != 0)
            {
                switch (LOWORD(wParam))
                {
                case IDC_EDIT_RMIN: SyncMinMaxPair(hwnd, IDC_EDIT_RMIN, IDC_EDIT_RMAX); break;
                case IDC_EDIT_GMIN: SyncMinMaxPair(hwnd, IDC_EDIT_GMIN, IDC_EDIT_GMAX); break;
                case IDC_EDIT_BMIN: SyncMinMaxPair(hwnd, IDC_EDIT_BMIN, IDC_EDIT_BMAX); break;
                }
            }
            return TRUE;

        case IDC_EDIT_RMAX:
        case IDC_EDIT_GMAX:
        case IDC_EDIT_BMAX:
            if (HIWORD(wParam) == EN_CHANGE && lParam != 0)
            {
                switch (LOWORD(wParam))
                {
                case IDC_EDIT_RMAX: SyncMaxMinPair(hwnd, IDC_EDIT_RMIN, IDC_EDIT_RMAX); break;
                case IDC_EDIT_GMAX: SyncMaxMinPair(hwnd, IDC_EDIT_GMIN, IDC_EDIT_GMAX); break;
                case IDC_EDIT_BMAX: SyncMaxMinPair(hwnd, IDC_EDIT_BMIN, IDC_EDIT_BMAX); break;
                }
            }
            return TRUE;

        case IDOK:
            if (SaveDialogIntoProperties(hwnd))
            {
                EndDialog(hwnd, IDOK);
            }
            return TRUE;

        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            return TRUE;
        }
        break;
    }

    return FALSE;
}