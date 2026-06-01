#pragma once

#include <afxwin.h>
#include "MFCMandelbrotView.h"

class CMandelbrotApp : public CWinApp
{
public:
    CMandelbrotApp();

    virtual BOOL InitInstance();
    virtual int ExitInstance();

DECLARE_MESSAGE_MAP()
private:
    CMandelbrotView* m_pMainView;
};

extern CMandelbrotApp theApp;
