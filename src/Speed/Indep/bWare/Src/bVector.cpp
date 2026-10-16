#include "Speed/Indep/bWare/Inc/bMath.hpp"
#include <cfloat>

bVector3 bUnitVector3(1.0f, 0.0f, 0.0f);
bVector4 bUnitVector4(1.0f, 0.0f, 0.0f, 0.0f);
bVector3 bVector3FLT_MAX(FLT_MAX, FLT_MAX, FLT_MAX);

#ifndef EA_PLATFORM_PLAYSTATION2
float bDistBetween(const bVector3 *v1, const bVector3 *v2) {
    float x = v1->x - v2->x;
    float y = v1->y - v2->y;
    float z = v1->z - v2->z;

    return bSqrt(x * x + y * y + z * z);
}
#endif

// Dropped by the linker (DWARF range 0xFFFFFFFF). Written from the DWARF: the locals x, y, z, w
// and the inline bSqrt.
float bDistBetween(const bVector4 *v1, const bVector4 *v2) {
    float x = v1->x - v2->x;
    float y = v1->y - v2->y;
    float z = v1->z - v2->z;
    float w = v1->w - v2->w;

    return bSqrt(x * x + y * y + z * z + w * w);
}

bVector2 *bNormalize(bVector2 *dest, const bVector2 *v) {
    float len = bLength(v);

    if (len != 0.0f) {
        float x = v->x;
        float y = v->y;
        dest->x = x / len;
        dest->y = y / len;
    } else {
        dest->x = 1.0f;
        dest->y = 0.0f;
    }
    return dest;
}

// STRIPPED
bVector2 *bNormalize(bVector2 *dest, const bVector2 *v, float length) {
    float len = bLength(v) / length;

    if (len != 0.0f) {
        float result = 1.0f / len;
        float x = v->x;
        float y = v->y;
        dest->x = x * result;
        dest->y = y * result;
    } else {
        dest->x = length;
        dest->y = 0.0f;
    }
    return dest;
}

#ifndef EA_PLATFORM_PLAYSTATION2
bVector3 *bNormalize(bVector3 *dest, const bVector3 *v) {
    float len = bLength(v);

    if (len != 0.0f) {
        float inv_len = 1.0f / len;
        float x = v->x;
        float y = v->y;
        float z = v->z;
        dest->x = x * inv_len;
        dest->y = y * inv_len;
        dest->z = z * inv_len;
    } else {
        dest->x = 1.0f;
        dest->y = 0.0f;
        dest->z = 0.0f;
    }
    return dest;
}

bVector3 *bNormalize(bVector3 *dest, const bVector3 *v, float length) {
    float len = bLength(v) / length;

    if (len != 0.0f) {
        float inv_len = 1.0f / len;
        float x = v->x;
        float y = v->y;
        float z = v->z;
        dest->x = x * inv_len;
        dest->y = y * inv_len;
        dest->z = z * inv_len;
    } else {
        dest->x = length;
        dest->y = 0.0f;
        dest->z = 0.0f;
    }
    return dest;
}
#endif

// En PS2 va en linea y en la VU0 (bMath.hpp): el original no la emite fuera de linea.
#ifndef EA_PLATFORM_PLAYSTATION2
bVector4 *bNormalize(bVector4 *dest, const bVector4 *v) {
    float len = bLength(v);
    if (len != 0.0f) {
        float inv_len = 1.0f / len;
        float x = v->x;
        float y = v->y;
        float z = v->z;
        dest->x = x * inv_len;
        dest->y = y * inv_len;
        dest->z = z * inv_len;
    } else {
        dest->x = 1.0f;
        dest->y = 0.0f;
        dest->z = 0.0f;
    }
    return dest;
}
#endif

bVector2 *bScaleAdd(bVector2 *dest, const bVector2 *v1, const bVector2 *v2, float scale) {
    float x = v2->x * scale + v1->x;
    float y = v2->y * scale + v1->y;
    dest->x = x;
    dest->y = y;
    return dest;
}

#ifndef EA_PLATFORM_PLAYSTATION2
bVector3 *bScaleAdd(bVector3 *dest, const bVector3 *v1, const bVector3 *v2, float scale) {
    float x = v2->x * scale + v1->x;
    float y = v2->y * scale + v1->y;
    float z = v2->z * scale + v1->z;
    dest->x = x;
    dest->y = y;
    dest->z = z;
    return dest;
}
#endif

