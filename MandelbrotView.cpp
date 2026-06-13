#include "framework.h"
#include "MandelbrotView.h"
#include "Resource.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <limits>
#include <string>

BEGIN_MESSAGE_MAP(CMandelbrotView, CFrameWnd)
    ON_WM_CREATE()
    ON_WM_PAINT()
    ON_WM_SIZE()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_RBUTTONDOWN()
    ON_WM_RBUTTONUP()
    ON_WM_MOUSEWHEEL()
    ON_WM_KEYDOWN()
    ON_WM_DESTROY()
    ON_COMMAND(IDM_PROPERTIES, &CMandelbrotView::OnProperties)
    ON_COMMAND(IDM_EXIT, &CMandelbrotView::OnAppExit)
    ON_COMMAND(ID_VIEW_RESET, &CMandelbrotView::OnViewReset)
    ON_COMMAND(ID_ITER_INC, &CMandelbrotView::OnIterInc)
    ON_COMMAND(ID_ITER_DEC, &CMandelbrotView::OnIterDec)
    ON_COMMAND(IDM_ABOUT, &CMandelbrotView::OnAppAbout)
END_MESSAGE_MAP()

BOOL CMandelbrotView::PreCreateWindow(CREATESTRUCT& cs)
{
    if (!CFrameWnd::PreCreateWindow(cs))
    {
        return FALSE;
    }

    cs.lpszClass = AfxRegisterWndClass(
        CS_HREDRAW | CS_VREDRAW,
        ::LoadCursor(nullptr, IDC_ARROW),
        reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1),
        AfxGetApp()->LoadIcon(IDI_MANDELBROT));

    return TRUE;
}

int CMandelbrotView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
    {
        return -1;
    }

    BuildMenu();
    SetIcon(AfxGetApp()->LoadIcon(IDI_MANDELBROT), TRUE);
    SetIcon(AfxGetApp()->LoadIcon(IDI_MANDELBROT), FALSE);

    g_state.rmin = 100;
    g_state.rmax = 255;
    g_state.gmin = 0;
    g_state.gmax = 255;
    g_state.bmin = 0;
    g_state.bmax = 0;

    CRect client;
    GetClientRect(&client);
    CreateOrResizeBitmap(client.Width(), client.Height(), true);
    return 0;
}

