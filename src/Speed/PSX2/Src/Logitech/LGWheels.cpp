#include "Speed/PSX2/Src/Logitech/LGWheels.h"

#include <string.h>

LGWheels::LGWheels() {
    InitVars();
}

void LGWheels::InitVars() {
    for (int ii = 0; ii < 8; ii++) {
        for (int jj = 0; jj < 2; jj++) {
            condition.EffectID[jj][ii] = -1;
            condition.Playing[jj][ii] = false;
            constant.EffectID[jj][ii] = -1;
            constant.Playing[jj][ii] = false;
            periodic.EffectID[jj][ii] = -1;
            periodic.Playing[jj][ii] = false;
            ramp.EffectID[jj][ii] = -1;
            ramp.Playing[jj][ii] = false;
        }
    }

    for (int ii = 0; ii < 2; ii++) {
        damperWasPlaying[ii] = false;
        springWasPlaying[ii] = false;
        IsAirborne[ii] = false;

        for (int jj = 0; jj < LG_NUMBER_FORCE_EFFECTS; jj++) {
            wasPlayingBeforeAirborne[ii][jj] = false;
        }
    }
}

void LGWheels::ReadAll() {
    GetState();

    for (int ii = 0; ii < 2; ii++) {
        if (IsConnected(ii)) {
            ReadPort(ii);
        }
    }
}

void LGWheels::GetState() {
    if (wheels.EnumIsNeeded()) {
        wheels.DestroyDevices();
        InitVars();
        wheels.EnumerateDevices();
    }
}

void LGWheels::ReadPort(int channel) {
    wheels.ReadPort(channel);
    memcpy(Position, wheels.Position, sizeof(Position));
}

