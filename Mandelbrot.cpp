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

#include <windows.h>
#include <windowsx.h>
#include <stdint.h>
#include <math.h>
#include <string>

struct AppState
{
    //    int width = 800;
    //    int height = 600;
    int width = 1600;
    int height = 1200;
    HBITMAP hBitmap = nullptr;
    void* pixels = nullptr; // pointer returned by CreateDIBSection
    BITMAPINFO bmi;
    double centerX = -0.75;
    double centerY = 0.0;
    double scale = 3.0 / 800.0; // complex units per pixel (initial)
    //    int maxIter = 900;
    int maxIter = 100;
    bool dragging = false;
    POINT dragStart;
    double dragCenterX, dragCenterY;
    bool needRender = true;

    // Selection/right-drag support:
    bool selecting = false;   // currently dragging right-button
    POINT selStart;           // selection start in client coords
    RECT selRect = { 0,0,0,0 };  // normalized selection rect (client coords)
    bool hasSelection = false; // whether a selection exists to act on

    // Ownership: whether this AppState was heap-allocated (for new windows)
    bool owned = false;
} g_state;

static void NormalizeRect(RECT& r)
{
    if (r.left > r.right) std::swap(r.left, r.right);
    if (r.top > r.bottom) std::swap(r.top, r.bottom);
}

static bool PointInRectEx(const RECT& r, int x, int y)
{
    return (x >= r.left && x <= r.right && y >= r.top && y <= r.bottom);
}

