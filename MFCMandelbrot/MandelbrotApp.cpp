#include "framework.h"
#include "MandelbrotApp.h"
#include "MandelbrotView.h"

BOOL CMandelbrotApp::InitInstance()
{
    CWinApp::InitInstance();

    auto* pMainFrame = new CMandelbrotView();
    if (!pMainFrame->Create(nullptr,
        _T("Mandelbrot Renderer"),
        WS_OVERLAPPEDWINDOW,
        CRect(0, 0, defaultWindowWidth, defaultWindowHeight)))
    {
        delete pMainFrame;
        return FALSE;
    }

    m_pMainWnd = pMainFrame;
    pMainFrame->ShowWindow(SW_SHOW);
    pMainFrame->UpdateWindow();

    return TRUE;
}
