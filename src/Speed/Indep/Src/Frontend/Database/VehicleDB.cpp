#include "Speed/Indep/Src/Frontend/Database/VehicleDB.hpp"
#include "Speed/Indep/Src/Camera/CameraInfo.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/FEng/FEList.h"
#include "Speed/Indep/Src/Main/AttribSupport.h"
#include "Speed/Indep/Src/Misc/EasterEggs.hpp"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/fecooling.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/frontend.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/infractions.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/presetride.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/pvehicle.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/pursuitlevels.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/ecar.h"
#include "Speed/Indep/Src/Generated/AttribSys/GenericAccessor.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/camerainfo.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IPlayer.h"
#include "Speed/Indep/Src/Interfaces/Simables/IVehicle.h"
#include "Speed/Indep/Src/Ecstasy/EcstasyE.hpp"
#include "Speed/Indep/Src/Frontend/Careers/UnlockSystem.hpp"
#include "Speed/Indep/Src/World/CarInfo.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Src/Gameplay/GManager.h"

#include "types.h"

#include <string.h>

#ifndef EA_BUILD_A124 // la A124 no tiene la variable: compara con 5
int g_MaximumMaximumTimesBusted = 5;
#endif
float g_fImpoundPercentageOfOriginalCost = 0.25f;
// r75: fuera extern GetTextureInfo duplicado; Texture.hpp ya lo declara (via GRaceStatus.h).
extern bool DoesStringExist(unsigned int hash);

extern CarPartDatabase CarPartDB;
PresetCar *FindFEPresetCar(unsigned int key);
int GetNumPresetCars();
PresetCar *GetPresetCarAt(int index);
extern bool ShowAllCarsInFE;
extern bool ShowAllPresetsInFE;

POVTypes GetPOVTypeFromPlayerCamera(ePlayerSettingsCameras cam) {
    switch (cam) {
        case PSC_BUMPER:
            return POV_BUMPER;
        case PSC_HOOD:
            return POV_HOOD;
        case PSC_CLOSE:
            return POV_OUTSIDE_CLOSE;
        case PSC_FAR:
            return POV_OUTSIDE_FAR;
        case PSC_SUPER_FAR:
            return POV_SUPER_FAR;
        case PSC_DRIFT:
            return POV_DRIFT;
        case PSC_PURSUIT:
            return POV_PURSUIT;
        default:
            return POV_OUTSIDE_CLOSE;
    }
}

bool IsPlayerCameraSelectable(POVTypes pov_type) {
    Attrib::Gen::ecar model_atrs(0xeec2271a, 0, nullptr);
    static unsigned int prevModelNameHash = 0;
    static unsigned int modelNameKey = 0;

    IPlayer *player = IPlayer::First(PLAYER_LOCAL);
    if (player != nullptr) {
        ISimable *isimable = player->GetSimable();
        if (isimable == nullptr) {
            const Attrib::StringKey &modelName = isimable->GetAttributes()->MODEL(0);
            if (static_cast<unsigned int>(modelName) != prevModelNameHash) {
                prevModelNameHash = modelName;
                const char *name = modelName.GetString();
                CarTypeInfo *car_type = GetCarTypeInfoFromHash(bStringHash(name));
                modelNameKey = Attrib::StringToLowerCaseKey(car_type ? car_type->CarTypeName : "");
            }
        }
    }

    model_atrs.ChangeWithDefault(modelNameKey);

    Attrib::Gen::camerainfo camera_atrs(0xeec2271a, 0, nullptr);

    switch (pov_type) {
        case POV_BUMPER:
            camera_atrs.Change(model_atrs.CameraInfo_Bumper());
            break;
        case POV_HOOD:
            camera_atrs.Change(model_atrs.CameraInfo_Hood());
            break;
        case POV_OUTSIDE_CLOSE:
            camera_atrs.Change(model_atrs.CameraInfo_Close());
            break;
        case POV_OUTSIDE_FAR:
            camera_atrs.Change(model_atrs.CameraInfo_Far());
            break;
        case POV_SUPER_FAR:
            camera_atrs.Change(model_atrs.CameraInfo_SuperFar());
            break;
        case POV_DRIFT:
            camera_atrs.Change(model_atrs.CameraInfo_Drift());
            break;
        case POV_PURSUIT:
            camera_atrs.Change(model_atrs.CameraInfo_Pursuit());
            break;
        default:
            camera_atrs.Change(0xeec2271a);
            break;
    }

    return camera_atrs.SELECTABLE(eGetCurrentViewMode() == EVIEWMODE_TWOH);
}

ePlayerSettingsCameras GetPlayerCameraFromPOVType(POVTypes type) {
    switch (type) {
        case 0:
            return static_cast<ePlayerSettingsCameras>(0);
        case 1:
            return static_cast<ePlayerSettingsCameras>(1);
        case 2:
            return static_cast<ePlayerSettingsCameras>(2);
        case 3:
            return static_cast<ePlayerSettingsCameras>(3);
        case 4:
            return static_cast<ePlayerSettingsCameras>(4);
        case 5:
            return static_cast<ePlayerSettingsCameras>(5);
        case 6:
            return static_cast<ePlayerSettingsCameras>(6);
        default:
            return static_cast<ePlayerSettingsCameras>(2);
    }
}

void AdjustStableHeat_EvadePursuit(int playerNum) {
    FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(playerNum);
    for (int i = 0; i < MAX_CARS_IN_STABLE; i++) {
        FECarRecord *fe_car = stable->GetCarByIndex(i);
        FECareerRecord *fe_career = stable->GetCareerRecordByHandle(fe_car->CareerHandle);
        if (fe_career) {
            fe_career->AdjustHeatOnEvadePursuit();
        }
    }
}

void AdjustStableHeat_EventWin(int playerNum) {
    FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(playerNum);
    for (int i = 0; i < MAX_CARS_IN_STABLE; i++) {
        FECarRecord *fe_car = stable->GetCarByIndex(i);
        FECareerRecord *fe_career = stable->GetCareerRecordByHandle(fe_car->CareerHandle);
        if (fe_career) {
            fe_career->AdjustHeatOnEventWin();
        }
    }
}

void AdjustStableHeat_DebugIncrease(int playerNum, float heat) {
    FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(playerNum);
    for (int i = 0; i < MAX_CARS_IN_STABLE; i++) {
        FECarRecord *fe_car = stable->GetCarByIndex(i);
        FECareerRecord *fe_career = stable->GetCareerRecordByHandle(fe_car->CareerHandle);
        if (fe_career) {
            fe_career->VehicleHeat = heat;
        }
    }
}

void AdjustStableImpound_EventWin(int playerNum) {
    FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(playerNum);
    for (int i = 0; i < MAX_CARS_IN_STABLE; i++) {
        FECarRecord *fe_car = stable->GetCarByIndex(i);
        FECareerRecord *fe_career = stable->GetCareerRecordByHandle(fe_car->CareerHandle);
        if (fe_career) {
            if (fe_career->TheImpoundData.NotifyWin()) {
                GManager::Get().AddSMS(0x78);
            }
        }
    }
}

