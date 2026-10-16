#ifndef SUPPORT_UTILITY_USTANDARD_H
#define SUPPORT_UTILITY_USTANDARD_H

// Orden de parseo del original (banco pool STL, docs/analisis/2026-10-12_d1-m1-banco-pool-bun.md):
// UVectorMath.h y UTLAllocator.h (gFastMem) ANTES que las cabeceras STL, que arrastran
// stlgc/stl/_alloc.h. Con ese orden los literales de UVectorMath (%f,%f,%f / %f,%f,%f,%f) quedan
// delante de "STL" y el pool comun de .rodata reproduce el del original (STL en 0x58).
#include "UVectorMath.h"
#include "UDefs.h"
#include "UTLAllocator.h"

#include <list>
#include <map>
#include <set>
#include <vector>
#include <deque>

// change to DECL_CONTAINER_TYPE
// clang-format off
// r29: la forma original, leida del DWARF-1 del GOWE69 (CU zDynamics 0xb2eca5..0xb2ef71):
// name() es una funcion miembro STATIC inline (dos DIE global_subroutine con AT_inline por tag,
// la segunda a nivel top con el local _name dentro) que retorna `const char *` y guarda
// `static const char* _name = FAST_NAME(#Type)`. Con FAST_NAME(NAME)==NULL el static queda
// inicializado a cero y cae en .data al PARSEAR la cabecera (medido con el compilador real);
// el original retiene 8 de estos _name de 4 B al principio del .data de cada unidad de juego
// (ver docs/analisis/2026-10-04_ronda29-tags.md). Orden de declaracion: vector, list, deque,
// map, set (deque TERCERO, es el orden de los DIE del original).
#define DECLARE_CONTAINER_TYPE(Type)\
struct _type_##Type { \
    static const char* name() {\
        static const char* _name = FAST_NAME(#Type);\
        return _name;\
    }\
};
// clang-format on

DECLARE_CONTAINER_TYPE(vector);
DECLARE_CONTAINER_TYPE(list);
DECLARE_CONTAINER_TYPE(deque);
DECLARE_CONTAINER_TYPE(map);
DECLARE_CONTAINER_TYPE(set);

namespace UTL {
namespace Std {

template <typename T, typename Tag> struct vector : public std::vector<T, UTL::Std::Allocator<T, Tag> > {
  private:
    typedef std::vector<T, UTL::Std::Allocator<T, Tag> > _Base;

  public:
    void push_back(const typename _Base::value_type &_Val) {
        _Base::push_back(_Val);
    }

    typename _Base::iterator insert(typename _Base::iterator _Where, typename _Base::const_reference _Val) {
        return _Base::insert(_Where, _Val);
    }

    typename _Base::reference operator[](typename _Base::size_type _Pos) {
        return *(_Base::begin() + _Pos);
    }
};

template <typename T, typename Tag> struct list : public std::list<T, UTL::Std::Allocator<T, Tag> > {};
template <typename Key, typename T, typename Tag> struct map : public std::map<Key, T, std::less<Key>, UTL::Std::Allocator<T, Tag> > {};
template <typename T, typename Tag> struct set : public std::set<T, std::less<T>, UTL::Std::Allocator<T, Tag> > {};
template <typename T, typename Tag> struct deque : public std::deque<T, UTL::Std::Allocator<T, Tag> > {};

}; // namespace Std
}; // namespace UTL

#endif
