#ifndef PSX2_LOGITECH_LIBLGDEV_H
#define PSX2_LOGITECH_LIBLGDEV_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

// La libreria del volante de Logitech para PS2 (liblgdev.c en el .mdebug de la A124). Es C y se compilo sin
// stabs, asi que sus funciones no traen firma: los argumentos salen de como las llaman LogiWheels y LogiForce
// (a0..t3 del EE), y los tipos de sus estructuras, de los stabs de quien las usa.

// total size: 0x18
struct lgDevPosition {
    unsigned short Status;      // offset 0x0, size 0x2
    short X1;                   // offset 0x2, size 0x2
    short Y1;                   // offset 0x4, size 0x2
    short X2;                   // offset 0x6, size 0x2
    short Y2;                   // offset 0x8, size 0x2
    unsigned short Throttle;    // offset 0xA, size 0x2
    short Twist;                // offset 0xC, size 0x2
    unsigned short Accelerator; // offset 0xE, size 0x2
    unsigned short Brake;       // offset 0x10, size 0x2
    unsigned char POV0;         // offset 0x12, size 0x1
    unsigned char POV1;         // offset 0x13, size 0x1
    unsigned int Buttons;       // offset 0x14, size 0x4
};

typedef lgDevPosition lgDevPosition2[2] __attribute__((aligned(8)));

// total size: 0x18
struct lgDevDeviceDesc {
    unsigned int DevClass;           // offset 0x0, size 0x4
    unsigned int AdditionalControls; // offset 0x4, size 0x4
    unsigned int Flags;              // offset 0x8, size 0x4
    unsigned char Topology[8];       // offset 0xC, size 0x8
    int ExistingHandle;              // offset 0x14, size 0x4
};

// total size: 0x10
struct lgDevEnvelopeParams {
    unsigned int AttackTime;  // offset 0x0, size 0x4
    unsigned int AttackLevel; // offset 0x4, size 0x4
    unsigned int FadeTime;    // offset 0x8, size 0x4
    unsigned int FadeLevel;   // offset 0xC, size 0x4
};

// total size: 0x18
struct lgDevConstantForceParams {
    int Magnitude;                // offset 0x0, size 0x4
    unsigned int Direction;       // offset 0x4, size 0x4
    lgDevEnvelopeParams envelope; // offset 0x8, size 0x10
};

// total size: 0xC
struct lgDevRampForceParams {
    int MagnitudeStart;     // offset 0x0, size 0x4
    int MagnitudeEnd;       // offset 0x4, size 0x4
    unsigned int Direction; // offset 0x8, size 0x4
};

// total size: 0x24
struct lgDevPeriodicForceParams {
    unsigned int Magnitude;       // offset 0x0, size 0x4
    unsigned int Direction;       // offset 0x4, size 0x4
    unsigned int Period;          // offset 0x8, size 0x4
    unsigned int Phase;           // offset 0xC, size 0x4
    int Offset;                   // offset 0x10, size 0x4
    lgDevEnvelopeParams envelope; // offset 0x14, size 0x10
};

// total size: 0x18
struct lgDevConditionForceParams {
    int Offset;                 // offset 0x0, size 0x4
    unsigned int Deadband;      // offset 0x4, size 0x4
    unsigned int SaturationNeg; // offset 0x8, size 0x4
    unsigned int SaturationPos; // offset 0xC, size 0x4
    int CoefficientNeg;         // offset 0x10, size 0x4
    int CoefficientPos;         // offset 0x14, size 0x4
};

// total size: 0x3C
struct lgDevForceEffect {
    unsigned char Type;      // offset 0x0, size 0x1
    unsigned int Duration;   // offset 0x4, size 0x4
    unsigned int StartDelay; // offset 0x8, size 0x4
    union {
        lgDevConstantForceParams constant;     // offset 0xC, size 0x18
        lgDevRampForceParams ramp;             // offset 0xC, size 0xC
        lgDevPeriodicForceParams periodic;     // offset 0xC, size 0x24
        lgDevConditionForceParams condition[2]; // offset 0xC, size 0x30
    } p; // offset 0xC, size 0x30
};

#ifdef __cplusplus
extern "C" {
#endif

int lgDevInit(void);
int lgDevEnumHint(int *);
int lgDevEnumerate(int, lgDevDeviceDesc *);
int lgDevOpen(int, int *);
int lgDevClose(int);
int lgDevGetDeviceProperty(int, int, int *);
int lgDevSetDeviceProperty(int, int, int);
int lgDevDownloadForceEffect(int, int *, lgDevForceEffect *);
int lgDevDestroyForceEffect(int);
int lgDevASync(int, int *);
int lgDevARead(int, int, lgDevPosition *);
int lgDevAUpdateForceEffect(int, lgDevForceEffect *);
int lgDevAStartForceEffect(int);
int lgDevAStopForceEffect(int);
int lgDevPrepareForIOPReboot(void);

#ifdef __cplusplus
}
#endif

#endif
