#ifndef CAMERA_ICE_ICEPOINT_H
#define CAMERA_ICE_ICEPOINT_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/Camera/ICE/ICEMath.hpp"

namespace ICE {

// total size: 0x2C
struct Cubic1D {
    Cubic1D(short type = 0, float dur = 1.0f) {
        Val = 0.0f; dVal = 0.0f; ValDesired = 0.0f; dValDesired = 0.0f; time = 0.0f; duration = dur; state = 0; flags = type;

        Coeff[0] = 0.0f;
        Coeff[1] = 0.0f;
        Coeff[2] = 0.0f;
        Coeff[3] = 0.0f;
    }

    void ZeroCoeffs() {
        Coeff[0] = 0.0f;
        Coeff[1] = 0.0f;
        Coeff[2] = 0.0f;
        Coeff[3] = 0.0f;
    }

    void Snap() {
        time = 1.0f;
        Val = ValDesired;
        dVal = dValDesired;
        state = 0;
    }

    void SetVal(float pV) {
        Val = pV;
        if (pV != ValDesired) {
            state = 2;
        }
    }

    void SetdVal(float pV) {
        dVal = pV;
        if (pV != dValDesired) {
            state = 2;
        }
    }

    void SetValDesired(float pV) {
        ValDesired = pV;
        if (pV != Val) {
            state = 2;
        }
    }

    void SetdValDesired(float v) {
        dValDesired = v;
    }

    void SetDuration(float t) {
        duration = t;
    }

    void SetState(short s) {
        state = s;
    }

    void SetFlags(short f) {
        flags = f;
    }

    float GetVal() const {
        return Val;
    }

    float GetdVal() const {
        return dVal;
    }

    float GetddVal() const {
        return Coeff[1] + Coeff[1];
    }

    int HasArrived() const {
        return state == 0;
    }

    void MakeCoeffs();
    float GetVal(float t) const;
    float GetdVal(float t) const;
    float GetddVal(float t) const;
    float GetValDesired() const;
    float GetdValDesired() const;
    float GetDerivative(float t) const;
    float GetSecondDerivative(float t) const;
    void ClampDerivative(float fMag);
    void ClampSecondDerivative(float fMag);
    void Update(float fSeconds, float fDClamp, float fDDClamp);

    float Val;         // offset 0x0, size 0x4
    float dVal;        // offset 0x4, size 0x4
    float ValDesired;  // offset 0x8, size 0x4
    float dValDesired; // offset 0xC, size 0x4
    float Coeff[4];    // offset 0x10, size 0x10
    float time;        // offset 0x20, size 0x4
    float duration;    // offset 0x24, size 0x4
    short state;       // offset 0x28, size 0x2
    short flags;       // offset 0x2A, size 0x2
};

// total size: 0x84
struct Cubic3D {
    Cubic3D(short type = 0, float dur = 1.0f) : x(type, dur), y(type, dur), z(type, dur) {}

    int HasArrived() const {
        return x.HasArrived() && y.HasArrived() && z.HasArrived();
    }

    void Snap() {
        x.Snap();
        y.Snap();
        z.Snap();
    }

    void SetVal(float vx, float vy, float vz) {
        x.SetVal(vx);
        y.SetVal(vy);
        z.SetVal(vz);
    }

    void SetdVal(float vx, float vy, float vz) {
        x.SetdVal(vx);
        y.SetdVal(vy);
        z.SetdVal(vz);
    }

    void SetValDesired(float vx, float vy, float vz) {
        x.SetValDesired(vx);
        y.SetValDesired(vy);
        z.SetValDesired(vz);
    }

    void SetdValDesired(float vx, float vy, float vz) {
        x.SetdValDesired(vx);
        y.SetdValDesired(vy);
        z.SetdValDesired(vz);
    }

    void SetDuration(float t) {
        x.SetDuration(t);
        y.SetDuration(t);
        z.SetDuration(t);
    }

    void SetDuration(float tx, float ty, float tz) {
        x.SetDuration(tx);
        y.SetDuration(ty);
        z.SetDuration(tz);
    }

    void SetState(short s) {
        x.SetState(s);
        y.SetState(s);
        z.SetState(s);
    }

    void SetFlags(short s) {
        x.SetFlags(s);
        y.SetFlags(s);
        z.SetFlags(s);
    }

    void MakeCoeffs() {
        x.MakeCoeffs();
        y.MakeCoeffs();
        z.MakeCoeffs();
    }

