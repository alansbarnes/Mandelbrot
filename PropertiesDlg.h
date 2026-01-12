#pragma once
#pragma once
#include <windows.h>

struct AppState
{
    //    int width = 800;
    //    int height = 600;
    int width = 1600;
    int height = 1200;
    HBITMAP hBitmap = nullptr;
    void* pixels = nullptr; // pointer returned by CreateDIBSection

    // Color ramp bounds
    int rmin, rmax;
    int gmin, gmax;
    int bmin, bmax;

    BITMAPINFO bmi;

    // world/view
    double centerX = -0.75;
    double centerY = 0.0;
    double scale = 3.0 / 800.0; // complex units per pixel (initial)
    //    int maxIter = 900;
    int maxIter = 50;

    // render state
    bool dragging = false;
    POINT dragStart;
    double dragCenterX, dragCenterY;
    bool needRender = true;

    // Selection/right-drag support:
    bool selecting = false;   // currently dragging right-button
    POINT selStart;           // selection start in client coords
    RECT selRect = { 0,0,0,0 };  // normalized selection rect (client coords)
    bool hasSelection = false; // whether a selection exists to act on

    // row stride (bytes per scanline). 0 if no bitmap.
    int pitch = 0;

    // Ownership: whether this AppState was heap-allocated (for new windows)
    bool owned = false;
};

INT_PTR CALLBACK PropertiesDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void ApplySelectionToWindow(HWND hwnd);
