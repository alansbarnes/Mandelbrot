#include "PropertiesDlg.h"
#include "resource.h"
#include <string>
#include <cstdlib> // for atof

MandelbrotProperties g_props = {
    50,    // maxIter
    0.0,   // centerReal
    0.0,   // centerImag
    4.0,   // height
    100, 255, // rmin, rmax
    0, 255,   // gmin, gmax
    0, 0      // bmin, bmax
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
    return atof(buf);
}

static int GetDlgInt(HWND hDlg, int controlId)
{
    char buf[64];
    GetDlgItemTextA(hDlg, controlId, buf, (int)sizeof(buf));
    return atoi(buf);
}

INT_PTR CALLBACK PropertiesDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        // Initialize controls from g_props
        SetDlgItemInt(hDlg, IDC_MAX_ITER, g_props.maxIter, FALSE);
        SetDlgDouble(hDlg, IDC_CENTER_REAL, g_props.centerReal);
        SetDlgDouble(hDlg, IDC_CENTER_IMAG, g_props.centerImag);
        SetDlgDouble(hDlg, IDC_HEIGHT, g_props.height);
        SetDlgItemInt(hDlg, IDC_RED_MIN, g_props.rmin, FALSE);
        SetDlgItemInt(hDlg, IDC_RED_MAX, g_props.rmax, FALSE);
        SetDlgItemInt(hDlg, IDC_GREEN_MIN, g_props.gmin, FALSE);
        SetDlgItemInt(hDlg, IDC_GREEN_MAX, g_props.gmax, FALSE);
        SetDlgItemInt(hDlg, IDC_BLUE_MIN, g_props.bmin, FALSE);
        SetDlgItemInt(hDlg, IDC_BLUE_MAX, g_props.bmax, FALSE);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            // Read values back into g_props
            g_props.maxIter = GetDlgItemInt(hDlg, IDC_MAX_ITER, nullptr, FALSE);
            g_props.centerReal = GetDlgDouble(hDlg, IDC_CENTER_REAL);
            g_props.centerImag = GetDlgDouble(hDlg, IDC_CENTER_IMAG);
            g_props.height = GetDlgDouble(hDlg, IDC_HEIGHT);

            g_props.rmin = GetDlgInt(hDlg, IDC_RED_MIN);
            g_props.rmax = GetDlgInt(hDlg, IDC_RED_MAX);
            g_props.gmin = GetDlgInt(hDlg, IDC_GREEN_MIN);
            g_props.gmax = GetDlgInt(hDlg, IDC_GREEN_MAX);
            g_props.bmin = GetDlgInt(hDlg, IDC_BLUE_MIN);
            g_props.bmax = GetDlgInt(hDlg, IDC_BLUE_MAX);

            EndDialog(hDlg, IDOK);
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
