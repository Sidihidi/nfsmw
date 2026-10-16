#include "Point.hpp"

// ---------------------------------------------------------------------------
// Forma original (mapa $LM del .mdebug de la A124 + orden del zMisc.o original
// en A124 y SLUS): TODO el bloque tCubic1D/2D/3D + cPoint::Update/ SplineSeek
// viva en Point.cpp, no en Spline.cpp. Lineas del alpha (mdebug_lineas):
//   10-59  tCubic1D (MakeCoeffs 10, GetVal 12, GetdVal 14, GetddVal 15,
//          GetDerivative 22, GetSecondDerivative 26, ClampDerivative 30,
//          ClampSecondDerivative 34, Update 46)
//   61-62  tCubic2D (SetValDesired 61, GetVal 62)
//   66-70  tCubic3D (SetVal/SetdVal/SetValDesired 66, GetVal 67,
//          GetValDesired 69, Update 70)
//   72-73  cPoint::Update
//   75-89  cPoint::SplineSeek (1D en 75-87, 2D en 89)
//   283    ~cPoint (inline al final del fuente original)
// Las lineas 90-282 del original no dejaron NI UN marcador en ningun binario
// (0 expansiones inline): eran codigo muerto o desactivado (los ~60 accesores
// de cPoint declarados en Point.hpp, sin llamante en ninguna version); quedan
// como declaraciones, que es la unica forma no inventada.
// ---------------------------------------------------------------------------

void tCubic1D::MakeCoeffs() {
    Coeff[0] = (dVal + dValDesired) - 2.0f * (ValDesired - Val);
    Coeff[1] = ((ValDesired - Val) * 3.0f - dValDesired) - (dVal + dVal);
    Coeff[2] = dVal;
    Coeff[3] = Val;
}

float tCubic1D::GetVal(float t) {
    return ((Coeff[0] * t + Coeff[1]) * t + Coeff[2]) * t + Coeff[3];
}

float tCubic1D::GetdVal(float t) {
    return (3.0f * Coeff[0] * t + 2.0f * Coeff[1]) * t + Coeff[2];
}

float tCubic1D::GetddVal(float t) {
    return 6.0f * Coeff[0] * t + 2.0f * Coeff[1];
}

float tCubic1D::GetDerivative(float t) {
    float fDG = 1.0f / duration;
    float fG = GetdVal(t * fDG);
    float fDF = fG * fDG;
    return fDF;
}

float tCubic1D::GetSecondDerivative(float t) {
    float fDG = 1.0f / duration;
    float fG = GetddVal(t * fDG);
    float fDDF = fG * (fDG * fDG);
    return fDDF;
}

void tCubic1D::ClampDerivative(float fMag) {
    float fDf = GetDerivative(duration);
    float fDfAbs = bAbs(fDf);
    if (fDfAbs > fMag) {
        float fSign = fDfAbs / fDf;
        SetdValDesired(fSign * fMag * duration);
    }
}

void tCubic1D::ClampSecondDerivative(float fMag) {
    float fAcc0 = GetSecondDerivative(0.0f);
    float fAcc0Abs = bAbs(fAcc0);
    float fAcc1 = GetSecondDerivative(duration);
    float fAcc1Abs = bAbs(fAcc1);
    bool bNeedFix = false;
    if (fAcc0Abs > fMag) {
        float fSign = fAcc0Abs / fAcc0;
        fAcc0 = fSign * fMag;
        bNeedFix = true;
    }
    if (fAcc1Abs > fMag) {
        float fSign = fAcc1Abs / fAcc1;
        fAcc1 = fSign * fMag;
        bNeedFix = true;
    }
    if (bNeedFix) {
        float fDurationSquared = duration * duration;
        fAcc0 *= fDurationSquared;
        Coeff[1] = fAcc0 * 0.5f;
        Coeff[0] = (fAcc1 * fDurationSquared - fAcc0) * 0.16666667f; // scaf-data r71: bloque +0x187C (1/6, GCC ya lo plegaba a fmuls)
    }
}

void tCubic1D::Update(float dt, float fDClamp, float fDDClamp) {
    switch (state) {
        case 2: {
            time = 0;
            if (flags == 0) {
                state = 1;
            }
            if (fDClamp > 0.0f) {
                ClampDerivative(fDClamp);
            }
            MakeCoeffs();
            if (fDDClamp > 0.0f) {
                ClampSecondDerivative(fDDClamp);
            }
        }
        case 1: {
            if (duration > 1e-05f) {
                float interval = dt / duration;
                time += interval;
            } else {
                time = 1.0f;
            }
            if (time > 1.0f) {
                Snap();
            }
            float t = time;
            Val = GetVal(t);
            dVal = GetdVal(t);
            break;
        }
        case 0:
            break;
    }
}

void tCubic2D::SetValDesired(bVector2 *pV) {
    SetValDesired(pV->x, pV->y);
}

void tCubic2D::GetVal(bVector2 *v) {
    v->x = x.Val;
    v->y = y.Val;
}

void tCubic3D::SetVal(const bVector3 *pV) {
    SetVal(pV->x, pV->y, pV->z);
}

void tCubic3D::SetdVal(bVector3 *pV) {
    SetdVal(pV->x, pV->y, pV->z);
}

void tCubic3D::SetValDesired(bVector3 *pV) {
    SetValDesired(pV->x, pV->y, pV->z);
}

void tCubic3D::GetVal(bVector3 *v) {
    v->x = x.Val;
    v->y = y.Val;
    v->z = z.Val;
}

void tCubic3D::GetValDesired(bVector3 *v) {
    v->x = x.ValDesired;
    v->y = y.ValDesired;
    v->z = z.ValDesired;
}

void tCubic3D::Update(float dt, float maxDeriv, float maxSecondDeriv) {
    x.Update(dt, maxDeriv, maxSecondDeriv);
    y.Update(dt, maxDeriv, maxSecondDeriv);
    z.Update(dt, maxDeriv, maxSecondDeriv);
}

void cPoint::Update(float time) {
    if (mUpdateFunc) {
        mUpdateFunc(this, time);
    }
}

void cPoint::SplineSeek(tCubic1D *p, float time, float fDClamp, float fDDClamp) {
    switch (p->state) {
        case 2: {
            p->time = 0;
            if (p->flags == 0) {
                p->state = 1;
            }
            if (fDClamp > 0.0f) {
                p->ClampDerivative(fDClamp);
            }
            p->MakeCoeffs();
            if (fDDClamp > 0.0f) {
                p->ClampSecondDerivative(fDDClamp);
            }
        }
        case 1: {
            float interval = time / p->duration;
            p->time = p->time + interval;
            if (p->time > 1.0f) {
                p->time = 1.0f;
                p->Snap();
            }
            float t = p->time;
            p->Val = p->GetVal(t);
            p->dVal = p->GetdVal(t);
            break;
        }
        case 0:
            break;
    }
}

void cPoint::SplineSeek(tCubic2D *cubic, float dt) {
    SplineSeek(&cubic->x, dt, 0.0f, 0.0f);
    SplineSeek(&cubic->y, dt, 0.0f, 0.0f);
}

// STRIPPED (original: inline al final del Point.cpp, linea 283 del alpha)
cPoint::~cPoint() {}
