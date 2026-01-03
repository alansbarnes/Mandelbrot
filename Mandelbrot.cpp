// Simple Win32 Mandelbrot renderer
// Build with MSVC (x86/x64):
//   cl /EHsc /O2 mandelbrot.cpp /link gdi32.lib user32.lib
//
// Or with MinGW-w64 (use -municode to enable wide-character WinMain if needed):
//   g++ -O2 -std=c++17 mandelbrot.cpp -lgdi32 -luser32 -municode -o mandelbrot.exe
//
// Controls:
//   Mouse wheel - zoom in/out centered on mouse
//   Left-drag   - pan
//   R           - reset view
//   + / -       - increase/decrease max iterations
//   Esc / Close - exit

#include <windows.h>
#include <windowsx.h>
#include <stdint.h>
#include <math.h>
#include <string>

struct AppState {
    int width = 800;
    int height = 600;
    HBITMAP hBitmap = nullptr;
    void* pixels = nullptr; // pointer returned by CreateDIBSection
    BITMAPINFO bmi;
    double centerX = -0.75;
    double centerY = 0.0;
    double scale = 3.0 / 800.0; // complex units per pixel (initial)
    int maxIter = 900;
    bool dragging = false;
    POINT dragStart;
    double dragCenterX, dragCenterY;
    bool needRender = true;
} g_state;

