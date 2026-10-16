#include "Spline.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"

// El Point.cpp original (A124 y SLUS) ya contenia las definiciones out-of-line
// de tCubic1D/2D/3D; viven alli (ver Point.cpp). Este TU del original no
// aportaba NINGUNA funcion al binario (0 funciones con marcador de fichero
// Spline.cpp en el .mdebug; la clase Spline no tiene definiciones en el juego:
// solo QuickSpline, en su propio TU). La clase Spline queda declarada en
// Spline.hpp sin definiciones, como en el original.
