#include "Speed/Indep/Src/FEng/FETypes.h"
#include "Speed/Indep/Src/FEng/FEObject.h"
#include "Speed/Indep/Src/FEng/FEScript.h"
#include "Speed/Indep/Src/FEng/FEGroup.h"
#include "Speed/Indep/Src/FEng/FEImage.h"
#include "Speed/Indep/Src/FEng/FEString.h"
#include "Speed/Indep/Src/FEng/FEMultiImage.h"
#include "Speed/Indep/Src/FEng/FEPackage.h"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngFont.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"
#include "Speed/Indep/Src/Ecstasy/Texture.hpp"
#include "Speed/Indep/Src/Frontend/FEObjectCallbacks.hpp"
#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"

FEObject *FEngFindObject(const char *pkg_name, unsigned int obj_hash) {
    if (pkg_name != nullptr) {
        FEPackage *pkg = cFEng::Get()->FindPackage(pkg_name);
        if (pkg != nullptr) {
            FEObject *obj = pkg->FindObjectByHash(obj_hash);
            if (obj != nullptr) {
                return obj;
            }
        }
    }
    return nullptr;
}

FEGroup *FEngFindGroup(const char *pkg_name, unsigned int grp_hash) {
    FEObject *obj = FEngFindObject(pkg_name, grp_hash);
    if (obj != nullptr && obj->Type == FE_Group) {
        return static_cast<FEGroup *>(obj);
    }
    return nullptr;
}

void FEngSetInvisible(FEObject *obj) {
    if (obj != nullptr) {
#ifdef EA_PLATFORM_PLAYSTATION2
        // Las PS2 emiten los OR por separado (or 0x400001 y or 0x2000000, banco 100% en A124 y SLUS);
        // GameCube los pliega y casa con la forma combinada.
        obj->Flags |= 1 | FF_DirtyCode;
        obj->Flags |= FF_Dirty;
#else
        obj->Flags |= 1;
        obj->Flags |= FF_DirtyCode | FF_Dirty;
#endif
        if (obj->Type == FE_Group) {
            FEGroup *group = static_cast<FEGroup *>(obj);
            FEObject *child = group->GetFirstChild();
            int num = group->GetNumChildren();
            for (int i = 0; i < num; i++) {
                FEngSetInvisible(child);
                child = child->GetNext();
            }
        }
    }
}

