#pragma once

#include <windows.h>

inline constexpr double initialHeight = 4.0;
inline constexpr int defaultWindowWidth = 1600;
inline constexpr int defaultWindowHeight = 1200;

struct AppState
{
    double centerX = -0.75;
    double centerY = 0.0;
    double scale = 0.0;
    int maxIter = 50;

    int width = defaultWindowWidth;
    int height = defaultWindowHeight;

    HBITMAP hBitmap = nullptr;
    void* pixels = nullptr;
    int pitch = 0;
    BITMAPINFO bmi{};

    bool needRender = true;

    int rmin = 100;
    int rmax = 255;
    int gmin = 0;
    int gmax = 255;
    int bmin = 0;
    int bmax = 0;
};
