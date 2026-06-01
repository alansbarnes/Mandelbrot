#pragma once

#include "MandelbrotDoc.h"

// CMandelbrotView view

class CMandelbrotView : public CView
{
	DECLARE_DYNCREATE(CMandelbrotView)

protected:
	CMandelbrotView();           // protected constructor used by dynamic creation
	virtual ~CMandelbrotView();

public:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
#ifdef _DEBUG
	virtual void AssertValid() const;
#ifndef _WIN32_WCE
	virtual void Dump(CDumpContext& dc) const;
#endif
#endif

protected:
	// Message handlers
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

	DECLARE_MESSAGE_MAP()

private:
	// Helper methods
	void CreateOrResizeBitmap(int w, int h, bool bCreate);
	void RenderMandelbrot();
	void BlitMemoryDC(CDC* pDC);
	
	static inline double PixelToWorldX(int px);
	static inline double PixelToWorldY(int py);
};
