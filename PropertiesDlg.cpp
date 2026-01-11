#include "PropertiesDlg.h"
#include "resource.h"
#include <string>
#include <cstdlib> // for atof
#include <commctrl.h> // for NMUPDOWN

AppState g_state;

/*MandelbrotProperties g_props = {
    50,    // maxIter
    0.0,   // centerReal
    0.0,   // centerImag
    4.0,   // height
    100, 255, // rmin, rmax
    0, 255,   // gmin, gmax
    0, 0      // bmin, bmax
};*/

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
                UINT cur = GetDlgItemInt(hDlg, buddyId, &translated, FALSE);
                int newVal = (int)cur - p->iDelta; // invert direction
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
            // Read values back into g_props
            g_state.maxIter = GetDlgItemInt(hDlg, IDC_MAX_ITER, nullptr, FALSE);
            g_state.centerX = GetDlgDouble(hDlg, IDC_CENTER_REAL);
            g_state.centerY = GetDlgDouble(hDlg, IDC_CENTER_IMAG);
            g_state.height = (int)(GetDlgDouble(hDlg, IDC_HEIGHT) / g_state.scale);
            g_state.rmin = GetDlgInt(hDlg, IDC_RED_MIN);
            g_state.rmax = GetDlgInt(hDlg, IDC_RED_MAX);
            g_state.gmin = GetDlgInt(hDlg, IDC_GREEN_MIN);
            g_state.gmax = GetDlgInt(hDlg, IDC_GREEN_MAX);
            g_state.bmin = GetDlgInt(hDlg, IDC_BLUE_MIN);
            g_state.bmax = GetDlgInt(hDlg, IDC_BLUE_MAX);

            EndDialog(hDlg, IDOK);

            ApplySelectionToWindow(GetParent(hDlg));
            g_state.needRender = TRUE;
            InvalidateRect(GetParent(hDlg), nullptr, FALSE);
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