// En PS2 va en linea y en la VU0 (bMath.hpp): el original no la emite fuera de linea.
#ifndef EA_PLATFORM_PLAYSTATION2
bVector4 *bScaleAdd(bVector4 *dest, const bVector4 *v1, const bVector4 *v2, float scale) {
    float x = v2->x * scale + v1->x;
    float y = v2->y * scale + v1->y;
    float z = v2->z * scale + v1->z;
    float w = v2->w * scale + v1->w;
    dest->x = x;
    dest->y = y;
    dest->z = z;
    dest->w = w;
    return dest;
}
#endif

int bEqual(const bVector2 *v1, const bVector2 *v2, float epsilon) {
    // Las dos comparaciones son `> epsilon' (c.lt.s epsilon,abs en la A124): la segunda con `<= epsilon' daria
    // c.le.s y bc1t en vez de c.lt.s y bc1f.
    if (bAbs(v1->x - v2->x) > epsilon) {
        return 0;
    }
    if (bAbs(v1->y - v2->y) > epsilon) {
        return 0;
    }
    return 1;
}

// STRIPPED
int bEqual(const bVector3 *v1, const bVector3 *v2, float epsilon) {}

// STRIPPED
int bEqual(const bVector4 *v1, const bVector4 *v2, float epsilon) {}

#ifndef EA_PLATFORM_PLAYSTATION2
bVector3 *bCross(bVector3 *dest, const bVector3 *v1, const bVector3 *v2) {
    float x = v1->y * v2->z - v1->z * v2->y;
    float y = v1->z * v2->x - v1->x * v2->z;
    float z = v1->x * v2->y - v1->y * v2->x;
    dest->x = x;
    dest->y = y;
    dest->z = z;
    return dest;
}
#endif

void bInitializeBoundingBox(bVector2 *bbox_min, bVector2 *bbox_max) {
    bbox_min->x = FLT_MAX;
    bbox_min->y = FLT_MAX;

    bbox_max->x = -FLT_MAX;
    bbox_max->y = -FLT_MAX;
}

// STRIPPED
void bInitializeBoundingBox(bVector2 *bbox_min, bVector2 *bbox_max, const bVector2 *point) {}

void bExpandBoundingBox(bVector2 *bbox_min, bVector2 *bbox_max, const bVector2 *point) {
    float min_x = bMin(bbox_min->x, point->x);
    float min_y = bMin(bbox_min->y, point->y);

    float max_x = bMax(bbox_max->x, point->x);
    float max_y = bMax(bbox_max->y, point->y);

    bFill(bbox_min, min_x, min_y);
    bFill(bbox_max, max_x, max_y);
}

// STRIPPED
void bExpandBoundingBox(bVector2 *bbox_min, bVector2 *bbox_max, const bVector2 *point, float extra_width) {}

// STRIPPED
void bExpandBoundingBox(bVector2 *bbox_min, bVector2 *bbox_max, const bVector2 *bbox2_min, const bVector2 *bbox2_max) {}

int bBoundingBoxIsInside(const bVector2 *bbox_min, const bVector2 *bbox_max, const bVector2 *point, float extra_width) {
    if ((point->x < bbox_min->x - extra_width) || (point->x > bbox_max->x + extra_width) || (point->y < bbox_min->y - extra_width) ||
        (point->y > bbox_max->y + extra_width)) {
        return false;
    }
    return true;
}

// STRIPPED
int bBoundingBoxIsInside(const bVector2 *bbox_min, const bVector2 *bbox_max, const bVector2 *bbox2_min, const bVector2 *bbox2_max) {}

int bBoundingBoxOverlapping(const bVector2 *bbox_min, const bVector2 *bbox_max, const bVector2 *bbox2_min, const bVector2 *bbox2_max) {
    if ((bbox2_min->x < bbox_max->x) && (bbox2_max->x > bbox_min->x) && (bbox2_min->y < bbox_max->y) && (bbox2_max->y > bbox_min->y)) {
        return true;
    }
    return false;
}

// STRIPPED
float bBoundingBoxDistOutside(const bVector2 *bbox_min, const bVector2 *bbox_max, const bVector2 *point) {}

