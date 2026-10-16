// d:/mw/speed/indep/src/Physics/dynamics/Geometry.cpp

#include "Speed/Indep/Src/Physics/Dynamics/Collision.h"

static inline float RayExitSphere(const UMath::Vector4 &P, const UMath::Vector4 &D, const UMath::Vector4 &C, float r) {
    float x = 0.0f;
    float rr = r * r;
    UMath::Vector4 Pc;
    const float a = 1.0f;
    float b;
    float c;
    float q;

    UMath::Subxyz(P, C, Pc);
    b = UMath::Dotxyz(Pc, D) * 2.0f;
    c = UMath::Dotxyz(Pc, Pc) - rr;
    // Salida temprana: la A124 salta al final con `c.le.s 0,c; bc1t` (c >= 0), no con `c.lt.s; bc1f`.
    if (c >= 0.0f) {
        return x;
    }
    q = (b * b) - (c * 4.0f * a);
    if (q >= 0.0f) {
        float numerator = UMath::Sqrt(q) - b;
        const float denominator = 2.0f * a;
        x = numerator / denominator;
    }
    return x;
}

namespace Dynamics {
namespace Collision {

Geometry::Geometry() {
    mShape = UNKNOWN;
}

Geometry::Geometry(const UMath::Matrix4 &orient, const UMath::Vector3 &position, const UMath::Vector3 &dimension, Shape shape, const UMath::Vector3 &delta) {
    Set(orient, position, dimension, shape, delta);
}

void Geometry::Move(const UMath::Vector3 &deltaP) {
    UMath::Add(reinterpret_cast<UMath::Vector3 &>(mPosition), deltaP, reinterpret_cast<UMath::Vector3 &>(mPosition));
    UMath::Add(mDelta, deltaP, mDelta);
}

void Geometry::Set(const UMath::Matrix4 &orient, const UMath::Vector3 &position, const UMath::Vector3 &dimension, Shape shape, const UMath::Vector3 &delta) {
    mShape = shape;
    switch (shape) {
    case BOX:
        UMath::Vector4To3(mPosition) = position;
        mDimension[0] = dimension.x;
        mDimension[1] = dimension.y;
        mDimension[2] = dimension.z;
        mDelta = delta;
        for (unsigned int i = 0; i < 3; i++) {
            *reinterpret_cast<UMath::Vector3 *>(&mNormal[i]) = *reinterpret_cast<const UMath::Vector3 *>(&orient[i]);
            UMath::Scalexyz(mNormal[i], mDimension[i], mExtent[i]);
        }
        break;
    case SPHERE: {
        mNormal[0] = UMath::Vector4::kIdentity;
        mNormal[1] = UMath::Vector4::kIdentity;
        mNormal[2] = UMath::Vector4::kIdentity;
        mExtent[0] = UMath::Vector4::kIdentity;
        mExtent[1] = UMath::Vector4::kIdentity;
        mExtent[2] = UMath::Vector4::kIdentity;
        mPosition = UMath::Vector4Make(position, 1.0f);
        float radius = UMath::Max(dimension.x, UMath::Max(dimension.y, dimension.z));
        mDimension[2] = radius;
        mDimension[1] = radius;
        mDimension[0] = radius;
        mDelta = delta;
        break;
    }
    default:
        mShape = UNKNOWN;
        break;
    }
    mCollision_normal.w = 0.0f;
    mCollision_point.w = 0.0f;
    mOverlap = -100000.0f;
    mPenetratesOther = 0;
}

static inline bool PlaneIntersect(const UMath::Vector4 &P1, const UMath::Vector4 &P2, const UMath::Vector4 &PtOnPlane, const UMath::Vector4 &Normal,
                                 UMath::Vector4 &intersectionPt) {
    float n;
    float d;
    UMath::Vector4 PtOnPlanemP1;
    UMath::Vector4 P2mP1;

    UMath::Subxyz(PtOnPlane, P1, PtOnPlanemP1);
    UMath::Subxyz(P2, P1, P2mP1);
    d = UMath::Dotxyz(Normal, P2mP1);
    if (UMath::Abs(d) <= 0.000001f) {
        return false;
    }
    n = UMath::Dotxyz(Normal, PtOnPlanemP1);
    {
        float t = n / d;
        if (t >= 0.0f) {
            if (t <= 1.0f) {
                UMath::Subxyz(P2, P1, intersectionPt);
                UMath::ScaleAddxyz(intersectionPt, t, P1, intersectionPt);
                return true;
            }
        }
    }
    return false;
}

bool Geometry::BoxVsBox(const Geometry *GeometryA, const Geometry *GeometryB, Geometry *result) {
    const Geometry *a = GeometryA, *b = GeometryB;

    const Geometry *penetrated = 0;

    float best_interval = -100000.0f;
    int best_normal_index = -1;
    UMath::Vector4 best_penetrated_normal;

    {
        int cycle;

        for (cycle = 0; cycle <= 1; cycle++) {
            UMath::Vector4 a_vel;
            UMath::Vector4 rel_position;
            UMath::Vector4 rel_position_prev;

            if (cycle == 1) {
                a = GeometryB;
                b = GeometryA;
            }
            UMath::Sub(a->mDelta, b->mDelta, reinterpret_cast<UMath::Vector3 &>(a_vel));
            UMath::Subxyz(a->mPosition, b->mPosition, rel_position);
            UMath::Subxyz(rel_position, a_vel, rel_position_prev);
            {
                int a_lp;

                for (a_lp = 0; a_lp <= 2; a_lp++) {
                    const int index_map[3] = {0, 2, 1};
                    int penetrated_index;
                    UMath::Vector4 penetrated_normal;
                    UMath::Vector4 collision_point;
                    float dir;
                    float projected_interval;
                    const UMath::Vector4 *b_extent;
                    float v_dot;

                    penetrated_index = index_map[a_lp];
                    penetrated_normal = a->mNormal[penetrated_index];
                    collision_point = b->mPosition;
                    dir = UMath::Dotxyz(rel_position_prev, penetrated_normal);
                    if (!(dir >= 0.0f)) {
                        penetrated_normal.x = -penetrated_normal.x;
                        penetrated_normal.y = -penetrated_normal.y;
                        penetrated_normal.z = -penetrated_normal.z;
                    }
                    projected_interval = UMath::Dotxyz(rel_position, penetrated_normal);
                    projected_interval = projected_interval - a->mDimension[penetrated_index];
                    b_extent = b->mExtent;
                    {
                        int b_normal_index;

                        for (b_normal_index = 0; b_normal_index <= 2; b_normal_index++, b_extent++) {
                            const float b_projected_interval = UMath::Dotxyz(penetrated_normal, *b_extent);

                            projected_interval = projected_interval - fabsf(b_projected_interval);
                            if (!(b_projected_interval >= 0.0f)) {
                                UMath::Subxyz(collision_point, *b_extent, collision_point);
                            } else if (!(b_projected_interval <= 0.0f)) {
                                UMath::Addxyz(collision_point, *b_extent, collision_point);
                            }
                        }
                    }
                    if (projected_interval > 0.0f) {
                        return false;
                    }
                    v_dot = UMath::Dotxyz(a_vel, penetrated_normal);
                    if (!(projected_interval - v_dot <= best_interval)) {
                        best_interval = projected_interval - v_dot;
                        result->mOverlap = projected_interval;
                        result->mCollision_point = collision_point;
                        best_penetrated_normal = penetrated_normal;
                        penetrated = a;
                        best_normal_index = penetrated_index;
                        if (result != penetrated) {
                            result->mCollision_normal.x = -penetrated_normal.x;
                            result->mCollision_normal.y = -penetrated_normal.y;
                            result->mCollision_normal.z = -penetrated_normal.z;
                            result->mPenetratesOther = 1;
                        } else {
                            result->mCollision_normal.x = penetrated_normal.x;
                            result->mCollision_normal.y = penetrated_normal.y;
                            result->mCollision_normal.z = penetrated_normal.z;
                            result->mPenetratesOther = 0;
                        }
                    }
                }
            }
        }
    }

    if (penetrated != 0) {
        if (best_normal_index >= 0) {
            const int joinedfaces[3][2] = {{1, 2}, {0, 2}, {0, 1}};
            const Geometry *penetrator;
            UMath::Vector4 p_vel;
            UMath::Vector4 r_cpoint;
            UMath::Vector4 r_cpoint_prev;
            int passed;

            if (penetrated == GeometryA) {
                penetrator = GeometryB;
            } else {
                penetrator = GeometryA;
            }
            const UMath::Vector4 &cpoint = result->mCollision_point;
            UMath::Sub(penetrated->mDelta, penetrator->mDelta, reinterpret_cast<UMath::Vector3 &>(p_vel));
            UMath::Subxyz(cpoint, penetrated->mPosition, r_cpoint);
            UMath::Addxyz(r_cpoint, p_vel, r_cpoint_prev);
            passed = 0;
            {
                int pi;

                for (pi = 0; pi <= 1; pi++) {
                    int p_normal_index = joinedfaces[best_normal_index][pi];
                    UMath::Vector4 p_normal = penetrated->mNormal[p_normal_index];
                    float dir = UMath::Dotxyz(r_cpoint_prev, p_normal);

                    if (!(dir >= 0.0f)) {
                        UMath::Scalexyz(p_normal, -1.0f, p_normal);
                    }
                    const float dim = penetrated->mDimension[p_normal_index];
                    float penetration = UMath::Dotxyz(r_cpoint, p_normal);
                    UMath::Vector4 plane_point;
                    UMath::Vector4 arm;

                    if (!(penetration > dim)) {
                        passed = passed + 1;
                        continue;
                    }
                    UMath::ScaleAddxyz(p_normal, dim, penetrated->mPosition, plane_point);
                    UMath::Subxyz(cpoint, penetrator->mPosition, arm);
                    {
                        int edge_index;

                        for (edge_index = 0; edge_index <= 2; edge_index++) {
                            UMath::Vector4 edge_start;
                            UMath::Vector4 edge_intersection;
                            float edge_len;

                            edge_len = UMath::Dotxyz(arm, penetrator->mNormal[edge_index]);
                            edge_len = -(edge_len + edge_len);
                            UMath::ScaleAddxyz(penetrator->mNormal[edge_index], edge_len, cpoint, edge_start);
                            if (!PlaneIntersect(edge_start, cpoint, plane_point, p_normal, edge_intersection)) {
                                continue;
                            }
                            {
                                UMath::Vector4 r_edgepoint;
                                float overlap;

                                UMath::Subxyz(edge_intersection, penetrated->mPosition, r_edgepoint);
                                overlap = -UMath::Dotxyz(best_penetrated_normal, r_edgepoint) - penetrated->mDimension[best_normal_index];
                                if (overlap < 0.0f) {
                                    UMath::Vector4 r_edgepoint_prev;
                                    int fi;

                                    UMath::Addxyz(r_edgepoint, p_vel, r_edgepoint_prev);
                                    for (fi = 0; fi <= 1; fi++) {
                                        int face = joinedfaces[p_normal_index][fi];

                                        if (face == best_normal_index) {
                                            float dist = -UMath::Dotxyz(r_edgepoint, best_penetrated_normal);

                                            if (dist > penetrated->mDimension[face]) {
                                                break;
                                            }
                                        } else {
                                            const UMath::Vector4 &face_normal = penetrated->mNormal[face];
                                            float dist = UMath::Dotxyz(r_edgepoint, face_normal);
                                            float dir = UMath::Dotxyz(r_edgepoint_prev, face_normal);

                                            if (!(dir >= 0.0f)) {
                                                dist = -dist;
                                            }
                                            if (dist > penetrated->mDimension[face]) {
                                                break;
                                            }
                                        }
                                    }
                                    if (fi == 2) {
                                        result->mOverlap = overlap;
                                        result->mCollision_point = edge_intersection;
                                        return true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (passed == 2) {
                return true;
            }
        }
    }
    return false;
}

/* SphereVsBox: el primer bucle es un `for` y la salida por la cara va en el `else` de un if/else.
   - `for`: el mapa de GameCube pone `a_lp = 0` y los invariantes izados en la MISMA linea (447) y el
     incremento con la prueba en la llave de cierre (474), como el segundo bucle (500); los stabs de la A124
     envuelven {normal, b_dim} en un bloque sin variables, que es el del `for`.
   - if/else: el `return false;` es la linea 471 del original (nota sin codigo: su `li r3,0` se funde con los
     demas `return false`), despues del if que termina en 468.  La etiqueta del else parte el bloque: `a_lp++`
     ya no sube por encima de los stores (sched2 lo deja el ultimo, como el objetivo), las dos inserciones de
     PRE caen en las ranuras de la cadena FP y el reparto queda como el del DWARF (b_dim f31, p_dot f30, in
     r20).  En la A124 `normal` vuelve a registro (v1) y el segundo bucle deja de reducirse, como el original.
   - normal_dir con ternario (una sola linea, la 455): la A124 hace los dos stores con `bc1fl` y el mismo
     puntero.  `d >= radius * radius` y la salida temprana de RayExitSphere: la comparacion de la A124.
   - memset: GameCube lo llama como variadica (`crclr cr1eq`) y en la A124 la llamada devuelve valor (v0),
     que es la declaracion implicita `int memset(...)` del original; como aqui string.h ya lo declara, el
     cast lo reproduce (antes era `void (*)(...)`, que en la A124 cambiaba el reparto tras la llamada).
   Lo que queda en la A124 y SLUS es UMath::Sqrt (sqrt.s con dos nop en un registro preservado,
   UVectorMath.hpp:1748): cabecera. */
bool Geometry::SphereVsBox(const Geometry *sphereA, const Geometry *boxB, Geometry *result) {
    UMath::Vector4 sphere_vel;
    UMath::Vector4 sphere_rpos;
    UMath::Vector4 sphere_rpos_prev;

    UMath::Sub(sphereA->mDelta, boxB->mDelta, reinterpret_cast<UMath::Vector3 &>(sphere_vel));
    UMath::Subxyz(sphereA->mPosition, boxB->mPosition, sphere_rpos);
    UMath::Subxyz(sphere_rpos, sphere_vel, sphere_rpos_prev);

    const float radius = sphereA->mDimension[0];
    result->mCollision_point = UMath::Vector4::kIdentity;
    result->mCollision_normal = UMath::Vector4::kIdentity;

    int a_lp;
    float dists[3];
    float prev_dists[3];
    float normal_dir[3];
    float penetration[3];
    float prev_penetration[3];
    float area_box[3];
    float volume_box;
    int in;
    float crossed_planes[3];
    UMath::Vector4 rel_collision_point;
    unsigned int count;

    area_box[0] = (boxB->mDimension[1] * boxB->mDimension[2]) * 4.0f;
    area_box[1] = (boxB->mDimension[0] * boxB->mDimension[2]) * 4.0f;
    area_box[2] = (boxB->mDimension[0] * boxB->mDimension[1]) * 4.0f;

    volume_box = (area_box[0] * area_box[1]) * area_box[2];
    in = 0;

    for (a_lp = 0; a_lp <= 2; a_lp++) {
        const UMath::Vector4 &normal = boxB->mNormal[a_lp];
        const float b_dim = boxB->mDimension[a_lp];
        float p_dot = UMath::Dotxyz(sphere_rpos_prev, normal);
        normal_dir[a_lp] = p_dot > 0.0f ? 1.0f : -1.0f;
        dists[a_lp] = normal_dir[a_lp] * UMath::Dotxyz(sphere_rpos, normal);
        prev_dists[a_lp] = normal_dir[a_lp] * p_dot;
        if (dists[a_lp] < b_dim + radius) {
            if (dists[a_lp] <= b_dim) {
                in++;
            }
            penetration[a_lp] = b_dim - (dists[a_lp] - radius);
            prev_penetration[a_lp] = b_dim - (prev_dists[a_lp] - radius);
        } else {
            return false;
        }
    }

    ((int (*)(...))memset)(crossed_planes, 0, 12);
    rel_collision_point = UMath::Vector4::kZero;
    count = 0;

    if (!(in > 2)) {
        for (a_lp = 0; a_lp <= 2; a_lp++) {
            const float b_dim = boxB->mDimension[a_lp];
            if (prev_dists[a_lp] > b_dim) {
                count = count + 1;
                const UMath::Vector4 &normal = boxB->mNormal[a_lp];
                const float depth = penetration[a_lp] - radius;
                UMath::ScaleAddxyz(normal, normal_dir[a_lp] * depth, rel_collision_point, rel_collision_point);
                crossed_planes[a_lp] = 1.0f;
            }
        }
    }

    if (count != 0) {
        UMath::Vector4 point_c;
        UMath::Addxyz(rel_collision_point, sphereA->mPosition, result->mCollision_point);
        const UMath::Vector4 &point_1 = result->mCollision_point;
        UMath::Vector4 point_0;
        UMath::Subxyz(sphereA->mPosition, sphere_vel, point_0);
        UMath::Vector4 v;
        UMath::Subxyz(sphereA->mPosition, point_0, v);
        UMath::Vector4 w;
        UMath::Subxyz(point_1, point_0, w);
        float c2 = UMath::Dotxyz(v, v);
        float c1 = UMath::Dotxyz(w, v);

        if (c2 > 0.000001f && c1 < c2 && c1 > 0.0f) {
            float t = c1 / c2;
            UMath::ScaleAddxyz(v, t, point_0, point_c);
            const float rr = radius * radius;
            float xx = UMath::DistanceSquarexyz(point_c, point_1);
            float rr_xx = rr - xx;
            if (!(rr_xx >= 0.000001f)) {
                return false;
            }
            if (volume_box <= 0.0f) {
                for (a_lp = 0; a_lp <= 2; a_lp++) {
                    if (crossed_planes[a_lp] * area_box[a_lp] > 0.0f) {
                        UMath::Scalexyz(boxB->mNormal[a_lp], normal_dir[a_lp], result->mCollision_normal);
                        result->mOverlap = -penetration[a_lp];
                        break;
                    }
                }
                if (a_lp == 3) {
                    return false;
                }
            } else {
                float y = UMath::Sqrt(rr_xx);
                UMath::Vector4 norm_motion;
                UMath::Vector4 point_t;
                UMath::Scalexyz(v, 1.0f / UMath::Sqrt(c2), norm_motion);
                UMath::ScaleAddxyz(norm_motion, -y, point_c, point_t);
                UMath::Subxyz(point_t, point_1, result->mCollision_normal);
                UMath::Unitxyz(result->mCollision_normal);
                result->mOverlap = UMath::Sqrt(xx) - radius;
            }
        } else if (volume_box <= 0.0f) {
            UMath::Vector4 dd;
            float d = UMath::Dot(rel_collision_point, rel_collision_point);
            if (d >= radius * radius) {
                return false;
            }
            for (a_lp = 0; a_lp <= 2; a_lp++) {
                if (crossed_planes[a_lp] * area_box[a_lp] > 0.0f) {
                    UMath::Scalexyz(boxB->mNormal[a_lp], normal_dir[a_lp], result->mCollision_normal);
                    break;
                }
            }
            if (a_lp == 3) {
                return false;
            }
            UMath::Addxyz(rel_collision_point, sphereA->mPosition, result->mCollision_point);
            UMath::Scalexyz(result->mCollision_normal, -1.0f, dd);
            d = RayExitSphere(result->mCollision_point, dd, sphereA->mPosition, radius);
            if (!(d > 0.000001f)) {
                return false;
            }
            result->mOverlap = -d;
        } else {
            UMath::Vector4 colldir;
            UMath::Unitxyz(rel_collision_point, colldir);
            float v_dot = UMath::Dotxyz(sphere_vel, colldir);
            float coldist = UMath::Lengthxyz(rel_collision_point);
            if (coldist > radius + v_dot || !(coldist > 0.000001f)) {
                return false;
            }
            result->mOverlap = UMath::Dotxyz(rel_collision_point, colldir) - radius;
            if (!(result->mOverlap < 0.0f)) {
                return false;
            }
            UMath::Addxyz(rel_collision_point, sphereA->mPosition, result->mCollision_point);
            UMath::Scalexyz(colldir, -1.0f, result->mCollision_normal);
        }

        if (!(result->mOverlap < 0.0f)) {
            return false;
        }
    } else {
        int nearestface = -1;
        for (a_lp = 0; a_lp <= 2; a_lp++) {
            if ((nearestface == -1 || prev_penetration[a_lp] < prev_penetration[nearestface]) && area_box[a_lp] > 0.0f) {
                nearestface = a_lp;
            }
        }
        if (nearestface == -1) {
            return false;
        }
        UMath::Scalexyz(boxB->mNormal[nearestface], normal_dir[nearestface], result->mCollision_normal);
        result->mOverlap = -penetration[nearestface];
        UMath::ScaleAddxyz(result->mCollision_normal, penetration[nearestface], sphereA->mPosition, result->mCollision_point);
    }

    if (result != sphereA) {
        UMath::Scalexyz(result->mCollision_normal, -1.0f, result->mCollision_normal);
        result->mPenetratesOther = 1;
    } else {
        result->mPenetratesOther = 0;
    }
    return true;
}

bool Geometry::SphereVsSphere(const Geometry *a, const Geometry *b, Geometry *result) {
    UMath::Vector4 rPos;
    UMath::Subxyz(a->mPosition, b->mPosition, rPos);
    float dist = UMath::Lengthxyz(rPos);
    float radius_total = a->mDimension[0] + b->mDimension[0];

    if (dist == 0.0f) {
        result->mCollision_normal.x = 0.0f;
        result->mCollision_normal.z = 0.0f;
        result->mCollision_normal.y = -1.0f;
        result->mCollision_point.x = a->mPosition.x;
        result->mCollision_point.x = a->mPosition.y + a->mDimension[0];
        result->mCollision_point.x = a->mPosition.z;
        result->mOverlap = -a->mDimension[0];
    } else if (dist < radius_total) {
        UMath::Unitxyz(rPos, result->mCollision_normal);
        result->mOverlap = dist - radius_total;
        UMath::ScaleAddxyz(result->mCollision_normal, b->mDimension[0] + result->mOverlap, b->mPosition, result->mCollision_point);
    } else {
        return false;
    }
    if (a != result) {
        result->mCollision_normal.x = -result->mCollision_normal.x;
        result->mCollision_normal.y = -result->mCollision_normal.y;
        result->mCollision_normal.z = -result->mCollision_normal.z;
        result->mPenetratesOther = 0;
    } else {
        result->mPenetratesOther = 1;
    }
    return true;
}

bool Geometry::BoxVsSphere(const Geometry *boxA, const Geometry *sphereB, Geometry *result) {
    return SphereVsBox(sphereB, boxA, result);
}

bool Geometry::FindIntersection(const Geometry *A, const Geometry *B, Geometry *result) {
    typedef bool (*IntersectionAlgo)(const Geometry *, const Geometry *, Geometry *);

    result->mCollision_normal.w = 0.0f;
    result->mCollision_point.w = 0.0f;
    result->mOverlap = -100000.0f;
    result->mPenetratesOther = 0;
    static const IntersectionAlgo algos[Geometry::MAXSHAPES][Geometry::MAXSHAPES] = {
        {0, 0, 0},
        {0, BoxVsBox, BoxVsSphere},
        {0, SphereVsBox, SphereVsSphere},
    };
    IntersectionAlgo handler = algos[A->mShape][B->mShape];
    if (handler == 0) {
        return false;
    }
    return handler(A, B, result);
}

}; // namespace Collision
}; // namespace Dynamics
