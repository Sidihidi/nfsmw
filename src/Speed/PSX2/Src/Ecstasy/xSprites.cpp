#include "Speed/PSX2/Src/Ecstasy/XSprites.hpp"
#include "Speed/Indep/Libs/Support/Utility/UMath.h"
#include "Speed/Indep/Src/Ecstasy/Ecstasy.hpp"
#include "Speed/Indep/Src/Ecstasy/eMath.hpp"
#include "Speed/Indep/Src/Ecstasy/Texture.hpp"

XSpriteManager NGSpriteManager;

// Como en GameCube, pero sin dar la vuelta al color: PS2 es little-endian.
void XSpriteManager::AddSpark(const NGParticle &particle, TextureInfo *CurrentTexture) {
    if (this->position > 299) {
        return;
    }

    {
        UMath::Vector3 startPos;
        UMath::ScaleAdd(particle.vel, particle.age, particle.initialPos, startPos);
        startPos.z += particle.gravity * particle.age * particle.age;

        float endAge = particle.age + static_cast<float>(static_cast<int>(particle.length)) * (1.0f / 2048.0f);
        UMath::Vector3 endPos;
        UMath::ScaleAdd(particle.vel, endAge, particle.initialPos, endPos);
        endPos.z += particle.gravity * endAge * endAge;

        SpriteDef *XSpriteBufferP = &this->XSpriteBuffer[this->position];
        XSpriteBufferP->texture_info = CurrentTexture;
        XSpriteBufferP->color = particle.color;
        XSpriteBufferP->startPos = startPos;
        XSpriteBufferP->EndPosPos = endPos;
        XSpriteBufferP->width = static_cast<float>(static_cast<int>(particle.width)) * (1.0f / 2048.0f);
        this->position++;
    }
}

void XSpriteManager::RenderAll(eView *view) {
    ePoly pPoly;
    SpriteDef *XSpriteBufferP = this->XSpriteBuffer;

    for (int i = 0; i < this->position; i++) {
        *reinterpret_cast<unsigned int *>(&pPoly.Colours[0][0]) = XSpriteBufferP->color;
        *reinterpret_cast<unsigned int *>(&pPoly.Colours[1][0]) = XSpriteBufferP->color;
        *reinterpret_cast<unsigned int *>(&pPoly.Colours[2][0]) = XSpriteBufferP->color;
        *reinterpret_cast<unsigned int *>(&pPoly.Colours[3][0]) = XSpriteBufferP->color;

        pPoly.Vertices[0] = XSpriteBufferP->startPos;
        pPoly.Vertices[1] = XSpriteBufferP->startPos;
        pPoly.Vertices[1].z += XSpriteBufferP->width;

        pPoly.Vertices[3] = XSpriteBufferP->EndPosPos;
        pPoly.Vertices[2] = XSpriteBufferP->EndPosPos;
        pPoly.Vertices[2].z += XSpriteBufferP->width;

        view->Render(&pPoly, XSpriteBufferP->texture_info, eGetIdentityMatrix(), 0, 0.0f);
        XSpriteBufferP++;
    }

    this->position = 0;
}