void bInitializeBoundingBox(bVector3 *bbox_min, bVector3 *bbox_max) {
#ifdef EA_PLATFORM_PLAYSTATION2
    // En PS2 va en la VU0 (lqc2/sqc2/vsub de la A124, con la local `t' de bNeg en los stabs): una sola carga de
    // bVector3FLT_MAX que se reutiliza para el minimo y su negado para el maximo.
    bVector3Reg v(bVector3FLT_MAX);
    *bbox_min = v;
    *bbox_max = bNeg(v);
#else
    bbox_min->x = FLT_MAX;
    bbox_min->y = FLT_MAX;
    bbox_min->z = FLT_MAX;

    bbox_max->x = -FLT_MAX;
    bbox_max->y = -FLT_MAX;
    bbox_max->z = -FLT_MAX;
#endif
}

void bInitializeBoundingBox(bVector3 *bbox_min, bVector3 *bbox_max, const bVector3 *point) {
#ifdef EA_PLATFORM_PLAYSTATION2
    // En PS2 es una sola carga de 128 bits (lqc2) y dos guardados (sqc2): las dos copias son la misma sentencia
    // (la conversion a bVector3Reg) y el compilador reutiliza la carga.
    bVector3Reg v(*point);
    *bbox_min = v;
    *bbox_max = v;
#else
    float x = point->x;
    float y = point->y;
    float z = point->z;

    bbox_min->x = x;
    bbox_min->y = y;
    bbox_min->z = z;

    bbox_max->x = x;
    bbox_max->y = y;
    bbox_max->z = z;
#endif
}

// STRIPPED
void bExpandBoundingBox(bVector3 *bbox_min, bVector3 *bbox_max, const bVector3 *point) {}

void bExpandBoundingBox(bVector3 *bbox_min, bVector3 *bbox_max, const bVector3 *point, float extra_width) {
#ifdef EA_PLATFORM_PLAYSTATION2
    // En la VU0 (A124 0x1608a0, 64 B): el extra se difunde a un registro con tres swc1 del float y un lqc2 (el bVector3
    // local, cuya pad queda sin escribir), punto menos/mas extra (vsub/vadd) y minimo/maximo con la caja (vmini/vmax).
    bVector3 extra(extra_width, extra_width, extra_width);
    bVector3Reg p(*point);
    bVector3Reg e(extra);
    bVector3Reg lo = p - e;
    bVector3Reg hi = p + e;
    bVector3Reg mn = bMin(bVector3Reg(*bbox_min), lo);
    bVector3Reg mx = bMax(bVector3Reg(*bbox_max), hi);

    *bbox_min = mn;
    *bbox_max = mx;
#else
    float x_min = point->x - extra_width;
    float y_min = point->y - extra_width;
    float z_min = point->z - extra_width;

    float x_max = point->x + extra_width;
    float y_max = point->y + extra_width;
    float z_max = point->z + extra_width;

    if (x_min < bbox_min->x) {
        bbox_min->x = x_min;
    }
    if (y_min < bbox_min->y) {
        bbox_min->y = y_min;
    }
    if (z_min < bbox_min->z) {
        bbox_min->z = z_min;
    }

    if (x_max > bbox_max->x) {
        bbox_max->x = x_max;
    }
    if (y_max > bbox_max->y) {
        bbox_max->y = y_max;
    }
    if (z_max > bbox_max->z) {
        bbox_max->z = z_max;
    }
#endif
}

void bExpandBoundingBox(bVector3 *bbox_min, bVector3 *bbox_max, const bVector3 *bbox2_min, const bVector3 *bbox2_max) {
#ifdef EA_PLATFORM_PLAYSTATION2
    // En la VU0 con bMin/bMax (vmini/vmax .xyzw): dos cargas, minimo, dos cargas, maximo y dos guardados (A124
    // 0x1608e0, 40 B). Los resultados se guardan al final para que el planificador no adelante el primer sqc2.
    bVector3Reg mn = bMin(bVector3Reg(*bbox_min), bVector3Reg(*bbox2_min));
    bVector3Reg mx = bMax(bVector3Reg(*bbox_max), bVector3Reg(*bbox2_max));

    *bbox_min = mn;
    *bbox_max = mx;
#else
    float x_min = bbox2_min->x;
    float y_min = bbox2_min->y;
    float z_min = bbox2_min->z;

    float x_max = bbox2_max->x;
    float y_max = bbox2_max->y;
    float z_max = bbox2_max->z;

    if (x_min < bbox_min->x) {
        bbox_min->x = bbox2_min->x;
    }
    if (y_min < bbox_min->y) {
        bbox_min->y = y_min;
    }
    if (z_min < bbox_min->z) {
        bbox_min->z = z_min;
    }

    if (x_max > bbox_max->x) {
        bbox_max->x = x_max;
    }
    if (y_max > bbox_max->y) {
        bbox_max->y = y_max;
    }
    if (z_max > bbox_max->z) {
        bbox_max->z = z_max;
    }
#endif
}

