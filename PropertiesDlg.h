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

// Properties exposed by the Properties dialog (used to transfer dialog values)
struct Properties
{
    int maxIter;

    // center (real + imag)
    double centerReal;
    double centerImag;

    // height in world units (dialog shows Height), used to compute scale = height / window_height
    double height;

    // color ramp bounds
    int rmin, rmax;
    int gmin, gmax;
    int bmin, bmax;
};

// Global instance that the dialog updates and other modules can read
extern Properties g_props;

INT_PTR CALLBACK PropertiesDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void ApplySelectionToWindow(HWND hwnd);
