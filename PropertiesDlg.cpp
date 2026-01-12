#include "PropertiesDlg.h"
#include "resource.h"
#include <string>
#include <cstdlib> // for atof
#include <commctrl.h> // for NMUPDOWN

AppState g_state;

static void SetDlgDouble(HWND hDlg, int controlId, double value)
{
    char buf[64];
    sprintf_s(buf, "%g", value);
    SetDlgItemTextA(hDlg, controlId, buf);
}

static double GetDlgDouble(HWND hDlg, int controlId)
{
    char buf[64];
    GetDlgItemTextA(hDlg, controlId, buf, (int)sizeof(buf));
    return atof(buf);
}

static int GetDlgInt(HWND hDlg, int controlId)
{
    char buf[64];
    GetDlgItemTextA(hDlg, controlId, buf, (int)sizeof(buf));
    return atoi(buf);
}

static int MapSpinToBuddy(int spinId)
{
    switch (spinId)
    {
    case IDC_MAX_ITER_SPIN:     return IDC_MAX_ITER;
    case IDC_RED_MIN_SPIN:      return IDC_RED_MIN;
    case IDC_RED_MAX_SPIN:      return IDC_RED_MAX;
    case IDC_GREEN_MIN_SPIN:    return IDC_GREEN_MIN;
    case IDC_GREEN_MAX_SPIN:    return IDC_GREEN_MAX;
    case IDC_BLUE_MIN_SPIN:     return IDC_BLUE_MIN;
    case IDC_BLUE_MAX_SPIN:     return IDC_BLUE_MAX;
    default: return 0;
    }
}

INT_PTR CALLBACK PropertiesDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        // Initialize controls from g_props
        SetDlgItemInt(hDlg, IDC_MAX_ITER, g_state.maxIter, FALSE);
        SetDlgDouble(hDlg, IDC_CENTER_REAL, g_state.centerX);
        SetDlgDouble(hDlg, IDC_CENTER_IMAG, g_state.centerY);
        SetDlgDouble(hDlg, IDC_HEIGHT, g_state.height * g_state.scale);
        SetDlgItemInt(hDlg, IDC_RED_MIN, g_state.rmin, FALSE);
        SetDlgItemInt(hDlg, IDC_RED_MAX, g_state.rmax, FALSE);
        SetDlgItemInt(hDlg, IDC_GREEN_MIN, g_state.gmin, FALSE);
        SetDlgItemInt(hDlg, IDC_GREEN_MAX, g_state.gmax, FALSE);
        SetDlgItemInt(hDlg, IDC_BLUE_MIN, g_state.bmin, FALSE);
        SetDlgItemInt(hDlg, IDC_BLUE_MAX, g_state.bmax, FALSE);
        return (INT_PTR)TRUE;

    case WM_NOTIFY:
    {
        LPNMHDR hdr = (LPNMHDR)lParam;
        if (hdr && hdr->code == UDN_DELTAPOS)
        {
            LPNMUPDOWN p = (LPNMUPDOWN)lParam;
            int spinId = (int)hdr->idFrom;
            int buddyId = MapSpinToBuddy(spinId);
            if (buddyId != 0)
            {
                // Read current integer value, apply the inverted delta and write it back.
                // Invert the requested delta so arrow keys/arrows operate in the opposite direction.
                BOOL translated = FALSE;
                UINT cur = GetDlgInt(hDlg, buddyId);
                int newVal = (int)cur - p->iDelta; // invert direction

                switch (buddyId)
                {
                    case IDC_MAX_ITER:
                    {
                        newVal = max(0, newVal);
                        break;
                    }
                    case IDC_RED_MIN:
                    {
                        newVal = max(0, newVal);
                        newVal = min(newVal, GetDlgInt(hDlg, IDC_RED_MAX));
                        break;
                    }
                    case IDC_RED_MAX:
                    {
                        newVal = min(newVal, 255);
                        newVal = max(GetDlgInt(hDlg, IDC_RED_MIN), newVal);
                        break;
                    }
                    case IDC_GREEN_MIN:
                    {
                        newVal = max(0, newVal);
                        newVal = min(newVal, GetDlgInt(hDlg, IDC_GREEN_MAX));
                        break;
                    }
                    case IDC_GREEN_MAX:
                    {
                        newVal = min(newVal, 255);
                        newVal = max(GetDlgInt(hDlg, IDC_GREEN_MIN), newVal);
                        break;
                    }
                    case IDC_BLUE_MIN:
                    {
                        newVal = max(0, newVal);
                        newVal = min(newVal, GetDlgInt(hDlg, IDC_BLUE_MAX));
                        break;
                    }
                    case IDC_BLUE_MAX:
                    {
                        newVal = min(newVal, 255);
                        newVal = max(GetDlgInt(hDlg, IDC_BLUE_MIN), newVal);
                        break;
                    }
                }

                SetDlgItemInt(hDlg, buddyId, newVal, FALSE);

                // Indicate we've handled the notification so the control won't also
                // do its automatic buddy update (we removed UDS_SETBUDDYINT to be safe).
                return (INT_PTR)TRUE;
            }
        }
        break;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            HWND hwndMain = GetParent(hDlg);
            wchar_t buf[128] = { 0 };

            GetDlgItemTextW(hDlg, IDC_HEIGHT, buf, _countof(buf));
            wchar_t* endptr = nullptr;

            // Convert to double; wcstod ignores leading whitespace and understands locale decimal separator for C locale;
            // If you want locale-aware for user's locale, you can use _wtof or set locale accordingly.
            double enteredHeight = wcstod(buf, &endptr);
            if (endptr == buf || enteredHeight <= 0.0 || enteredHeight > 1e7)
            {
                MessageBoxW(hDlg, L"Please enter a valid positive height.", L"Invalid value", MB_OK | MB_ICONERROR);
                return TRUE; // keep dialog open
            }

            // User supplied a world-space height (complex-plane units) and wants the current client area to represent that height.
            // Compute new scale = enteredHeight / clientPixelHeight
            double newScale = enteredHeight / static_cast<double>(g_state.height);
            if (!(newScale > 0.0))
            {
                MessageBoxW(hDlg, L"Computed scale invalid.", L"Error", MB_OK | MB_ICONERROR);
                return TRUE;
            }

            g_state.scale = newScale;
            g_state.needRender = true;

            InvalidateRect(hwndMain, NULL, FALSE);
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
    }
    return (INT_PTR)FALSE;
}