// Cada fuerza vive en una ranura de uno de los objetos de fuerza: el muelle, el amortiguador, el derrape y el tope
// en condition; la constante y la colision lateral en constant; y las demas en periodic.
void LGWheels::StopForce(int channel, int forceType) {
    switch (forceType) {
    case LG_SPRING:
        if (IsPlaying(channel, LG_SPRING)) {
            condition.Stop(channel, LG_CONDITION_0);
        }
        break;

    case LG_SOFTSTOP:
        if (IsPlaying(channel, LG_SOFTSTOP)) {
            condition.Stop(channel, LG_CONDITION_3);
        }
        break;

    case LG_CONSTANT:
        if (IsPlaying(channel, LG_CONSTANT)) {
            constant.Stop(channel, LG_CONSTANT_0);
        }
        break;

    case LG_DAMPER:
        if (IsPlaying(channel, LG_DAMPER)) {
            condition.Stop(channel, LG_CONDITION_1);
        }
        break;

    case LG_SIDE_COLLISION:
        if (IsPlaying(channel, LG_SIDE_COLLISION)) {
            constant.Stop(channel, LG_CONSTANT_1);
        }
        break;

    case LG_FRONTAL_COLLISION:
        if (IsPlaying(channel, LG_FRONTAL_COLLISION)) {
            periodic.Stop(channel, LG_PERIODIC_0);
        }
        break;

    case LG_DIRT_ROAD:
        if (IsPlaying(channel, LG_DIRT_ROAD)) {
            periodic.Stop(channel, LG_PERIODIC_1);
        }
        break;

    case LG_BUMPY_ROAD:
        if (IsPlaying(channel, LG_BUMPY_ROAD)) {
            periodic.Stop(channel, LG_PERIODIC_2);
        }
        break;

    // Parar el derrape devuelve el amortiguador y el muelle, que se habian apartado para dejarle sitio.
    case LG_SLIPPERY_ROAD:
        if (IsPlaying(channel, LG_SLIPPERY_ROAD)) {
            condition.Stop(channel, LG_CONDITION_2);
        }

        if (damperWasPlaying[channel]) {
            PlayDamperForce(channel, DamperForceParams[channel].coefficient);
            condition.Playing[channel][LG_CONDITION_1] = true;
            damperWasPlaying[channel] = false;
        }

        if (springWasPlaying[channel]) {
            PlaySpringForce(channel, SpringForceParams[channel].offset, SpringForceParams[channel].saturation,
                            SpringForceParams[channel].coefficient);
            condition.Playing[channel][LG_CONDITION_0] = true;
            springWasPlaying[channel] = false;
        }
        break;

    case LG_SURFACE_EFFECT:
        if (IsPlaying(channel, LG_SURFACE_EFFECT)) {
            periodic.Stop(channel, LG_PERIODIC_3);
        }
        break;

    // Aterrizar: vuelve a sonar todo lo que sonaba al despegar.
    case LG_CAR_AIRBORNE:
        if (IsPlaying(channel, LG_CAR_AIRBORNE)) {
            IsAirborne[channel] = false;

            if (wasPlayingBeforeAirborne[channel][LG_SPRING] == true) {
                PlaySpringForce(channel, SpringForceParams[channel].offset, SpringForceParams[channel].saturation,
                                SpringForceParams[channel].coefficient);
            }

            if (wasPlayingBeforeAirborne[channel][LG_CONSTANT] == true) {
                PlayConstantForce(channel, ConstantForceParams[channel].magnitude, ConstantForceParams[channel].direction);
            }

            if (wasPlayingBeforeAirborne[channel][LG_DAMPER] == true) {
                PlayDamperForce(channel, DamperForceParams[channel].coefficient);
            }

            if (wasPlayingBeforeAirborne[channel][LG_DIRT_ROAD] == true) {
                PlayDirtRoadEffect(channel, DirtRoadParams[channel].magnitude);
            }

            if (wasPlayingBeforeAirborne[channel][LG_BUMPY_ROAD] == true) {
                PlayBumpyRoadEffect(channel, BumpyRoadParams[channel].magnitude);
            }

            if (wasPlayingBeforeAirborne[channel][LG_SLIPPERY_ROAD] == true) {
                PlaySlipperyRoadEffect(channel, SlipperyRoadParams[channel].magnitude);
            }

            if (wasPlayingBeforeAirborne[channel][LG_SURFACE_EFFECT] == true) {
                PlaySurfaceEffect(channel, SurfaceEffectParams[channel].type, SurfaceEffectParams[channel].magnitude,
                                  SurfaceEffectParams[channel].period);
            }

            for (int jj = 0; jj < LG_NUMBER_FORCE_EFFECTS; jj++) {
                wasPlayingBeforeAirborne[channel][jj] = false;
            }
        }
        break;
    }
}

bool LGWheels::IsConnected(int channel) {
    return wheels.IsConnected(channel);
}

bool LGWheels::IsPlaying(int channel, int forceType) {
    switch (forceType) {
    case LG_SPRING:
        if (condition.Playing[channel][LG_CONDITION_0]) {
            return true;
        }
        break;

    case LG_SOFTSTOP:
        if (condition.Playing[channel][LG_CONDITION_3]) {
            return true;
        }
        break;

    case LG_CONSTANT:
        if (constant.Playing[channel][LG_CONSTANT_0]) {
            return true;
        }
        break;

    case LG_DAMPER:
        if (condition.Playing[channel][LG_CONDITION_1]) {
            return true;
        }
        break;

    case LG_SIDE_COLLISION:
        if (constant.Playing[channel][LG_CONSTANT_1]) {
            return true;
        }
        break;

    case LG_FRONTAL_COLLISION:
        if (periodic.Playing[channel][LG_PERIODIC_0]) {
            return true;
        }
        break;

    case LG_DIRT_ROAD:
        if (periodic.Playing[channel][LG_PERIODIC_1]) {
            return true;
        }
        break;

    case LG_BUMPY_ROAD:
        if (periodic.Playing[channel][LG_PERIODIC_2]) {
            return true;
        }
        break;

    case LG_SLIPPERY_ROAD:
        if (condition.Playing[channel][LG_CONDITION_2]) {
            return true;
        }
        break;

    case LG_SURFACE_EFFECT:
        if (periodic.Playing[channel][LG_PERIODIC_3]) {
            return true;
        }
        break;

    case LG_CAR_AIRBORNE:
        if (IsAirborne[channel] == true) {
            return true;
        }
        break;
    }

    return false;
}