void CMandelbrotView::OnPaint()
{
    CPaintDC dc(this);

    if (g_state.needRender)
    {
        RenderMandelbrot();
    }

    if (g_state.width <= 0 || g_state.height <= 0)
    {
        dc.FillSolidRect(&dc.m_ps.rcPaint, ::GetSysColor(COLOR_WINDOW));
        return;
    }

    CDC imageDC;
    imageDC.CreateCompatibleDC(&dc);
    HGDIOBJ oldImageBitmap = nullptr;
    if (g_state.hBitmap != nullptr)
    {
        oldImageBitmap = ::SelectObject(imageDC.GetSafeHdc(), g_state.hBitmap);
    }

    CDC paintDC;
    paintDC.CreateCompatibleDC(&dc);

    CBitmap paintBitmap;
    paintBitmap.CreateCompatibleBitmap(&dc, g_state.width, g_state.height);
    HGDIOBJ oldPaintBitmap = ::SelectObject(paintDC.GetSafeHdc(), paintBitmap.GetSafeHandle());

    if (g_state.hBitmap != nullptr)
    {
        ::BitBlt(paintDC.GetSafeHdc(), 0, 0, g_state.width, g_state.height, imageDC.GetSafeHdc(), 0, 0, SRCCOPY);
    }
    else
    {
        RECT clientRect{ 0, 0, g_state.width, g_state.height };
        ::FillRect(paintDC.GetSafeHdc(), &clientRect, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
    }

    constexpr int precision = std::numeric_limits<double>::max_digits10;
    std::string info =
        "Center: " + std::format("{:.{}g}", g_state.centerX, precision) +
        " + " + std::format("{:.{}g}", g_state.centerY, precision) + "i" +
        "  Height: " + std::format("{:.{}g}", g_state.scale * g_state.height, precision) + "i" +
        "  Iter: " + std::to_string(g_state.maxIter);

    ::SetTextColor(paintDC.GetSafeHdc(), RGB(255, 255, 255));
    ::SetBkMode(paintDC.GetSafeHdc(), TRANSPARENT);
    RECT infoRect{ 8, 8, g_state.width - 8, 40 };
    ::DrawTextA(paintDC.GetSafeHdc(), info.c_str(), static_cast<int>(info.size()), &infoRect, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);

    if (g_state.selecting || g_state.hasSelection)
    {
        RECT selection = g_state.selRect;
        NormalizeRect(selection);

        HPEN selectionPen = ::CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
        HGDIOBJ oldPen = ::SelectObject(paintDC.GetSafeHdc(), selectionPen);
        HGDIOBJ oldBrush = ::SelectObject(paintDC.GetSafeHdc(), ::GetStockObject(NULL_BRUSH));
        ::Rectangle(paintDC.GetSafeHdc(), selection.left, selection.top, selection.right, selection.bottom);
        ::SelectObject(paintDC.GetSafeHdc(), oldBrush);
        ::SelectObject(paintDC.GetSafeHdc(), oldPen);
        ::DeleteObject(selectionPen);

        ::DrawFocusRect(paintDC.GetSafeHdc(), &selection);
    }

    ::BitBlt(dc.GetSafeHdc(), 0, 0, g_state.width, g_state.height, paintDC.GetSafeHdc(), 0, 0, SRCCOPY);

    ::SelectObject(paintDC.GetSafeHdc(), oldPaintBitmap);
    if (oldImageBitmap != nullptr)
    {
        ::SelectObject(imageDC.GetSafeHdc(), oldImageBitmap);
    }
}

void CMandelbrotView::OnSize(UINT nType, int cx, int cy)
{
    CFrameWnd::OnSize(nType, cx, cy);

    if (cx > 0 && cy > 0)
    {
        CreateOrResizeBitmap(cx, cy, false);
        Invalidate(FALSE);
    }
}

void CMandelbrotView::OnLButtonDown(UINT nFlags, CPoint point)
{
    if (g_state.hasSelection)
    {
        RECT selection = g_state.selRect;
        NormalizeRect(selection);
        if (PointInRectEx(selection, point.x, point.y))
        {
            ApplySelectionToWindow();
            return;
        }

        g_state.hasSelection = false;
        g_state.selRect = { 0, 0, 0, 0 };
        Invalidate(FALSE);
    }

    g_state.dragging = true;
    g_state.dragStart.x = point.x;
    g_state.dragStart.y = point.y;
    g_state.dragCenterX = g_state.centerX;
    g_state.dragCenterY = g_state.centerY;
    SetCapture();

    CFrameWnd::OnLButtonDown(nFlags, point);
}

void CMandelbrotView::OnLButtonUp(UINT nFlags, CPoint point)
{
    g_state.dragging = false;
    if (GetCapture() == this)
    {
        ReleaseCapture();
    }

    CFrameWnd::OnLButtonUp(nFlags, point);
}

void CMandelbrotView::OnMouseMove(UINT nFlags, CPoint point)
{
    if (g_state.dragging)
    {
        const int dx = point.x - g_state.dragStart.x;
        const int dy = point.y - g_state.dragStart.y;

        g_state.centerX = g_state.dragCenterX - dx * g_state.scale;
        g_state.centerY = g_state.dragCenterY + dy * g_state.scale;
        g_state.needRender = true;
        Invalidate(FALSE);
    }
    else if (g_state.selecting)
    {
        const int startX = g_state.selStart.x;
        const int startY = g_state.selStart.y;
        const int dx = point.x - startX;
        const int dy = point.y - startY;
        const int signX = (dx >= 0) ? 1 : -1;
        const int signY = (dy >= 0) ? 1 : -1;
        const int absDx = (dx >= 0) ? dx : -dx;
        const int absDy = (dy >= 0) ? dy : -dy;

        const double aspect = static_cast<double>(g_state.width) / static_cast<double>(g_state.height);
        double desiredW = 0.0;
        double desiredH = 0.0;

        if (absDx == 0 && absDy == 0)
        {
            desiredW = desiredH = 0.0;
        }
        else if (absDy == 0)
        {
            desiredW = absDx;
            desiredH = desiredW / aspect;
        }
        else if (absDx == 0)
        {
            desiredH = absDy;
            desiredW = desiredH * aspect;
        }
        else if (static_cast<double>(absDx) / static_cast<double>(absDy) > aspect)
        {
            desiredH = absDy;
            desiredW = desiredH * aspect;
        }
        else
        {
            desiredW = absDx;
            desiredH = desiredW / aspect;
        }

        const int adjW = static_cast<int>(desiredW + 0.5);
        const int adjH = static_cast<int>(desiredH + 0.5);

        g_state.selRect.left = startX;
        g_state.selRect.top = startY;
        g_state.selRect.right = startX + signX * adjW;
        g_state.selRect.bottom = startY + signY * adjH;
        NormalizeRect(g_state.selRect);
        Invalidate(FALSE);
    }

    CFrameWnd::OnMouseMove(nFlags, point);
}

void CMandelbrotView::OnRButtonDown(UINT nFlags, CPoint point)
{
    g_state.selecting = true;
    g_state.selStart.x = point.x;
    g_state.selStart.y = point.y;
    g_state.selRect.left = g_state.selRect.right = point.x;
    g_state.selRect.top = g_state.selRect.bottom = point.y;
    g_state.hasSelection = false;
    SetCapture();
    Invalidate(FALSE);

    CFrameWnd::OnRButtonDown(nFlags, point);
}

void CMandelbrotView::OnRButtonUp(UINT nFlags, CPoint point)
{
    g_state.selecting = false;
    if (GetCapture() == this)
    {
        ReleaseCapture();
    }

    NormalizeRect(g_state.selRect);

    const int selW = g_state.selRect.right - g_state.selRect.left;
    const int selH = g_state.selRect.bottom - g_state.selRect.top;
    g_state.hasSelection = (selW > 4 && selH > 4);

    Invalidate(FALSE);
    CFrameWnd::OnRButtonUp(nFlags, point);
}

BOOL CMandelbrotView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    ScreenToClient(&pt);

    const double oldScale = g_state.scale;
    const double factor = (zDelta > 0) ? 0.8 : 1.25;
    const double zoomFactor = std::pow(factor, std::abs(zDelta) / 120.0);
    const double newScale = oldScale * zoomFactor;

    const double worldX = PixelToWorldX(pt.x);
    const double worldY = PixelToWorldY(pt.y);

    const double halfW = g_state.width / 2.0;
    const double halfH = g_state.height / 2.0;

    g_state.centerX = worldX - (pt.x - halfW) * newScale;
    g_state.centerY = worldY + (pt.y - halfH) * newScale;
    g_state.scale = newScale;
    g_state.needRender = true;
    Invalidate(FALSE);
    return TRUE;
}

