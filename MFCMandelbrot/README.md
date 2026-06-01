# MFC Mandelbrot Renderer - 64-bit Visual C++

This is a 64-bit refactor of the original Win32 Mandelbrot renderer using Microsoft Foundation Classes (MFC).

## Architecture

### CMandelbrotApp (CWinApp)
- Application class that manages the MFC application lifecycle
- Initializes the main window (CMandelbrotView) on startup
- Cleans up resources on exit

### CMandelbrotView (CFrameWnd)
- Main window frame class that handles rendering and user interaction
- Implements MFC message map handlers:
  - **OnCreate**: Initializes the DIB bitmap for rendering and color parameters
  - **OnPaint**: Renders the Mandelbrot set to a memory DC and blits to the screen
  - **OnSize**: Recreates the bitmap when the window is resized
  - **OnDestroy**: Cleans up bitmap resources

## Key Features

### Rendering Pipeline
1. **Memory DC Rendering**: The `RenderMandelbrotToMemoryDC()` method renders the Mandelbrot set to a DIB (Device-Independent Bitmap) in memory
2. **Double Buffering**: Drawing happens off-screen to a memory device context, eliminating flicker
3. **Efficient Blitting**: The completed bitmap is copied to the screen using `BitBlt()`

### Mandelbrot Algorithm
- Standard iterative escape-time algorithm
- Configurable iteration depth (default: 50)
- Color gradient based on iteration count
- Default gradient: Red (rmin=100, rmax=255) to black, with green and blue components for variation

### State Management
- **MandelbrotState** struct encapsulates all rendering state
- Properties:
  - Center coordinates (centerX, centerY)
  - Zoom scale
  - Iteration limit
  - Window dimensions and bitmap pitch
  - Color channel ranges (rmin/rmax, gmin/gmax, bmin/bmax)

## Building

Requirements:
- Visual Studio 2022 (or later) with C++ workload
- MFC development libraries (included with Visual C++)
- Windows 10 or later SDK
- 64-bit toolset (v143 or compatible)

### Steps:
1. Open the MFCMandelbrot.vcxproj in Visual Studio
2. Build configuration: Debug|x64 or Release|x64
3. Output executable: `MFCMandelbrot.exe`

## Initial Render

On startup, the application displays the default Mandelbrot view:
- Center: -0.75 + 0i (standard Mandelbrot view)
- Vertical extent: 4.0 (showing the full set)
- Default iteration count: 50
- Red gradient coloring

## Future Enhancements

Potential additions for future versions:
- Mouse zoom (wheel) and pan (drag) support
- Selection rectangle for zooming to a region
- Properties dialog for advanced settings
- Real-time iteration and color adjustments
- Export to image file