bool LGWheels::ButtonIsPressed(int channel, int buttonMask) {
    return wheels.ButtonIsPressed(channel, buttonMask);
}

bool LGWheels::IsWheelClassA(int channel) {
    return wheels.IsWheelClassA(channel);
}

bool LGWheels::IsMultiturnCapable(int channel) {
    return wheels.IsMultiturnCapable(channel);
}

bool LGWheels::IsMultiturnEnabled(int channel) {
    return wheels.IsMultiturnEnabled(channel);
}

void LGWheels::SetMultiturn(int channel, int value) {
    wheels.SetMultiturn(channel, value);
}

// El muelle cede el paso al derrape.
void LGWheels::PlaySpringForce(int channel, int offset, int saturation, int coefficient) {
    if (IsPlaying(channel, LG_SLIPPERY_ROAD)) {
        return;
    }

    if (wheels.IsConnected(channel)) {
        if (!IsAirborne[channel]) {
            if (IsPlaying(channel, LG_SPRING)) {
                if (!SameSpringForceParams(channel, offset, saturation, coefficient)) {
                    if (condition.UpdateForce(channel, LG_CONDITION_0, 7, -1, 0, offset, 0, saturation, saturation, coefficient,
                                              coefficient) >= 0) {
                        SpringForceParams[channel].offset = offset;
                        SpringForceParams[channel].saturation = saturation;
                        SpringForceParams[channel].coefficient = coefficient;
                    }
                }
            } else if (condition.EffectID[channel][LG_CONDITION_0] == -1) {
                if (condition.DownloadForce(channel, LG_CONDITION_0, wheels.WheelHandles[channel], 7, -1, 0, offset, 0, saturation,
                                            saturation, coefficient, coefficient) >= 0) {
                    SpringForceParams[channel].offset = offset;
                    SpringForceParams[channel].saturation = saturation;
                    SpringForceParams[channel].coefficient = coefficient;
                }
                condition.Start(channel, LG_CONDITION_0);
            } else if (SameSpringForceParams(channel, offset, saturation, coefficient)) {
                condition.Start(channel, LG_CONDITION_0);
            } else {
                if (condition.UpdateForce(channel, LG_CONDITION_0, 7, -1, 0, offset, 0, saturation, saturation, coefficient,
                                          coefficient) >= 0) {
                    SpringForceParams[channel].offset = offset;
                    SpringForceParams[channel].saturation = saturation;
                    SpringForceParams[channel].coefficient = coefficient;
                }
                condition.Start(channel, LG_CONDITION_0);
            }
        }
    }
}

void LGWheels::StopSpringForce(int channel) {
    StopForce(channel, LG_SPRING);
}

bool LGWheels::SameSpringForceParams(int channel, int offset, int saturation, int coefficient) {
    return SpringForceParams[channel].offset == offset && SpringForceParams[channel].saturation == saturation &&
           SpringForceParams[channel].coefficient == coefficient;
}

void LGWheels::PlayConstantForce(int channel, int magnitude, int direction) {
    if (wheels.IsConnected(channel)) {
        if (!IsAirborne[channel]) {
            if (IsPlaying(channel, LG_CONSTANT)) {
                if (!SameConstantForceParams(channel, magnitude, direction)) {
                    if (constant.UpdateForce(channel, LG_CONSTANT_0, -1, 0, magnitude, direction, 0, 0, 0, 0) >= 0) {
                        ConstantForceParams[channel].magnitude = magnitude;
                        ConstantForceParams[channel].direction = direction;
                    }
                }
            } else if (constant.EffectID[channel][LG_CONSTANT_0] == -1) {
                if (constant.DownloadForce(channel, LG_CONSTANT_0, wheels.WheelHandles[channel], -1, 0, magnitude, direction, 0, 0, 0,
                                           0) >= 0) {
                    ConstantForceParams[channel].magnitude = magnitude;
                    ConstantForceParams[channel].direction = direction;
                }
                constant.Start(channel, LG_CONSTANT_0);
            } else if (SameConstantForceParams(channel, magnitude, direction)) {
                constant.Start(channel, LG_CONSTANT_0);
            } else {
                if (constant.UpdateForce(channel, LG_CONSTANT_0, -1, 0, magnitude, direction, 0, 0, 0, 0) >= 0) {
                    ConstantForceParams[channel].magnitude = magnitude;
                    ConstantForceParams[channel].direction = direction;
                }
                constant.Start(channel, LG_CONSTANT_0);
            }
        }
    }
}