void CMandelbrotView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if (nChar == 'R')
    {
        g_state.centerX = -0.75;
        g_state.centerY = 0.0;
        if (g_state.height > 0)
        {
            g_state.scale = initialHeight / g_state.height;
        }
        g_state.needRender = true;
        Invalidate(FALSE);
        return;
    }

    if (nChar == VK_ESCAPE)
    {
        PostMessage(WM_CLOSE);
        return;
    }

    if (nChar == VK_ADD || nChar == VK_OEM_PLUS)
    {
        OnIterInc();
        return;
    }

    if (nChar == VK_SUBTRACT || nChar == VK_OEM_MINUS)
    {
        OnIterDec();
        return;
    }

    CFrameWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CMandelbrotView::OnDestroy()
{
    if (g_state.hBitmap != nullptr)
    {
        ::DeleteObject(g_state.hBitmap);
    }

    g_state.hBitmap = nullptr;
    g_state.pixels = nullptr;
    g_state.pitch = 0;

    CFrameWnd::OnDestroy();
}

void CMandelbrotView::OnProperties()
{
    g_props.maxIter = g_state.maxIter;
    g_props.centerReal = g_state.centerX;
    g_props.centerImag = g_state.centerY;
    g_props.height = (g_state.height > 0)
        ? (g_state.scale * static_cast<double>(g_state.height))
        : 0.0;

    g_props.rmin = g_state.rmin;
    g_props.rmax = g_state.rmax;
    g_props.gmin = g_state.gmin;
    g_props.gmax = g_state.gmax;
    g_props.bmin = g_state.bmin;
    g_props.bmax = g_state.bmax;

    const INT_PTR dialogResult = ::DialogBox(
        AfxGetInstanceHandle(),
        MAKEINTRESOURCE(IDD_PROPERTIES),
        GetSafeHwnd(),
        PropertiesDlgProc);

    if (dialogResult == -1)
    {
        const DWORD error = ::GetLastError();
        wchar_t message[256]{};
        wsprintfW(message, L"DialogBox failed. GetLastError = %lu", error);
        ::MessageBoxW(GetSafeHwnd(), message, L"Dialog Error", MB_OK | MB_ICONERROR);
        return;
    }

    if (dialogResult != IDOK)
    {
        return;
    }

    g_state.maxIter = g_props.maxIter;
    g_state.centerX = g_props.centerReal;
    g_state.centerY = g_props.centerImag;
    if (g_state.height > 0)
    {
        g_state.scale = g_props.height / static_cast<double>(g_state.height);
    }

    g_state.rmin = g_props.rmin;
    g_state.rmax = g_props.rmax;
    g_state.gmin = g_props.gmin;
    g_state.gmax = g_props.gmax;
    g_state.bmin = g_props.bmin;
    g_state.bmax = g_props.bmax;
    g_state.needRender = true;
    Invalidate(FALSE);
}