static void CreateOrResizeBitmap(AppState& s, int w, int h) {
    if (s.hBitmap) {
        DeleteObject(s.hBitmap);
        s.hBitmap = nullptr;
        s.pixels = nullptr;
    }

    s.width = w;
    s.height = h;

    // Prepare BITMAPINFO for 32bpp (top-down)
    ZeroMemory(&s.bmi, sizeof(s.bmi));
    s.bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    s.bmi.bmiHeader.biWidth = w;
    s.bmi.bmiHeader.biHeight = -h; // top-down
    s.bmi.bmiHeader.biPlanes = 1;
    s.bmi.bmiHeader.biBitCount = 32;
    s.bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    s.hBitmap = CreateDIBSection(NULL, &s.bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    s.pixels = bits;
    s.needRender = true;
}

static void HSVtoRGB(double h, double s, double v, uint8_t& outR, uint8_t& outG, uint8_t& outB) {
    // h in [0,360)
    double c = v * s;
    double hh = h / 60.0;
    double x = c * (1 - fabs(fmod(hh, 2.0) - 1.0));
    double r = 0, g = 0, b = 0;
    if (0.0 <= hh && hh < 1.0) { r = c; g = x; b = 0; }
    else if (1.0 <= hh && hh < 2.0) { r = x; g = c; b = 0; }
    else if (2.0 <= hh && hh < 3.0) { r = 0; g = c; b = x; }
    else if (3.0 <= hh && hh < 4.0) { r = 0; g = x; b = c; }
    else if (4.0 <= hh && hh < 5.0) { r = x; g = 0; b = c; }
    else { r = c; g = 0; b = x; }
    double m = v - c;
    outR = (uint8_t)round((r + m) * 255.0);
    outG = (uint8_t)round((g + m) * 255.0);
    outB = (uint8_t)round((b + m) * 255.0);
}

static void RenderMandelbrot(AppState& s) {
    if (!s.pixels) return;
    const int w = s.width;
    const int h = s.height;
    const double cx = s.centerX;
    const double cy = s.centerY;
    const double scale = s.scale;
    const int maxIter = s.maxIter;

    uint32_t* buf = (uint32_t*)s.pixels;

    // For each pixel
    for (int y = 0; y < h; ++y) {
        double imag = cy + (y - h / 2.0) * scale;
        for (int x = 0; x < w; ++x) {
            double real = cx + (x - w / 2.0) * scale;

            double zx = 0.0, zy = 0.0;
            int iter = 0;
            double zx2 = 0.0, zy2 = 0.0;

            while (zx2 + zy2 <= 4.0 && iter < maxIter) {
                zy = 2.0 * zx * zy + imag;
                zx = zx2 - zy2 + real;
                zx2 = zx * zx;
                zy2 = zy * zy;
                ++iter;
            }

            uint8_t r = 0, g = 0, b = 0;
            if (iter >= maxIter) {
                // inside - black
                r = g = b = 0;
            }
            else {
                // smooth coloring
                double log_zn = log(zx2 + zy2) / 2.0;
                double nu = log(log_zn / log(2.0)) / log(2.0);
                double mu = iter + 1 - nu;
                double hue = fmod(360.0 * mu / maxIter + 360.0, 360.0); // wrap
                double sat = 1.0;
                double val = 1.0;
                HSVtoRGB(hue, sat, val, r, g, b);

                // optionally, reduce brightness for low iteration counts:
                double t = (double)iter / maxIter;
                // enhance contrast
                double brightness = 0.5 + 0.5 * t;
                r = (uint8_t)(r * brightness);
                g = (uint8_t)(g * brightness);
                b = (uint8_t)(b * brightness);
            }

            // For 32bpp DIB (BI_RGB) the memory order is generally B, G, R, [0]
            uint32_t pixel = (uint32_t)(b) | ((uint32_t)g << 8) | ((uint32_t)r << 16);
            buf[y * w + x] = pixel;
        }
    }
    s.needRender = false;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        CreateOrResizeBitmap(g_state, g_state.width, g_state.height);
        return 0;

    case WM_SIZE: {
        int w = LOWORD(lParam);
        int h = HIWORD(lParam);
        if (w > 0 && h > 0) {
            CreateOrResizeBitmap(g_state, w, h);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONDOWN: {
        g_state.dragging = true;
        g_state.dragStart.x = LOWORD(lParam);
        g_state.dragStart.y = HIWORD(lParam);
        g_state.dragCenterX = g_state.centerX;
        g_state.dragCenterY = g_state.centerY;
        SetCapture(hwnd);
        return 0;
    }

    case WM_LBUTTONUP: {
        g_state.dragging = false;
        ReleaseCapture();
        return 0;
    }

    case WM_MOUSEMOVE: {
        if (g_state.dragging) {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            int dx = x - g_state.dragStart.x;
            int dy = y - g_state.dragStart.y;
            g_state.centerX = g_state.dragCenterX - dx * g_state.scale;
            g_state.centerY = g_state.dragCenterY - dy * g_state.scale;
            g_state.needRender = true;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_MOUSEWHEEL: {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        POINT mp;
        mp.x = GET_X_LPARAM(lParam);
        mp.y = GET_Y_LPARAM(lParam);

        // Convert to client coordinates
        ScreenToClient(hwnd, &mp);

        double oldScale = g_state.scale;
        double factor = (delta > 0) ? 0.8 : 1.25;
        // Use exponential for smooth zoom
        double zoomFactor = pow(factor, abs(delta) / 120.0);
        double newScale = oldScale * zoomFactor;

        // Keep mouse point stable in world coords
        double worldX = g_state.centerX + (mp.x - g_state.width / 2.0) * oldScale;
        double worldY = g_state.centerY + (mp.y - g_state.height / 2.0) * oldScale;
        g_state.centerX = worldX - (mp.x - g_state.width / 2.0) * newScale;
        g_state.centerY = worldY - (mp.y - g_state.height / 2.0) * newScale;
        g_state.scale = newScale;

        g_state.needRender = true;
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_KEYDOWN: {
        if (wParam == 'R') {
            g_state.centerX = -0.75;
            g_state.centerY = 0.0;
            g_state.scale = 3.0 / 800.0;
            g_state.needRender = true;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        else if (wParam == VK_ESCAPE) {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
        }
        else if (wParam == VK_ADD || wParam == VK_OEM_PLUS) {
            g_state.maxIter = (int)(g_state.maxIter * 1.25) + 10;
            if (g_state.maxIter > 5000) g_state.maxIter = 5000;
            g_state.needRender = true;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        else if (wParam == VK_SUBTRACT || wParam == VK_OEM_MINUS) {
            g_state.maxIter = (int)(g_state.maxIter * 0.8) - 10;
            if (g_state.maxIter < 10) g_state.maxIter = 10;
            g_state.needRender = true;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        if (g_state.needRender) {
            RenderMandelbrot(g_state);
        }

        if (g_state.hBitmap) {
            HDC memDC = CreateCompatibleDC(hdc);
            HGDIOBJ old = SelectObject(memDC, g_state.hBitmap);
            BitBlt(hdc, 0, 0, g_state.width, g_state.height, memDC, 0, 0, SRCCOPY);
            SelectObject(memDC, old);
            DeleteDC(memDC);
        }
        else {
            FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
        }

        // Draw simple overlay text
        {
            std::string info = "Center: " + std::to_string(g_state.centerX) + ", " + std::to_string(g_state.centerY)
                + "  Scale: " + std::to_string(g_state.scale) + "  Iter: " + std::to_string(g_state.maxIter);
            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, TRANSPARENT);
            RECT r = { 8, 8, g_state.width - 8, 40 };
            DrawTextA(hdc, info.c_str(), (int)info.size(), &r, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        if (g_state.hBitmap) DeleteObject(g_state.hBitmap);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASSEXA wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "MandelbrotWindowClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassExA(&wc)) {
        MessageBoxA(NULL, "RegisterClassEx failed", "Error", MB_ICONERROR);
        return 1;
    }

    RECT r = { 0, 0, g_state.width, g_state.height };
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowExA(0, wc.lpszClassName, "Mandelbrot Renderer", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top,
        NULL, NULL, hInstance, NULL);

    if (!hwnd) {
        MessageBoxA(NULL, "CreateWindowEx failed", "Error", MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Create initial bitmap sized to client area
    RECT client;
    GetClientRect(hwnd, &client);
    CreateOrResizeBitmap(g_state, client.right - client.left, client.bottom - client.top);

    // Main loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}