void AdjustStableImpound_EvadePursuit(int playerNum) {
    FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(playerNum);
    for (int i = 0; i < MAX_CARS_IN_STABLE; i++) {
        FECarRecord *fe_car = stable->GetCarByIndex(i);
        FECareerRecord *fe_career = stable->GetCareerRecordByHandle(fe_car->CareerHandle);
        if (fe_career) {
            fe_career->TheImpoundData.NotifyEvade();
        }
    }
}

FEPlayerCarDB::FEPlayerCarDB() {}

FEPlayerCarDB::~FEPlayerCarDB() {}

FECarRecord *FEPlayerCarDB::GetCarRecordByHandle(uint32 handle) {
    for (int i = 0; i < MAX_CARS_IN_STABLE; i++) {
        if (CarTable[i].Handle == handle) {
            return &CarTable[i];
        }
    }
    return nullptr;
}

FECarRecord *FEPlayerCarDB::GetCarByIndex(int index) {
    if (index < MAX_CARS_IN_STABLE) {
        return &CarTable[index];
    }
    return nullptr;
}

FECarRecord *FEPlayerCarDB::CreateNewCarRecord() {
    for (int i = 0; i < MAX_CARS_IN_STABLE; i++) {
        if (CarTable[i].Handle == INVALID_CAR_HANDLE) {
            FECarRecord *record = &CarTable[i];

            record->Default();
            record->Handle = i;
            return record;
        }
    }
    return nullptr;
}

bool FEPlayerCarDB::CanCreateNewCarRecord() {
    for (int i = 0; i < MAX_CARS_IN_STABLE; i++) {
        if (this->CarTable[i].Handle == INVALID_CAR_HANDLE) {
            return true;
        }
    }
    return false;
}

bool FEPlayerCarDB::CanCreateNewCustomizationRecord() {
    for (int i = 0; i < MAX_CUSTOMIZE_BLOCKS; i++) {
        if (Customizations[i].Handle == 0xFF) {
            return true;
        }
    }
    return false;
}

FECustomizationRecord *FEPlayerCarDB::CreateNewCustomizationRecord() {
    for (int i = 0; i < MAX_CUSTOMIZE_BLOCKS; i++) {
        if (Customizations[i].Handle == 0xFF) {
            FECustomizationRecord *record = &Customizations[i];

            record->Default();
            Customizations[i].Handle = i;
            return record;
        }
    }
    return nullptr;
}

FECareerRecord *FEPlayerCarDB::CreateNewCareerRecord() {
    for (int i = 0; i < MAX_CAREER_TOTAL_CARS; i++) {
        if (CareerRecords[i].Handle == 0xFF) {
            CareerRecords[i].Default();
            CareerRecords[i].Handle = i;
            return &CareerRecords[i];
        }
    }
    return nullptr;
}


uint16 FEPlayerCarDB::GetNumInfraction(GInfractionManager::InfractionType type, bool get_unserved) {
    class NumInfraction : public FEPlayerCarDB::MyCallback {
      public:
        NumInfraction(GInfractionManager::InfractionType t, bool unserved) : type(t), get_unserved(unserved) {}
        uint32 Callback(const FECareerRecord &record) const override {
            return record.GetNumInfraction(type, get_unserved);
        }

        GInfractionManager::InfractionType type;
        bool get_unserved;
    };

    uint16 history = static_cast<uint16>(get_unserved ? SoldHistoryUnservedInfractions.GetValue(type) : SoldHistoryServedInfractions.GetValue(type));
    return static_cast<uint16>(history + ForAllCareerRecordsSum(NumInfraction(type, get_unserved)));
}

uint32 FEPlayerCarDB::GetTotalNumInfractions(bool get_unserved) {
    class TotalNumInfractions : public FEPlayerCarDB::MyCallback {
      public:
        TotalNumInfractions(bool unserved) : get_unserved(unserved) {}
        uint32 Callback(const FECareerRecord &record) const override {
            return record.GetInfractions(get_unserved).NumInfractions();
        }

        bool get_unserved;
    };

    uint16 history = get_unserved ? SoldHistoryUnservedInfractions.NumInfractions() : SoldHistoryServedInfractions.NumInfractions();
    return ForAllCareerRecordsSum(TotalNumInfractions(get_unserved)) + history;
}

uint16 FEPlayerCarDB::GetNumInfractionsOnCar(uint32 car_handle, bool get_unserved) {
    FECarRecord *fe_car = GetCarRecordByHandle(car_handle);
    FECareerRecord *record = GetCareerRecordByHandle(fe_car->CareerHandle);
    if (record != nullptr) {
        return static_cast<uint16>(record->GetInfractions(get_unserved).NumInfractions());
    }

    return 0;
}

uint32 FEPlayerCarDB::GetTotalBounty() {
    class Bounty : public FEPlayerCarDB::MyCallback {
        uint32 Callback(const FECareerRecord &record) const override {
            return record.GetBounty();
        }
    };

    return ForAllCareerRecordsSum(Bounty()) + SoldHistoryBounty;
}

uint32 FEPlayerCarDB::GetTotalEvadedPursuits() {
    class EvadedPursuits : public FEPlayerCarDB::MyCallback {
        uint32 Callback(const FECareerRecord &record) const override {
            return record.GetNumEvadedPursuits();
        }
    };

    return ForAllCareerRecordsSum(EvadedPursuits()) + SoldHistoryNumEvadedPursuits;
}

uint32 FEPlayerCarDB::GetTotalBustedPursuits() {
    class BustedPursuits : public FEPlayerCarDB::MyCallback {
      public:
        uint32 Callback(const FECareerRecord &record) const override {
            return record.GetNumBustedPursuits();
        }
    };

    return ForAllCareerRecordsSum(BustedPursuits()) + SoldHistoryNumBustedPursuits;
}

uint32 FEPlayerCarDB::GetNumImpoundedCars() {
    class IsImpounded : public FEPlayerCarDB::MyCallback {
      public:
        uint32 Callback(const FECareerRecord &record) const override {
            return record.TheImpoundData.IsImpounded();
        }
    };

    return ForAllCareerRecordsSum(IsImpounded());
}

uint32 FEPlayerCarDB::GetTotalFines(bool get_unserved) {
    class Fines : public FEPlayerCarDB::MyCallback {
      public:
        Fines(bool unserved) : get_unserved(unserved) {}
        uint32 Callback(const FECareerRecord &record) const override {
            return record.GetInfractions(get_unserved).GetFineValue();
        }

        bool get_unserved;
    };

    return ForAllCareerRecordsSum(Fines(get_unserved));
}

uint32 FEPlayerCarDB::GetNumCareerCarsWithARecord() {
    class NumCars : public FEPlayerCarDB::MyCallback {
      public:
        uint32 Callback(const FECareerRecord &record) const override {
            return 1;
        }
    };

    return ForAllCareerRecordsSum(NumCars());
}