void CMandelbrotView::OnAppExit()
{
    PostMessage(WM_CLOSE);
}

void CMandelbrotView::OnViewReset()
{
    g_state.centerX = -0.75;
    g_state.centerY = 0.0;
    if (g_state.height > 0)
    {
        g_state.scale = initialHeight / g_state.height;
    }
    g_state.maxIter = 50;
    g_state.needRender = true;
    Invalidate(FALSE);
}

void CMandelbrotView::OnIterInc()
{
    g_state.maxIter = static_cast<int>(g_state.maxIter * 1.25) + 10;
    if (g_state.maxIter > 5000)
    {
        g_state.maxIter = 5000;
    }
    g_state.needRender = true;
    Invalidate(FALSE);
}

void CMandelbrotView::OnIterDec()
{
    g_state.maxIter = static_cast<int>(g_state.maxIter * 0.8) - 10;
    if (g_state.maxIter < 10)
    {
        g_state.maxIter = 10;
    }
    g_state.needRender = true;
    Invalidate(FALSE);
}

void CMandelbrotView::OnAppAbout()
{
    ::MessageBoxW(GetSafeHwnd(), L"Mandelbrot Renderer\n\nSimple Win32 Mandelbrot explorer", L"About", MB_OK | MB_ICONINFORMATION);
}

double CMandelbrotView::PixelToWorldX(int px)
{
    return g_state.centerX + (px - (g_state.width / 2.0)) * g_state.scale;
}

double CMandelbrotView::PixelToWorldY(int py)
{
    return g_state.centerY - (py - (g_state.height / 2.0)) * g_state.scale;
}

void CMandelbrotView::NormalizeRect(RECT& rect)
{
    if (rect.left > rect.right)
    {
        std::swap(rect.left, rect.right);
    }
    if (rect.top > rect.bottom)
    {
        std::swap(rect.top, rect.bottom);
    }
}

bool CMandelbrotView::PointInRectEx(const RECT& rect, int x, int y)
{
    return x >= rect.left && x <= rect.right && y >= rect.top && y <= rect.bottom;
}