void FEngSetVisible(FEObject *obj) {
#ifdef EA_PLATFORM_WIN32
    // Como FEngHidePCObjects::Callback: 0x40 en PC, 0x8 en las demas.
    if (obj != nullptr && !(obj->Flags & 0x40)) {
#else
    if (obj != nullptr && !(obj->Flags & 8)) {
#endif
        obj->Flags = (obj->Flags & ~1) | FF_DirtyCode;
        obj->Flags |= FF_Dirty;
        if (obj->Type == FE_Group) {
            FEGroup *group = static_cast<FEGroup *>(obj);
            FEObject *child = group->GetFirstChild();
            int num = group->GetNumChildren();
            for (int i = 0; i < num; i++) {
                FEngSetVisible(child);
                child = child->GetNext();
            }
        }
    }
}

void FEngSetAllObjectsInPackageVisibility(const char *pkg_name, bool visible) {
    FEPackage *pkg = cFEng::Get()->FindPackage(pkg_name);
    if (pkg != nullptr) {
        ObjectVisibilitySetter vis(visible);
        pkg->ForAllObjects(vis);
    }
}

void FEngSetScript(FEObject *object, uint32 script_hash, bool start_at_beginning) {
    if (object != nullptr) {
        FEScript *script = object->FindScript(script_hash);
        if (script != nullptr) {
            object->SetScript(script, false);
            if (start_at_beginning) {
                script->CurTime = 0;
            }
        }
    }
}

void FEngSetScript(const char *pkg_name, uint32 obj_hash, uint32 script_hash, bool start_at_beginning) {
    FEObject *object = FEngFindObject(pkg_name, obj_hash);
    FEngSetScript(object, script_hash, start_at_beginning);
}

FEScript *FEngGetScript(FEObject *object, uint32 script_hash) {
    if (object != nullptr) {
        return object->FindScript(script_hash);
    }
    return nullptr;
}

FEScript *FEngGetScript(const char *pkg_name, unsigned int obj_hash, unsigned int script_hash) {
    FEObject *obj = FEngFindObject(pkg_name, obj_hash);
    return FEngGetScript(obj, script_hash);
}

bool FEngIsScriptSet(FEObject *obj, unsigned int script_hash);
bool FEngIsScriptRunning(FEObject *object, unsigned int script_hash);

bool FEngIsScriptSet(const char *pkg_name, unsigned int obj_hash, unsigned int script_hash) {
    FEObject *object = FEngFindObject(pkg_name, obj_hash);
    return FEngIsScriptSet(object, script_hash);
}

bool FEngIsScriptSet(FEObject *obj, unsigned int script_hash) {
    if (obj == nullptr)
        return false;
    FEScript *script = obj->pCurrentScript;
    if (script == nullptr || script->ID != script_hash)
        return false;
    return true;
}

bool FEngIsScriptRunning(const char *pkg_name, unsigned int obj_hash, unsigned int script_hash) {
    FEObject *object = FEngFindObject(pkg_name, obj_hash);
    return FEngIsScriptRunning(object, script_hash);
}

bool FEngIsScriptRunning(FEObject *object, unsigned int script_hash) {
    if (object != nullptr) {
        FEScript *cur = object->pCurrentScript;
        if (cur != nullptr && cur->ID == script_hash && cur->CurTime <= cur->Length) {
            return true;
        }
    }
    return false;
}

void FEngSetRotationZ(FEObject *obj, float angle_degrees) {
    if (obj != nullptr) {
        float rad_angle = bDegToRad(angle_degrees);
        bMatrix4 b;
        bIdentity(&b);
        float cosVal = bCos(rad_angle);
        b.v1.y = cosVal;
        b.v0.x = cosVal;
        float sinVal = bSin(rad_angle);
        b.v0.y = sinVal;
        b.v1.x = -sinVal;
        bQuaternion q;
        bMatrixToQuaternion(q, b);
        FEQuaternion feq;
        feq.x = q.x;
        feq.y = q.y;
        feq.z = q.z;
        feq.w = q.w;
        obj->SetRotation(feq, false);
    }
}

void FEngSetMultiImageRot(FEMultiImage *image, float angle_degrees) {
    if (image == nullptr) {
        return;
    }
    FEMultiImageData *image_data = static_cast<FEMultiImageData *>(image->GetObjData());
    image_data->PivotRot.z = angle_degrees;
    image->Flags |= FF_DirtyCode;
}

void FEngSetMultiImageBottomRightUVs(FEMultiImage *image, FEVector2 &bottomRightUVs, int textureNumber) {
    if (image != nullptr) {
        FEVector2 currTopLeftUVs;
        FEVector2 currBottomRightUVs;
        image->GetUVs(textureNumber, currTopLeftUVs, currBottomRightUVs);
        image->SetUVs(textureNumber, currTopLeftUVs, bottomRightUVs);
        image->Flags |= FF_DirtyCode;
    }
}

void FEngGetTopLeft(FEObject *object, float &x, float &y) {
    if (object == nullptr) {
        return;
    }

    FEObjData *data = object->GetObjData();
    FEVector3 &pos = data->Pos;
    FEVector3 &size = data->Size;

    switch (object->Type) {
        case FE_Group:
            break;
        case FE_Image:
        case FE_Movie:
        case FE_ColoredImage:
        case FE_MultiImage:
            x = pos.x - size.x * 0.5f;
            y = pos.y - size.y * 0.5f;
            break;
        case FE_String: {
            FEngFont *pFont = FindFont(object->Handle);
            if (pFont != nullptr) {
                FEString *pStr = static_cast<FEString *>(object);
                const short *characters = nullptr;
                int label_hash = pStr->GetLabelHash();
                if (!(object->Flags & 2)) {
                    short localized_string_buffer[1024];
                    if (GetLocalizedWideString(localized_string_buffer, 0x800, label_hash)) {
                        characters = localized_string_buffer;
                    }
                }
                if (characters == nullptr) {
                    characters = pStr->GetString();
                }
                float width = size.x * pFont->GetTextWidth(characters, object->Flags);
                float height = size.y * pFont->GetTextHeight(characters, pStr->Leading, object->Flags, pStr->MaxWidth, pStr->Format & FESTRING_FORMAT_WORDWRAP);
                x = pos.x + pFont->CalculateXOffset(pStr->Format, width);
                y = pos.y + pFont->CalculateYOffset(pStr->Format, height);
            }
            break;
        }
        case FE_Model:
        case FE_List:
        case FE_CodeList:
        default:
            x = pos.x;
            y = pos.y;
            break;
    }
}

void FEngSetTopLeft(FEObject *object, float x, float y) {
    if (object == nullptr) {
        return;
    }

    FEObjData *data = object->GetObjData();
    FEVector3 pos(data->Pos);
    FEVector3 size(data->Size);

    switch (object->Type) {
        case FE_Image:
        case FE_Movie:
        case FE_ColoredImage:
        case FE_MultiImage:
            pos.x = x + size.x * 0.5f;
            pos.y = y + size.y * 0.5f;
            break;
        case FE_String: {
            FEngFont *pFont = FindFont(object->Handle);
            if (pFont != nullptr) {
                FEString *pStr = static_cast<FEString *>(object);
                float width = size.x * pFont->GetTextWidth(pStr->GetString(), 0);
                float height = size.y * pFont->GetTextHeight(pStr->GetString(), pStr->Leading, 0, 0, false);
                pos.x = x - pFont->CalculateXOffset(pStr->Format, width);
                pos.y = y - pFont->CalculateYOffset(pStr->Format, height);
            }
            break;
        }
        case FE_Model:
        case FE_List:
        case FE_Group:
        case FE_CodeList:
        default:
            pos.x = x;
            pos.y = y;
            break;
    }

    object->SetPosition(pos, false);
}

void FEngGetBottomRight(FEObject *object, float &x, float &y) {
    if (object == nullptr) {
        return;
    }

    FEObjData *data = object->GetObjData();
    FEVector3 &pos = data->Pos;
    FEVector3 &size = data->Size;

    switch (object->Type) {
        case FE_Image:
        case FE_Movie:
        case FE_ColoredImage:
        case FE_MultiImage:
            x = pos.x + size.x * 0.5f;
            y = pos.y + size.y * 0.5f;
            break;
        case FE_String: {
            FEngFont *pFont = FindFont(object->Handle);
            if (pFont != nullptr) {
                FEString *pStr = static_cast<FEString *>(object);
                const short *characters = nullptr;
                int label_hash = pStr->GetLabelHash();
                if (!(object->Flags & 2)) {
                    short localized_string_buffer[1024];
                    if (GetLocalizedWideString(localized_string_buffer, 0x800, label_hash)) {
                        characters = localized_string_buffer;
                    }
                }
                if (characters == nullptr) {
                    characters = pStr->GetString();
                }
                float width = size.x * pFont->GetTextWidth(characters, 0);
                float height = size.y * pFont->GetTextHeight(characters, pStr->Leading, 0, 0, false);
                x = pos.x + pFont->CalculateXOffset(pStr->Format, width) + width;
                y = pos.y + pFont->CalculateYOffset(pStr->Format, height) + height;
            }
            break;
        }
        case FE_Model:
        case FE_List:
        case FE_Group:
        case FE_CodeList:
        default:
            x = pos.x;
            y = pos.y;
            break;
    }
}

void FEngSetBottomRight(FEObject *object, float x, float y) {
    if (object == nullptr) {
        return;
    }

    FEObjData *data = object->GetObjData();
    FEVector3 pos(data->Pos);
    FEVector3 size(data->Size);

    switch (object->Type) {
        case FE_Image:
        case FE_Movie:
        case FE_ColoredImage:
        case FE_MultiImage:
            pos.x = x - size.x * 0.5f;
            pos.y = y - size.y * 0.5f;
            break;
        case FE_String:
        case FE_Model:
        case FE_List:
        case FE_Group:
        case FE_CodeList:
        default:
            pos.x = x;
            pos.y = y;
            break;
    }

    object->SetPosition(pos, false);
}

void FEngGetCenter(FEObject *object, float &x, float &y) {
    if (object == nullptr) {
        return;
    }

    FEObjData *data = object->GetObjData();
    FEVector3 &pos = data->Pos;
    FEVector3 size(data->Size);

    switch (object->Type) {
        case FE_String: {
            FEngFont *pFont = FindFont(object->Handle);
            if (pFont != nullptr) {
                FEString *pStr = static_cast<FEString *>(object);
                const short *characters = nullptr;
                int label_hash = pStr->GetLabelHash();
                if (!(object->Flags & 2)) {
                    short localized_string_buffer[1024];
                    if (GetLocalizedWideString(localized_string_buffer, 0x800, label_hash)) {
                        characters = localized_string_buffer;
                    }
                }
                if (characters == nullptr) {
                    characters = pStr->GetString();
                }
                float width = size.x * pFont->GetTextWidth(characters, 0);
                float height = size.y * pFont->GetTextHeight(characters, pStr->Leading, 0, 0, false);
                x = pos.x + pFont->CalculateXOffset(pStr->Format, width) + width * 0.5f;
                y = pos.y + pFont->CalculateYOffset(pStr->Format, height) + height * 0.5f;
            }
            break;
        }
        case FE_Image:
        case FE_Movie:
        case FE_ColoredImage:
        case FE_MultiImage:
            x = pos.x;
            y = pos.y;
            break;
        case FE_Model:
        case FE_List:
        case FE_Group:
        case FE_CodeList:
        default:
            x = pos.x;
            y = pos.y;
            break;
    }
}

void FEngSetCenter(FEObject *object, float x, float y) {
    if (object == nullptr) {
        return;
    }

    FEObjData *data = object->GetObjData();
    FEVector3 pos(data->Pos);

    switch (object->Type) {
        case FE_String: {
            FEngFont *pFont = FindFont(object->Handle);
            if (pFont != nullptr) {
                FEString *pStr = static_cast<FEString *>(object);
                FEVector3 &size = data->Size;
                float width = size.x * pFont->GetTextWidth(pStr->GetString(), 0);
                float height = size.y * pFont->GetTextHeight(pStr->GetString(), pStr->Leading, 0, 0, false);
                pos.x = x - (pFont->CalculateXOffset(pStr->Format, width) + width * 0.5f);
                pos.y = y - (pFont->CalculateYOffset(pStr->Format, height) + height * 0.5f);
            }
            break;
        }
        case FE_Image:
        case FE_Movie:
        case FE_ColoredImage:
        case FE_MultiImage:
            pos.x = x;
            pos.y = y;
            break;
        case FE_Model:
        case FE_List:
        case FE_Group:
        case FE_CodeList:
        default:
            pos.x = x;
            pos.y = y;
            break;
    }

    object->SetPosition(pos, false);
}

float FEngGetScaleX(FEObject *object) {
    FEObjData *data = object->GetObjData();

    switch (object->Type) {
        case FE_Image:
        case FE_Movie:
        case FE_ColoredImage:
        case FE_MultiImage: {
            TextureInfo *pTex = GetTextureInfo(object->Handle, 1, 0);
            return data->Size.x / static_cast<float>(pTex->Width);
        }
        case FE_String:
            return data->Size.x;
        case FE_Model:
        case FE_List:
        case FE_Group:
        case FE_CodeList:
        default:
            return 1.0f;
    }
}

float FEngGetScaleY(FEObject *object) {
    FEObjData *data = object->GetObjData();

    switch (object->Type) {
        case FE_Image:
        case FE_Movie:
        case FE_ColoredImage:
        case FE_MultiImage: {
            TextureInfo *pTex = GetTextureInfo(object->Handle, 1, 0);
            return data->Size.y / static_cast<float>(pTex->Height);
        }
        case FE_String:
            return data->Size.y;
        case FE_Model:
        case FE_List:
        case FE_Group:
        case FE_CodeList:
        default:
            return 1.0f;
    }
}

void FEngSetScaleX(FEObject *object, float x) {
    if (object == nullptr) {
        return;
    }
    FEObjData *data = object->GetObjData();
    float scale = x;
    float size = data->Size.x;
    switch (object->Type) {
        case FE_Model:
        case FE_List:
        case FE_CodeList:
            break;
        case FE_Image:
        case FE_Movie:
        case FE_ColoredImage:
        case FE_MultiImage: {
            TextureInfo *pTex = GetTextureInfo(object->Handle, 1, 0);
            scale = x * static_cast<float>(pTex->Width);
#ifdef EA_PLATFORM_GAMECUBE
            // r54: el original de GC comparte el stfs con String/Group (fallthrough);
            // el ngccc de la epoca fundia los dos stores del break y el 3.9.3 no, asi
            // que GC lleva el fallthrough y las PS2 su break duplicado (r32-fe): las
            // cuatro formas medidas al 100/99,3 en su plataforma.
#else
            data->Size.x = scale;
            break;
#endif
        }
        // r32-fe: el GOWE69 comparte UN stfs entre Image (fallthrough) y
        // String/Group, pero el SLES-A124/SLUS original DUPLICA el store (el
        // swc1 fv1 va en el delay-slot del b del caso Image): con el
        // fallthrough nuestras PS2 pierden el 100 % (95,3, 224 vs 232 B) y con
        // el break el GC queda a 96,0. Ninguna forma casa en las cinco a la
        // vez -- se queda el break (PS2 100 %) y el GC a 96,0 (solo r30/r31
        // de asignacion de registros + los dos store que el ngccc de la epoca
        // si sabia fundir y el nuestro no).
        case FE_String:
        case FE_Group:
            data->Size.x = scale;
            break;
    }
    const float SizeEpsilon = 1e-06f;
    if (scale + SizeEpsilon < size || scale - SizeEpsilon > size) {
        object->Flags |= FF_DirtyCode;
    }
}

void FEngSetScaleY(FEObject *object, float y) {
    if (object == nullptr) {
        return;
    }
    FEObjData *data = object->GetObjData();
    float scale = y;
    float size = data->Size.y;
    switch (object->Type) {
        case FE_Model:
        case FE_List:
        case FE_CodeList:
            break;
        case FE_Image:
        case FE_Movie:
        case FE_ColoredImage:
        case FE_MultiImage: {
            TextureInfo *pTex = GetTextureInfo(object->Handle, 1, 0);
            scale = y * static_cast<float>(pTex->Height);
#ifdef EA_PLATFORM_GAMECUBE
            // r54: el original de GC comparte el stfs con String/Group (fallthrough);
            // el ngccc de la epoca fundia los dos stores del break y el 3.9.3 no, asi
            // que GC lleva el fallthrough y las PS2 su break duplicado (r32-fe): las
            // cuatro formas medidas al 100/99,3 en su plataforma.
#else
            data->Size.y = scale;
            break;
#endif
        }
        case FE_String:
        case FE_Group:
            data->Size.y = scale;
            break;
    }
    const float SizeEpsilon = 1e-06f;
    if (scale + SizeEpsilon < size || scale - SizeEpsilon > size) {
        object->Flags |= FF_DirtyCode;
    }
}

void FEngGetSize(FEObject *object, float &x, float &y) {
    if (object == nullptr) {
        return;
    }

    FEObjData *data = object->GetObjData();

    switch (object->Type) {
        case FE_Image:
        case FE_Movie:
        case FE_ColoredImage:
        case FE_MultiImage:
            x = data->Size.x;
            y = data->Size.y;
            return;
        case FE_String: {
            FEngFont *pFont = FindFont(object->Handle);
            if (pFont == nullptr) {
                return;
            }
            FEString *pStr = static_cast<FEString *>(object);
            FEVector3 &size = data->Size;
            x = size.x * pFont->GetTextWidth(pStr->GetString(), 0);
            y = size.y * pFont->GetTextHeight(pStr->GetString(), pStr->Leading, 0, 0, false);
            break;
        }
        case FE_Model:
        case FE_List:
        case FE_Group:
        case FE_CodeList:
        default:
            x = 0.0f;
            y = 0.0f;
            break;
    }
}

void FEngSetSize(FEObject *object, float x, float y) {
    if (object == nullptr) {
        return;
    }

    FEObjData *data = object->GetObjData();

    switch (object->Type) {
        case FE_Image:
        case FE_Movie:
        case FE_ColoredImage:
        case FE_MultiImage:
            data->Size.x = x;
            data->Size.y = y;
            break;
        case FE_String:
        case FE_Model:
        case FE_List:
        case FE_Group:
        case FE_CodeList:
        default:
            break;
    }

    data->Size.x = x;
    data->Size.y = y;
    object->Flags |= FF_DirtyCode;
}

void FEngGetBottomRightUV(FEImage *img, float &u, float &v) {
    if (img != nullptr) {
        TextureInfo *pTex = GetTextureInfo(img->Handle, 1, 0);
        u = img->GetImageData()->LowerRight.x * static_cast<float>(pTex->Width);
        v = img->GetImageData()->LowerRight.y * static_cast<float>(pTex->Height);
    }
}

void FEngSetBottomRightUV(FEImage *img, float u, float v) {
    if (img != nullptr) {
        TextureInfo *pTex = GetTextureInfo(img->Handle, 1, 0);
        img->GetImageData()->LowerRight.x = u / static_cast<float>(pTex->Width);
        img->GetImageData()->LowerRight.y = v / static_cast<float>(pTex->Height);
        img->Flags |= FF_DirtyCode;
    }
}

void FEngSetColor(FEObject *object, unsigned int color) {
    if (object != nullptr) {
        FEColor col(color);
        object->SetColor(col, false);
    }
}

FEColor FEngGetObjectColor(FEObject *obj) {
    if (obj != nullptr) {
        FEObjData *data = obj->GetObjData();
        return data->Col;
    }
    return FEColor(0);
}

void FixInvertedRect(FERect &rect) {
    if (rect.left > rect.right) {
        float tmp = rect.left;
        rect.left = rect.right;
        rect.right = tmp;
    }
    if (rect.top > rect.bottom) {
        float tmp = rect.top;
        rect.top = rect.bottom;
        rect.bottom = tmp;
    }
}

bool FEngGet2DExtentsForMouse(FEObject *pObject, FERect &Rect, FEVector2 offset);

bool FEngTestForIntersection(float xPos, float yPos, FEObject *obj) {
    if (obj == nullptr) {
        return false;
    }
    FERect Rect(40000.0f, 40000.0f, -40000.0f, -40000.0f);
    FEngGet2DExtentsForMouse(obj, Rect, FEVector2());
    FixInvertedRect(Rect);
    if (xPos >= Rect.left && xPos <= Rect.right && yPos >= Rect.top && yPos <= Rect.bottom) {
        return true;
    }
    return false;
}

// STRIPPED en GameCube (Range 0xFFFFFFFF en el DWARF); las dos PS2 la tienen
bool FEngTestForIntersection(const float xPos, const float yPos, const bVector2 &top_left, const bVector2 &size) {
    FERect rect(top_left.x, top_left.y, top_left.x + size.x, top_left.y + size.y);

    FixInvertedRect(rect);
    if (xPos >= rect.left && xPos <= rect.right && yPos >= rect.top && yPos <= rect.bottom) {
        return true;
    }
    return false;
}

bool FEngGet2DExtentsForMouse(FEObject *pObject, FERect &Rect, FEVector2 offset) {
    if (pObject == nullptr) {
        return false;
    }

    switch (pObject->Type) {
        case FE_Group: {
            FEGroup *grp = static_cast<FEGroup *>(pObject);
            FEObject *pChild = grp->GetFirstChild();
            if (pChild != nullptr) {
                do {
                    FERect ChildRect(40000.0f, 40000.0f, -40000.0f, -40000.0f);
                    float cx = FEngGetCenterX(pObject);
                    float cy = FEngGetCenterY(pObject);
                    if (FEngGet2DExtentsForMouse(pChild, ChildRect, offset + FEVector2(cx, cy))) {
                        if (ChildRect.left < Rect.left) {
                            Rect.left = ChildRect.left;
                        }
                        if (ChildRect.right > Rect.right) {
                            Rect.right = ChildRect.right;
                        }
                        if (ChildRect.top < Rect.top) {
                            Rect.top = ChildRect.top;
                        }
                        if (ChildRect.bottom > Rect.bottom) {
                            Rect.bottom = ChildRect.bottom;
                        }
                    }
                    pChild = pChild->GetNext();
                } while (pChild != nullptr);
            }
            break;
        }
        case FE_Image:
            FEngGetTopLeft(pObject, Rect.left, Rect.top);
            FEngGetBottomRight(pObject, Rect.right, Rect.bottom);

            Rect.left += offset.x;
            Rect.right += offset.x;
            Rect.top += offset.y;
            Rect.bottom += offset.y;
            break;
        case FE_String:
            FEngGetTopLeft(pObject, Rect.left, Rect.top);
            FEngGetBottomRight(pObject, Rect.right, Rect.bottom);

            Rect.left += offset.x;
            Rect.right += offset.x;
            Rect.top += offset.y;
            Rect.bottom += offset.y;
            break;
        case FE_Movie:
        case FE_ColoredImage:
        case FE_AnimImage:
        case FE_SimpleImage:
        case FE_MultiImage:
            FEngGetTopLeft(pObject, Rect.left, Rect.top);
            FEngGetBottomRight(pObject, Rect.right, Rect.bottom);

            Rect.left += offset.x;
            Rect.right += offset.x;
            Rect.top += offset.y;
            Rect.bottom += offset.y;
            break;
        case FE_Model:
        case FE_List:
        case FE_CodeList:
            FEngGetTopLeft(pObject, Rect.left, Rect.top);
            FEngGetBottomRight(pObject, Rect.right, Rect.bottom);

            Rect.left += offset.x;
            Rect.right += offset.x;
            Rect.top += offset.y;
            Rect.bottom += offset.y;
            break;
        default:
            return false;
    }

    return true;
}
