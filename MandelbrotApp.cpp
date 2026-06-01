#include "framework.h"
#include "MandelbrotApp.h"
#include "MandelbrotView.h"
#include "PropertiesDlg.h"

#include <commctrl.h>

AppState g_state{};
Properties g_props{};

BOOL CMandelbrotApp::InitInstance()
{
    INITCOMMONCONTROLSEX initControls{};
    initControls.dwSize = sizeof(initControls);
    initControls.dwICC = ICC_WIN95_CLASSES | ICC_UPDOWN_CLASS;
    InitCommonControlsEx(&initControls);

    if (!CWinApp::InitInstance())
    {
        return FALSE;
    }

    auto* mainWindow = new CMandelbrotView();
    CRect initialRect(0, 0, g_state.width, g_state.height);
    ::AdjustWindowRect(initialRect, WS_OVERLAPPEDWINDOW, FALSE);

    if (!mainWindow->Create(nullptr, L"Mandelbrot Renderer", WS_OVERLAPPEDWINDOW, initialRect))
    {
        delete mainWindow;
        return FALSE;
    }

    m_pMainWnd = mainWindow;
    mainWindow->ShowWindow(m_nCmdShow);
    mainWindow->UpdateWindow();
    return TRUE;
}
