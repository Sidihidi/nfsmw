#ifndef INTERFACES_ILISTENER_H
#define INTERFACES_ILISTENER_H

// Como en el original: _vt.Q33Sim9Collision9IListener solo se define en zCamera (implementacion en
// SourceLists/zCamera.cpp); las demas unidades la referencian sin emitirla.
#pragma interface

namespace Sim {
namespace Collision {

class IListener {
  public:
    // Pura en el original: la ranura de OnCollision en _vt.Q33Sim9Collision9IListener
    // (0x803D3298, 24 B) es __pure_virtual. Declarada no-pura y sin definir en
    // ningun sitio, GCC 2.9 no tiene "key method" y NO emite la vtable: era uno de
    // los 3 simbolos que impedian enlazar zCamera.
    virtual void OnCollision(const class Info &cinfo) = 0;
};

} // namespace Collision
} // namespace Sim

#endif