void LGWheels::StopConstantForce(int channel) {
    StopForce(channel, LG_CONSTANT);
}

bool LGWheels::SameConstantForceParams(int channel, int magnitude, int direction) {
    return ConstantForceParams[channel].magnitude == magnitude && ConstantForceParams[channel].direction == direction;
}

// El amortiguador tambien cede el paso al derrape.
void LGWheels::PlayDamperForce(int channel, int coefficient) {
    if (IsPlaying(channel, LG_SLIPPERY_ROAD)) {
        return;
    }

    if (wheels.IsConnected(channel)) {
        if (!IsAirborne[channel]) {
            if (IsPlaying(channel, LG_DAMPER)) {
                if (!SameDamperForceParams(channel, coefficient)) {
                    if (condition.UpdateForce(channel, LG_CONDITION_1, 8, -1, 0, 0, 0, 255, 255, coefficient, coefficient) >= 0) {
                        DamperForceParams[channel].coefficient = coefficient;
                    }
                }
            } else if (condition.EffectID[channel][LG_CONDITION_1] == -1) {
                if (condition.DownloadForce(channel, LG_CONDITION_1, wheels.WheelHandles[channel], 8, -1, 0, 0, 0, 255, 255, coefficient,
                                            coefficient) >= 0) {
                    DamperForceParams[channel].coefficient = coefficient;
                }
                condition.Start(channel, LG_CONDITION_1);
            } else if (SameDamperForceParams(channel, coefficient)) {
                condition.Start(channel, LG_CONDITION_1);
            } else {
                if (condition.UpdateForce(channel, LG_CONDITION_1, 8, -1, 0, 0, 0, 255, 255, coefficient, coefficient) >= 0) {
                    DamperForceParams[channel].coefficient = coefficient;
                }
                condition.Start(channel, LG_CONDITION_1);
            }
        }
    }
}

void LGWheels::StopDamperForce(int channel) {
    StopForce(channel, LG_DAMPER);
}

bool LGWheels::SameDamperForceParams(int channel, int coefficient) {
    return DamperForceParams[channel].coefficient == coefficient;
}

// Colision frontal: ranura 0 de periodic, onda 3, 150 de duracion con 20 de caida. Suena aunque el coche vuele.
void LGWheels::PlayFrontalCollisionForce(int channel, int magnitude) {
    if (wheels.IsConnected(channel)) {
        if (IsPlaying(channel, LG_FRONTAL_COLLISION)) {
            if (SameFrontalCollisionForceParams(channel, magnitude)) {
                periodic.Start(channel, LG_PERIODIC_0);
            } else {
                if (periodic.UpdateForce(channel, LG_PERIODIC_0, 3, 150, 0, magnitude, 90, 75, 0, 0, 0, 20, 0, 0) >= 0) {
                    FrontalCollisionParams[channel].magnitude = magnitude;
                }
                periodic.Start(channel, LG_PERIODIC_0);
            }
        } else if (periodic.EffectID[channel][LG_PERIODIC_0] == -1) {
            if (periodic.DownloadForce(channel, LG_PERIODIC_0, wheels.WheelHandles[channel], 3, 150, 0, magnitude, 90, 75, 0, 0, 0, 20, 0,
                                       0) >= 0) {
                FrontalCollisionParams[channel].magnitude = magnitude;
            }
            periodic.Start(channel, LG_PERIODIC_0);
        } else if (SameFrontalCollisionForceParams(channel, magnitude)) {
            periodic.Start(channel, LG_PERIODIC_0);
        } else {
            if (periodic.UpdateForce(channel, LG_PERIODIC_0, 3, 150, 0, magnitude, 90, 75, 0, 0, 0, 20, 0, 0) >= 0) {
                FrontalCollisionParams[channel].magnitude = magnitude;
            }
            periodic.Start(channel, LG_PERIODIC_0);
        }
    }
}

