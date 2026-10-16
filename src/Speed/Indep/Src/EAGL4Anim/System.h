#ifndef EAGL4ANIM_SYSTEM_H
#define EAGL4ANIM_SYSTEM_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include <cstddef>

#include "eagl4runtimetransform.h"

namespace EAGL4Anim {

class Initializer {
  public:
    static void InitInternal(size_t memorySize, bool enableStats);
};

inline void Init(size_t memorySize, bool enableStats) {
    Initializer::InitInternal(memorySize, enableStats);
}

typedef void (*MatrixMultiplier)(EAGL4::Transform *, const EAGL4::Transform *, const EAGL4::Transform *);

extern MatrixMultiplier MatrixMultiply;

}; // namespace EAGL4Anim

#endif
