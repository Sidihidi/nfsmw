#include "Speed/Indep/Src/FEng/FETypeLib.h"
#include "Speed/Indep/Src/FEng/FETypeNode.h"
#include "Speed/Indep/Src/FEng/FETypes.h"
#include "Speed/Indep/Src/FEng/FEngStandard.h"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"
#include "Speed/Indep/Src/FEng/FEObject.h"
#include "Speed/Indep/Src/FEng/FEImage.h"
#include "Speed/Indep/Src/FEng/FEString.h"
#include "Speed/Indep/Src/FEng/FEModel.h"
#include "Speed/Indep/Src/FEng/FEListBox.h"
#include "Speed/Indep/Src/FEng/FEGroup.h"
#include "Speed/Indep/Src/FEng/FECodeListBox.h"
#include "Speed/Indep/Src/FEng/FEMovie.h"
#include "Speed/Indep/Src/FEng/FEColoredImage.h"
#include "Speed/Indep/Src/FEng/FEAnimImage.h"
#include "Speed/Indep/Src/FEng/FEMultiImage.h"
#include "Speed/Indep/Src/FEng/FEScript.h"
#include "Speed/Indep/Src/FEng/FEKeyTrack.h"




static const char *FEColor1Name = "Top Left";       // size: 0x4, address: 0x8041D160, Decl: speed/indep/src/feng/FETypeLib.cpp:23
static const char *FEColor2Name = "Top Right";      // size: 0x4, address: 0x8041D164, Decl: speed/indep/src/feng/FETypeLib.cpp:24
static const char *FEColor3Name = "Bottom Right";   // size: 0x4, address: 0x8041D168, Decl: speed/indep/src/feng/FETypeLib.cpp:25
static const char *FEColor4Name = "Bottom Left";    // size: 0x4, address: 0x8041D16C, Decl: speed/indep/src/feng/FETypeLib.cpp:26
static const char *FEFrameNumName = "Frame Number"; // size: 0x4, address: 0x8041D170, Decl: speed/indep/src/feng/FETypeLib.cpp:27






FETypeNode *FETypeLib::CreateBaseObjectType(const char *pName) {
    FETypeNode *pType;
    FEFieldNode *pField;
    FEVector3 ZeroVect, SizeVect;
    FEQuaternion ZeroQuat;
    FEColor White;

    pType = FNEW FETypeNode();
    pType->SetName(pName);

    pType->AddField("Color", PT_Color);
    pType->AddField("Pivot", PT_Vector3);
    pType->AddField("Position", PT_Vector3);
    pType->AddField("Rotation", PT_Quaternion);
    pType->AddField("Size", PT_Vector3);

    ZeroVect.x = ZeroVect.y = ZeroVect.z = 0.0f;
    ZeroQuat.x = ZeroQuat.y = ZeroQuat.z = 0.0f;
    SizeVect.x = SizeVect.y = SizeVect.z = ZeroQuat.w = 1.0f;

    White = FEColor(0xFFFFFFFF);
    pField = pType->GetFirstField();
    pField->SetDefault(&White);
    pField = pField->GetNext();
    pField->SetDefault(&ZeroVect);
    pField = pField->GetNext();
    pField->SetDefault(&ZeroVect);
    pField = pField->GetNext();
    pField->SetDefault(&ZeroQuat);
    pField = pField->GetNext();
    pField->SetDefault(&SizeVect);

    return pType;
}







FETypeNode *FETypeLib::CreateImageObjectType(const char *pName) {
    FETypeNode *pType;
    FEFieldNode *pField;
    FEVector2 ZeroVect, OneVect;

    pType = CreateBaseObjectType(pName);

    pType->AddField("Upper Left", 3);
    pType->AddField("Lower Right", 3);

    ZeroVect = FEVector2(0.0f, 0.0f);
    pField = pType->GetField("Upper Left");
    pField->SetDefault(&ZeroVect);
    OneVect = FEVector2(1.0f, 1.0f);
    pField->GetNext()->SetDefault(&OneVect);

    return pType;
}


FETypeNode *FETypeLib::CreateMultiImageObjectType(const char *pName) {
    FETypeNode *pType;
    FEFieldNode *pField;

    pType = CreateImageObjectType(pName);

    char sztemp[32];
    FEVector3 pivot_rot(0.0f, 0.0f, 0.0f);
    FEVector2 top_left(0.0f, 0.0f);
    FEVector2 bottom_right(1.0f, 1.0f);

    for (int i = 1; i <= 3; i++) {
        bSPrintf(sztemp, "Tex %d: Top Left", i);
        pType->AddField(sztemp, 3);
        pField = pType->GetField(sztemp);
        pField->SetDefault(&top_left);
    }
    for (int i = 1; i <= 3; i++) {
        bSPrintf(sztemp, "Tex %d: Bottom Right", i);
        pType->AddField(sztemp, 3);
        pField = pType->GetField(sztemp);
        pField->SetDefault(&bottom_right);
    }

    bSPrintf(sztemp, "Pivot Rot (Z)");
    pType->AddField(sztemp, 4);
    pField = pType->GetField(sztemp);
    pField->SetDefault(&pivot_rot);

    return pType;
}