bool LGWheels::SameFrontalCollisionForceParams(int channel, int magnitude) {
    return FrontalCollisionParams[channel].magnitude == magnitude;
}

// Los dos efectos de firme son el mismo codigo con otra ranura, otra onda y otro periodo: tierra usa la 1 con onda
// 2 y periodo 65, y baches la 2 con onda 3 y periodo 100.
void LGWheels::PlayDirtRoadEffect(int channel, int magnitude) {
    if (wheels.IsConnected(channel)) {
        if (!IsAirborne[channel]) {
            if (IsPlaying(channel, LG_DIRT_ROAD)) {
                if (!SameDirtRoadEffectParams(channel, magnitude)) {
                    if (periodic.UpdateForce(channel, LG_PERIODIC_1, 2, -1, 0, magnitude, 90, 65, 0, 0, 0, 0, 0, 0) >= 0) {
                        DirtRoadParams[channel].magnitude = magnitude;
                    }
                }
            } else if (periodic.EffectID[channel][LG_PERIODIC_1] == -1) {
                if (periodic.DownloadForce(channel, LG_PERIODIC_1, wheels.WheelHandles[channel], 2, -1, 0, magnitude, 90, 65, 0, 0, 0, 0,
                                           0, 0) >= 0) {
                    DirtRoadParams[channel].magnitude = magnitude;
                }
                periodic.Start(channel, LG_PERIODIC_1);
            } else if (SameDirtRoadEffectParams(channel, magnitude)) {
                periodic.Start(channel, LG_PERIODIC_1);
            } else {
                if (periodic.UpdateForce(channel, LG_PERIODIC_1, 2, -1, 0, magnitude, 90, 65, 0, 0, 0, 0, 0, 0) >= 0) {
                    DirtRoadParams[channel].magnitude = magnitude;
                }
                periodic.Start(channel, LG_PERIODIC_1);
            }
        }
    }
}

void LGWheels::StopDirtRoadEffect(int channel) {
    StopForce(channel, LG_DIRT_ROAD);
}

bool LGWheels::SameDirtRoadEffectParams(int channel, int magnitude) {
    return DirtRoadParams[channel].magnitude == magnitude;
}

void LGWheels::PlayBumpyRoadEffect(int channel, int magnitude) {
    if (wheels.IsConnected(channel)) {
        if (!IsAirborne[channel]) {
            if (IsPlaying(channel, LG_BUMPY_ROAD)) {
                if (!SameBumpyRoadEffectParams(channel, magnitude)) {
                    if (periodic.UpdateForce(channel, LG_PERIODIC_2, 3, -1, 0, magnitude, 90, 100, 0, 0, 0, 0, 0, 0) >= 0) {
                        BumpyRoadParams[channel].magnitude = magnitude;
                    }
                }
            } else if (periodic.EffectID[channel][LG_PERIODIC_2] == -1) {
                if (periodic.DownloadForce(channel, LG_PERIODIC_2, wheels.WheelHandles[channel], 3, -1, 0, magnitude, 90, 100, 0, 0, 0, 0,
                                           0, 0) >= 0) {
                    BumpyRoadParams[channel].magnitude = magnitude;
                }
                periodic.Start(channel, LG_PERIODIC_2);
            } else if (SameBumpyRoadEffectParams(channel, magnitude)) {
                periodic.Start(channel, LG_PERIODIC_2);
            } else {
                if (periodic.UpdateForce(channel, LG_PERIODIC_2, 3, -1, 0, magnitude, 90, 100, 0, 0, 0, 0, 0, 0) >= 0) {
                    BumpyRoadParams[channel].magnitude = magnitude;
                }
                periodic.Start(channel, LG_PERIODIC_2);
            }
        }
    }
}

