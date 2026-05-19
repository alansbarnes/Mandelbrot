#pragma once

#include <windows.h>
#include <cstdint>
#include "resource.h"

inline constexpr double initialHeight = 4.0;

struct Properties
{
    int maxIter = 50;
    double centerReal = -0.75;
    double centerImag = 0.0;
    double height = initialHeight;

    int rmin = 100;
    int rmax = 255;
    int gmin = 0;
    int gmax = 255;
    int bmin = 0;
    int bmax = 0;
};

struct AppState
{
    double centerX = -0.75;
    double centerY = 0.0;
    double scale = 0.0;
    int maxIter = 50;

    int width = 1600;
    int height = 1200;

    HBITMAP hBitmap = nullptr;
    void* pixels = nullptr;
    int pitch = 0;
    BITMAPINFO bmi{};

    bool needRender = true;
    bool owned = false;

    bool dragging = false;
    POINT dragStart{};
    double dragCenterX = -0.75;
    double dragCenterY = 0.0;

    bool selecting = false;
    bool hasSelection = false;
    POINT selStart{};
    RECT selRect{ 0, 0, 0, 0 };

    int rmin = 100;
    int rmax = 255;
    int gmin = 0;
    int gmax = 255;
    int bmin = 0;
    int bmax = 0;
};

INT_PTR CALLBACK PropertiesDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

extern AppState g_state;
extern Properties g_props;