// Los ocho tipos locales anteriores tienen destructor y Callback virtuales (0x8017FAA4..0x8017FD14, 624 B). GCC
// 2.95 difiere esos overrides (su DECL_CONTEXT pasa a ser MyCallback) y solo los emite con -fkeep-inline-functions:
// las unidades del juego se compilaron asi (su DWARF trae una copia fuera de linea, estripada al enlazar, de TODAS
// sus static inline: GXEnd, OSSetGQR2...). Sin la opcion en los cflags del juego salen sin definir.
// Los temporales, Fines(bool) y el uint16 de history son los del original: el DWARF no tiene la local callback, el
// mapa de lineas pone la vtable en la linea del return y la A124 los destruye sin llamada de cola.
uint32 FEPlayerCarDB::ForAllCareerRecordsSum(const MyCallback &callback) {
    uint32 val = 0;

    for (int i = 0; i < MAX_CARS_IN_STABLE; i++) {
        FECarRecord *fe_car = GetCarByIndex(i);
        if (fe_car->IsValid() && fe_car->MatchesFilter(0xF0002)) {
            const FECareerRecord *record = GetCareerRecordByHandle(fe_car->CareerHandle);
            if (record != nullptr) {
                val += callback.Callback(*record);
            }
        }
    }

    return val;
}

void FEPlayerCarDB::BackupSoldCarHistory(uint8 sold_car) {
    FECareerRecord *careerRecord = GetCareerRecordByHandle(sold_car);
    if (careerRecord == nullptr) {
        return;
    }

    SoldHistoryBounty += careerRecord->GetBounty();
    SoldHistoryNumEvadedPursuits += careerRecord->GetNumEvadedPursuits();
    SoldHistoryNumBustedPursuits += careerRecord->GetNumBustedPursuits();
    SoldHistoryUnservedInfractions += careerRecord->GetInfractions(true);
    SoldHistoryServedInfractions += careerRecord->GetInfractions(false);
}

uint32 FEPlayerCarDB::GetPreferedCarName() {
    unsigned int max_pursuits = 0;
    unsigned int name = 0;

    for (int i = 0; i < MAX_CARS_IN_STABLE; i++) {
        FECarRecord *fe_car = GetCarByIndex(i);
        if (fe_car->IsValid() && fe_car->MatchesFilter(0xF0002)) {
            FECareerRecord *career_record = GetCareerRecordByHandle(fe_car->CareerHandle);
            if (career_record != nullptr) {
                unsigned int pursuits = career_record->GetNumBustedPursuits() + career_record->GetNumEvadedPursuits();
                if (pursuits > max_pursuits) {
                    max_pursuits = pursuits;
                    name = fe_car->GetNameHash();
                }
            }
        }
    }

    return name;
}

int FEPlayerCarDB::GetNumQuickRaceCars() {
    return GetNumCars(0xF0004);
}

int FEPlayerCarDB::GetNumCareerCars() {
    return GetNumCars(0xF0002);
}

int FEPlayerCarDB::GetNumPurchasedCars() {
    int total = 0;

    for (int i = 0; i < MAX_CARS_IN_STABLE; i++) {
        FECarRecord *record = &CarTable[i];
        if (record->IsValid() && record->MatchesFilter(0xF0002) && (record->FilterBits & 0x40) == 0) {
            total++;
        }
    }

    return total;
}

int FEPlayerCarDB::GetNumAvailableCareerCars() {
    int total = 0;

    for (int i = 0; i < MAX_CARS_IN_STABLE; i++) {
        FECarRecord *record = &CarTable[i];
        if (record->IsValid()) {
            bool has_career = record->CareerHandle != 0xFF;
            if (has_career) {
                FECareerRecord *career_record = GetCareerRecordByHandle(record->CareerHandle);
                if (!career_record->TheImpoundData.IsImpounded()) {
                    total++;
                }
            }
        }
    }

    return total;
}

int FEPlayerCarDB::GetNumCars(uint32 filter) {
    int total = 0;

    for (int i = 0; i < MAX_CARS_IN_STABLE; i++) {
        FECarRecord *car = &CarTable[i];
        if (car->IsValid() && car->MatchesFilter(filter)) {
            total++;
        }
    }

    return total;
}

FECarRecord *FEPlayerCarDB::CreateNewCustomCar(uint32 fromCar) {
    if (GetNumQuickRaceCars() >= 20) {
        return nullptr;
    }
#ifdef EA_BUILD_A124
    return CreateCar(fromCar, FE_CAR_FILTER_LIST_QUICK_RACE);
#else
    return CreateCar(fromCar, 0xF0004);
#endif
}

#ifdef EA_BUILD_A124
void FEPlayerCarDB::AwardRivalCar(uint32 preset) {
    PresetCar *presetCar = FindFEPresetCar(preset);
    FECarRecord *new_rec = CreateNewCareerCar(CreateNewPresetCar(presetCar->PresetName)->Handle);
    FECustomizationRecord *customizations = GetCustomizationRecordByHandle(new_rec->Customization);
    RideInfo ride;

    ride.FillWithPreset(FEHashUpper(presetCar->PresetName));
    customizations->WriteRideIntoRecord(&ride);

    Attrib::Gen::presetride physics_preset(Attrib::StringToLowerCaseKey(presetCar->PresetName), 0, nullptr);
    if (physics_preset.IsValid() && customizations != nullptr) {
        Attrib::Gen::pvehicle pvehicle(new_rec->VehicleKey, 0, nullptr);
        if (Physics::Upgrades::ApplyPreset(pvehicle, physics_preset)) {
            customizations->WritePhysicsIntoRecord(pvehicle);
        }
    }
}
#else
FECarRecord *FEPlayerCarDB::AwardRivalCar(uint32 preset) {
    if (preset == 0x03A94520) {
        FEDatabase->GetCareerSettings()->SetHasBeenAwardedEpicCar();
    }

    PresetCar *presetCar = FindFEPresetCar(preset);
    FECarRecord *presetRecord = GetCarRecordByHandle(preset);
    if (presetRecord == nullptr) {
        CreateNewPresetCar(presetCar->PresetName);
    }

    FECarRecord *car = CreateNewCareerCar(presetRecord->Handle);

    car->FilterBits |= 0x40;

    FECustomizationRecord *customization = GetCustomizationRecordByHandle(car->Customization);
    RideInfo ride;

    ride.FillWithPreset(FEHashUpper(presetCar->PresetName));
    customization->WriteRideIntoRecord(&ride);

    Attrib::Gen::presetride ridePreset(Attrib::StringToLowerCaseKey(presetCar->PresetName), 0, nullptr);
    if (ridePreset.IsValid() && customization != nullptr) {
        Attrib::Gen::pvehicle vehicle(car->VehicleKey, 0, nullptr);
        if (Physics::Upgrades::ApplyPreset(vehicle, ridePreset)) {
            customization->WritePhysicsIntoRecord(vehicle);
        }
    }

    return car;
}
#endif

