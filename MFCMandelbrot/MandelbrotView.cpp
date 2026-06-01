#include "framework.h"
#include "MandelbrotView.h"

#include <cstdint>
#include <limits>
#include <sstream>
#include <string>

namespace
{
    constexpr int kTextMargin = 8;
    constexpr int kTextBottom = 40;
    constexpr COLORREF kOverlayTextColor = RGB(255, 255, 255);
}

BEGIN_MESSAGE_MAP(CMandelbrotView, CFrameWnd)
    ON_WM_CREATE()
    ON_WM_PAINT()
    ON_WM_SIZE()
END_MESSAGE_MAP()

CMandelbrotView::CMandelbrotView() = default;

CMandelbrotView::~CMandelbrotView()
{
    if (m_state.hBitmap)
    {
        ::DeleteObject(m_state.hBitmap);
        m_state.hBitmap = nullptr;
    }
}

void CMandelbrotView::CreateOrResizeBitmap(int w, int h, bool createScale)
{
    if (m_state.hBitmap)
    {
        ::DeleteObject(m_state.hBitmap);
        m_state.hBitmap = nullptr;
        m_state.pixels = nullptr;
        m_state.pitch = 0;
    }

    m_state.width = w;
    m_state.height = h;

    if (createScale && m_state.height > 0)
    {
        m_state.scale = initialHeight / m_state.height;
    }

    ZeroMemory(&m_state.bmi, sizeof(m_state.bmi));
    m_state.bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    m_state.bmi.bmiHeader.biWidth = w;
    m_state.bmi.bmiHeader.biHeight = -h;
    m_state.bmi.bmiHeader.biPlanes = 1;
    m_state.bmi.bmiHeader.biBitCount = 32;
    m_state.bmi.bmiHeader.biCompression = BI_RGB;

    CClientDC dc(this);
    void* bits = nullptr;
    m_state.hBitmap = ::CreateDIBSection(dc.GetSafeHdc(), &m_state.bmi, DIB_RGB_COLORS, &bits, nullptr, 0);

    if (!m_state.hBitmap || !bits)
    {
        m_state.hBitmap = nullptr;
        m_state.pixels = nullptr;
        m_state.pitch = 0;
        return;
    }

    m_state.pixels = bits;
    m_state.pitch = w * static_cast<int>(sizeof(uint32_t));
    m_state.needRender = true;
}

void CMandelbrotView::RenderMandelbrot()
{
    if (!m_state.pixels)
    {
        return;
    }

    CClientDC dc(this);
    CRect rect;
    GetClientRect(&rect);
    dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(kOverlayTextColor);
    dc.DrawText(_T("Rendering..."), &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    const int w = m_state.width;
    const int h = m_state.height;
    const double cx = m_state.centerX;
    const double cy = m_state.centerY;
    const double scale = m_state.scale;
    const int maxIter = m_state.maxIter;

    uint32_t* buf = static_cast<uint32_t*>(m_state.pixels);

    const double halfW = w / 2.0;
    const double halfH = h / 2.0;
    const size_t pitchPixels = (m_state.pitch > 0)
        ? (static_cast<size_t>(m_state.pitch) / sizeof(uint32_t))
        : static_cast<size_t>(w);

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

    m_state.needRender = false;
}

int CMandelbrotView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
    {
        return -1;
    }

    CRect client;
    GetClientRect(&client);
    CreateOrResizeBitmap(client.Width(), client.Height(), true);
    return 0;
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

void CMandelbrotView::OnPaint()
{
    CPaintDC dc(this);

    if (m_state.needRender)
    {
        RenderMandelbrot();
    }

    if (m_state.hBitmap)
    {
        CDC memDC;
        memDC.CreateCompatibleDC(&dc);
        HGDIOBJ old = memDC.SelectObject(m_state.hBitmap);
        dc.BitBlt(0, 0, m_state.width, m_state.height, &memDC, 0, 0, SRCCOPY);
        memDC.SelectObject(old);
    }
    else
    {
        dc.FillSolidRect(0, 0, m_state.width, m_state.height, RGB(255, 255, 255));
    }

    constexpr int maxDecimalDigits = std::numeric_limits<double>::max_digits10;
    std::wostringstream ss;
    ss.precision(maxDecimalDigits);
    ss << L"Center: " << m_state.centerX
       << L" + " << m_state.centerY << L"i"
       << L"  Height: " << (m_state.scale * m_state.height) << L"i"
       << L"  Iter: " << m_state.maxIter;

    const std::wstring info = ss.str();

    dc.SetTextColor(kOverlayTextColor);
    dc.SetBkMode(TRANSPARENT);
    if (m_state.width > (2 * kTextMargin))
    {
        CRect textRect(kTextMargin, kTextMargin, m_state.width - kTextMargin, kTextBottom);
        ::DrawTextW(dc.GetSafeHdc(), info.c_str(), static_cast<int>(info.length()), &textRect, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
    }
}
