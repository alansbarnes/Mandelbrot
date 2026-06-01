#pragma once

// CMandelbrotDoc document

// Global state structure for Mandelbrot rendering
struct AppState
{
	int width = 800;
	int height = 600;
	double centerX = -0.75;
	double centerY = 0.0;
	double scale = 0.005;
	int maxIter = 50;
	
	// Color scheme
	uint8_t rmin = 100;
	uint8_t rmax = 255;
	uint8_t gmin = 0;
	uint8_t gmax = 255;
	uint8_t bmin = 0;
	uint8_t bmax = 0;
	
	// Rendering state
	HBITMAP hBitmap = nullptr;
	void* pixels = nullptr;
	int pitch = 0;
	BITMAPINFO bmi{};
	bool needRender = true;
	
	// Interaction state
	bool dragging = false;
	POINT dragStart{};
	double dragCenterX = 0.0;
	double dragCenterY = 0.0;
	
	bool selecting = false;
	POINT selStart{};
	RECT selRect{};
	bool hasSelection = false;
	
	bool owned = false;
};

// Constants
constexpr double initialHeight = 3.0;

class CMandelbrotDoc : public CDocument
{
	DECLARE_DYNCREATE(CMandelbrotDoc)

public:
	CMandelbrotDoc();
	virtual ~CMandelbrotDoc();

	// Global application state
	static AppState g_state;

#ifdef _DEBUG
	virtual void AssertValid() const;
#ifndef _WIN32_WCE
	virtual void Dump(CDumpContext& dc) const;
#endif
#endif

protected:
	virtual BOOL OnNewDocument();

	DECLARE_MESSAGE_MAP()
};
