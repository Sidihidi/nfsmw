#include "Speed/Indep/Src/Frontend/HUD/FeHudElement.hpp"

#include "Speed/Indep/Src/FEng/FEGroup.h"
#include "Speed/Indep/Src/FEng/FEMultiImage.h"
#include "Speed/Indep/Src/FEng/FEString.h"
#include "Speed/Indep/Src/FEng/FEImage.h"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEImages.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEStrings.hpp"

HudElement::HudElement(const char *pkg_name, HudFeaturesType mask)
    : pPackageName(pkg_name), //
      Mask(mask),             //
      CurrentHudFeatures(0),  //
      mCurrentlySetVisible(false) {}

FEString *HudElement::RegisterString(unsigned int hash) {
    FEString *str = FEngFindString(pPackageName, hash);

    if (str != nullptr) {
        Objects.AddTail(str);
    }

    return str;
}

FEImage *HudElement::RegisterImage(unsigned int hash) {
    FEImage *image = FEngFindImage(pPackageName, hash);

    if (image != nullptr) {
        Objects.AddTail(image);
    }

    return image;
}

FEMultiImage *HudElement::RegisterMultiImage(unsigned int hash) {
    FEMultiImage *image = static_cast<FEMultiImage *>(FEngFindObject(pPackageName, hash));

    if (image != nullptr) {
        Objects.AddTail(image);
    }

    return image;
}

FEObject *HudElement::RegisterObject(unsigned int hash) {
    FEObject *object = FEngFindObject(pPackageName, hash);

    if (object != nullptr) {
        Objects.AddTail(object);
    }

    return object;
}

FEGroup *HudElement::RegisterGroup(unsigned int hash) {
    FEGroup *group = static_cast<FEGroup *>(FEngFindGroup(pPackageName, hash));

    if (group != nullptr) {
        for (FEObject *object = group->GetFirstChild(); object != nullptr; object = static_cast<FEObject *>(object->FEMinNode::GetNext())) {
            if (object->Type == FE_Group) {
                RegisterGroup(object->NameHash);
            } else {
                Objects.AddTail(object);
            }
        }
    }

    return group;
}

void HudElement::Toggle(HudFeaturesType hud_features) {
    CurrentHudFeatures = hud_features;
    bool onoff = (hud_features & Mask) != 0;
    for (bPNode *n = Objects.GetHead(); n != Objects.EndOfList(); n = n->GetNext()) {
        FEObject *obj = static_cast<FEObject *>(n->GetObject());

        if (onoff) {
            FEngSetVisible(obj);
        } else {
            FEngSetInvisible(obj);
        }
    }
}
