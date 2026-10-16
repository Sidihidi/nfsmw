// Juice 1.5.2a: JuiceUtil.cpp, utilidades sueltas de la plataforma.
// Escrito del asm de la A124 (JuiceUtil.o: .text 0x4BB770, 1 etiqueta).
// Unidad solo enlaza en A124. En PS2 no hay IP de depuracion de Xbox que
// devolver: la funcion devuelve el literal tal cual.

namespace Juice {
namespace Util {

const char *GetXboxDebugIP() {
    return "This only works on xbox.";
}

}
}