int bBoundingBoxIsInside(const bVector3 *bbox_min, const bVector3 *bbox_max, const bVector3 *point, float extra_width) {
    if ((point->x < bbox_min->x - extra_width) || (point->x > bbox_max->x + extra_width) || (point->y < bbox_min->y - extra_width) ||
        (point->y > bbox_max->y + extra_width || (point->z < bbox_min->z - extra_width) || (point->z > bbox_max->z + extra_width))) {
        return false;
    }
    return true;
}

// STRIPPED
int bBoundingBoxIsInside(const bVector3 *bbox_min, const bVector3 *bbox_max, const bVector3 *bbox2_min, const bVector3 *bbox2_max) {}

// STRIPPED
int bBoundingBoxOverlapping(const bVector3 *bbox_min, const bVector3 *bbox_max, const bVector3 *bbox2_min, const bVector3 *bbox2_max) {}

// STRIPPED
float bBoundingBoxDistOutside(const bVector3 *bbox_min, const bVector3 *bbox_max, const bVector3 *point) {}

// UNSOLVED, ProStreet scratch: https://decomp.me/scratch/VE8bd
float bDistToLine(const bVector2 *point, const bVector2 *line_p1, const bVector2 *line_p2) {
    bVector2 p = *point - *line_p1;
    bVector2 tangent(line_p2->x - line_p1->x, line_p2->y - line_p1->y);
    float length = bLength(&tangent);
    bNormalize(&tangent, &tangent);
    bVector2 normal(tangent.y, -tangent.x);
    float l = bDot(&tangent, &p);
    float d = bDot(&normal, &p);
    float distance;

    if (l < 0.0f) {
        distance = bSqrt(l * l + d * d);
    } else if (l > length) {
        distance = bSqrt((l - length) * (l - length) + d * d);
    } else {
        distance = bAbs(d);
    }

    return distance;
}

// Dropped by the linker (DWARF range 0xFFFFFFFF). Written from the DWARF: the local area and
// the nested blocks of i and j (the shoelace formula).
float bGetPolyArea(const bVector2 *points, int num_points) {
    float area = 0.0f;
    for (int i = 0; i < num_points; i++) {
        int j = (i + 1 == num_points) ? 0 : i + 1;
        area += points[i].x * points[j].y - points[j].x * points[i].y;
    }
    return area * 0.5f;
}

bool bIsPointInPoly(const bVector2 *point, const bVector2 *points, int num_points) {
    float x = point->x;
    float y = point->y;
    bool inside = false;
    int j = num_points - 1;

    for (int i = 0; i < num_points; i++) {
        if ((((points[i].y <= y) && (y < points[j].y)) || (points[j].y <= y && (y < points[i].y))) &&
            (x < ((points[j].x - points[i].x) * (y - points[i].y)) / (points[j].y - points[i].y) + points[i].x)) {
            inside = !inside;
        }
        j = i;
    }
    return inside;
}

bool bIsPointInPoly(const bVector2 *point, const bVector3 *points, int num_points) {
    float x = point->x;
    float y = point->y;
    bool inside = false;
    int j = num_points - 1;

    for (int i = 0; i < num_points; i++) {
        if ((((points[i].y <= y) && (y < points[j].y)) || (points[j].y <= y && (y < points[i].y))) &&
            (x < ((points[j].x - points[i].x) * (y - points[i].y)) / (points[j].y - points[i].y) + points[i].x)) {
            inside = !inside;
        }
        j = i;
    }
    return inside;
}
