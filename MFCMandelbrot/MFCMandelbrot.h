#pragma once

#include "afxwinappex.h"
#include "afxdialogex.h"

// CMandelbrotApp:
// See MFCMandelbrot.cpp for the implementation of this class
//

class CMandelbrotApp : public CWinAppEx
{
public:
	CMandelbrotApp();
	virtual ~CMandelbrotApp();

// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

	DECLARE_MESSAGE_MAP()
};

extern CMandelbrotApp theApp;
