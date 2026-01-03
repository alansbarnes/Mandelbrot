# Mandelbrot Renderer (Windows C++)

A small Win32 C++ application that renders a colored Mandelbrot set to a 32-bit DIB and displays it in a window.

Features:
- Smooth coloring using a continuous escape-time smoothing.
- Mouse wheel zoom (centered on cursor).
- Click-and-drag panning.
- Keyboard:
  - R: reset view
  - + / - : increase/decrease max iterations
  - Esc: exit

Build instructions:

- Microsoft Visual C++ (cl):
  1. Open "Developer Command Prompt for VS".
  2. Run:
     ```
     cl /EHsc /O2 mandelbrot.cpp /link gdi32.lib user32.lib
     ```
  3. Run `mandelbrot.exe`.

- MinGW-w64 (g++):
  ```
  g++ -O2 -std=c++17 mandelbrot.cpp -lgdi32 -luser32 -municode -o mandelbrot.exe
  ```

Notes:
- The program creates a top-down 32-bit DIBSection and writes pixels directly to the bitmap memory for performance.
- The initial view is centered around (-0.75, 0.0) which shows the main cardioid of the Mandelbrot set.
- Increasing iterations will produce more detail but will be slower; you can pan/zoom interactively.

License: public domain / use as you wish.