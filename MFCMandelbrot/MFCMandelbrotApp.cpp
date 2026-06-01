#include "MFCMandelbrotApp.h"
#include "MFCMandelbrotView.h"

CMandelbrotApp theApp;

BEGIN_MESSAGE_MAP(CMandelbrotApp, CWinApp)
END_MESSAGE_MAP()

CMandelbrotApp::CMandelbrotApp()
    : m_pMainView(nullptr)
{
}

BOOL CMandelbrotApp::InitInstance()
{
    CWinApp::InitInstance();

    // Create the main application window
    m_pMainView = new CMandelbrotView();
    if (!m_pMainView)
    {
        return FALSE;
    }

    // Register the frame window class
    WNDCLASS wc = { 0 };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = ::DefWindowProc;
    wc.hInstance = AfxGetInstanceHandle();
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = _T("MandelbrotWindowClass");
    
    if (!RegisterClass(&wc))
    {
        MessageBox(nullptr, _T("Failed to register window class"), _T("Error"), MB_OK | MB_ICONERROR);
        return FALSE;
    }

    // Create the main frame window
    if (!m_pMainView->Create(
        _T("MandelbrotWindowClass"),
        _T("Mandelbrot Renderer - MFC 64-bit"),
        WS_OVERLAPPEDWINDOW,
        CRect(0, 0, 1600, 1200),
        nullptr,
        nullptr))
    {
        MessageBox(nullptr, _T("Failed to create main window"), _T("Error"), MB_OK | MB_ICONERROR);
        return FALSE;
    }

    m_pMainWnd = m_pMainView;
    m_pMainView->ShowWindow(SW_SHOW);
    m_pMainView->UpdateWindow();

    return TRUE;
}

int CMandelbrotApp::ExitInstance()
{
    if (m_pMainView)
    {
        m_pMainView->DestroyWindow();
        delete m_pMainView;
        m_pMainView = nullptr;
    }

    return CWinApp::ExitInstance();
}