bool FETypeLib::Startup() {
    FETypeNode *pType;
    FEFieldNode *pField;
    FEColor White;
    i32 DefaultFrame;

    pType = CreateImageObjectType("Image");
    pType->SetID(1);
    AddType(pType);

    pType = CreateImageObjectType("CBV Image");
    pType->AddField(FEColor1Name, 6);
    pType->AddField(FEColor2Name, 6);
    pType->AddField(FEColor3Name, 6);
    pType->AddField(FEColor4Name, 6);
    White = FEColor(0xFFFFFFFF);

    pField = pType->GetField(FEColor1Name);
    pField->SetDefault(&White);
    pField = pType->GetField(FEColor2Name);
    pField->SetDefault(&White);
    pField = pType->GetField(FEColor3Name);
    pField->SetDefault(&White);
    pField = pType->GetField(FEColor4Name);
    pField->SetDefault(&White);
    pType->SetID(9);
    AddType(pType);

    pType = CreateImageObjectType("Anim Image");
    pType->AddField(FEFrameNumName, 1);
    DefaultFrame = 0;
    pField = pType->GetField(FEFrameNumName);
    pField->SetDefault(&DefaultFrame);
    pType->SetID(10);
    AddType(pType);

    pType = CreateBaseObjectType("Simple Image");
    pType->SetID(11);
    AddType(pType);

    pType = CreateMultiImageObjectType("MultiImage");
    pType->SetID(12);
    AddType(pType);

    pType = CreateBaseObjectType("String");
    pType->SetID(2);
    AddType(pType);

    pType = CreateBaseObjectType("Model");
    pType->SetID(3);
    AddType(pType);

    pType = CreateBaseObjectType("Movie");
    pType->SetID(7);
    AddType(pType);

    pType = CreateBaseObjectType("Effect");
    pType->SetID(8);
    AddType(pType);

    pType = CreateBaseObjectType("List");
    pType->SetID(4);
    AddType(pType);

    pType = CreateBaseObjectType("Group");
    pType->SetID(5);
    AddType(pType);

    pType = CreateBaseObjectType("Code List");
    pType->SetID(6);
    AddType(pType);

    return true;
}
















FETypeNode *FETypeLib::FindType(u32 TypeID) {
    FETypeNode *pNode = GetFirstType();
    while (pNode && pNode->GetID() != TypeID) {
        pNode = pNode->GetNext();
    }
    return pNode;
}















FEObject *FETypeLib::CreateFEObject(FETypeNode *pType, bool bInitScript) {
    u32 Size;
    FEObject *pObj;
    FEScript *pScript;

    switch (pType->GetID()) {

        case FE_Image: pObj = FNEW FEImage(); break;
        case FE_String: pObj = FNEW FEString(); break;
        case FE_Model: pObj = FNEW FEModel(); break;
        case FE_List: pObj = FNEW FEListBox(); break;
        case FE_Group: pObj = FNEW FEGroup(); break;
        case FE_CodeList: pObj = FNEW FECodeListBox(); break;
        case FE_Movie: pObj = FNEW FEMovie(); break;
        case FE_ColoredImage: pObj = FNEW FEColoredImage(); break;
        case FE_AnimImage: pObj = FNEW FEAnimImage(); break;
        case FE_MultiImage: pObj = FNEW FEMultiImage(); break;
        default: pObj = FNEW FEObject(); break;
    }

    Size = pType->GetTypeSize();
    pObj->SetDataSize(Size);
    FEngMemSet(pObj->pData, 0, Size);

    if (bInitScript) {

        pScript = CreateObjectScript(pType);
        pScript->SetName("Init");

        pObj->Scripts.AddTail(pScript);
        pObj->SetCurrentScript(pScript);

        if (bAutoCreateHideScripts) {

            pScript = CreateObjectScript(pType);
            pScript->SetName("Hide");


            FEKeyTrack *pTrack = &pScript->pTracks[FETrack_Color];
            FEColor c(0);
            pTrack->SetKeyValueAt(-1, c);

            pObj->Scripts.AddTail(pScript);
        }
    }
    pObj->Type = static_cast<FEObjType>(pType->GetID());

    return pObj;
}






























FEScript *FETypeLib::CreateObjectScript(FETypeNode *pType) {
    FEScript *pScript;
    FEKeyTrack *pTrack;
    FEFieldNode *pField;
    FEKeyNode *pKey;
    i32 TrackNum;
    i32 Offset;

    pScript = new FEScript;
    pScript->SetTrackCount(pType->GetFieldCount());


    TrackNum = 0;
    Offset = 0;

    pField = pType->GetFirstField();

    while (pField) {
        pTrack = &pScript->pTracks[TrackNum];
        pTrack->ParamType = pField->GetType();
        pTrack->ParamSize = pField->GetSize();
        pTrack->InterpType = FEKeyInterpDefault[pTrack->ParamType];
        pTrack->InterpAction = pScript->Flags & SF_ActionMask;
        pTrack->LongOffset = Offset;

        pKey = pTrack->GetBaseKey();
        pKey->tTime = -1;
        pField->GetDefault(&pKey->Val);

        Offset += pField->GetSize() / 4;
        TrackNum++;
        pField = pField->GetNext();
    }

    return pScript;
}
