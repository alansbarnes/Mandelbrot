#pragma once

#include <afxwin.h>
#include <cstdint>
#include <math.h>

// Mandelbrot rendering state
struct MandelbrotState
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
    BITMAPINFO bmi = { 0 };

    bool needRender = true;

    int rmin = 100;
    int rmax = 255;
    int gmin = 0;
    int gmax = 255;
    int bmin = 0;
    int bmax = 0;
};

class CMandelbrotView : public CFrameWnd
{
    DECLARE_DYNCREATE(CMandelbrotView)

public:
    CMandelbrotView();
    virtual ~CMandelbrotView();

protected:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnPaint();
    afx_msg void OnDestroy();
    afx_msg void OnSize(UINT nType, int cx, int cy);

    DECLARE_MESSAGE_MAP()

private:
    MandelbrotState m_state;

    void CreateOrResizeBitmap(int w, int h, bool bCreate);
    void RenderMandelbrotToMemoryDC(CDC& memDC);
    double PixelToWorldX(int px) const;
    double PixelToWorldY(int py) const;
};
