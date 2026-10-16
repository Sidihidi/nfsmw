#ifndef PSX2_ECSTASY_ESTRIPDATA_H
#define PSX2_ECSTASY_ESTRIPDATA_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/bWare/Inc/bMath.hpp"

// Los stabs de la A124 la dan con 1 B y ningun campo: es una VISTA sobre el bloque de quadwords del strip, y
// todos sus metodos leen y escriben ese bloque a partir de `this`. La lista de metodos y sus argumentos
// decorados salen tambien de los stabs (scripts/clase_stabs.py), en su orden.
struct eStripData {
    int GetNumLoops();
    int GetCompression();
    int GetLightMatIndex();
    void GetADCBits(unsigned int *lo, unsigned int *hi);
    void GetAABB(bVector4 *min, bVector4 *max);
    void GetXYZ(int vert, float *x, float *y, float *z);
    void GetRGBA(int vert, unsigned int *rgba);
    void GetNORM(int vert, float *x, float *y, float *z);
    void GetST0(int vert, float *s, float *t);
    void SetMipMapK(unsigned char k);
    void SetLightMaterialIndex(unsigned char index);
    void SetADCBits(unsigned int lo, unsigned int hi);
    void SetXYZ(int vert, float x, float y, float z);
    void SetRGBA(int vert, unsigned int rgba);
    void SetNORM(int vert, float x, float y, float z);
    void SetST0(int vert, float s, float t);
};

// Tamano en bytes del relleno de los quads incompletos de XYZ y de ST (datos de EcstasyE: {0, 12, 16, 24} y
// {0, 12, 12, 20}), indexados por el numero de vertices que sobran de un grupo de cuatro.
extern int XYZslop[];
extern int STTSslop[];

// La parte de PS2: el bloque del strip son cabeceras de tamano fijo (info, ADC, AABB) y luego los arrays de
// vertices, cuyo tamano depende de la compresion. Todos los metodos son en linea (las lineas 89-167 del
// eStripData.hpp de la A124) salvo los GetElement*, que estan en eStripDataE.cpp. Los bloques de los stabs dan
// el anidado: cada Get*Base es el anterior mas su tamano.
struct eStripDataE : public eStripData {
    int GetNumVerts() {
        return reinterpret_cast<unsigned char *>(this)[4];
    }
    int GetStripInfoSize() {
        return 16;
    }
    int GetCONESize() {
        return (GetCompression() & 2) ? 20 : 0;
    }
    int GetADCSize() {
        return 16;
    }
    int GetAABBSize() {
        return 24;
    }
    int GetXYZSize() {
        if (!(GetCompression() & 4)) {
            int num_verts = GetNumVerts();
            int num_packs = num_verts >> 2;
            int num_extra = 0;

            if (num_packs * 4 < num_verts) {
                num_extra = num_packs ? 4 : 0;
            }
            return 4 + num_verts * 12 + num_extra;
        } else {
            int num_verts = GetNumVerts();
            int num_quads = num_verts >> 2;
            int num_slop = num_verts & 3;
            int num_extra = XYZslop[num_slop];

            return (num_quads ? 4 : 0) + num_quads * 24 + num_extra;
        }
    }
    int GetSTSize() {
        if (!(GetCompression() & 1)) {
            int num_verts = GetNumVerts();
            int num_pairs = num_verts >> 1;
            int num_extra = 0;

            if (num_pairs * 2 < num_verts) {
                num_extra = num_pairs ? 4 : 0;
            }
            return 4 + num_verts * 8 + num_extra;
        } else {
            int num_verts = GetNumVerts();
            int num_qwords = num_verts >> 2;
            int num_slop = num_verts & 3;
            int num_extra = STTSslop[num_slop];

            return (num_qwords ? 4 : 0) + num_qwords * 16 + num_extra;
        }
    }
    int GetRGBASize() {
        if (GetCompression() & 8) {
            return GetNumVerts() * 4 + 4;
        }
        return 0;
    }
    int GetNORMSize();
    unsigned char *GetStripInfoBase() {
        return reinterpret_cast<unsigned char *>(this);
    }
    unsigned char *GetADCBase() {
        return GetStripInfoBase() + GetStripInfoSize();
    }
    unsigned char *GetAABBBase() {
        return GetADCBase() + GetADCSize();
    }
    unsigned char *GetCONEBase() {
        return GetAABBBase() + GetAABBSize();
    }
    unsigned char *GetXYZBase() {
        return GetCONEBase() + GetCONESize();
    }
    unsigned char *GetST0Base() {
        return GetXYZBase() + GetXYZSize();
    }
    unsigned char *GetRGBABase() {
        return GetST0Base() + GetSTSize();
    }
    unsigned char *GetNORMBase() {
        return GetRGBABase() + GetRGBASize();
    }
    void GetElementXYZ(int vert, float **x, float **y, float **z);
    void GetElementST0(int vert, float **s, float **t);
    void GetElementRGBA(int vert, unsigned int **rgba);
    void GetElementNORM(int vert, signed char **x, signed char **y, signed char **z);
    void GetElementNORM(int vert, float **x, float **y, float **z);
};

#endif
