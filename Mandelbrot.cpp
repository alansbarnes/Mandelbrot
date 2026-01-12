// Simple Win32 Mandelbrot renderer
// Build with MSVC (x86/x64):
//   cl /EHsc /O2 mandelbrot.cpp /link gdi32.lib user32.lib
//
// Or with MinGW-w64 (use -municode to enable wide-character WinMain if needed):
//   g++ -O2 -std=c++17 mandelbrot.cpp -lgdi32 -luser32 -municode -o mandelbrot.exe
//
// Controls:
//   Mouse wheel - zoom in/out centered on mouse
//   Left-drag   - pan (click-and-drag)
//   Right-drag  - select a rectangle (selection is shown as an overlay)
//   Left-click inside the selection - open a new window framed on that selection
//   R           - reset view
//   + / -       - increase/decrease max iterations
//   Esc / Close - exit

#include "PropertiesDlg.h"
#include "resource.h"

#include <windows.h>
#include <windowsx.h>
#include <stdint.h>
#include <math.h>
#include <string>

extern AppState g_state;

// Menu command IDs
#define ID_FILE_EXIT    9001
#define ID_VIEW_RESET   9002
#define ID_ITER_INC     9003
#define ID_ITER_DEC     9004
#define ID_HELP_ABOUT   9005

static inline double PixelToWorldX(int px)
{
    return g_state.centerX + (px - (g_state.width / 2.0)) * g_state.scale;
}

static inline double PixelToWorldY(int py)
{
    // Note the '-' here: pixel y grows downward, world imaginary should grow upward.
    return g_state.centerY - (py - (g_state.height / 2.0)) * g_state.scale;
}

static void NormalizeRect(RECT& r)
{
    if (r.left > r.right) std::swap(r.left, r.right);
    if (r.top > r.bottom) std::swap(r.top, r.bottom);
}

static bool PointInRectEx(const RECT& r, int x, int y)
{
    return (x >= r.left && x <= r.right && y >= r.top && y <= r.bottom);
}