FECarRecord *FEPlayerCarDB::CreateNewCareerCar(uint32 fromCar) {
#ifdef EA_BUILD_A124
    if (GetNumCareerCars() > 10) {
#else
    if (GetNumCareerCars() >= 0x19) {
#endif

        return nullptr;
    }

#ifdef EA_BUILD_A124
    FECarRecord *car = CreateCar(fromCar, 2);
#else
    FECarRecord *car = CreateCar(fromCar, 0xF0002);
#endif

    if (car != nullptr) {
        FECareerRecord *careerRecord = CreateNewCareerRecord();
        if (careerRecord == nullptr) {

            GetCustomizationRecordByHandle(car->Customization)->Handle = 0xFF;
            car->Handle = 0xFFFFFFFF;
            return nullptr;
        }
        car->CareerHandle = careerRecord->Handle;
    }

    return car;
}

FECarRecord *FEPlayerCarDB::CreateNewPresetCar(const char *preset_name) {
    unsigned int presetHash = FEHashUpper(preset_name);
    PresetCar *preset = FindFEPresetCar(presetHash);
    Attrib::Gen::pvehicle vehicle(static_cast<unsigned int>(preset->VehicleKey), 0, nullptr);

    if (!vehicle.IsValid()) {
        return nullptr;
    }

    FECarRecord *car = CreateNewCarRecord();
    if (car == nullptr || preset == nullptr) {
        return nullptr;
    }

    car->Handle = presetHash;
    FECustomizationRecord *customization = CreateNewCustomizationRecord();
    if (customization == nullptr) {
        car->Handle = 0xFFFFFFFF;
        return nullptr;
    }

    car->Customization = customization->Handle;
    SetCarToPreset(car->Handle, preset);

#ifndef EA_BUILD_A124
    Attrib::Gen::presetride ridePreset(Attrib::StringToLowerCaseKey(preset->PresetName), 0, nullptr);
    if (ridePreset.IsValid()) {
        Attrib::Gen::pvehicle vehicleWithPreset(vehicle);
        if (Physics::Upgrades::ApplyPreset(vehicleWithPreset, ridePreset)) {
            customization->WritePhysicsIntoRecord(vehicleWithPreset);
        }
        Physics::Upgrades::Clear(vehicleWithPreset);
    }
#endif

    car->FilterBits = 0xF0010;
    return car;
}

FECarRecord *FEPlayerCarDB::CreateCar(uint32 fromCar, int FilterBits) {
    FECarRecord *source = GetCarRecordByHandle(fromCar);
    if (source == nullptr) {
        return nullptr;
    }

    FECarRecord *car = CreateNewCarRecord();
    if (car == nullptr) {
        return nullptr;
    }

    *car = *source;
    FECustomizationRecord *customization = CreateNewCustomizationRecord();
    if (customization == nullptr) {
        car->Handle = 0xFFFFFFFF;
        return nullptr;
    }

    car->Customization = customization->Handle;
    car->FilterBits = (car->FilterBits & 0xFFFF0000) | static_cast<unsigned int>(FilterBits);

    RideInfo ride;
    ride.Init(car->GetType(), CarRenderUsage_Player, 0, 0);
    ride.SetRandomPaint();
    ride.SetStockParts();
    customization->WriteRideIntoRecord(&ride);
    return car;
}

void FEPlayerCarDB::DeleteCustomCar(unsigned int handle) {
    DeleteCar(handle, 4, false);
}

void FEPlayerCarDB::DeleteCareerCar(unsigned int handle, bool was_sold) {
    DeleteCar(handle, 2, was_sold);
}

bool FEPlayerCarDB::DeleteCar(uint32 handle, uint32 filter, bool was_sold) {
    FECarRecord *car = GetCarRecordByHandle(handle);

    if (car == nullptr || !car->IsValid() ||

        (car->FilterBits & filter) == 0) {
        return false;
    }
    if (was_sold) {
        BackupSoldCarHistory(car->CareerHandle);
    }

    car->Handle = 0xFFFFFFFF;
#ifdef EA_BUILD_A124
    if (car->IsCustomized()) {
        car->Customization = 0xFF;
    }
    if (car->IsCareer()) {
        FECareerRecord *ccar = &CareerRecords[car->CareerHandle];
        ccar->Handle = 0xFF;
    }
#else
    if (car->IsCustomized()) {
        GetCustomizationRecordByHandle(car->Customization)->Handle = 0xFF;
    }
    if (car->IsCareer()) {
        GetCareerRecordByHandle(car->CareerHandle)->Handle = 0xFF;
    }
#endif

    return true;
}

void FEPlayerCarDB::DeleteAllCars() {
    for (int i = 0; i < MAX_CARS_IN_STABLE; i++) {
#ifdef EA_BUILD_A124
        if (!DeleteCar(CarTable[i].Handle, 0xFFFFFFFF, false)) {
            return;
        }
#else
        CarTable[i].Handle = INVALID_CAR_HANDLE;
#endif
    }
}

void FEPlayerCarDB::DeleteAllCustomizations() {
    for (int i = 0; i < MAX_CUSTOMIZE_BLOCKS; i++) {
#ifdef EA_BUILD_A124
        if (Customizations[i].Handle == 0xFF) {
            return;
        }
#endif
        Customizations[i].Handle = 0xFF;
    }
}

void FEPlayerCarDB::DeleteAllCareerRecords() {
    for (int i = 0; i < MAX_CAREER_TOTAL_CARS; i++) {
#ifdef EA_BUILD_A124
        if (CareerRecords[i].Handle == 0xFF) {
            return;
        }
#endif
        CareerRecords[i].Handle = 0xFF;
    }
}

bool FEPlayerCarDB::IsBonusCar(const char *preset_name) {
    switch (FEHashUpper(preset_name)) {
        case STRINGHASH_BONUS_SL65:
        case STRINGHASH_BONUS_C6R:
        case STRINGHASH_BONUS_GT2:
        case STRINGHASH_CASTROLGT:
        case STRINGHASH_E3_DEMO_BMW:
        case STRINGHASH_BL2:
        case STRINGHASH_BL3:
        case STRINGHASH_BL4:
        case STRINGHASH_BL5:
        case STRINGHASH_BL6:
        case STRINGHASH_BL7:
        case STRINGHASH_BL8:
        case STRINGHASH_BL9:
        case STRINGHASH_BL10:
        case STRINGHASH_BL11:
        case STRINGHASH_BL12:
        case STRINGHASH_BL13:
        case STRINGHASH_BL14:
        case STRINGHASH_BL15:
            return true;
    }
    return false;
}

#ifdef EA_BUILD_A124
void FEPlayerCarDB::Default() {
    DeleteAllCars();
    DeleteAllCustomizations();
    DeleteAllCareerRecords();

    for (int i = 0; i < GetNumPresetCars(); i++) {
        PresetCar *preset = GetPresetCarAt(i);

        if (!bStrICmp(preset->PresetName, "M3GTRCAREERSTART") || ShowAllCarsInFE) {
            CreateNewPresetCar(preset->PresetName);
        } else if (GetIsCollectorsEdition() && UnlockSystem::IsBonusCarCEOnly(FEHashUpper(preset->PresetName))) {
            FECarRecord *feCar = CreateNewPresetCar(preset->PresetName);
            if (feCar) {
                feCar->FilterBits |= 0x8;
            }
        }
    }

    const Attrib::Class *carClass = Attrib::Database::Get().GetClass(Attrib::ClassName::pvehicle);
    unsigned int key = carClass->GetFirstCollection();
    while (key != 0) {
        Attrib::Gen::pvehicle pv_car(key, 0, nullptr);
        Attrib::Gen::frontend fe_car(pv_car.frontend(), 0, nullptr);
        Attrib::StringKey modelName;
        pv_car.MODEL(modelName);
        if (pv_car.PlayerUsable() || ShowAllCarsInFE) {
            if (modelName.IsNotEmpty() && !pv_car.IsDynamic()) {
                FECarRecord *newCar = CreateNewCarRecord();
                if (newCar) {
                    newCar->FEKey = fe_car.GetCollection();
                    newCar->VehicleKey = pv_car.GetCollection();
                    newCar->Default();
                }
            }
        }
        key = carClass->GetNextCollection(key);
    }

    SoldHistoryBounty = 0;
    SoldHistoryNumEvadedPursuits = 0;
    SoldHistoryNumBustedPursuits = 0;
    SoldHistoryUnservedInfractions = FEInfractionsData();
    SoldHistoryServedInfractions = FEInfractionsData();
}
#else
void FEPlayerCarDB::Default() {
    DeleteAllCars();
    DeleteAllCustomizations();
    DeleteAllCareerRecords();

    FECarRecord *careerStart = CreateNewPresetCar("M3GTRCAREERSTART");
    careerStart->Handle = 0x12345678;
    careerStart->FilterBits = 0xF0020;

    for (int i = 0; i < GetNumPresetCars(); i++) {
        PresetCar *preset = GetPresetCarAt(i);

        if (UnlockSystem::IsBonusCarCEOnly(FEHashUpper(preset->PresetName))) {
            FECarRecord *feCar = CreateNewPresetCar(preset->PresetName);
            if (feCar) {
                feCar->FilterBits = 0xF0008;
            }
        } else if (IsBonusCar(preset->PresetName)) {
            FECarRecord *feCar = CreateNewPresetCar(preset->PresetName);
            if (feCar) {
                feCar->FilterBits = 0xF0008;
            }
        } else if (!bStrICmp(preset->PresetName, "M3GTRCAREERSTART") || ShowAllPresetsInFE) {
            FECarRecord *feCar = CreateNewPresetCar(preset->PresetName);
        }
        Physics::Upgrades::Flush();
    }

    const Attrib::Class *carClass = Attrib::Database::Get().GetClass(Attrib::ClassName::pvehicle);
    unsigned int key = carClass->GetFirstCollection();
    while (key != 0) {
        Attrib::Gen::pvehicle pv_car(key, 0, nullptr);
        if (!pv_car.IsDynamic()) {
            Attrib::Gen::frontend fe_car(pv_car.frontend(), 0, nullptr);
            Attrib::StringKey modelName;
            pv_car.MODEL(modelName);
            if (pv_car.PlayerUsable() || ShowAllCarsInFE) {
                if (modelName.IsNotEmpty()) {
                    if (!pv_car.IsDynamic()) {
                        FECarRecord *car = CreateNewCarRecord();
                        if (car) {
                            car->FEKey = fe_car.GetCollection();
                            car->VehicleKey = pv_car.GetCollection();
                            car->Default();
                        }
                    }
                }
            }
            Physics::Upgrades::Flush();
        }
        key = carClass->GetNextCollection(key);
    }

    SoldHistoryBounty = 0;
    SoldHistoryNumEvadedPursuits = 0;
    SoldHistoryNumBustedPursuits = 0;
    SoldHistoryUnservedInfractions = FEInfractionsData();
    SoldHistoryServedInfractions = FEInfractionsData();
}
#endif

char *FEPlayerCarDB::SaveToBuffer(char *buffer, int bufsize) {
    bMemCpy(buffer, this, sizeof(FEPlayerCarDB));
    return buffer + sizeof(FEPlayerCarDB);
}

char *FEPlayerCarDB::LoadFromBuffer(char *buffer, int bufsize) {
    bMemCpy(this, buffer, sizeof(FEPlayerCarDB));
    return buffer + sizeof(FEPlayerCarDB);
}

int32 FEPlayerCarDB::GetSaveBufferSize() {
    return sizeof(FEPlayerCarDB);
}

void FEPlayerCarDB::AwardBonusCars() {
    if (gEasterEggs.IsEasterEggUnlocked(EASTER_EGG_CASTROL)) {
#ifdef EA_BUILD_A124
        bool found = false;

        for (int i = 0; i < MAX_CARS_IN_STABLE; i++) {
            FECarRecord *car = &CarTable[i];
            if (car->IsValid() && car->Handle == FEHashUpper("CastrolGT")) {
                found = true;
                break;
            }
        }
        if (!found) {
            FECarRecord *car = CreateNewPresetCar("CastrolGT");
            car->FilterBits = 0xF0008;
        }
#else
        if (!FEDatabase->GetCareerSettings()->HasBeenAwardedCastrolGT()) {
            FEDatabase->GetCareerSettings()->SetAwardedCastrolGT();
        }
#endif
    }
}

void FEPlayerCarDB::SetCarToPreset(uint32 car, PresetCar *preset) {
    FECarRecord *record = GetCarRecordByHandle(car);

    record->FEKey = static_cast<unsigned int>(preset->FEKey);
    record->VehicleKey = static_cast<unsigned int>(preset->VehicleKey);
    record->FilterBits = preset->FilterBits;

    FECustomizationRecord *customization = GetCustomizationRecordByHandle(record->Customization);
    if (customization != nullptr) {
        customization->BecomePreset(preset);
    }
}

void FEPlayerCarDB::BuildRideForPlayer(uint32 car, int player, RideInfo *ride) {
    FECarRecord *record = GetCarRecordByHandle(car);

    ride->Init(record->GetType(), CarRenderUsage_Player, 0, 0);
    FECustomizationRecord *customization = GetCustomizationRecordByHandle(record->Customization);
    if (customization != nullptr) {
        customization->WriteRecordIntoRide(ride);
    } else {
        ride->SetRandomPaint();
        ride->SetStockParts();
    }
}

FECustomizationRecord *FEPlayerCarDB::GetCustomizationRecordByHandle(uint8 handle) {
    if (handle > MAX_CUSTOMIZE_BLOCKS) {
        return nullptr;
    }
    return &Customizations[handle];
}

FECareerRecord *FEPlayerCarDB::GetCareerRecordByHandle(unsigned char handle) {
    if (handle > MAX_CAREER_TOTAL_CARS || this->CareerRecords[handle].Handle == 0xFF) {
        return nullptr;
    }
    return &this->CareerRecords[handle];
}

bool FEPlayerCarDB::WriteRecordIntoPhysics(uint32 car, Attrib::Gen::pvehicle &attributes) {
    FECarRecord *record = GetCarRecordByHandle(car);
    if (record == nullptr) {
        return false;
    }
    FECustomizationRecord *customization = GetCustomizationRecordByHandle(record->Customization);
    if (customization == nullptr) {
        return false;
    }
    return customization->WriteRecordIntoPhysics(attributes);
}

FECarRecord::FECarRecord() {
    FEKey = 0;
    VehicleKey = 0;
    Handle = 0xFFFFFFFF;
    Customization = 0xFF;
    CareerHandle = 0xFF;
    FilterBits = 0;
}

FECarRecord &FECarRecord::operator=(const FECarRecord &other_record) {
    FEKey = other_record.FEKey;
    VehicleKey = other_record.VehicleKey;
    FilterBits = other_record.FilterBits;
    return *this;
}

void FECarRecord::Default() {
    this->Customization = 0xFF;
    this->CareerHandle = 0xFF;
    this->FilterBits = 0;

#ifdef EA_BUILD_A124
    if (!IsValid()) {
        return;
    }
#else
    bool has_car = (this->Handle != -1);

    if (!has_car) {
        return;
    }
#endif

    Attrib::Gen::pvehicle pvehicle(this->VehicleKey, 0, nullptr);
    Attrib::Gen::frontend frontend(this->FEKey, 0, nullptr);

    switch (frontend.region()) {
    case 0:
        this->FilterBits |= 0x10000;
        break;
    case 1:
        this->FilterBits |= 0x20000;
        break;
    case 2:
        this->FilterBits |= 0x40000;
        break;
    case 3:
        this->FilterBits |= 0x80000;
        break;
    }

#ifdef EA_BUILD_A124
    if (frontend.region() == 3) {
        this->FilterBits |= 0x20;
    } else if (!frontend.IsCustomizable() && UnlockSystem::IsBonusCarAvailable(GetNameHash())) {
        this->FilterBits |= 0x8;
    } else {
        this->FilterBits |= 0x1;
    }
#else
    if (frontend.region() == 3 || !frontend.IsCustomizable()) {
        this->FilterBits |= 0x20;
    } else {
        this->FilterBits |= 0x1;
    }
#endif
}

CarType FECarRecord::GetType() {
    Attrib::Gen::pvehicle pvehicle(this->VehicleKey, 0, nullptr);

    return CarPartDB.GetCarType(bStringHash(pvehicle.MODEL().GetString()));
}

unsigned int FECarRecord::GetCost() {
    Attrib::Gen::frontend frontend(FEKey, 0, 0);

    return frontend.Cost();
}

unsigned int FECarRecord::GetReleaseFromImpoundCost() {
    float cost = GetCost();
    cost *= g_fImpoundPercentageOfOriginalCost;
    return bFloatToInt(cost);
}

unsigned int FECarRecord::GetNameHash() {
    const char *manu = GetManufacturerName();
    if (bStrCmp(manu, "")) {
        char buf[128];
        Attrib::Gen::frontend frontend(FEKey, 0, nullptr);
        FEngSNPrintf(buf, 0x80, "CARNAME_%s_%s", manu, frontend.CollectionName());
        unsigned int hash = FEHashUpper(buf);
        if (!DoesStringExist(hash)) {
            return 0x9BB9CCC3;
        }
        return hash;
    }
    return 0x9BB9CCC3;
}

unsigned int FECarRecord::GetLogoHash() {
    const char *manu = GetManufacturerName();
    if (bStrCmp(manu, "")) {
        char buf[128];
        Attrib::Gen::frontend frontend(FEKey, 0, 0);
        FEngSNPrintf(buf, 0x80, "SECONDARY_LOGO_%s", frontend.CollectionName());
        unsigned int texHash = FEHashUpper(buf);
        if (GetTextureInfo(texHash, 0, 0)) {
            return texHash;
        }
    }
    return FEHashUpper("GENERIC_LOGO_256");
}

unsigned int FECarRecord::GetManuLogoHash() {
    const char *manu = GetManufacturerName();
    if (bStrCmp(manu, "")) {
        char buf[128];
        FEngSNPrintf(buf, 0x80, "CARSELECT_MANUFACTURER_%s", manu);
        unsigned int texHash = FEHashUpper(buf);
        if (GetTextureInfo(texHash, 0, 0)) {
            return texHash;
        }
    }
    return FEHashUpper("GENERIC_LOGO_128");
}

const char *FECarRecord::GetManufacturerName() {
    Attrib::Gen::frontend fe(FEKey, 0, 0);
    unsigned char Manufacturer = static_cast<unsigned char>(fe.manufacturer());
    switch (Manufacturer) {
        case 0:
            return "";
        case 1:
            return "BMW";
        case 2:
            return "FORD";
        case 3:
            return "SUBARU";
        case 4:
            return "PORSCHE";
        case 5:
            return "AUDI";
        case 6:
            return "MAZDA";
        case 7:
            return "GENERAL_MOTORS";
        case 8:
            return "DODGE";
        case 9:
            return "TOYOTA";
        case 10:
            return "MITSUBISHI";
        case 11:
            return "MCLAREN";
        case 12:
            return "MERCEDES";
        case 13:
            return "NISSAN";
        case 14:
            return "LOTUS";
        case 15:
            return "LAMBORGHINI";
        case 16:
            return "RENAULT";
        case 17:
            return "LEXUS";
        case 18:
            return "PONTIAC";
        case 19:
            return "CHEVROLET";
        case 20:
            return "VAUXHALL";
        case 21:
            return "ASTONMARTIN";
        case 22:
            return "VOLKSWAGEN";
        case 23:
            return "FIAT";
        case 24:
            return "CADILLAC";
        case 25:
            return "CORVETTE";
        default:
            return "";
    }
}

bool FECarRecord::MatchesFilter(int theFilter) {
    int theRegion = theFilter & FE_CAR_FILTER_REGION_MASK;
    int theList = theFilter & FE_CAR_FILTER_LIST_MASK;
    int myRegion = FilterBits & FE_CAR_FILTER_REGION_MASK;
    int myList = FilterBits & FE_CAR_FILTER_LIST_MASK;
    bool regionCompare = (theRegion & myRegion) != 0;
    bool listCompare = (theList & myList) != 0;

    return regionCompare && listCompare;
}

const char *FECarRecord::GetDebugName() {
    Attrib::Gen::pvehicle vehicle(VehicleKey, 0, 0);
    const unsigned char *vehicleLayout = reinterpret_cast<const unsigned char *>(vehicle.GetLayoutPointer());

    return *reinterpret_cast<const char *const *>(vehicleLayout + 0x24);
}

uint32 GetFECarNameHashFromFEKey(Attrib::Key feKey) {
    if (!feKey) {
        return 0;
    }
    FECarRecord rec;
    rec.FEKey = feKey;
    return rec.GetNameHash();
}

void FECustomizationRecord::Default() {
    for (int i = 0; i < 139; i++) {
        InstalledPartIndices[i] = INVALID_CAR_PART_RECORD_INDEX;
    }

    InstalledPhysics.Default();

    for (int i = 0; i < NUM_CUSTOM_TUNINGS; i++) {
        Tunings[i].Default();
    }

    ActiveTuning = CTT_SETTING_1;
    Preset = 0;
}

bool FECustomizationRecord::WriteRecordIntoPhysics(Attrib::Gen::pvehicle &attributes) const {
    if (!Physics::Upgrades::SetPackage(attributes, InstalledPhysics)) {
        return false;
    }
    return true;
}

void FECustomizationRecord::WritePhysicsIntoRecord(const Attrib::Gen::pvehicle &attributes) {
    Physics::Upgrades::GetPackage(attributes, InstalledPhysics);
}

CarPart *FECustomizationRecord::GetInstalledPart(CarType cartype, int carslotid) const {
    return CarPartDB.GetCarPartByIndex(static_cast<short>(InstalledPartIndices[carslotid]));
}

void FECustomizationRecord::SetInstalledPart(int carslotid, CarPart *part) {
    if (part != nullptr) {
        CarPartDB.GetPartIndex(part);

        InstalledPartIndices[carslotid] = static_cast<short>(CarPartDB.GetPartIndex(part));
    } else {
        InstalledPartIndices[carslotid] = INVALID_CAR_PART_RECORD_INDEX;
    }
}

void FECustomizationRecord::WriteRecordIntoRide(RideInfo *ride) const {
    for (int i = 0; i < 0x8B; i++) {
        CarPart *part = GetInstalledPart(ride->Type, i);
        ride->SetPart(i, part, true);
    }
}

void FECustomizationRecord::WriteRideIntoRecord(const RideInfo *ride) {
    for (int i = 0; i < 0x8B; i++) {
        SetInstalledPart(i, ride->GetPart(i));
    }
}

FECustomizationRecord::FECustomizationRecord() : InstalledPhysics(), Tunings(), Handle(INVALID_CUSTOMIZATION_HANDLE) {
    Default();
}

void FECustomizationRecord::BecomePreset(PresetCar *preset) {
    Default();

    CarType cartype = CarPartDB.GetCarType(bStringHash(preset->CarTypeName));
    for (int i = 0; i < 0x8B; i++) {
#ifdef EA_PLATFORM_XENON
        int part_name_hash = preset->PartNameHashes[i];
        if (part_name_hash != 1 && part_name_hash != 0) {
#else
        unsigned int part_name_hash = preset->PartNameHashes[i];
        if (part_name_hash > 1) {
#endif
            struct CarPart *part = CarPartDB.NewGetCarPart(cartype, i, part_name_hash, nullptr, -1);
            InstalledPartIndices[i] = static_cast<short>(CarPartDB.GetPartIndex(part));
        }
    }

    Preset = bStringHashUpper(preset->PresetName);
}

FEInfractionsData::FEInfractionsData(unsigned int infractions) {
    *this = FEInfractionsData();

    if ((infractions & 8) != 0) {
        Assault++;
    }
    if ((infractions & 0x20) != 0) {
        Damage++;
    }
    if ((infractions & 0x10) != 0) {
        HitAndRun++;
    }
    if ((infractions & 0x80) != 0) {
        OffRoad++;
    }
    if ((infractions & 2) != 0) {
        Racing++;
    }
    if ((infractions & 4) != 0) {
        Reckless++;
    }
    if ((infractions & 0x40) != 0) {
        Resist++;
    }
    if ((infractions & 1) != 0) {
        Speeding++;
    }
}

unsigned short FEInfractionsData::NumInfractions() const {
    return Speeding + Racing + Reckless + Assault + HitAndRun + Damage + Resist + OffRoad;
}

void FEInfractionsData::operator+=(const FEInfractionsData &rhs) {
    Speeding += rhs.Speeding;
    Racing += rhs.Racing;
#ifndef EA_BUILD_A124
    Reckless += rhs.Reckless;
#endif
    Assault += rhs.Assault;
    HitAndRun += rhs.HitAndRun;
    Damage += rhs.Damage;
    Resist += rhs.Resist;
    OffRoad += rhs.OffRoad;
}

unsigned short FEInfractionsData::GetValue(GInfractionManager::InfractionType type) const {
    switch (type) {
    case GInfractionManager::kInfraction_Assault:
        return Assault;
    case GInfractionManager::kInfraction_Damage:
        return Damage;
    case GInfractionManager::kInfraction_HitAndRun:
        return HitAndRun;
    case GInfractionManager::kInfraction_OffRoad:
        return OffRoad;
    case GInfractionManager::kInfraction_Racing:
        return Racing;
    case GInfractionManager::kInfraction_Reckless:
        return Reckless;
    case GInfractionManager::kInfraction_Resist:
        return Resist;
    case GInfractionManager::kInfraction_Speeding:
        return Speeding;
    default:
        return 0;
    }
}

unsigned int FEInfractionsData::GetFineValue() const {
    if (NumInfractions() == 0) {
        return 0;
    }
    unsigned int fines = 0;
    Attrib::Gen::infractions AssaultFine(Attrib::StringToKey("assault"), 0, nullptr);
    if (AssaultFine.IsValid()) {
        fines = static_cast<unsigned int>(Assault) * AssaultFine.amount();
    }
    Attrib::Gen::infractions DamageFine(Attrib::StringToKey("damage"), 0, nullptr);
    if (DamageFine.IsValid()) {
        fines += static_cast<unsigned int>(Damage) * DamageFine.amount();
    }
    Attrib::Gen::infractions HitAndRunFine(Attrib::StringToKey("hit_and_run"), 0, nullptr);
    if (HitAndRunFine.IsValid()) {
        fines += static_cast<unsigned int>(HitAndRun) * HitAndRunFine.amount();
    }
    Attrib::Gen::infractions OffRoadFine(Attrib::StringToKey("off_road"), 0, nullptr);
    if (OffRoadFine.IsValid()) {
        fines += static_cast<unsigned int>(OffRoad) * OffRoadFine.amount();
    }
    Attrib::Gen::infractions RacingFine(Attrib::StringToKey("racing"), 0, nullptr);
    if (RacingFine.IsValid()) {
        fines += static_cast<unsigned int>(Racing) * RacingFine.amount();
    }
    Attrib::Gen::infractions RecklessFine(Attrib::StringToKey("reckless_driving"), 0, nullptr);
    if (RecklessFine.IsValid()) {
        fines += static_cast<unsigned int>(Reckless) * RecklessFine.amount();
    }
    Attrib::Gen::infractions ResistFine(Attrib::StringToKey("resisting_arrest"), 0, nullptr);
    if (ResistFine.IsValid()) {
        fines += static_cast<unsigned int>(Resist) * ResistFine.amount();
    }
    Attrib::Gen::infractions SpeedingFine(Attrib::StringToKey("speeding"), 0, nullptr);
    if (SpeedingFine.IsValid()) {
        fines += static_cast<unsigned int>(Speeding) * SpeedingFine.amount();
    }
    return fines;
}

void FEImpoundData::Default() {
    MaxBusted = 3;
    TimesBusted = 0;
    ImpoundedState = 0;
    DaysBeforeRelease = 0;
    EvadeCount = 0;
    Pad1 = Pad2 = 0;
}

void FEImpoundData::BecomeImpounded(eImpoundReasons reason) {
    ImpoundedState = reason;
    TimesBusted = MaxBusted;
    DaysBeforeRelease = 5;
}

void FEImpoundData::NotifyPlayerPaidToRelease() {
    TimesBusted = 0;
    ImpoundedState = 0;
    DaysBeforeRelease = 0;
}

void FEImpoundData::NotifyPlayerUsedMarkerToRelease() {
    NotifyPlayerPaidToRelease();
}

bool FEImpoundData::NotifyWin() {
    if (IsImpounded()) {
        if (DaysBeforeRelease > 0) {
            DaysBeforeRelease--;
        }
        if (DaysBeforeRelease == 0) {
            if (ImpoundedState != IMPOUND_RELEASED) {
                ImpoundedState = IMPOUND_RELEASED;
                return true;
            }
        }
    }
    return false;
}

bool FEImpoundData::NotifyBusted() {
    EvadeCount = 0;
    TimesBusted = TimesBusted + 1;
    return TimesBusted >= MaxBusted ? true : false;
}

bool FEImpoundData::NotifyEvade() {
    if (!IsImpounded()) {
        EvadeCount = EvadeCount + 1;
        if (EvadeCount > 2) {
            TimesBusted--;
            EvadeCount = 0;
        }
        if (TimesBusted < 0) {
            TimesBusted = 0;
        }
    }

    return NotifyWin();
}

bool FEImpoundData::CanAddMaxBusted() {
#ifdef EA_BUILD_A124
    if (MaxBusted < 5 && !IsImpounded()) {
#else
    if (static_cast<int>(MaxBusted) < g_MaximumMaximumTimesBusted && !IsImpounded()) {
#endif
        return true;
    }
    return false;
}

void FEImpoundData::AddMaxBusted() {
    MaxBusted++;
#ifdef EA_BUILD_A124
    if (MaxBusted > 5) {
        MaxBusted = 5;
    }
#else
    if (static_cast<int>(MaxBusted) > g_MaximumMaximumTimesBusted) {
        MaxBusted = static_cast<unsigned char>(g_MaximumMaximumTimesBusted);
    }
#endif
}

void FECareerRecord::Default() {
    Handle = 0xFF;
    TheImpoundData.Default();
    VehicleHeat = 0.0f;
    Bounty = 0;
    NumEvadedPursuits = 0;
    NumBustedPursuits = 0;
    FEInfractionsData *pUnserved = &UnservedInfractions;
    FEInfractionsData *pServed = &ServedInfractions;
    {
        FEInfractionsData unserved;
        *pUnserved = unserved;
    }
    {
        FEInfractionsData served;
        *pServed = served;
    }
}

void FECareerRecord::SetVehicleHeat(float h) {
    VehicleHeat = h;
}

float FECareerRecord::GetVehicleHeat() {
    return VehicleHeat;
}

void FECareerRecord::AdjustHeatOnEventWin() {
    Attrib::Gen::pursuitlevels pursuitLevels(0xEEC2271A, 0, 0);

    VehicleHeat = VehicleHeat * pursuitLevels.EventWinHeatAdjust();
}

void FECareerRecord::AdjustHeatOnEvadePursuit() {
    Attrib::Gen::pursuitlevels pursuitLevels(0xEEC2271A, 0, 0);

    VehicleHeat = VehicleHeat * pursuitLevels.EvadeSuccessHeatAdjust();
}

void FECareerRecord::AdjustHeatOnDecalApplied(float extraAdjust) {
    Attrib::Gen::fecooling cooling(0xEEC2271A, 0, 0);

    VehicleHeat *= cooling.NewDecal() * extraAdjust;
}

void FECareerRecord::AdjustHeatOnPaintApplied(float extraAdjust) {
    Attrib::Gen::fecooling cooling(0xEEC2271A, 0, 0);

    VehicleHeat *= cooling.NewPaint() * extraAdjust;
}

void FECareerRecord::AdjustHeatOnVinylApplied(float extraAdjust) {
    Attrib::Gen::fecooling cooling(0xEEC2271A, 0, 0);

    VehicleHeat *= cooling.NewVinyl() * extraAdjust;
}

void FECareerRecord::AdjustHeatOnBodyKitApplied(float extraAdjust) {
    Attrib::Gen::fecooling cooling(0xEEC2271A, 0, 0);

    VehicleHeat *= cooling.NewBodyKit() * extraAdjust;
}

void FECareerRecord::AdjustHeatOnHoodApplied(float extraAdjust) {
    Attrib::Gen::fecooling cooling(0xEEC2271A, 0, 0);

    VehicleHeat *= cooling.NewHood() * extraAdjust;
}

void FECareerRecord::AdjustHeatOnRimApplied(float extraAdjust) {
    Attrib::Gen::fecooling cooling(0xEEC2271A, 0, 0);

    VehicleHeat *= cooling.NewRim() * extraAdjust;
}

void FECareerRecord::AdjustHeatOnRimPaintApplied(float extraAdjust) {
    Attrib::Gen::fecooling cooling(0xEEC2271A, 0, 0);

    VehicleHeat *= cooling.NewRimPaint() * extraAdjust;
}

void FECareerRecord::AdjustHeatOnRoofScoopApplied(float extraAdjust) {
    Attrib::Gen::fecooling cooling(0xEEC2271A, 0, 0);

    VehicleHeat *= cooling.NewRoofScoop() * extraAdjust;
}

void FECareerRecord::AdjustHeatOnSpoilerApplied(float extraAdjust) {
    Attrib::Gen::fecooling cooling(0xEEC2271A, 0, 0);

    VehicleHeat *= cooling.NewSpoiler() * extraAdjust;
}

void FECareerRecord::AdjustHeatOnWindowTintApplied(float extraAdjust) {
    Attrib::Gen::fecooling cooling(0xEEC2271A, 0, 0);

    VehicleHeat *= cooling.NewWindowTint() * extraAdjust;
}

void FECareerRecord::CommitPursuitCarData(unsigned int infractions, uint32 accumulated_bounty, bool pursuit_evaded) {
    UnservedInfractions += FEInfractionsData(infractions);

    if (pursuit_evaded) {
        Bounty += accumulated_bounty;
        NumEvadedPursuits++;
    } else {
        NumBustedPursuits++;
    }
}

void FECareerRecord::WaiveIncractions(unsigned int infractions) {
    if ((infractions & GInfractionManager::kInfraction_Assault) != 0) {
        UnservedInfractions.Assault--;
    }
    if ((infractions & GInfractionManager::kInfraction_Damage) != 0) {
        UnservedInfractions.Damage--;
    }
    if ((infractions & GInfractionManager::kInfraction_HitAndRun) != 0) {
        UnservedInfractions.HitAndRun--;
    }
    if ((infractions & GInfractionManager::kInfraction_OffRoad) != 0) {
        UnservedInfractions.OffRoad--;
    }
    if ((infractions & GInfractionManager::kInfraction_Racing) != 0) {
        UnservedInfractions.Racing--;
    }
    if ((infractions & GInfractionManager::kInfraction_Reckless) != 0) {
        UnservedInfractions.Reckless--;
    }
    if ((infractions & GInfractionManager::kInfraction_Resist) != 0) {
        UnservedInfractions.Resist--;
    }
    if ((infractions & GInfractionManager::kInfraction_Speeding) != 0) {
        UnservedInfractions.Speeding--;
    }
    NumBustedPursuits--;
}

void FECareerRecord::ServeAllIncractions() {
    FEInfractionsData *pUnserved = &UnservedInfractions;
    ServedInfractions += UnservedInfractions;
    {
        FEInfractionsData cleared;
        *pUnserved = cleared;
    }
}

uint32 FECareerRecord::GetNumInfraction(GInfractionManager::InfractionType type, bool get_unserved) const {
    return GetInfractions(get_unserved).GetValue(type);
}
