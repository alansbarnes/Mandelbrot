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
    const int initialWidth = (g_state.width > 0) ? g_state.width : 1600;
    const int initialHeight = (g_state.height > 0) ? g_state.height : 1200;
    RECT initialRect{ 0, 0, initialWidth, initialHeight };
    ::AdjustWindowRect(&initialRect, WS_OVERLAPPEDWINDOW, TRUE);

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