static void CreateOrResizeBitmap(int w, int h)
{
    if (g_state.hBitmap)
    {
        DeleteObject(g_state.hBitmap);
        g_state.hBitmap = nullptr;
        g_state.pixels = nullptr;
        g_state.pitch = 0; // clear pitch when freeing existing bitmap
    }

    g_state.width = w;
    g_state.height = h;

    // Prepare BITMAPINFO for 32bpp (top-down)
    ZeroMemory(&g_state.bmi, sizeof(g_state.bmi));
    g_state.bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    g_state.bmi.bmiHeader.biWidth = w;
    g_state.bmi.bmiHeader.biHeight = -h; // negative = top-down DIB
    g_state.bmi.bmiHeader.biPlanes = 1;
    g_state.bmi.bmiHeader.biBitCount = 32;
    g_state.bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    g_state.hBitmap = CreateDIBSection(NULL, &g_state.bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    g_state.pixels = bits;

    // Compute pitch (bytes per scanline), DWORD-aligned:
    int bpp = g_state.bmi.bmiHeader.biBitCount;
    g_state.pitch = ((bpp * w + 31) / 32) * 4;

    g_state.needRender = true;
}

static void RenderMandelbrot()
{
    if (!g_state.pixels) return;
    const int w = g_state.width;
    const int h = g_state.height;
    const double cx = g_state.centerX;
    const double cy = g_state.centerY;
    const double scale = g_state.scale;
    const int maxIter = g_state.maxIter;

    uint32_t* buf = static_cast<uint32_t*>(g_state.pixels);

    const double halfW = w / 2.0;
    const double halfH = h / 2.0;

    // If your surface has a row stride (pitch) different from w*4, use it.
    // Example: size_t pitchPixels = g_state.pitch ? (g_state.pitch / 4) : (size_t)w;
    size_t pitchPixels = (g_state.pitch && g_state.pitch > 0) ? (g_state.pitch / sizeof(uint32_t)) : (size_t)w;

    for (int y = 0; y < h; ++y)
    {
        double imag = cy - (y - halfH) * scale;
        uint32_t* row = buf + (size_t)y * pitchPixels;
        for (int x = 0; x < w; ++x)
        {
            double real = cx + (x - halfW) * scale; // <-- fixed mapping

            double zx = 0.0, zy = 0.0;
            double zx2 = 0.0, zy2 = 0.0;
            int iter = 0;

            while (zx2 + zy2 <= 4.0 && iter < maxIter)
            {
                zy = 2.0 * zx * zy + imag;
                zx = zx2 - zy2 + real;
                zx2 = zx * zx;
                zy2 = zy * zy;
                ++iter;
            }

            uint8_t r = 0, g = 0, b = 0;
            if (iter >= maxIter)
            {
                // inside - black
                r = g = b = 0;
            }
            else
            {
                r = (uint8_t)(g_state.rmin + (((g_state.rmax - g_state.rmin) * iter) / maxIter));
                g = (uint8_t)(g_state.gmin + (((g_state.gmax - g_state.gmin) * iter) / maxIter));
                b = (uint8_t)(g_state.bmin + (((g_state.bmax - g_state.bmin) * iter) / maxIter));
            }

            // memory order: B G R [A/0]
            uint32_t pixel = (uint32_t)b | ((uint32_t)g << 8) | ((uint32_t)r << 16);
            row[x] = pixel;
        }
    }
    g_state.needRender = false;
}

void ApplySelectionToWindow(HWND hwnd)
{
    if (!g_state.hasSelection) return;

    RECT sel = g_state.selRect;
    NormalizeRect(sel);
    int selW = sel.right - sel.left;
    int selH = sel.bottom - sel.top;
    if (selW <= 0 || selH <= 0) return;

    // selection center in client coordinates
    double selCenterPx = (sel.left + sel.right) / 2.0;
    double selCenterPy = (sel.top + sel.bottom) / 2.0;

    // world coords of selection center (use PixelToWorldY with the correct sign)
    double worldX = PixelToWorldX(static_cast<int>(selCenterPx + 0.5));
    double worldY = PixelToWorldY(static_cast<int>(selCenterPy + 0.5));

    // new scale: selected width in pixels maps to full window width
    double newScale = g_state.scale * (static_cast<double>(selW) / static_cast<double>(g_state.width));

    // apply to current AppState and re-render
    g_state.centerX = worldX;
    g_state.centerY = worldY;
    g_state.scale = newScale;
    g_state.selecting = false;
    g_state.hasSelection = false;
    g_state.needRender = true;

    InvalidateRect(hwnd, NULL, FALSE);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        g_state.rmin = 100;
        g_state.rmax = 255;
        g_state.gmin = 0;
        g_state.gmax = 255;
        g_state.bmin = 0;
        g_state.bmax = 0;

        // Create initial bitmap sized to client area
        RECT client;
        GetClientRect(hwnd, &client);
        CreateOrResizeBitmap((client.right - client.left), (client.bottom - client.top));
        return 0;
    }

    case WM_SIZE:
    {
        int w = LOWORD(lParam);
        int h = HIWORD(lParam);
        if (w > 0 && h > 0)
        {
            CreateOrResizeBitmap(w, h);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        switch (id)
        {
            case IDM_PROPERTIES:
            {
                INT_PTR res = DialogBox(nullptr, MAKEINTRESOURCE(IDD_PROPERTIES), hwnd, PropertiesDlgProc);
                if (res == IDOK)
                {
                    // apply values from dialog
                    extern Properties g_props;
                    g_state.maxIter = g_props.maxIter;
                    g_state.centerX = g_props.centerReal;
                    g_state.centerY = g_props.centerImag;
                    // convert dialog "Height (world units)" to scale (world units per pixel)
                    if (g_state.height > 0)
                        g_state.scale = g_props.height / (double)g_state.height;
                    g_state.rmin = g_props.rmin; g_state.rmax = g_props.rmax;
                    g_state.gmin = g_props.gmin; g_state.gmax = g_props.gmax;
                    g_state.bmin = g_props.bmin; g_state.bmax = g_props.bmax;

                    g_state.needRender = true;
                    InvalidateRect(hwnd, NULL, FALSE);
                }
                break;
            }
            case ID_FILE_EXIT:
                PostMessage(hwnd, WM_CLOSE, 0, 0);
                break;
            case ID_VIEW_RESET:
                g_state.centerX = -0.75;
                g_state.centerY = 0.0;
                g_state.scale = 3.0 / 800.0;
                g_state.needRender = true;
                InvalidateRect(hwnd, NULL, FALSE);
                break;
            case ID_ITER_INC:
                g_state.maxIter = static_cast<int>(g_state.maxIter * 1.25) + 10;
                if (g_state.maxIter > 5000) g_state.maxIter = 5000;
                g_state.needRender = true;
                InvalidateRect(hwnd, NULL, FALSE);
                break;
            case ID_ITER_DEC:
                g_state.maxIter = static_cast<int>(g_state.maxIter * 0.8) - 10;
                if (g_state.maxIter < 10) g_state.maxIter = 10;
                g_state.needRender = true;
                InvalidateRect(hwnd, NULL, FALSE);
                break;
            case ID_HELP_ABOUT:
                MessageBoxW(hwnd, L"Mandelbrot Renderer\n\nSimple Win32 Mandelbrot explorer", L"About", MB_OK | MB_ICONINFORMATION);
                break;
            }

        return 0;
    }

    case WM_LBUTTONDOWN:
    {
        int mx = LOWORD(lParam);
        int my = HIWORD(lParam);
        // If there's an existing selection and the click is inside it, apply that selection to this window.
        if (g_state.hasSelection)
        {
            RECT sel = g_state.selRect;
            NormalizeRect(sel);
            if (PointInRectEx(sel, mx, my))
            {
                ApplySelectionToWindow(hwnd);
                return 0;
            }
            else
            {
                // Clicking outside the selection cancels it.
                g_state.hasSelection = false;
                g_state.selRect = { 0, 0, 0, 0 };
                InvalidateRect(hwnd, NULL, FALSE);
                // fall through to start panning
            }
        }

        // Otherwise, start panning (original behavior)
        g_state.dragging = true;
        g_state.dragStart.x = mx;
        g_state.dragStart.y = my;
        g_state.dragCenterX = g_state.centerX;
        g_state.dragCenterY = g_state.centerY;
        SetCapture(hwnd);
        return 0;
    }

    case WM_LBUTTONUP:
    {
        g_state.dragging = false;
        ReleaseCapture();
        return 0;
    }

    case WM_MOUSEMOVE:
    {
        if (g_state.dragging)
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            int dx = x - g_state.dragStart.x;
            int dy = y - g_state.dragStart.y;
            g_state.centerX = g_state.dragCenterX - dx * g_state.scale;
            g_state.centerY = g_state.dragCenterY + dy * g_state.scale;
            g_state.needRender = true;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        else if (g_state.selecting)
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);

            int startX = g_state.selStart.x;
            int startY = g_state.selStart.y;
            int dx = x - startX;
            int dy = y - startY;
            int signX = (dx >= 0) ? 1 : -1;
            int signY = (dy >= 0) ? 1 : -1;
            int absDx = (dx >= 0) ? dx : -dx;
            int absDy = (dy >= 0) ? dy : -dy;

            // Keep selection aspect ratio equal to client area aspect ratio
            double aspect = (double)g_state.width / (double)g_state.height;
            double desiredW = 0.0, desiredH = 0.0;

            if (absDx == 0 && absDy == 0)
            {
                desiredW = desiredH = 0.0;
            }
            else if (absDy == 0)
            {
                // avoid divide-by-zero: base on width
                desiredW = absDx;
                desiredH = desiredW / aspect;
            }
            else if (absDx == 0)
            {
                desiredH = absDy;
                desiredW = desiredH * aspect;
            }
            else
            {
                if ((double)absDx / (double)absDy > aspect)
                {
                    // width is proportionally larger than allowed -> limit width to match height
                    desiredH = absDy;
                    desiredW = desiredH * aspect;
                }
                else
                {
                    // height is proportionally larger -> limit height to match width
                    desiredW = absDx;
                    desiredH = desiredW / aspect;
                }
            }

            int adjW = (int)(desiredW + 0.5);
            int adjH = (int)(desiredH + 0.5);

            g_state.selRect.left = startX;
            g_state.selRect.top = startY;
            g_state.selRect.right = startX + signX * adjW;
            g_state.selRect.bottom = startY + signY * adjH;
            NormalizeRect(g_state.selRect);
            InvalidateRect(hwnd, NULL, FALSE);
        }

        return 0;
    }

    case WM_RBUTTONDOWN:
    {
        g_state.selecting = true;
        g_state.selStart.x = LOWORD(lParam);
        g_state.selStart.y = HIWORD(lParam);
        g_state.selRect.left = g_state.selRect.right = g_state.selStart.x;
        g_state.selRect.top = g_state.selRect.bottom = g_state.selStart.y;
        g_state.hasSelection = false; // selection being created/changed
        SetCapture(hwnd);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_RBUTTONUP:
    {
        g_state.selecting = false;
        ReleaseCapture();
        NormalizeRect(g_state.selRect);
        // ignore too-small selections (click without drag)
        int selW = g_state.selRect.right - g_state.selRect.left;
        int selH = g_state.selRect.bottom - g_state.selRect.top;
        if (selW <= 4 || selH <= 4) {
            g_state.hasSelection = false;
        }
        else
        {
            g_state.hasSelection = true;
        }
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_MOUSEWHEEL:
    {
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

        // Compute world position under mouse using the same sign convention as the renderer:
        double worldX = PixelToWorldX(mp.x);
        double worldY = PixelToWorldY(mp.y);

        // New center: keep the world point under the mouse stationary:
        // For X: worldX = newCenterX + (mp.x - halfW)*newScale  => newCenterX = worldX - (mp.x - halfW)*newScale
        // For Y: worldY = newCenterY - (mp.y - halfH)*newScale  => newCenterY = worldY + (mp.y - halfH)*newScale
        double halfW = g_state.width / 2.0;
        double halfH = g_state.height / 2.0;

        g_state.centerX = worldX - (mp.x - halfW) * newScale;
        g_state.centerY = worldY + (mp.y - halfH) * newScale; // note the + here to match PixelToWorldY's '-'

        g_state.scale = newScale;

        g_state.needRender = true;
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_KEYDOWN:
    {
        if (wParam == 'R')
        {
            g_state.centerX = -0.75;
            g_state.centerY = 0.0;
            g_state.scale = 3.0 / 800.0;
            g_state.needRender = true;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        else if (wParam == VK_ESCAPE)
        {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
        }
        else if (wParam == VK_ADD || wParam == VK_OEM_PLUS)
        {
            g_state.maxIter = static_cast<int>(g_state.maxIter * 1.25) + 10;
            if (g_state.maxIter > 5000) g_state.maxIter = 5000;
            g_state.needRender = true;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        else if (wParam == VK_SUBTRACT || wParam == VK_OEM_MINUS) {
            g_state.maxIter = static_cast<int>(g_state.maxIter * 0.8) - 10;
            if (g_state.maxIter < 10) g_state.maxIter = 10;
            g_state.needRender = true;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        if (g_state.needRender)
        {
            RenderMandelbrot();
        }

        if (g_state.hBitmap)
        {
            HDC memDC = CreateCompatibleDC(hdc);
            HGDIOBJ old = SelectObject(memDC, g_state.hBitmap);
            BitBlt(hdc, 0, 0, g_state.width, g_state.height, memDC, 0, 0, SRCCOPY);
            SelectObject(memDC, old);
            DeleteDC(memDC);
        }
        else
        {
            FillRect(hdc, &ps.rcPaint, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
        }

        // Draw simple overlay text
        {
            std::string info = "Center: " + std::to_string(g_state.centerX) + ", " + std::to_string(g_state.centerY)
                + "  Scale: " + std::to_string(g_state.scale) + "  Iter: " + std::to_string(g_state.maxIter);
            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, TRANSPARENT);
            RECT r = { 8, 8, g_state.width - 8, 40 };
            DrawTextA(hdc, info.c_str(), static_cast<int>(info.size()), &r, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
        }

        // Draw selection rectangle overlay if any
        if (g_state.selecting || g_state.hasSelection)
        {
            RECT r = g_state.selRect;
            NormalizeRect(r);

            // Draw a solid rectangle with a semi-transparent fill-like border (GDI has no alpha here,
            // so we do a simple dashed frame and XOR focus rect for visibility).
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
            HGDIOBJ oldPen = SelectObject(hdc, hPen);
            HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, r.left, r.top, r.right, r.bottom);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            DeleteObject(hPen);

            // Draw a focus rect for extra visibility
            DrawFocusRect(hdc, &r);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
    {
        if (g_state.hBitmap) DeleteObject(g_state.hBitmap);
        g_state.hBitmap = nullptr;
        g_state.pixels = nullptr;
        g_state.pitch = 0;
        PostQuitMessage(0);
        return 0;
    }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    // Use Unicode window class and CreateWindowExW to ensure the caption is set correctly
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"MandelbrotWindowClass";
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

    if (!RegisterClassExW(&wc))
    {
        MessageBoxA(NULL, "RegisterClassEx failed", "Error", MB_ICONERROR);
        return 1;
    }

    // Create a simple menu and hook up menu commands
    HMENU hMenu = CreateMenu();
    if (hMenu)
    {
        HMENU hFile = CreatePopupMenu();
        AppendMenuW(hFile, MF_STRING, ID_VIEW_RESET, L"&Reset\tR");
        AppendMenuW(hFile, MF_STRING, IDM_PROPERTIES, L"&Properties");
        AppendMenuW(hFile, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hFile, MF_STRING, ID_FILE_EXIT, L"E&xit\tEsc");
        AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFile, L"&File");

        HMENU hView = CreatePopupMenu();
        AppendMenuW(hView, MF_STRING, ID_ITER_INC, L"Increase Iterations\t+");
        AppendMenuW(hView, MF_STRING, ID_ITER_DEC, L"Decrease Iterations\t-");
        AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hView, L"&View");

        HMENU hHelp = CreatePopupMenu();
        AppendMenuW(hHelp, MF_STRING, ID_HELP_ABOUT, L"&About");
        AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hHelp, L"&Help");
    }

    RECT r = { 0, 0, g_state.width, g_state.height };
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);

    // Create window with a Unicode title and the menu
    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"Mandelbrot Renderer", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top,
        NULL, hMenu, hInstance, &g_state);

    if (!hwnd)
    {
        MessageBoxA(NULL, "CreateWindowEx failed", "Error", MB_ICONERROR);
        if (hMenu) DestroyMenu(hMenu);
        return 1;
    }

    // Explicitly set the Unicode window title as well (defensive)
    SetWindowTextW(hwnd, L"Mandelbrot Renderer");

    // Mark global state as not owned
    g_state.owned = false;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Create initial bitmap sized to client area (redundant with WM_CREATE but ensures correct size)
    RECT client;
    GetClientRect(hwnd, &client);
    CreateOrResizeBitmap(client.right - client.left, client.bottom - client.top);

    // Main loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Clean up menu we created
    if (hMenu) DestroyMenu(hMenu);

    return (int)msg.wParam;
}
