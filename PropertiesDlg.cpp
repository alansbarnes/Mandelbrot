#include "PropertiesDlg.h"
#include "resource.h"
#include <string>
#include <cstdlib> // for atof
#include <commctrl.h> // for NMUPDOWN

AppState g_state;

// Define the properties object declared extern in PropertiesDlg.h.
// Initialize with values consistent with g_state defaults so the dialog has sane defaults.
Properties g_props = {
    g_state.maxIter,                 // maxIter (50)
    g_state.centerX,                 // centerReal (-0.75)
    g_state.centerY,                 // centerImag (0.0)
    g_state.height * g_state.scale,  // height in world units (1200 * 3/800 = 4.5)
    100, 255,                        // rmin, rmax
    0, 255,                          // gmin, gmax
    0, 0                             // bmin, bmax
};

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
    return std::stod(buf);
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
        // Initialize controls from g_state (existing behavior)
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
                    newVal = min(255, newVal);
                    newVal = max(newVal, GetDlgInt(hDlg, IDC_RED_MIN));
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
                    newVal = min(255, newVal);
                    newVal = max(newVal, GetDlgInt(hDlg, IDC_GREEN_MIN));
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
                    newVal = min(255, newVal);
                    newVal = max(newVal, GetDlgInt(hDlg, IDC_BLUE_MIN));
                    break;
                }
                }

                SetDlgItemInt(hDlg, buddyId, newVal, FALSE);
                return (INT_PTR)TRUE;
            }
        }
        break;
    }

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDOK:
        {
            // Read current control values into g_props
            g_props.maxIter = GetDlgInt(hDlg, IDC_MAX_ITER);
            g_props.centerReal = GetDlgDouble(hDlg, IDC_CENTER_REAL);
            g_props.centerImag = GetDlgDouble(hDlg, IDC_CENTER_IMAG);
            g_props.height = GetDlgDouble(hDlg, IDC_HEIGHT);
            g_props.rmin = GetDlgInt(hDlg, IDC_RED_MIN);
            g_props.rmax = GetDlgInt(hDlg, IDC_RED_MAX);
            g_props.gmin = GetDlgInt(hDlg, IDC_GREEN_MIN);
            g_props.gmax = GetDlgInt(hDlg, IDC_GREEN_MAX);
            g_props.bmin = GetDlgInt(hDlg, IDC_BLUE_MIN);
            g_props.bmax = GetDlgInt(hDlg, IDC_BLUE_MAX);

            // Sanitize/limit as appropriate
//          if (g_props.maxIter < 1) g_props.maxIter = 1;
//          if (g_props.maxIter > 50000) g_props.maxIter = 50000; // arbitrary safety cap

            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        break;
    }

    // other message handling omitted for brevity...
    }

    // Ensure every control path returns a value to satisfy the compiler
    return (INT_PTR)FALSE;
}
