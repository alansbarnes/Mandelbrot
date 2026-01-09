#pragma once
#pragma once
#include <windows.h>

INT_PTR CALLBACK PropertiesDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

// Structure to hold properties (example)
struct MandelbrotProperties {
    int maxIter;
    double centerReal;
    double centerImag;
    double height;
    int rmin, rmax;
    int gmin, gmax;
    int bmin, bmax;
};

extern MandelbrotProperties g_props;