void LGWheels::StopBumpyRoadEffect(int channel) {
    StopForce(channel, LG_BUMPY_ROAD);
}

bool LGWheels::SameBumpyRoadEffectParams(int channel, int magnitude) {
    return BumpyRoadParams[channel].magnitude == magnitude;
}

// El derrape vive en la ranura 2 de condition (tipo 8, coeficientes negados: el volante se afloja). Antes de sonar
// aparta al amortiguador y al muelle, y apunta cuales eran para devolverlos al parar.
void LGWheels::PlaySlipperyRoadEffect(int channel, int magnitude) {
    if (IsPlaying(channel, LG_DAMPER)) {
        StopDamperForce(channel);
        condition.Playing[channel][LG_CONDITION_1] = false;
        damperWasPlaying[channel] = true;
    }

    if (IsPlaying(channel, LG_SPRING)) {
        StopSpringForce(channel);
        condition.Playing[channel][LG_CONDITION_0] = false;
        springWasPlaying[channel] = true;
    }

    if (wheels.IsConnected(channel)) {
        if (!IsAirborne[channel]) {
            if (IsPlaying(channel, LG_SLIPPERY_ROAD)) {
                if (!SameSlipperyRoadEffectParams(channel, magnitude)) {
                    if (condition.UpdateForce(channel, LG_CONDITION_2, 8, -1, 0, 0, 0, 255, 255, -magnitude, -magnitude) >= 0) {
                        SlipperyRoadParams[channel].magnitude = magnitude;
                    }
                }
            } else if (condition.EffectID[channel][LG_CONDITION_2] == -1) {
                if (condition.DownloadForce(channel, LG_CONDITION_2, wheels.WheelHandles[channel], 8, -1, 0, 0, 0, 255, 255, -magnitude,
                                            -magnitude) >= 0) {
                    SlipperyRoadParams[channel].magnitude = magnitude;
                }
                condition.Start(channel, LG_CONDITION_2);
            } else if (SameSlipperyRoadEffectParams(channel, magnitude)) {
                condition.Start(channel, LG_CONDITION_2);
            } else {
                if (condition.UpdateForce(channel, LG_CONDITION_2, 8, -1, 0, 0, 0, 255, 255, -magnitude, -magnitude) >= 0) {
                    SlipperyRoadParams[channel].magnitude = magnitude;
                }
                condition.Start(channel, LG_CONDITION_2);
            }
        }
    }
}

void LGWheels::StopSlipperyRoadEffect(int channel) {
    StopForce(channel, LG_SLIPPERY_ROAD);
}

bool LGWheels::SameSlipperyRoadEffectParams(int channel, int magnitude) {
    return SlipperyRoadParams[channel].magnitude == magnitude;
}