static void CreateOrResizeBitmap(AppState& s, int w, int h)
{
    if (s.hBitmap)
    {
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

static void HSVtoRGB(double h, double s, double v, uint8_t& outR, uint8_t& outG, uint8_t& outB)
{
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
    outR = static_cast<uint8_t>(round((r + m) * 255.0));
    outG = static_cast<uint8_t>(round((g + m) * 255.0));
    outB = static_cast<uint8_t>(round((b + m) * 255.0));
}

static void RenderMandelbrot(AppState& s)
{
    if (!s.pixels) return;
    const int w = s.width;
    const int h = s.height;
    const double cx = s.centerX;
    const double cy = s.centerY;
    const double scale = s.scale;
    const int maxIter = s.maxIter;

    uint32_t* buf = static_cast<uint32_t*>(s.pixels);

    // For each pixel
    for (int y = 0; y < h; ++y)
    {
        double imag = cy + (y - h / 2.0) * scale;
        for (int x = 0; x < w; ++x)
        {
            double real = cx + (x - w / 2.0) * scale;

            double zx = 0.0, zy = 0.0;
            int iter = 0;
            double zx2 = 0.0, zy2 = 0.0;

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
                // smooth coloring
                double log_zn = log(zx2 + zy2) / 2.0;
                double nu = log(log_zn / log(2.0)) / log(2.0);
                double mu = iter + 1 - nu;
                double hue = fmod(360.0 * mu / maxIter + 360.0, 360.0); // wrap
                double sat = 1.0;
                double val = 1.0;
                HSVtoRGB(hue, sat, val, r, g, b);

                // optionally, reduce brightness for low iteration counts:
                double t = static_cast<double>(iter) / maxIter;
                // enhance contrast
                double brightness = 0.5 + 0.5 * t;
                r = static_cast<uint8_t>(r * brightness);
                g = static_cast<uint8_t>(g * brightness);
                b = static_cast<uint8_t>(b * brightness);
            }

            // For 32bpp DIB (BI_RGB) the memory order is generally B, G, R, [0]
            uint32_t pixel = static_cast<uint32_t>(b) | (static_cast<uint32_t>(g) << 8) | (static_cast<uint32_t>(r) << 16);
            buf[y * w + x] = pixel;
        }
    }
    s.needRender = false;
}

static void ApplySelectionToWindow(AppState* s, HWND hwnd)
{
    if (!s || !s->hasSelection) return;

    // Normalize and measure selection
    RECT sel = s->selRect;
    NormalizeRect(sel);
    int selW = sel.right - sel.left;
    int selH = sel.bottom - sel.top;
    if (selW <= 0 || selH <= 0) return;

    // selection center in client coordinates
    double selCenterPx = (sel.left + sel.right) / 2.0;
    double selCenterPy = (sel.top + sel.bottom) / 2.0;

    // world coords of selection center
    double worldX = s->centerX + (selCenterPx - s->width / 2.0) * s->scale;
    double worldY = s->centerY + (selCenterPy - s->height / 2.0) * s->scale;

    // new scale: selected width in pixels maps to full window width
    double newScale = s->scale * (static_cast<double>(selW) / static_cast<double>(s->width));

    // apply to current AppState and re-render here
    s->centerX = worldX;
    s->centerY = worldY;
    s->scale = newScale;
    s->selecting = false;
    s->hasSelection = false;
    s->needRender = true;

    InvalidateRect(hwnd, NULL, FALSE);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Retrieve per-window AppState pointer
    AppState* s = reinterpret_cast<AppState*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg)
    {
        case WM_CREATE:
        {
            // The lpCreateParams contains the AppState* we passed when creating the window
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            AppState* passed = nullptr;
            if (cs)
            {
                passed = reinterpret_cast<AppState*>(cs->lpCreateParams);
            }
            if (passed)
            {
                s = passed;
            }
            else
            {
                // fallback to global state if none provided
                s = &g_state;
                s->owned = false;
            }
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(s));
    
            // Create initial bitmap sized to client area
            RECT client;
            GetClientRect(hwnd, &client);
            CreateOrResizeBitmap(*s, (client.right - client.left), (client.bottom - client.top));
            return 0;
        }
    
        case WM_SIZE:
        {
            if (!s) break;
            int w = LOWORD(lParam);
            int h = HIWORD(lParam);
            if (w > 0 && h > 0)
            {
                CreateOrResizeBitmap(*s, w, h);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }
    
        case WM_LBUTTONDOWN:
        {
            if (!s) break;
            int mx = LOWORD(lParam);
            int my = HIWORD(lParam);
            // If there's an existing selection and the click is inside it, apply that selection to this window.
            if (s->hasSelection)
            {
                RECT sel = s->selRect;
                NormalizeRect(sel);
                if (PointInRectEx(sel, mx, my))
                {
                    ApplySelectionToWindow(s, hwnd);
                    return 0;
                }
            }
    
            // Otherwise, start panning (original behavior)
            s->dragging = true;
            s->dragStart.x = mx;
            s->dragStart.y = my;
            s->dragCenterX = s->centerX;
            s->dragCenterY = s->centerY;
            SetCapture(hwnd);
            return 0;
        }
    
        case WM_LBUTTONUP:
        {
            if (!s) break;
            s->dragging = false;
            ReleaseCapture();
            return 0;
        }
    
        case WM_MOUSEMOVE:
        {
            if (!s) break;
            if (s->dragging)
            {
                int x = LOWORD(lParam);
                int y = HIWORD(lParam);
                int dx = x - s->dragStart.x;
                int dy = y - s->dragStart.y;
                s->centerX = s->dragCenterX - dx * s->scale;
                s->centerY = s->dragCenterY - dy * s->scale;
                s->needRender = true;
                InvalidateRect(hwnd, NULL, FALSE);
            }
            else if (s->selecting)
            {
                int x = LOWORD(lParam);
                int y = HIWORD(lParam);
    
                int startX = s->selStart.x;
                int startY = s->selStart.y;
                int dx = x - startX;
                int dy = y - startY;
                int signX = (dx >= 0) ? 1 : -1;
                int signY = (dy >= 0) ? 1 : -1;
                int absDx = (dx >= 0) ? dx : -dx;
                int absDy = (dy >= 0) ? dy : -dy;
    
                // Keep selection aspect ratio equal to client area aspect ratio
                double aspect = (double)s->width / (double)s->height;
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
    
                s->selRect.left = startX;
                s->selRect.top = startY;
                s->selRect.right = startX + signX * adjW;
                s->selRect.bottom = startY + signY * adjH;
                NormalizeRect(s->selRect);
                InvalidateRect(hwnd, NULL, FALSE);
            }
    
            return 0;
        }
    
        case WM_RBUTTONDOWN:
        {
            if (!s) break;
            s->selecting = true;
            s->selStart.x = LOWORD(lParam);
            s->selStart.y = HIWORD(lParam);
            s->selRect.left = s->selRect.right = s->selStart.x;
            s->selRect.top = s->selRect.bottom = s->selStart.y;
            s->hasSelection = false; // selection being created/changed
            SetCapture(hwnd);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
    
        case WM_RBUTTONUP:
        {
            if (!s) break;
            s->selecting = false;
            ReleaseCapture();
            NormalizeRect(s->selRect);
            // ignore too-small selections (click without drag)
            int selW = s->selRect.right - s->selRect.left;
            int selH = s->selRect.bottom - s->selRect.top;
            if (selW <= 4 || selH <= 4) {
                s->hasSelection = false;
            }
            else
            {
                s->hasSelection = true;
            }
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
    
        case WM_MOUSEWHEEL:
        {
            if (!s) break;
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            POINT mp;
            mp.x = GET_X_LPARAM(lParam);
            mp.y = GET_Y_LPARAM(lParam);
    
            // Convert to client coordinates
            ScreenToClient(hwnd, &mp);
    
            double oldScale = s->scale;
            double factor = (delta > 0) ? 0.8 : 1.25;
            // Use exponential for smooth zoom
            double zoomFactor = pow(factor, abs(delta) / 120.0);
            double newScale = oldScale * zoomFactor;
    
            // Keep mouse point stable in world coords
            double worldX = s->centerX + (mp.x - s->width / 2.0) * oldScale;
            double worldY = s->centerY + (mp.y - s->height / 2.0) * oldScale;
            s->centerX = worldX - (mp.x - s->width / 2.0) * newScale;
            s->centerY = worldY - (mp.y - s->height / 2.0) * newScale;
            s->scale = newScale;
    
            s->needRender = true;
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
    
        case WM_KEYDOWN:
        {
            if (!s) break;
            if (wParam == 'R')
            {
                s->centerX = -0.75;
                s->centerY = 0.0;
                s->scale = 3.0 / 800.0;
                s->needRender = true;
                InvalidateRect(hwnd, NULL, FALSE);
            }
            else if (wParam == VK_ESCAPE)
            {
                PostMessage(hwnd, WM_CLOSE, 0, 0);
            }
            else if (wParam == VK_ADD || wParam == VK_OEM_PLUS)
            {
                s->maxIter = static_cast<int>(s->maxIter * 1.25) + 10;
                if (s->maxIter > 5000) s->maxIter = 5000;
                s->needRender = true;
                InvalidateRect(hwnd, NULL, FALSE);
            }
            else if (wParam == VK_SUBTRACT || wParam == VK_OEM_MINUS) {
                s->maxIter = static_cast<int>(s->maxIter * 0.8) - 10;
                if (s->maxIter < 10) s->maxIter = 10;
                s->needRender = true;
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }
    
        case WM_PAINT:
        {
            if (!s) break;
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
    
            if (s->needRender)
            {
                RenderMandelbrot(*s);
            }
    
            if (s->hBitmap)
            {
                HDC memDC = CreateCompatibleDC(hdc);
                HGDIOBJ old = SelectObject(memDC, s->hBitmap);
                BitBlt(hdc, 0, 0, s->width, s->height, memDC, 0, 0, SRCCOPY);
                SelectObject(memDC, old);
                DeleteDC(memDC);
            }
            else
            {
                FillRect(hdc, &ps.rcPaint, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
            }
    
            // Draw simple overlay text
            {
                std::string info = "Center: " + std::to_string(s->centerX) + ", " + std::to_string(s->centerY)
                    + "  Scale: " + std::to_string(s->scale) + "  Iter: " + std::to_string(s->maxIter);
                SetTextColor(hdc, RGB(255, 255, 255));
                SetBkMode(hdc, TRANSPARENT);
                RECT r = { 8, 8, s->width - 8, 40 };
                DrawTextA(hdc, info.c_str(), static_cast<int>(info.size()), &r, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
            }
    
            // Draw selection rectangle overlay if any
            if (s->selecting || s->hasSelection)
            {
                RECT r = s->selRect;
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
            if (s)
            {
                if (s->hBitmap) DeleteObject(s->hBitmap);
                s->hBitmap = nullptr;
                s->pixels = nullptr;
                PostQuitMessage(0);
                // If this AppState was dynamically allocated for a created window, free it
                if (s->owned)
                {
                    delete s;
                    // Note: can't touch 's' after deleting; GWLP_USERDATA will be cleaned by system
                }
            }
            else
            {
                PostQuitMessage(0);
            }
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

    RECT r = { 0, 0, g_state.width, g_state.height };
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);

    // Create window with a Unicode title
    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"Mandelbrot Renderer", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top,
        NULL, NULL, hInstance, &g_state);

    if (!hwnd)
    {
        MessageBoxA(NULL, "CreateWindowEx failed", "Error", MB_ICONERROR);
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
    CreateOrResizeBitmap(g_state, client.right - client.left, client.bottom - client.top);

    // Main loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

