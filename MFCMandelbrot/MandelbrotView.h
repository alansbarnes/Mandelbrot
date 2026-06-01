#pragma once

#include "MandelbrotState.h"

class CMandelbrotView : public CFrameWnd
{
public:
    CMandelbrotView();
    ~CMandelbrotView() override;

protected:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnPaint();
    afx_msg void OnSize(UINT nType, int cx, int cy);

    DECLARE_MESSAGE_MAP()

private:
    void CreateOrResizeBitmap(int w, int h, bool createScale);
    void RenderMandelbrot();

    AppState m_state{};
};