// Efecto de superficie generico: ranura 3 de periodic. La onda no se puede cambiar en caliente: si cambia, se
// destruye y se vuelve a descargar la fuerza; si no, basta con actualizarla.
void LGWheels::PlaySurfaceEffect(int channel, int type, int magnitude, int period) {
    int ret = 0;

    if (wheels.IsConnected(channel)) {
        if (!IsAirborne[channel]) {
            if (IsPlaying(channel, LG_SURFACE_EFFECT)) {
                if (!SameSurfaceEffectParams(channel, type, magnitude, period)) {
                    if (type != SurfaceEffectParams[channel].type) {
                        periodic.Destroy(channel, LG_PERIODIC_3);
                        ret = periodic.DownloadForce(channel, LG_PERIODIC_3, wheels.WheelHandles[channel], type, -1, 0, magnitude, 90,
                                                     period, 0, 0, 0, 0, 0, 0);
                        periodic.Start(channel, LG_PERIODIC_3);
                    } else {
                        ret = periodic.UpdateForce(channel, LG_PERIODIC_3, type, -1, 0, magnitude, 90, period, 0, 0, 0, 0, 0, 0);
                    }

                    if (ret >= 0) {
                        SurfaceEffectParams[channel].type = type;
                        SurfaceEffectParams[channel].magnitude = magnitude;
                        SurfaceEffectParams[channel].period = period;
                    }
                }
            } else if (periodic.EffectID[channel][LG_PERIODIC_3] == -1) {
                if (periodic.DownloadForce(channel, LG_PERIODIC_3, wheels.WheelHandles[channel], type, -1, 0, magnitude, 90, period, 0, 0,
                                           0, 0, 0, 0) >= 0) {
                    SurfaceEffectParams[channel].type = type;
                    SurfaceEffectParams[channel].magnitude = magnitude;
                    SurfaceEffectParams[channel].period = period;
                }
                periodic.Start(channel, LG_PERIODIC_3);
            } else if (SameSurfaceEffectParams(channel, type, magnitude, period)) {
                periodic.Start(channel, LG_PERIODIC_3);
            } else {
                if (type != SurfaceEffectParams[channel].type) {
                    periodic.Destroy(channel, LG_PERIODIC_3);
                    ret = periodic.DownloadForce(channel, LG_PERIODIC_3, wheels.WheelHandles[channel], type, -1, 0, magnitude, 90, period,
                                                 0, 0, 0, 0, 0, 0);
                } else {
                    ret = periodic.UpdateForce(channel, LG_PERIODIC_3, type, -1, 0, magnitude, 90, period, 0, 0, 0, 0, 0, 0);
                }

                if (ret >= 0) {
                    SurfaceEffectParams[channel].type = type;
                    SurfaceEffectParams[channel].magnitude = magnitude;
                    SurfaceEffectParams[channel].period = period;
                }
                periodic.Start(channel, LG_PERIODIC_3);
            }
        }
    }
}

void LGWheels::StopSurfaceEffect(int channel) {
    StopForce(channel, LG_SURFACE_EFFECT);
}

bool LGWheels::SameSurfaceEffectParams(int channel, int type, int magnitude, int period) {
    return SurfaceEffectParams[channel].type == type && SurfaceEffectParams[channel].magnitude == magnitude &&
           SurfaceEffectParams[channel].period == period;
}

// Al despegar se para todo lo que suena y se apunta que era, para devolverlo al aterrizar (StopForce).
void LGWheels::PlayCarAirborne(int channel) {
    if (wheels.IsConnected(channel)) {
        if (!IsAirborne[channel]) {
            IsAirborne[channel] = true;

            if (IsPlaying(channel, LG_SPRING)) {
                StopSpringForce(channel);
                wasPlayingBeforeAirborne[channel][LG_SPRING] = true;
            }

            if (IsPlaying(channel, LG_CONSTANT)) {
                StopConstantForce(channel);
                wasPlayingBeforeAirborne[channel][LG_CONSTANT] = true;
            }

            if (IsPlaying(channel, LG_DAMPER)) {
                StopDamperForce(channel);
                wasPlayingBeforeAirborne[channel][LG_DAMPER] = true;
            }

            if (IsPlaying(channel, LG_DIRT_ROAD)) {
                StopDirtRoadEffect(channel);
                wasPlayingBeforeAirborne[channel][LG_DIRT_ROAD] = true;
            }

            if (IsPlaying(channel, LG_BUMPY_ROAD)) {
                StopBumpyRoadEffect(channel);
                wasPlayingBeforeAirborne[channel][LG_BUMPY_ROAD] = true;
            }

            if (IsPlaying(channel, LG_SLIPPERY_ROAD)) {
                StopSlipperyRoadEffect(channel);
                wasPlayingBeforeAirborne[channel][LG_SLIPPERY_ROAD] = true;
            }

            if (IsPlaying(channel, LG_SURFACE_EFFECT)) {
                StopSurfaceEffect(channel);
                wasPlayingBeforeAirborne[channel][LG_SURFACE_EFFECT] = true;
            }
        }
    }
}

void LGWheels::StopCarAirborne(int channel) {
    StopForce(channel, LG_CAR_AIRBORNE);
}
