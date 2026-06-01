#include "MFCMandelbrotView.h"
#include <algorithm>

DECLARE_DYNCREATE(CMandelbrotView)

BEGIN_MESSAGE_MAP(CMandelbrotView, CFrameWnd)
    ON_WM_CREATE()
    ON_WM_PAINT()
    ON_WM_DESTROY()
    ON_WM_SIZE()
END_MESSAGE_MAP()

CMandelbrotView::CMandelbrotView()
{
    m_state.scale = 4.0 / m_state.height;  // initialHeight = 4.0
}

CMandelbrotView::~CMandelbrotView()
{
    if (m_state.hBitmap)
    {
        DeleteObject(m_state.hBitmap);
        m_state.hBitmap = nullptr;
        m_state.pixels = nullptr;
        m_state.pitch = 0;
    }
}

int CMandelbrotView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    // Initialize color parameters for default red gradient
    m_state.rmin = 100;
    m_state.rmax = 255;
    m_state.gmin = 0;
    m_state.gmax = 255;
    m_state.bmin = 0;
    m_state.bmax = 0;

    // Create initial bitmap
    CRect rect;
    GetClientRect(&rect);
    int w = rect.Width();
    int h = rect.Height();
    if (w <= 0 || h <= 0)
    {
        w = m_state.width;
        h = m_state.height;
    }
    CreateOrResizeBitmap(w, h, true);

    return 0;
}

oid CMandelbrotView::OnPaint()
{
    CPaintDC dc(this);

    // Render if needed
    if (m_state.needRender && m_state.hBitmap && m_state.pixels)
    {
        CDC memDC;
        memDC.CreateCompatibleDC(&dc);
        HGDIOBJ oldBitmap = memDC.SelectObject(m_state.hBitmap);

        RenderMandelbrotToMemoryDC(memDC);

        // Blit memory DC to screen
        dc.BitBlt(0, 0, m_state.width, m_state.height, &memDC, 0, 0, SRCCOPY);

        memDC.SelectObject(oldBitmap);
        memDC.DeleteDC();

        m_state.needRender = false;
    }
    else if (m_state.hBitmap)
    {
        // Just blit the cached bitmap
        CDC memDC;
        memDC.CreateCompatibleDC(&dc);
        HGDIOBJ oldBitmap = memDC.SelectObject(m_state.hBitmap);

        dc.BitBlt(0, 0, m_state.width, m_state.height, &memDC, 0, 0, SRCCOPY);

        memDC.SelectObject(oldBitmap);
        memDC.DeleteDC();
    }
    else
    {
        // Fill with default background
        CRect rect;
        GetClientRect(&rect);
        dc.FillSolidRect(&rect, RGB(0, 0, 0));
    }

    // Draw info text
    dc.SetTextColor(RGB(255, 255, 255));
    dc.SetBkMode(TRANSPARENT);
    CString info;
    info.Format(_T("Center: %.6f + %.6fi  Height: %.6f  Iter: %d"),
        m_state.centerX, m_state.centerY,
        m_state.scale * m_state.height, m_state.maxIter);
    dc.TextOut(8, 8, info);
}

void CMandelbrotView::OnDestroy()
{
    if (m_state.hBitmap)
    {
        DeleteObject(m_state.hBitmap);
        m_state.hBitmap = nullptr;
        m_state.pixels = nullptr;
        m_state.pitch = 0;
    }

    CFrameWnd::OnDestroy();
}

void CMandelbrotView::OnSize(UINT nType, int cx, int cy)
{
    CFrameWnd::OnSize(nType, cx, cy);

    if (cx > 0 && cy > 0)
    {
        CreateOrResizeBitmap(cx, cy, false);
        InvalidateRect(nullptr, FALSE);
    }
}

void CMandelbrotView::CreateOrResizeBitmap(int w, int h, bool bCreate)
{
    if (m_state.hBitmap)
    {
        DeleteObject(m_state.hBitmap);
        m_state.hBitmap = nullptr;
        m_state.pixels = nullptr;
        m_state.pitch = 0;
    }

    m_state.width = w;
    m_state.height = h;

    if (bCreate && m_state.height > 0)
    {
        m_state.scale = 4.0 / m_state.height;  // initialHeight = 4.0
    }

    ZeroMemory(&m_state.bmi, sizeof(m_state.bmi));
    m_state.bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    m_state.bmi.bmiHeader.biWidth = w;
    m_state.bmi.bmiHeader.biHeight = -h;
    m_state.bmi.bmiHeader.biPlanes = 1;
    m_state.bmi.bmiHeader.biBitCount = 32;
    m_state.bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    m_state.hBitmap = CreateDIBSection(nullptr, &m_state.bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    m_state.pixels = bits;

    const int bpp = m_state.bmi.bmiHeader.biBitCount;
    m_state.pitch = ((bpp * w + 31) / 32) * 4;

    m_state.needRender = true;
}

void CMandelbrotView::RenderMandelbrotToMemoryDC(CDC& memDC)
{
    if (!m_state.pixels)
        return;

    const int w = m_state.width;
    const int h = m_state.height;
    const double cx = m_state.centerX;
    const double cy = m_state.centerY;
    const double scale = m_state.scale;
    const int maxIter = m_state.maxIter;

    uint32_t* buf = static_cast<uint32_t*>(m_state.pixels);

    const double halfW = w / 2.0;
    const double halfH = h / 2.0;

    const size_t pitchPixels = (m_state.pitch && m_state.pitch > 0)
        ? (m_state.pitch / sizeof(uint32_t))
        : static_cast<size_t>(w);

    // Render the Mandelbrot set
    for (int y = 0; y < h; ++y)
    {
        const double imag = cy - (y - halfH) * scale;
        uint32_t* row = buf + static_cast<size_t>(y) * pitchPixels;

        for (int x = 0; x < w; ++x)
        {
            const double real = cx + (x - halfW) * scale;

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
                r = static_cast<uint8_t>(m_state.rmin + (((m_state.rmax - m_state.rmin) * iter) / maxIter));
                g = static_cast<uint8_t>(m_state.gmin + (((m_state.gmax - m_state.gmin) * iter) / maxIter));
                b = static_cast<uint8_t>(m_state.bmin + (((m_state.bmax - m_state.bmin) * iter) / maxIter));
            }

            row[x] = static_cast<uint32_t>(b)
                | (static_cast<uint32_t>(g) << 8)
                | (static_cast<uint32_t>(r) << 16);
        }
    }
}

double CMandelbrotView::PixelToWorldX(int px) const
{
    return m_state.centerX + (px - (m_state.width / 2.0)) * m_state.scale;
}

double CMandelbrotView::PixelToWorldY(int py) const
{
    return m_state.centerY - (py - (m_state.height / 2.0)) * m_state.scale;
}