    void SetVal(const Vector3 *pV);
    void SetdVal(const Vector3 *pV);
    void SetValDesired(const Vector3 *pV);
    void SetdValDesired(const Vector3 *pV);
    void GetVal(Vector3 *pV) const;
    void GetdVal(Vector3 *pV) const;
    void GetVal(Vector3 *pV, float t) const;
    void GetValDesired(Vector3 *pV) const;
    void Update(float fSeconds, float fDClamp, float fDDClamp);

    Cubic1D x; // offset 0x0, size 0x2C
    Cubic1D y; // offset 0x2C, size 0x2C
    Cubic1D z; // offset 0x58, size 0x2C
};

// La stab de la A124 (Point:Tt22547=s376) lista la clase completa: layout
// IDENTICO al gemelo cPoint de Misc/Point.hpp (mUpdateFunc en 0x168, vptr al
// final) y los mismos ~80 metodos. Del original solo se emitieron DOS
// out-of-line: Update(float) y el dtor (las SplineSeek/SpringDamper/accessores
// murieron inline igual que las lineas 90-282 del Point.cpp original).
struct Cubic2D;

// total size: 0x178
class Point {
  public:
    Point();
    Point(const Point &);
    Point &operator=(const Point &);

    virtual ~Point();
    virtual void Update(float fSeconds);

    int HasArrived();
    void SetCallBack(void (*func)(int, int), int p1, int p2);
    void ClearCallBack();
    void CallBack();

    static void SplineSeek(Cubic1D *p, float time, float fDClamp, float fDDClamp);
    static void SplineSeek(Cubic2D *p, float time);
    static void SplineSeek(Cubic3D *p, float time);
    static void SpringDamper(Cubic1D *p, float time);
    static void SpringDamper(Cubic2D *p, float time);
    static void SpringDamper(Cubic3D *p, float time);
    void BuildMatrix(Matrix4 *pMatrix);
    static void PointVelocity(Point *p, float time);
    static void PointVelocityWrap(Point *p, float time);
    static void PointSpline(Point *p, float time);
    static void PointSpringDamper(Point *p, float time);
    void SetPos(const Vector3 *p);
    void SetPos(float x, float y, float z);
    void SetPosX(float x);
    void SetPosY(float y, bool ratio);
    void SetPosZ(float z);
    void SetPosDesired(const Vector3 *p);
    void SetPosDesired(float x, float y, float z);
    void SetPosDesiredX(float x);
    void SetPosDesiredY(float y, bool ratio);
    void SetPosDesiredZ(float z);
    void SetdPos(const Vector3 *p);
    void SetdPos(float x, float y, float z);
    void SetdPosX(float x);
    void SetdPosY(float y);
    void SetdPosZ(float z);
    void SetdPosDesired(const Vector3 *p);
    void SetdPosDesired(float x, float y, float z);
    void SetdPosDesiredX(float x);
    void SetdPosDesiredY(float y);
    void SetdPosDesiredZ(float z);
    void SetRot(const Vector3 *p);
    void SetRot(float x, float y, float z);
    void SetRotX(float x);
    void SetRotY(float y);
    void SetRotZ(float z);
    void SetRotDesired(const Vector3 *p);
    void SetRotDesired(float x, float y, float z);
    void SetRotDesiredX(float x);
    void SetRotDesiredY(float y);
    void SetRotDesiredZ(float z);
    void SetdRot(const Vector3 *p);
    void SetdRot(float x, float y, float z);
    void SetdRotX(float x);
    void SetdRotY(float y);
    void SetdRotZ(float z);
    void SetdRotDesired(const Vector3 *p);
    void SetdRotDesired(float x, float y, float z);
    void SetdRotDesiredX(float x);
    void SetdRotDesiredY(float y);
    void SetdRotDesiredZ(float z);
    void SetScale(float s);
    void SetScaleDesired(float s);
    void SetYScale(float s);
    void SetYScaleDesired(float s);
    void SetdScale(float s);
    void SetdScaleDesired(float s);
    void SetdYScale(float s);
    void SetdYScaleDesired(float s);
    void SetDuration(float seconds);
    void Snap();
    void SetBiasZ(float ratio);

    Cubic3D mPos;                        // offset 0x0, size 0x84
    Cubic3D mRot;                        // offset 0x84, size 0x84
    Cubic1D mScale;                      // offset 0x108, size 0x2C
    Cubic1D mYScale;                     // offset 0x134, size 0x2C
    float mZBias;                        // offset 0x160, size 0x4
    void (*mCallBackFunc)(int, int);     // offset 0x164, size 0x4
    void (*mUpdateFunc)(Point *, float); // offset 0x168, size 0x4
    int mCBParam1;                       // offset 0x16C, size 0x4
    int mCBParam2;                       // offset 0x170, size 0x4
};

}; // namespace ICE

#endif
