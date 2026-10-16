// eStrip.cpp de PC. Difiere del de GameCube: el strip no es un eSTRIP estatico sino un slot de un SlotPool (uno
// por BeginStrip), la identidad de e_strip_matrix es la guarda del else, y ex* son COPIAS separadas de e* (el
// mismo cuerpo dos veces en el binario, no llamadas).
#include "Speed/Indep/Src/Ecstasy/Ecstasy.hpp"
#include "Speed/Indep/Src/Ecstasy/eStrip.hpp"
#include "Speed/Indep/Src/Ecstasy/Texture.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/bWare/Inc/bSlotPool.hpp"

// El pool de strips y la cola de dibujo (nombres nuestros: el exe de PC no trae simbolos de esta capa).
extern SlotPool *eStripSlotPool;
extern "C" void eQueueStripDraw(class eStrip *strip, int use_previous_data, class eView *view, TextureInfo *texture,
                                bMatrix4 *matrix, int a5, int a6);

// La vista con la que se dibuja el strip (nombre nuestro).
extern class eView *e_current_view;

eStrip *e_current_strip = 0;
TextureInfo *e_strip_texture_info = 0;
int e_current_strip_vert = 0;
int e_current_strip_uv = 0;
int e_current_strip_col = 0;
bMatrix4 e_strip_matrix;

bool eBeginStrip(TextureInfo *a, int b, bMatrix4 *c) {
    e_current_strip = static_cast<eStrip *>(eStripSlotPool->Malloc());
    e_current_strip_vert = 0;
    e_current_strip_uv = 0;
    e_current_strip_col = 0;
    e_strip_texture_info = a;
    if (c) {
        bMemCpy(&e_strip_matrix, c, 0x40);
    } else {
        bIdentity(&e_strip_matrix);
    }
    return true;
}

bool eEndStrip(eView *view) {
    e_current_strip->m_NumVerts = e_current_strip_vert;
    eQueueStripDraw(e_current_strip, 0, e_current_view, e_strip_texture_info, &e_strip_matrix, 0, 0);
    return true;
}

// RGBA en memoria (r,g,b,a como bytes) a D3DCOLOR (a<<24 | r<<16 | g<<8 | b): leido byte a byte (PC 0x6BEBA0).
void eAddColour(unsigned int colour) {
    unsigned char *c = reinterpret_cast<unsigned char *>(&colour);

    e_current_strip->m_StripVerts[e_current_strip_col].col = (c[3] << 24) | (c[0] << 16) | (c[1] << 8) | c[2];
    e_current_strip_col++;
}

void eAddUV(float u, float v) {
    e_current_strip->m_StripVerts[e_current_strip_uv].u = u;
    e_current_strip->m_StripVerts[e_current_strip_uv].v = v;
    e_current_strip_uv++;
}

void eAddVertex(const bVector3 &v) {
    e_current_strip->m_StripVerts[e_current_strip_vert].x = v.x;
    e_current_strip->m_StripVerts[e_current_strip_vert].y = v.y;
    e_current_strip->m_StripVerts[e_current_strip_vert].z = v.z;
    e_current_strip_vert++;
}

bool exBeginStrip(TextureInfo *tex, int a, bMatrix4 *mat) {
    return eBeginStrip(tex, a, mat);
}

// El reparto de bytes de eAddColour, pero con union en vez de puntero: con puntero MSVC 7.1 agrupa las extracciones
// en dos copias de la palabra (el cuerpo de eAddColour); con union las hace en tres (shr 0x18 para el alto), que es
// lo que tiene el exe.
void exAddColour(unsigned int colour) {
    union {
        unsigned int u;
        unsigned char b[4];
    } u;
    u.u = colour;

    e_current_strip->m_StripVerts[e_current_strip_col].col = (u.b[3] << 24) | (u.b[0] << 16) | (u.b[1] << 8) | u.b[2];
    e_current_strip_col++;
}

void exAddUV(float u, float v) {
    e_current_strip->m_StripVerts[e_current_strip_uv].u = u;
    e_current_strip->m_StripVerts[e_current_strip_uv].v = v;
    e_current_strip_uv++;
}

void exAddVertex(const bVector3 &v) {
    e_current_strip->m_StripVerts[e_current_strip_vert].x = v.x;
    e_current_strip->m_StripVerts[e_current_strip_vert].y = v.y;
    e_current_strip->m_StripVerts[e_current_strip_vert].z = v.z;
    e_current_strip_vert++;
}

bool exEndStrip(eView *view) {
    e_current_strip->m_NumVerts = e_current_strip_vert;
    eQueueStripDraw(e_current_strip, 0, e_current_view, e_strip_texture_info, &e_strip_matrix, 0, 0);
    return true;
}
