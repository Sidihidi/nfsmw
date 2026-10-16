#include "AnimEntity_BasicCharacter.hpp"
#include "AnimEntity_Prop.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"

IAnimEntity *CAnimEntityFactory::CreateAnimEntity(int anim_entity_type) {
    switch (anim_entity_type) {
        case eAnimEntityType_BasicCharacter:
#ifdef EA_PLATFORM_XENON
            // X360 reserva con bMalloc (4 args) dentro del placement; el resto usa ::new con texto.
            return ::new (static_cast<CBasicCharacterAnimEntity *>(bMalloc(sizeof(CBasicCharacterAnimEntity), "CBasicCharacterAnimEntity", 0, 0)))
#else
            return new ("CBasicCharacterAnimEntity", 0)
#endif
                CBasicCharacterAnimEntity();
        case eAnimEntityType_Prop:
#ifdef EA_PLATFORM_XENON
            // X360 reserva con bMalloc (4 args) dentro del placement; el resto usa ::new con texto.
            return ::new (static_cast<CPropAnimEntity *>(bMalloc(sizeof(CPropAnimEntity), "CPropAnimEntity", 0, 0)))
#else
            return new ("CPropAnimEntity", 0)
#endif
                CPropAnimEntity();
        case eAnimEntityType_None:
        default:
            return nullptr;
    }
}

void CAnimEntityFactory::EndianSwapEntityData(void *data, int size) {
    bPlatEndianSwap(reinterpret_cast<uint32 *>(data));
    uint32 anim_entity_type = *reinterpret_cast<uint32 *>(data);
    switch (anim_entity_type) {
        case eAnimEntityType_BasicCharacter:
            CBasicCharacterAnimEntity::EndianSwapEntityData(data, size);
            break;
        case eAnimEntityType_Prop:
            CPropAnimEntity::EndianSwapEntityData(data, size);
            break;
        case eAnimEntityType_None:
        default:
            break;
    }
}