void CMandelbrotView::CreateOrResizeBitmap(int width, int height, bool initializeScale)
{
    if (g_state.hBitmap != nullptr)
    {
        ::DeleteObject(g_state.hBitmap);
        g_state.hBitmap = nullptr;
        g_state.pixels = nullptr;
        g_state.pitch = 0;
    }

    g_state.width = width;
    g_state.height = height;

    if (initializeScale && g_state.height > 0)
    {
        g_state.scale = initialHeight / g_state.height;
    }

    ZeroMemory(&g_state.bmi, sizeof(g_state.bmi));
    g_state.bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    g_state.bmi.bmiHeader.biWidth = width;
    g_state.bmi.bmiHeader.biHeight = -height;
    g_state.bmi.bmiHeader.biPlanes = 1;
    g_state.bmi.bmiHeader.biBitCount = 32;
    g_state.bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    g_state.hBitmap = ::CreateDIBSection(nullptr, &g_state.bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    g_state.pixels = bits;

    const int bitsPerPixel = g_state.bmi.bmiHeader.biBitCount;
    g_state.pitch = ((bitsPerPixel * width + 31) / 32) * 4;
    g_state.needRender = true;
}

void CMandelbrotView::RenderMandelbrot()
{
    if (g_state.pixels == nullptr)
    {
        return;
    }

    const int width = g_state.width;
    const int height = g_state.height;
    const double centerX = g_state.centerX;
    const double centerY = g_state.centerY;
    const double scale = g_state.scale;
    const int maxIter = g_state.maxIter;

    auto* buffer = static_cast<uint32_t*>(g_state.pixels);
    const double halfW = width / 2.0;
    const double halfH = height / 2.0;

    const size_t pitchPixels = (g_state.pitch > 0)
        ? (static_cast<size_t>(g_state.pitch) / sizeof(uint32_t))
        : static_cast<size_t>(width);

    for (int y = 0; y < height; ++y)
    {
        const double imag = centerY - (y - halfH) * scale;
        auto* row = buffer + static_cast<size_t>(y) * pitchPixels;

        for (int x = 0; x < width; ++x)
        {
            const double real = centerX + (x - halfW) * scale;

            double zx = 0.0;
            double zy = 0.0;
            double zx2 = 0.0;
            double zy2 = 0.0;
            int iter = 0;

            while (zx2 + zy2 <= 4.0 && iter < maxIter)
            {
                zy = 2.0 * zx * zy + imag;
                zx = zx2 - zy2 + real;
                zx2 = zx * zx;
                zy2 = zy * zy;
                ++iter;
            }

            uint8_t r = 0;
            uint8_t g = 0;
            uint8_t b = 0;

            if (iter < maxIter)
            {
                r = static_cast<uint8_t>(g_state.rmin + (((g_state.rmax - g_state.rmin) * iter) / maxIter));
                g = static_cast<uint8_t>(g_state.gmin + (((g_state.gmax - g_state.gmin) * iter) / maxIter));
                b = static_cast<uint8_t>(g_state.bmin + (((g_state.bmax - g_state.bmin) * iter) / maxIter));
            }

            row[x] = static_cast<uint32_t>(b)
                | (static_cast<uint32_t>(g) << 8)
                | (static_cast<uint32_t>(r) << 16);
        }
    }

    g_state.needRender = false;
}

void CMandelbrotView::ApplySelectionToWindow()
{
    if (!g_state.hasSelection)
    {
        return;
    }

    RECT selection = g_state.selRect;
    NormalizeRect(selection);

    const int selW = selection.right - selection.left;
    const int selH = selection.bottom - selection.top;
    if (selW <= 0 || selH <= 0)
    {
        return;
    }

    const double selCenterPx = (selection.left + selection.right) / 2.0;
    const double selCenterPy = (selection.top + selection.bottom) / 2.0;

    g_state.centerX = PixelToWorldX(static_cast<int>(selCenterPx + 0.5));
    g_state.centerY = PixelToWorldY(static_cast<int>(selCenterPy + 0.5));
    g_state.scale = g_state.scale * (static_cast<double>(selW) / static_cast<double>(g_state.width));
    g_state.selecting = false;
    g_state.hasSelection = false;
    g_state.needRender = true;
    Invalidate(FALSE);
}

void CMandelbrotView::BuildMenu()
{
    if (m_menu.GetSafeHmenu() != nullptr)
    {
        return;
    }

    HMENU menuHandle = ::CreateMenu();
    HMENU fileMenu = ::CreatePopupMenu();
    HMENU viewMenu = ::CreatePopupMenu();
    HMENU helpMenu = ::CreatePopupMenu();

    ::AppendMenuW(fileMenu, MF_STRING, IDM_PROPERTIES, L"&Properties...");
    ::AppendMenuW(fileMenu, MF_STRING, ID_VIEW_RESET, L"&Reset\tR");
    ::AppendMenuW(fileMenu, MF_SEPARATOR, 0, nullptr);
    ::AppendMenuW(fileMenu, MF_STRING, IDM_EXIT, L"E&xit\tEsc");
    ::AppendMenuW(menuHandle, MF_POPUP, reinterpret_cast<UINT_PTR>(fileMenu), L"&File");

    ::AppendMenuW(viewMenu, MF_STRING, ID_ITER_INC, L"Increase Iterations\t+");
    ::AppendMenuW(viewMenu, MF_STRING, ID_ITER_DEC, L"Decrease Iterations\t-");
    ::AppendMenuW(menuHandle, MF_POPUP, reinterpret_cast<UINT_PTR>(viewMenu), L"&View");

    ::AppendMenuW(helpMenu, MF_STRING, IDM_ABOUT, L"&About...");
    ::AppendMenuW(menuHandle, MF_POPUP, reinterpret_cast<UINT_PTR>(helpMenu), L"&Help");

    m_menu.Attach(menuHandle);

    SetMenu(&m_menu);
    DrawMenuBar();
}
