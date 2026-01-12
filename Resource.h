#pragma once

/* Icons / dialogs */
#define IDI_MANDELBROT      101
#define IDI_SMALL           102

#define IDD_ABOUTBOX        110
#define IDD_PROPERTIES      111

/* Menu commands */
#define IDM_PROPERTIES      40001
#define IDM_EXIT            40002
#define IDM_ABOUT           40003

#define IDC_MANDELBROT      200

/* Controls - provide both naming styles so .rc and .cpp both resolve */

/* Maximum iteration */
#define IDC_MAX_ITER        1000
#define IDC_MAXITER         1000      /* alias used in the .rc */
#define IDC_MAX_ITER_SPIN   1001
#define IDC_SPIN_MAXITER    1001      /* alias used in the .rc */

/* Center / Height */
#define IDC_CENTER_REAL     1002
#define IDC_CENTER_IMAG     1003
#define IDC_HEIGHT          1004

/* Red range */
#define IDC_RED_MIN         1010
#define IDC_RMIN            1010
#define IDC_RED_MIN_SPIN    1011
#define IDC_SPIN_RMIN       1011
#define IDC_RED_MAX         1012
#define IDC_RMAX            1012
#define IDC_RED_MAX_SPIN    1013
#define IDC_SPIN_RMAX       1013

/* Green range */
#define IDC_GREEN_MIN       1020
#define IDC_GMIN            1020
#define IDC_GREEN_MIN_SPIN  1021
#define IDC_SPIN_GMIN       1021
#define IDC_GREEN_MAX       1022
#define IDC_GMAX            1022
#define IDC_GREEN_MAX_SPIN  1023
#define IDC_SPIN_GMAX       1023

/* Blue range */
#define IDC_BLUE_MIN        1030
#define IDC_BMIN            1030
#define IDC_BLUE_MIN_SPIN   1031
#define IDC_SPIN_BMIN       1031
#define IDC_BLUE_MAX        1032
#define IDC_BMAX            1032
#define IDC_BLUE_MAX_SPIN   1033
#define IDC_SPIN_BMAX       1033

/* Strings */
#define IDS_APP_TITLE       5000

/* Standard fallbacks */
#ifndef IDC_STATIC
#define IDC_STATIC          -1
#endif

#ifndef IDOK
#define IDOK                1
#endif

#ifndef IDCANCEL
#define IDCANCEL            2
#endif
