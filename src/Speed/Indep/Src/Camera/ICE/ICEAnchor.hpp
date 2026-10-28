#ifndef CAMERA_ICE_ICEANCHOR_H
#define CAMERA_ICE_ICEANCHOR_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/Camera/ICE/ICEMath.hpp"

// total size: 0x94
struct ICEAnchor {
    ICE::Vector3 *GetVelocity() { return &mVelocity; }

    float GetVelocityMagnitude() { return mVelMag; }

    ICE::Vector3 *GetForwardVector() { return reinterpret_cast<ICE::Vector3 *>(&mGeomRot.v0); }

    ICE::Vector3 *GetUpVector() { return reinterpret_cast<ICE::Vector3 *>(&mGeomRot.v2); }

    ICE::Vector3 *GetGeometryPosition() { return &mGeomPos; }

    ICE::Vector3 *GetAcceleration() { return &mAccel; }

    ICE::Matrix4 *GetGeometryOrientation() { return &mGeomRot; }

    float GetTopSpeed() const { return mTopSpeed; }

    void SetTopSpeed(float s) { mTopSpeed = s; }

    float GetRPM() const { return mRPM; }

    void SetRPM(float s) { mRPM = s; }

    int GetNumWheels() const { return mNumWheels; }

    void SetNumWheels(int w) { mNumWheels = w; }

    int IsTouchingGround() const { return mIsTouchingGround; }

    void SetTouchingGround(bool touching) { mIsTouchingGround = touching; }

    float GetForwardSlip() const { return mForwardSlip; }

    void SetForwardSlip(float slip) { mForwardSlip = slip; }

    float GetSlipAngle() const { return mSlipAngle; }

    void SetSlipAngle(float a) { mSlipAngle = a; }

#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
    bool IsNosEngaged() const { return mIsNosEngaged; }

    void SetNosEngaged(bool engaged) { mIsNosEngaged = engaged; }
#else
    int IsNosEngaged() const { return mIsNosEngaged; }

    void SetNosEngaged(int engaged) { mIsNosEngaged = engaged; }
#endif

    float GetNosPercentageLeft() const { return mNosPercentageLeft; }

    void SetNosPercentageLeft(float percentage) { mNosPercentageLeft = percentage; }

    void Update(float dT, const ICE::Matrix4 &orientpos, const ICE::Vector3 &velocity, const ICE::Vector3 &acceleration);

    ICEAnchor();

    ICE::Vector3 mGeomPos;    // offset 0x0, size 0x10
    PS2ALIGN16 ICE::Matrix4 mGeomRot; // offset 0x10, size 0x40
    ICE::Vector3 mVelocity;   // offset 0x50, size 0x10
    ICE::Vector3 mAccel;      // offset 0x60, size 0x10
    float mVelMag;            // offset 0x70, size 0x4
    float mTopSpeed;          // offset 0x74, size 0x4
    float mRPM;               // offset 0x78, size 0x4
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
    // PC y X360 empaquetan los bytes (0x90 en total): exe 0x46A980 `mov [ecx+0x7C],al`, 0x46A9E0/0x46A9F0 byte en
    // +0x89, 0x46AA00/0x46AA10 float en +0x8C; xex 0x821800B8 `stb 0x7C`, 0x821800E8/F0 `lbz/stb 0x89`, F8/100 0x8C.
    unsigned char mNumWheels; // offset 0x7C, size 0x1
    float mForwardSlip;       // offset 0x80, size 0x4
    float mSlipAngle;         // offset 0x84, size 0x4
    bool mIsTouchingGround;   // offset 0x88, size 0x1
    bool mIsNosEngaged;       // offset 0x89, size 0x1
    float mNosPercentageLeft; // offset 0x8C, size 0x4
#else
    int mNumWheels;           // offset 0x7C, size 0x4
    float mForwardSlip;       // offset 0x80, size 0x4
    float mSlipAngle;         // offset 0x84, size 0x4
    bool mIsTouchingGround;   // offset 0x88, size 0x1
    int mIsNosEngaged;        // offset 0x8C, size 0x4
    float mNosPercentageLeft; // offset 0x90, size 0x4
#endif
};

#endif
