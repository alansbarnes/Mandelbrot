#pragma once

#include "PropertiesDlg.h"

class CMandelbrotView : public CFrameWnd
{
public:
    CMandelbrotView() = default;

protected:
    BOOL PreCreateWindow(CREATESTRUCT& cs) override;

    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnPaint();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnDestroy();

    afx_msg void OnProperties();
    afx_msg void OnAppExit();
    afx_msg void OnViewReset();
    afx_msg void OnIterInc();
    afx_msg void OnIterDec();
    afx_msg void OnAppAbout();

    DECLARE_MESSAGE_MAP()

private:
    static constexpr UINT ID_VIEW_RESET = 9002;
    static constexpr UINT ID_ITER_INC = 9003;
    static constexpr UINT ID_ITER_DEC = 9004;

    static double PixelToWorldX(int px);
    static double PixelToWorldY(int py);
    static void NormalizeRect(RECT& rect);
    static bool PointInRectEx(const RECT& rect, int x, int y);

    void CreateOrResizeBitmap(int width, int height, bool initializeScale);
    void RenderMandelbrot();
    void ApplySelectionToWindow();
    void BuildMenu();
};
