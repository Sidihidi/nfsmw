#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/customize/CustomizeManager.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/customize/CarCustomize.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/quickrace/uiShowcase.hpp"
#include "Speed/Indep/Src/Misc/EasterEggs.hpp"
#include "Speed/Indep/Src/Physics/PhysicsUpgrades.hpp"
#include "Speed/Indep/Src/World/CarInfo.hpp"
#include "Speed/Indep/Src/Frontend/Careers/UnlockSystem.hpp"
#include "Speed/Indep/Src/Physics/PhysicsInfo.hpp"
#include "Speed/Indep/Src/Ecstasy/eStreamingPack.hpp"
#include "Speed/Indep/Src/Frontend/FECarViewer.hpp"
#include "Speed/Indep/Src/World/CarSkin.hpp"

extern const float gTradeInFactor;

CarCustomizeManager gCarCustomizeManager;

void CarCustomizeManager::TakeControl(eCustomizeEntryPoint entry_point, FECarRecord *tuning_car) {
    if (!g_bCustomizeManagerHasControl) {
#ifndef EA_BUILD_A124
        FEDatabase->SetGameMode(eFE_GAME_MODE_CUSTOMIZE);
#endif
        g_bCustomizeManagerHasControl = 1;
#ifndef EA_BUILD_A124
        for (int i = 0; i < 3; i++) {
            Showcase::FromColor[i] = nullptr;
        }
        TheTempColoredPart = nullptr;
#endif
        EntryPoint = entry_point;
        TuningCar = tuning_car;
        FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(0);
        PreviewRecord = *stable->GetCustomizationRecordByHandle(TuningCar->Customization);
        Attrib::Gen::pvehicle vehicle(TuningCar->VehicleKey, 0, nullptr);
        stable->WriteRecordIntoPhysics(TuningCar->Handle, vehicle);
        ThePVehicle = vehicle;
        RideInfo info;
        stable->BuildRideForPlayer(TuningCar->Handle, 0, &info);
        CarViewer::SetRideInfo(&info, SET_RIDE_INFO_REASON_LOAD_CAR, eCARVIEWER_PLAYER1_CAR);
        NumPartsInCart = 0;
    }
}

void CarCustomizeManager::RelinquishControl() {
#ifndef EA_BUILD_A124
    FEDatabase->ClearGameMode(eFE_GAME_MODE_CUSTOMIZE);
    for (int i = 0; i <= 2; i++) {
        delete Showcase::FromColor[i];
        Showcase::FromColor[i] = nullptr;
    }
#endif
    ClearTempColoredPart();
    g_bCustomizeManagerHasControl = 0;
}

bool CarCustomizeManager::CanTradeIn(SelectablePart *part) {
    if (part->IsPerformancePkg()) {
        return false;
    }
    switch (part->GetSlotID()) {
        case 0x4c: case 0x4d: case 0x4e: case 0x4f: case 0x50: case 0x51: case 0x52: case 0x53:
        case 0x5b:
        case 0x63: case 0x64: case 0x65: case 0x66: case 0x67: case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6e: case 0x6f: case 0x70: case 0x71: case 0x72: case 0x73:
        case 0x7b:
        case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
            return false;
    }
    return true;
}

void CarCustomizeManager::AddToCart(SelectablePart *part) {
    ShoppingCartItem *existing = IsPartTypeInCart(part);
    SelectablePart *trade_in = nullptr;
    if (existing) {
        if (
#ifndef EA_BUILD_A124
            CanTradeIn(part) &&
#endif
            existing->TradeIn) {
            trade_in = new ("SelectablePart 9", 0) SelectablePart(existing->TradeIn);
        }
        RemoveFromCart(existing);
    } else {
        if (!part->IsPerformancePkg()
#ifndef EA_BUILD_A124
            && CanTradeIn(part)
#endif
        ) {
            CarPart *installed = GetInstalledCarPart(part->GetSlotID());
            if (installed) {
                trade_in = new ("SelectablePart 10", 0) SelectablePart(installed, part->GetSlotID(), installed->GetUpgradeLevel(), Physics::Upgrades::PUT_MAX, false,
                                              CPS_INSTALLED, 0, false);
                trade_in->SetPrice(GetPartPrice(trade_in));
            }
        }
    }
    SelectablePart *to_buy = new ("SelectablePart 11", 0) SelectablePart(part);
    ShoppingCartItem *item = new ("ShoppingCartItem", 0) ShoppingCartItem(to_buy, trade_in);
    ShoppingCart.AddTail(item);
    NumPartsInCart++;
}

bool CarCustomizeManager::RemoveFromCart(ShoppingCartItem *item) {
    if (item) {
        item->Remove();
        delete item;
        NumPartsInCart--;
        return true;
    }
    return false;
}

void CarCustomizeManager::AddRemovalCarPart(uint32 slot_id) {
    SelectablePart remover(nullptr, slot_id, 0, Physics::Upgrades::PUT_MAX, false, CPS_AVAILABLE, 0, false);
    gCarCustomizeManager.AddToCart(&remover);
}

ShoppingCartItem *CarCustomizeManager::IsPartTypeInCart(SelectablePart *to_find) {
    if (!to_find)
        return nullptr;
    ShoppingCartItem *end = reinterpret_cast<ShoppingCartItem *>(&ShoppingCart);
    ShoppingCartItem *item = GetFirstCartItem();
    while (true) {
        if (item == end) {
            break;
        }
        SelectablePart *buying = item->ToBuy;
        if (to_find->IsPerformancePkg()) {
            if (buying->GetPhysicsType() == to_find->GetPhysicsType()) {
                return item;
            }
        } else {
            if (buying->GetSlotID() == to_find->GetSlotID()) {
                return item;
            }
        }
        item = static_cast<ShoppingCartItem *>(item->Next);
    }
    return nullptr;
}

ShoppingCartItem *CarCustomizeManager::IsPartTypeInCart(uint32 slot_id) {
    SelectablePart temp(nullptr, slot_id, 0, Physics::Upgrades::PUT_MAX, false, CPS_AVAILABLE, 0, false);
    return IsPartTypeInCart(&temp);
}

ShoppingCartItem *CarCustomizeManager::IsPartTypeInCart(Physics::Upgrades::Type type) {
    SelectablePart test_part(nullptr, 0, 0, type, true, CPS_AVAILABLE, 0, false);
    return IsPartTypeInCart(&test_part);
}

ShoppingCartItem *CarCustomizeManager::IsPartInCart(SelectablePart *to_find) {
    ShoppingCartItem *end = reinterpret_cast<ShoppingCartItem *>(&ShoppingCart);
    ShoppingCartItem *item = GetFirstCartItem();
    while (true) {
        if (item == end) {
            break;
        }
        SelectablePart *buyPart = item->ToBuy;
        if (to_find->IsPerformancePkg()) {
            if (buyPart->GetPhysicsType() == to_find->GetPhysicsType() && buyPart->GetUpgradeLevel() == to_find->GetUpgradeLevel()) {
                return item;
            }
        } else {
            if (buyPart->GetPart() == to_find->GetPart() && buyPart->GetSlotID() == to_find->GetSlotID()) {
                return item;
            }
        }
        item = static_cast<ShoppingCartItem *>(item->Next);
    }
    return nullptr;
}

CarPart *CarCustomizeManager::GetActivePartFromSlot(uint32 slot_id) {
    ShoppingCartItem *item = gCarCustomizeManager.IsPartTypeInCart(slot_id);
    if (item) {
        return item->ToBuy->GetPart();
    }
    return gCarCustomizeManager.GetInstalledCarPart(slot_id);
}

int CarCustomizeManager::GetCartTotal(eCustomizeCartTotals type) {
    int total = 0;
    for (ShoppingCartItem *item = ShoppingCart.GetHead(); item != ShoppingCart.EndOfList(); item = item->GetNext()) {
        if (!item->IsActive())
            continue;
#ifdef EA_BUILD_A124
        // La A124 es la forma simple: sin paquetes de rendimiento ni ranuras excluidas (oraculo A124 0x5c0d18,
        // 252 B: type==0||type==2 -> backroom ? 1 : precio; trade-in con /4 de GetTradeInPrice).
        if (type == 0 || type == 2) {
            if (IsInBackRoom()) {
                total += 1;
            } else {
                total += item->GetBuyingPart()->GetPrice();
            }
        }
#else
        if (type == 0 || type == 2) {
            SelectablePart *buy = item->GetBuyingPart();
            if (buy->IsPerformancePkg()) {
                if (IsInBackRoom()) {
                    total += 1;
                } else {
                    total += item->GetBuyingPart()->GetPrice();
                }
            } else {
                switch (buy->GetSlotID()) {
                    case 0x4f: case 0x50: case 0x51: case 0x52:
                    case 0x72:
                    case 0x85: case 0x86: case 0x87:
                        break;
                    default:
                        if (IsInBackRoom()) {
                            total += 1;
                        } else {
                            total += item->GetBuyingPart()->GetPrice();
                        }
                        break;
                }
            }
        }
#endif
        if (type == 1 || (type == 2 && !IsInBackRoom())) {
            total -= item->GetTradeInPrice();
        }
    }
    return total;
}

void CarCustomizeManager::Checkout() {
    FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(0);
    FECareerRecord *career_record = stable->GetCareerRecordByHandle(TuningCar->CareerHandle);
    for (ShoppingCartItem *item = ShoppingCart.GetHead(); item != ShoppingCart.EndOfList(); item = item->GetNext()) {
        if (!item->IsActive())
            continue;
        SelectablePart *part = item->GetBuyingPart();
        if (IsCareerMode()
#ifndef EA_BUILD_A124
            && item->GetBuyingPart()->GetSlotID() != 0x72
#endif
        ) {
            if (!IsInBackRoom()) {
#ifdef EA_BUILD_A124
                FEDatabase->GetCareerSettings()->SpendCash(item->GetPartPrice() - item->GetTradeInPrice());
#else
                int cost = item->GetPartPrice() - item->GetTradeInPrice();
                if (cost >= 0) {
                    FEDatabase->GetCareerSettings()->SpendCash(cost);
                } else {
                    FEDatabase->GetCareerSettings()->AwardCash(-cost);
                }
#endif
            } else {
                if (item->GetBuyingPart()->IsPerformancePkg()) {
                    TheFEMarkerManager.UtilizeMarker(static_cast<Physics::Upgrades::Type>(static_cast<int>(item->GetBuyingPart()->GetPhysicsType())));
                } else {
                    TheFEMarkerManager.UtilizeMarker(item->GetBuyingPart()->GetSlotID());
                }
            }
        }
        if (part->IsPerformancePkg()) {
            InstallPerfPkg(static_cast<Physics::Upgrades::Type>(static_cast<int>(part->GetPhysicsType())), part->GetUpgradeLevel());
        } else {
            InstallPart(part->GetSlotID(), part->GetPart());
            UpdateHeatOnVehicle(part, career_record);
        }
    }
    ShoppingCart.DeleteAllElements();
    ResetPreview();
}

bool CarCustomizeManager::DoesCartHaveActiveParts() {
    for (ShoppingCartItem *item = GetFirstCartItem(); item != ShoppingCart.EndOfList(); item = item->GetNext()) {
        SelectablePart *buy = item->GetBuyingPart();
        if (buy && !buy->IsPerformancePkg()) {
            switch (buy->GetSlotID()) {
                case 0x4f:
                case 0x50:
                case 0x51:
                case 0x52:
                case 0x85:
                case 0x86:
                case 0x87:
                    continue;
                default:
                    break;
            }
        }
        if (item->IsActive())
            return true;
    }
    return false;
}

int CarCustomizeManager::GetPartPrice(SelectablePart *part) {
    int price = 0;
    if ((part != nullptr) && !IsInBackRoom()) {
        if (part->IsPerformancePkg()) {
#ifdef EA_BUILD_A124
            price = UnlockSystem::GetPerfPackageCost(GetUnlockFilter(), part->GetPhysicsType(), part->GetUpgradeLevel(), 0);
#else
            int level = GetMaxPackages(part->GetPhysicsType());
            level -= GetNumPackages(part->GetPhysicsType()) - part->GetUpgradeLevel();
            price = UnlockSystem::GetPerfPackageCost(GetUnlockFilter(), part->GetPhysicsType(), level, 0);
#endif
        } else {
            switch (part->GetSlotID()) {
#ifndef EA_BUILD_A124
                case CARSLOTID_VINYL_COLOUR0_0:
                case CARSLOTID_VINYL_COLOUR0_1:
                case CARSLOTID_VINYL_COLOUR0_2:
                case CARSLOTID_VINYL_COLOUR0_3:
                    break;
                case CARSLOTID_HUD_BACKING_COLOUR:
                case CARSLOTID_HUD_NEEDLE_COLOUR:
                case CARSLOTID_HUD_CHARACTER_COLOUR:
                    break;
#endif
                default:
                    price = UnlockSystem::GetCarPartCost(GetUnlockFilter(), part->GetSlotID(), part->GetPart(), 0);
            }
        }
    }
    return price;
}

#ifndef EA_BUILD_A124
void CarCustomizeManager::SetTempColoredPart(SelectablePart *part) {
    if (TheTempColoredPart) {
        delete TheTempColoredPart;
    }
    TheTempColoredPart = part;
}
#endif

void CarCustomizeManager::ClearTempColoredPart() {
    if (TheTempColoredPart) {
        delete TheTempColoredPart;
    }
    TheTempColoredPart = nullptr;
}

CarPart *CarCustomizeManager::GetStockCarPart(unsigned int slot_id) {
    RideInfo info;
    FEDatabase->GetPlayerCarStable(0)->BuildRideForPlayer(TuningCar->Handle, 0, &info);
    info.SetStockParts();
    return info.GetPart(slot_id);
}

void CarCustomizeManager::ResetToStockCarParts() {
    RideInfo ride;
    FEDatabase->GetPlayerCarStable(0)->BuildRideForPlayer(TuningCar->Handle, 0, &ride);
    ride.SetStockParts();
    PreviewRecord.WriteRideIntoRecord(&ride);
    CarViewer::SetRideInfo(&ride, SET_RIDE_INFO_REASON_LOAD_CAR, eCARVIEWER_PLAYER1_CAR);
}

void CarCustomizeManager::ResetPreview() {
    FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(0);
    FECustomizationRecord *src = stable->GetCustomizationRecordByHandle(TuningCar->Customization);
    PreviewRecord = *src;
    RideInfo ride;
    stable->BuildRideForPlayer(TuningCar->Handle, 0, &ride);
    PreviewRecord.WriteRecordIntoRide(&ride);
    CarViewer::SetRideInfo(&ride, SET_RIDE_INFO_REASON_LOAD_CAR, eCARVIEWER_PLAYER1_CAR);
    for (ShoppingCartItem *item = GetFirstCartItem(); item != ShoppingCart.EndOfList(); item = item->GetNext()) {
        SelectablePart *buy = item->GetBuyingPart();
        if (buy->IsPerformancePkg()) {
            PreviewPerfPkg(static_cast<Physics::Upgrades::Type>(static_cast<int>(buy->GetPhysicsType())), buy->GetUpgradeLevel());
        } else {
            PreviewPart(buy->GetSlotID(), buy->GetPart());
        }
    }
}

void CarCustomizeManager::PreviewPart(int slot_id, CarPart *part) {
    PreviewRecord.SetInstalledPart(slot_id, part);
    RideInfo ride;
    FEDatabase->GetPlayerCarStable(0)->BuildRideForPlayer(TuningCar->Handle, 0, &ride);
    PreviewRecord.WriteRecordIntoRide(&ride);
    CarViewer::SetRideInfo(&ride, SET_RIDE_INFO_REASON_LOAD_CAR, eCARVIEWER_PLAYER1_CAR);
}

void CarCustomizeManager::InstallPart(int slot_id, CarPart *part) {
    FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(0);
    FECustomizationRecord *record = stable->GetCustomizationRecordByHandle(TuningCar->Customization);
    record->SetInstalledPart(slot_id, part);
    PreviewPart(slot_id, part);
}

CarPart *CarCustomizeManager::GetInstalledCarPart(int slot_id) {
    FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(0);
    FECustomizationRecord *record = stable->GetCustomizationRecordByHandle(TuningCar->Customization);
    return record->GetInstalledPart(TuningCar->GetType(), slot_id);
}

void CarCustomizeManager::PreviewPerfPkg(Physics::Upgrades::Type part_type, int level) {
    if (level == 7) {
        PreviewRecord.SetInstalledJunkman(part_type, true);
        PreviewRecord.WriteRecordIntoPhysics(ThePVehicle);

    } else {
        PreviewRecord.SetInstalledPhysics(part_type, level);
        PreviewRecord.WriteRecordIntoPhysics(ThePVehicle);
    }
}

void CarCustomizeManager::InstallPerfPkg(Physics::Upgrades::Type part_type, int level) {
    FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(0);
    FECustomizationRecord *record = stable->GetCustomizationRecordByHandle(TuningCar->Customization);
    if (level == 7) {
        record->SetInstalledJunkman(part_type, true);
        if (!record->WriteRecordIntoPhysics(ThePVehicle)) {
            record->SetInstalledJunkman(part_type, false);
            record->WriteRecordIntoPhysics(ThePVehicle);
        }
    } else {
        record->SetInstalledPhysics(part_type, level);
        if (!record->WriteRecordIntoPhysics(ThePVehicle)) {
            record->SetInstalledPhysics(part_type, 0);
            record->WriteRecordIntoPhysics(ThePVehicle);
        }
    }
    PreviewPerfPkg(part_type, level);
}

bool CarCustomizeManager::IsJunkmanInstalled(Physics::Upgrades::Type type) {
    FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(0);
    FECustomizationRecord *record = stable->GetCustomizationRecordByHandle(TuningCar->Customization);
    return record->GetInstalledJunkman(type);
}

int CarCustomizeManager::GetInstalledPerfPkg(Physics::Upgrades::Type type) {
    FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(0);
    FECustomizationRecord *actual_record = stable->GetCustomizationRecordByHandle(TuningCar->Customization);
    return actual_record->GetInstalledPhysics()->Part[type];
}

int CarCustomizeManager::GetMaxPackages(Physics::Upgrades::Type type) {
#ifdef EA_BUILD_A124
    switch (type) {
        case 0:
            return 6;
        case 1:
            return 6;
        case 2:
            return 6;
        case 3:
            return 6;
        case 4:
            return 6;
        case 5:
            return 6;
        case 6:
            return 6;
        default:
            return -1;
    }
#else
    switch (type) {
        case 0:
            return 3;
        case 1:
            return 4;
        case 2:
            return 3;
        case 3:
            return 4;
        case 4:
            return 4;
        case 5:
            return 3;
        case 6:
            return 3;
        default:
            return -1;
    }
#endif
}

int CarCustomizeManager::GetNumPackages(Physics::Upgrades::Type type) {
    return Physics::Upgrades::GetMaxLevel(ThePVehicle, type);
}

void CarCustomizeManager::MaxOutPerformance() {
    for (int i = 0; i < 7; i++) {
        Physics::Upgrades::Type type = static_cast<Physics::Upgrades::Type>(i);
        int num_packages = GetNumPackages(type);
        int best_level = 0;

        for (int j = 0; j < num_packages; j++) {
            SelectablePart sp(nullptr, 0, j + 1, type, true, CPS_AVAILABLE, 0, false);
#ifndef EA_BUILD_A124
            int level_param = GetMaxPackages(type) - GetNumPackages(type) + j + 1;
#endif
            if (!IsPartLocked(&sp
#ifndef EA_BUILD_A124
                              ,
                              level_param
#endif
                              )) {
                best_level = j + 1;
            }
        }

        if (best_level != 0) {
            ShoppingCartItem *existing = IsPartTypeInCart(type);
            if (existing) {
                RemoveFromCart(existing);
            }

            SelectablePart *sp = new ("SelectablePart 13", 0) SelectablePart(nullptr, 0, best_level, type, true, CPS_AVAILABLE, 0, false);

            eCustomizePartState status = CPS_AVAILABLE;
#ifndef EA_BUILD_A124
            int level_param = GetMaxPackages(type) - GetNumPackages(type) + best_level + 1;
#endif
            if (IsPartLocked(sp
#ifndef EA_BUILD_A124
                             ,
                             level_param
#endif
                             )) {
                status = CPS_LOCKED;
            } else if (IsPartNew(sp
#ifndef EA_BUILD_A124
                                 ,
                                 level_param
#endif
                                 )) {
                status = CPS_NEW;
            }

            if (IsPartInstalled(sp)) {
                status = static_cast<eCustomizePartState>(status | CPS_INSTALLED);
            } else if (IsPartInCart(sp)) {
                status = static_cast<eCustomizePartState>(status | CPS_IN_CART);
            }

            sp->SetPartState(static_cast<unsigned int>(status));
            sp->SetPrice(gCarCustomizeManager.GetPartPrice(sp));

            AddToCart(sp);
            delete sp;
        }
    }
}

float CarCustomizeManager::GetPerformanceRating(ePerformanceRatingType type, bool preview) {
    Physics::Info::Performance perf;
    if (preview) {
        Physics::Info::EstimatePerformance(ThePVehicle, perf);
    } else {
        Attrib::Gen::pvehicle pveh(TuningCar->VehicleKey, 0, nullptr);
        FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(0);
        stable->WriteRecordIntoPhysics(TuningCar->Handle, pveh);
        Physics::Info::EstimatePerformance(pveh, perf);
    }
    switch (type) {
        case 0:
            return perf.TopSpeed;
        case 1:
            return perf.Handling;
        case 2:
            return perf.Acceleration;
        default:
            return 0.0f;
    }
}

void CarCustomizeManager::UpdateHeatOnVehicle(SelectablePart *part, FECareerRecord *record) {
    if (part && record && !part->IsPerformancePkg() && IsCareerMode()) {
        float heat_factor = CustomizeIsInBackRoom() ? 0.75f : 1.0f;

        switch (part->GetSlotID()) {
            case 0x17:
                record->AdjustHeatOnBodyKitApplied(heat_factor);
                break;
            case 0x2c:
                record->AdjustHeatOnSpoilerApplied(heat_factor);
                break;
            case 0x3f:
                record->AdjustHeatOnHoodApplied(heat_factor);
                break;
            case 0x3e:
                record->AdjustHeatOnRoofScoopApplied(heat_factor);
                break;
            case 0x42:
                record->AdjustHeatOnRimApplied(heat_factor);
                break;
            case 0x83:
                record->AdjustHeatOnWindowTintApplied(heat_factor);
                break;
            case 0x4c:
                record->AdjustHeatOnPaintApplied(heat_factor);
                break;
            case 0x4e:
                record->AdjustHeatOnRimPaintApplied(heat_factor);
                break;
            case 0x4d:
                record->AdjustHeatOnVinylApplied(heat_factor);
                break;
            case 0x69:
                record->AdjustHeatOnBodyKitApplied(heat_factor);
                break;
            case 0x53:
            case 0x5b:
            case 0x63: case 0x64: case 0x65: case 0x66: case 0x67: case 0x68:
            case 0x6b: case 0x6c: case 0x6d: case 0x6e: case 0x6f: case 0x70:
            case 0x73:
            case 0x7b:
                record->AdjustHeatOnDecalApplied(heat_factor);
                break;
        }
    }
}

eUnlockFilters CarCustomizeManager::GetUnlockFilter() {
    if (FEDatabase->IsCareerMode()) {
        if (IsInBackRoom()) {
            return UNLOCK_BACKROOM;
        }
        return UNLOCK_CAREER_MODE;
    }
    if ((FEDatabase->GetGameMode() & 40) == 40) {
        return UNLOCK_ONLINE;
    }
    if (FEDatabase->IsQuickRaceMode()) {
        return UNLOCK_QUICK_RACE;
    }
    return UNLOCK_QUICK_RACE;
}

unsigned int CarCustomizeManager::GetUnlockHash(eCustomizeCategory cat, int upgrade_lvl) {
    const char *name = nullptr;
    switch (cat) {
        case 0x101:
            name = "PARTS_BODYKITS";
            break;
        case 0x102:
            name = "PARTS_SPOILERS";
            break;
        case 0x104:
            name = "PARTS_HOODS";
            break;
        case 0x105:
            name = "PARTS_ROOFSCOOPS";
            break;
        case 0x701:
        case 0x702:
        case 0x703:
        case 0x704:
        case 0x705:
        case 0x706:
        case 0x707:
        case 0x708:
        case 0x709:
        case 0x70a:
        case 0x70b:
            name = "PARTS_RIMS";
            break;
        case 0x201:
            name = "PERF_ENGINE";
            break;
        case 0x202:
            name = "PERF_TRANSMISSION";
            break;
        case 0x203:
            name = "PERF_SUSPENSION";
            break;
        case 0x204:
            name = "PERF_NITROUS";
            break;
        case 0x205:
            name = "PERF_TIRES";
            break;
        case 0x206:
            name = "PERF_BRAKES";
            break;
        case 0x207:
            if (IsTurbo())
                name = "PERF_TURBO";
            else
                name = "PERF_SUPERCHARGER";
            break;
        case 0x301:
            name = "VISUAL_PAINT";
            break;
        case 0x303:
            name = "VISUAL_RIMPAINT";
            break;
        case 0x304:
            name = "VISUAL_WINDOWTINT";
            break;
        case 0x306:
            name = "VISUAL_NUMBERS";
            break;
        case 0x307:
            name = "VISUAL_HUDS";
            break;
        case 0x402:
        case 0x403:
        case 0x404:
        case 0x405:
        case 0x406:
        case 0x407:
        case 0x408:
        case 0x409:
            name = "VISUAL_VINYLS";
            break;
        case 0x501:
        case 0x502:
        case 0x503:
        case 0x504:
        case 0x505:
        case 0x506:
        case 0x601:
        case 0x602:
        case 0x603:
        case 0x604:
        case 0x605:
        case 0x606:
            name = "VISUAL_DECALS";
            break;
    }
    if (!name || !upgrade_lvl) {
        return 0x9bb9ccc3;
    } else {
        char buf[100];
        FEngSNPrintf(buf, 100, "CUSTOMIZATION_%s_%d", name, upgrade_lvl);
        unsigned int hash = FEngHashString(buf);
        if (DoesStringExist(hash) == 1) {
            return hash;
        }
    }
    return 0x9bb9ccc3;
}

bool CarCustomizeManager::IsPartInstalled(SelectablePart *part) {
    if (part) {
        if (part->IsPerformancePkg()) {
            if (part->IsJunkmanPart()) {
                return IsJunkmanInstalled(part->GetPhysicsType());
            }
            int lvl = GetInstalledPerfPkg(part->GetPhysicsType());
            if (static_cast<int>(part->GetUpgradeLevel()) == lvl) {
                return true;
            }
        } else {
            CarPart *installed = GetInstalledCarPart(part->GetSlotID());
            if (installed == part->GetPart()) {
                return true;
            }
        }
    }
    return false;
}

bool CarCustomizeManager::IsPartLocked(SelectablePart *part
#ifndef EA_BUILD_A124
                                       ,
                                       int perf_unlock_level
#endif
) {
#ifdef EA_BUILD_A124
    if (part->IsPerformancePkg()) {
        return !UnlockSystem::IsPerfPackageUnlocked(GetUnlockFilter(), part->GetPhysicsType(), part->GetUpgradeLevel(), 0, false);
    } else {
        return !UnlockSystem::IsCarPartUnlocked(GetUnlockFilter(), part->GetSlotID(), part->GetPart(), 0, false);
    }
#else
    if (part->IsPerformancePkg()) {
        return !UnlockSystem::IsPerfPackageUnlocked(GetUnlockFilter(), part->GetPhysicsType(),
#ifndef EA_BUILD_A124
                                                    perf_unlock_level,
#else
                                                    part->GetUpgradeLevel(), // the prototype unlocks by the level of the package itself
#endif
                                                    0, IsInBackRoom());
    } else {
        int slot = part->GetSlotID();
        switch (slot) {
            case 0x53:
            case 0x5b: {

                return !UnlockSystem::IsUnlockableUnlocked(GetUnlockFilter(), static_cast<eUnlockableEntity>(0x2c), 1, 0, IsInBackRoom());
                break;
            }
            case 0x63:
            case 0x64:
            case 0x65:
            case 0x66:
            case 0x67:
            case 0x68:
            case 0x6b:
            case 0x6c:
            case 0x6d:
            case 0x6e:
            case 0x6f:
            case 0x70: {
                return !UnlockSystem::IsUnlockableUnlocked(GetUnlockFilter(), static_cast<eUnlockableEntity>(0x2e), 2, 0, IsInBackRoom());
                break;
            }
            case 0x73:
            case 0x7b: {
                return !UnlockSystem::IsUnlockableUnlocked(GetUnlockFilter(), static_cast<eUnlockableEntity>(0x30), 3, 0, IsInBackRoom());
                break;
            }
            default: {
                return !UnlockSystem::IsCarPartUnlocked(GetUnlockFilter(), part->GetSlotID(), part->GetPart(), 0, IsInBackRoom());
                break;
            }
        }
    }
#endif
}

bool CarCustomizeManager::IsPartNew(SelectablePart *part
#ifndef EA_BUILD_A124
                                    ,
                                    int perf_unlock_level
#endif
) {
    eUnlockableEntity ent;
#ifdef EA_BUILD_A124
    // El prototipo desbloquea por el nivel del propio paquete: una sola llamada tras elegir la entidad.
    if (part->IsPerformancePkg()) {
        ent = MapPerfPkgToUnlockable(part->GetPhysicsType());
    } else {
        ent = MapCarPartToUnlockable(part->GetSlotID(), part->GetPart());
    }
    return UnlockSystem::IsUnlockableNew(GetUnlockFilter(), ent, part->GetUpgradeLevel());
#else
    if (part->IsPerformancePkg()) {
        ent = MapPerfPkgToUnlockable(part->GetPhysicsType());
        return UnlockSystem::IsUnlockableNew(GetUnlockFilter(), ent, perf_unlock_level);
    } else {
        ent = MapCarPartToUnlockable(part->GetSlotID(), part->GetPart());
        return UnlockSystem::IsUnlockableNew(GetUnlockFilter(), ent, part->GetUpgradeLevel());
    }
#endif
}

bool CarCustomizeManager::IsCategoryNew(unsigned int cat) {
#ifdef EA_BUILD_A124 // el prototipo numera de otra forma lo desbloqueable; las categorias compuestas suman tambien la suya
    bool answer = false;
    eUnlockableEntity titty;

    switch (cat) {
        case 0x801:
            titty = static_cast<eUnlockableEntity>(1);
            break;
        case 0x802:
            titty = static_cast<eUnlockableEntity>(2);
            break;
        case 0x803:
            titty = static_cast<eUnlockableEntity>(3);
            break;
        case 0x101:
            titty = static_cast<eUnlockableEntity>(0xb);
            break;
        case 0x102:
            titty = static_cast<eUnlockableEntity>(0xc);
            break;
        case 0x103:
            titty = static_cast<eUnlockableEntity>(0xd);
            break;
        case 0x104:
            titty = static_cast<eUnlockableEntity>(0xe);
            break;
        case 0x105:
            titty = static_cast<eUnlockableEntity>(0xf);
            break;
        case 0x307:
            titty = static_cast<eUnlockableEntity>(0x11);
            break;
        case 0x201:
            titty = static_cast<eUnlockableEntity>(8);
            break;
        case 0x202:
            titty = static_cast<eUnlockableEntity>(7);
            break;
        case 0x203:
            titty = static_cast<eUnlockableEntity>(6);
            break;
        case 0x204:
            titty = static_cast<eUnlockableEntity>(10);
            break;
        case 0x205:
            titty = static_cast<eUnlockableEntity>(4);
            break;
        case 0x206:
            titty = static_cast<eUnlockableEntity>(5);
            break;
        case 0x207:
            titty = static_cast<eUnlockableEntity>(9);
            break;
        case 0x301:
            titty = static_cast<eUnlockableEntity>(0x17);
            break;
        case 0x302:
            answer |= IsCategoryNew(0x402);
            answer |= IsCategoryNew(0x403);
            answer |= IsCategoryNew(0x404);
            answer |= IsCategoryNew(0x405);
            answer |= IsCategoryNew(0x406);
            answer |= IsCategoryNew(0x407);
            answer |= IsCategoryNew(0x408);
            answer |= IsCategoryNew(0x409);
        case 0x303:
            titty = static_cast<eUnlockableEntity>(0x18);
            break;
        case 0x304:
            titty = static_cast<eUnlockableEntity>(0x12);
            break;
        case 0x305:
            answer |= IsCategoryNew(0x501);
            answer |= IsCategoryNew(0x502);
            answer |= IsCategoryNew(0x505);
            answer |= IsCategoryNew(0x506);
            answer |= IsCategoryNew(0x601);
            answer |= IsCategoryNew(0x602);
            answer |= IsCategoryNew(0x603);
            answer |= IsCategoryNew(0x604);
            answer |= IsCategoryNew(0x605);
            answer |= IsCategoryNew(0x606);
        case 0x306:
            titty = static_cast<eUnlockableEntity>(0x2b);
            break;
        case 0x402:
            titty = static_cast<eUnlockableEntity>(0x23);
            break;
        case 0x403:
            titty = static_cast<eUnlockableEntity>(0x24);
            break;
        case 0x404:
            titty = static_cast<eUnlockableEntity>(0x25);
            break;
        case 0x405:
            titty = static_cast<eUnlockableEntity>(0x26);
            break;
        case 0x406:
            titty = static_cast<eUnlockableEntity>(0x27);
            break;
        case 0x407:
            titty = static_cast<eUnlockableEntity>(0x28);
            break;
        case 0x408:
            titty = static_cast<eUnlockableEntity>(0x29);
            break;
        case 0x409:
            titty = static_cast<eUnlockableEntity>(0x2a);
            break;
        case 0x702:
            titty = static_cast<eUnlockableEntity>(0x19);
            break;
        case 0x703:
            titty = static_cast<eUnlockableEntity>(0x1a);
            break;
        case 0x704:
            titty = static_cast<eUnlockableEntity>(0x1b);
            break;
        case 0x705:
            titty = static_cast<eUnlockableEntity>(0x1c);
            break;
        case 0x706:
            titty = static_cast<eUnlockableEntity>(0x1d);
            break;
        case 0x707:
            titty = static_cast<eUnlockableEntity>(0x1e);
            break;
        case 0x708:
            titty = static_cast<eUnlockableEntity>(0x1f);
            break;
        case 0x709:
            titty = static_cast<eUnlockableEntity>(0x20);
            break;
        case 0x70a:
            titty = static_cast<eUnlockableEntity>(0x21);
            break;
        case 0x70b:
            titty = static_cast<eUnlockableEntity>(0x22);
            break;
        case 0x501:
            titty = static_cast<eUnlockableEntity>(0x2c);
            break;
        case 0x502:
            titty = static_cast<eUnlockableEntity>(0x2d);
            break;
        case 0x503:
            titty = static_cast<eUnlockableEntity>(0x2e);
            break;
        case 0x504:
            titty = static_cast<eUnlockableEntity>(0x2f);
            break;
        case 0x505:
            titty = static_cast<eUnlockableEntity>(0x30);
            break;
        case 0x506:
            titty = static_cast<eUnlockableEntity>(0x31);
            break;
        case 0x601:
            titty = static_cast<eUnlockableEntity>(0x33);
            break;
        case 0x602:
            titty = static_cast<eUnlockableEntity>(0x34);
            break;
        case 0x603:
            titty = static_cast<eUnlockableEntity>(0x35);
            break;
        case 0x604:
            titty = static_cast<eUnlockableEntity>(0x36);
            break;
        case 0x605:
            titty = static_cast<eUnlockableEntity>(0x37);
            break;
        case 0x606:
            titty = static_cast<eUnlockableEntity>(0x38);
            break;
        default:
            return true;
    }

    answer |= UnlockSystem::IsUnlockableNew(GetUnlockFilter(), titty, -2);
    return answer;
#else
    bool answer = false;
    eUnlockableEntity titty;

    switch (cat) {
        case 0x801: {
            for (unsigned int i = 0x101; i <= 0x105; i++) {
                if (IsCategoryNew(i))
                    return true;
            }
            return false;
        }
        case 0x802: {
            for (unsigned int i = 0x201; i <= 0x207; i++) {
                if (IsCategoryNew(i))
                    return true;
            }
            return false;
        }
        case 0x803: {
            for (unsigned int i = 0x301; i <= 0x307; i++) {
                if (IsCategoryNew(i))
                    return true;
            }
            return false;
        }
        case 0x101:
            titty = static_cast<eUnlockableEntity>(0xb);
            break;
        case 0x102:
            titty = static_cast<eUnlockableEntity>(0xc);
            break;
        case 0x103: {
            for (unsigned int i = 0x702; i <= 0x70b; i++) {
                if (IsCategoryNew(i))
                    return true;
            }
            return false;
        }
        case 0x104:
            titty = static_cast<eUnlockableEntity>(0xe);
            break;
        case 0x105:
            titty = static_cast<eUnlockableEntity>(0xf);
            break;
        case 0x307:
            titty = static_cast<eUnlockableEntity>(0x11);
            break;
        case 0x201:
            titty = static_cast<eUnlockableEntity>(8);
            break;
        case 0x202:
            titty = static_cast<eUnlockableEntity>(7);
            break;
        case 0x203:
            titty = static_cast<eUnlockableEntity>(6);
            break;
        case 0x204:
            titty = static_cast<eUnlockableEntity>(10);
            break;
        case 0x205:
            titty = static_cast<eUnlockableEntity>(4);
            break;
        case 0x206:
            titty = static_cast<eUnlockableEntity>(5);
            break;
        case 0x207:
            titty = static_cast<eUnlockableEntity>(9);
            break;
        case 0x301:
            titty = static_cast<eUnlockableEntity>(0x17);
            break;
        case 0x302: {
            for (unsigned int i = 0x402; i <= 0x409; i++) {
                if (IsCategoryNew(i))
                    return true;
            }
            return false;
        }
        case 0x303:
            titty = static_cast<eUnlockableEntity>(0x18);
            break;
        case 0x304:
            titty = static_cast<eUnlockableEntity>(0x12);
            break;
        case 0x305:
            if (IsCategoryNew(0x501))
                return true;
            if (IsCategoryNew(0x505))
                return true;
            if (IsCategoryNew(0x503))
                return true;
            return false;
        case 0x306:
            titty = static_cast<eUnlockableEntity>(0x2b);
            break;
        case 0x402:
            titty = static_cast<eUnlockableEntity>(0x23);
            break;
        case 0x403:
            titty = static_cast<eUnlockableEntity>(0x24);
            break;
        case 0x404:
            titty = static_cast<eUnlockableEntity>(0x25);
            break;
        case 0x405:
            titty = static_cast<eUnlockableEntity>(0x26);
            break;
        case 0x406:
            titty = static_cast<eUnlockableEntity>(0x27);
            break;
        case 0x407:
            titty = static_cast<eUnlockableEntity>(0x28);
            break;
        case 0x408:
            titty = static_cast<eUnlockableEntity>(0x29);
            break;
        case 0x409:
            titty = static_cast<eUnlockableEntity>(0x2a);
            break;
        case 0x702:
            titty = static_cast<eUnlockableEntity>(0x19);
            break;
        case 0x703:
            titty = static_cast<eUnlockableEntity>(0x1a);
            break;
        case 0x704:
            titty = static_cast<eUnlockableEntity>(0x1b);
            break;
        case 0x705:
            titty = static_cast<eUnlockableEntity>(0x1c);
            break;
        case 0x706:
            titty = static_cast<eUnlockableEntity>(0x1d);
            break;
        case 0x707:
            titty = static_cast<eUnlockableEntity>(0x1e);
            break;
        case 0x708:
            titty = static_cast<eUnlockableEntity>(0x1f);
            break;
        case 0x709:
            titty = static_cast<eUnlockableEntity>(0x20);
            break;
        case 0x70a:
            titty = static_cast<eUnlockableEntity>(0x21);
            break;
        case 0x70b:
            titty = static_cast<eUnlockableEntity>(0x22);
            break;
        case 0x501:
        case 0x502:
            titty = static_cast<eUnlockableEntity>(0x2c);
            break;
        case 0x503:
        case 0x504:
        case 0x601:
        case 0x602:
        case 0x603:
        case 0x604:
        case 0x605:
        case 0x606:
            titty = static_cast<eUnlockableEntity>(0x2e);
            break;
        case 0x505:
        case 0x506:
            titty = static_cast<eUnlockableEntity>(0x30);
            break;
        default:
            return true;
    }

    answer |= UnlockSystem::IsUnlockableNew(GetUnlockFilter(), titty, -2);
    return answer;
#endif
}

bool CarCustomizeManager::IsCategoryLocked(unsigned int cat, bool backroom) {
#ifdef EA_BUILD_A124 // el prototipo numera de otra forma lo desbloqueable y mira cada categoria hija
    eUnlockableEntity titty;

    switch (cat) {
        case 0x801:
            titty = static_cast<eUnlockableEntity>(1);
            break;
        case 0x802:
            titty = static_cast<eUnlockableEntity>(2);
            break;
        case 0x803:
            titty = static_cast<eUnlockableEntity>(3);
            break;
        case 0x101:
            titty = static_cast<eUnlockableEntity>(0xb);
            break;
        case 0x102:
            titty = static_cast<eUnlockableEntity>(0xc);
            break;
        case 0x103:
            titty = static_cast<eUnlockableEntity>(0xd);
            break;
        case 0x104:
            titty = static_cast<eUnlockableEntity>(0xe);
            break;
        case 0x105:
            titty = static_cast<eUnlockableEntity>(0xf);
            break;
        case 0x307:
            titty = static_cast<eUnlockableEntity>(0x11);
            break;
        case 0x201:
            titty = static_cast<eUnlockableEntity>(8);
            break;
        case 0x202:
            titty = static_cast<eUnlockableEntity>(7);
            break;
        case 0x203:
            titty = static_cast<eUnlockableEntity>(6);
            break;
        case 0x204:
            titty = static_cast<eUnlockableEntity>(10);
            break;
        case 0x205:
            titty = static_cast<eUnlockableEntity>(4);
            break;
        case 0x206:
            titty = static_cast<eUnlockableEntity>(5);
            break;
        case 0x207:
            titty = static_cast<eUnlockableEntity>(9);
            break;
        case 0x301:
            titty = static_cast<eUnlockableEntity>(0x17);
            break;
        case 0x302: {
            bool locked = false;
            locked |= IsCategoryLocked(0x402, backroom);
            locked |= IsCategoryLocked(0x403, backroom);
            locked |= IsCategoryLocked(0x404, backroom);
            locked |= IsCategoryLocked(0x405, backroom);
            locked |= IsCategoryLocked(0x406, backroom);
            locked |= IsCategoryLocked(0x407, backroom);
            locked |= IsCategoryLocked(0x408, backroom);
            locked |= IsCategoryLocked(0x409, backroom);
            return locked;
        }
        case 0x303:
            titty = static_cast<eUnlockableEntity>(0x18);
            break;
        case 0x304:
            titty = static_cast<eUnlockableEntity>(0x12);
            break;
        case 0x305: {
            bool locked = false;
            locked |= IsCategoryLocked(0x501, backroom);
            locked |= IsCategoryLocked(0x502, backroom);
            locked |= IsCategoryLocked(0x505, backroom);
            locked |= IsCategoryLocked(0x506, backroom);
            locked |= IsCategoryLocked(0x601, backroom);
            locked |= IsCategoryLocked(0x602, backroom);
            locked |= IsCategoryLocked(0x603, backroom);
            locked |= IsCategoryLocked(0x604, backroom);
            locked |= IsCategoryLocked(0x605, backroom);
            locked |= IsCategoryLocked(0x606, backroom);
            return locked;
        }
        case 0x306:
            titty = static_cast<eUnlockableEntity>(0x2b);
            break;
        case 0x402:
            titty = static_cast<eUnlockableEntity>(0x23);
            break;
        case 0x403:
            titty = static_cast<eUnlockableEntity>(0x24);
            break;
        case 0x404:
            titty = static_cast<eUnlockableEntity>(0x25);
            break;
        case 0x405:
            titty = static_cast<eUnlockableEntity>(0x26);
            break;
        case 0x406:
            titty = static_cast<eUnlockableEntity>(0x27);
            break;
        case 0x407:
            titty = static_cast<eUnlockableEntity>(0x28);
            break;
        case 0x408:
            titty = static_cast<eUnlockableEntity>(0x29);
            break;
        case 0x409:
            titty = static_cast<eUnlockableEntity>(0x2a);
            break;
        case 0x702:
            titty = static_cast<eUnlockableEntity>(0x19);
            break;
        case 0x703:
            titty = static_cast<eUnlockableEntity>(0x1a);
            break;
        case 0x704:
            titty = static_cast<eUnlockableEntity>(0x1b);
            break;
        case 0x705:
            titty = static_cast<eUnlockableEntity>(0x1c);
            break;
        case 0x706:
            titty = static_cast<eUnlockableEntity>(0x1d);
            break;
        case 0x707:
            titty = static_cast<eUnlockableEntity>(0x1e);
            break;
        case 0x708:
            titty = static_cast<eUnlockableEntity>(0x1f);
            break;
        case 0x709:
            titty = static_cast<eUnlockableEntity>(0x20);
            break;
        case 0x70a:
            titty = static_cast<eUnlockableEntity>(0x21);
            break;
        case 0x70b:
            titty = static_cast<eUnlockableEntity>(0x22);
            break;
        case 0x501:
            titty = static_cast<eUnlockableEntity>(0x2c);
            break;
        case 0x502:
            titty = static_cast<eUnlockableEntity>(0x2d);
            break;
        case 0x503:
            titty = static_cast<eUnlockableEntity>(0x2e);
            break;
        case 0x504:
            titty = static_cast<eUnlockableEntity>(0x2f);
            break;
        case 0x505:
            titty = static_cast<eUnlockableEntity>(0x30);
            break;
        case 0x506:
            titty = static_cast<eUnlockableEntity>(0x31);
            break;
        case 0x601:
            titty = static_cast<eUnlockableEntity>(0x33);
            break;
        case 0x602:
            titty = static_cast<eUnlockableEntity>(0x34);
            break;
        case 0x603:
            titty = static_cast<eUnlockableEntity>(0x35);
            break;
        case 0x604:
            titty = static_cast<eUnlockableEntity>(0x36);
            break;
        case 0x605:
            titty = static_cast<eUnlockableEntity>(0x37);
            break;
        case 0x606:
            titty = static_cast<eUnlockableEntity>(0x38);
            break;
        default:
            return true;
    }

    if (backroom) {
        return !UnlockSystem::IsBackroomAvailable(GetUnlockFilter(), titty, 0);
    } else {
        return !UnlockSystem::IsUnlockableUnlocked(GetUnlockFilter(), titty, 0, 0, false);
    }
#else
    eUnlockableEntity titty;
    int level = 0;

    switch (cat) {
        case 0x801: {
            for (unsigned int i = 0x101; i <= 0x105; i++) {
                if (!IsCategoryLocked(i, backroom))
                    return false;
            }
            return true;
        }
        case 0x802: {
            for (unsigned int i = 0x201; i <= 0x207; i++) {
                if (!IsCategoryLocked(i, backroom))
                    return false;
            }
            return true;
        }
        case 0x803: {
            for (unsigned int i = 0x301; i <= 0x307; i++) {
                if (!IsCategoryLocked(i, backroom))
                    return false;
            }
            return true;
        }
        case 0x101:
            titty = static_cast<eUnlockableEntity>(0xb);
            break;
        case 0x102:
            titty = static_cast<eUnlockableEntity>(0xc);
            break;
        case 0x103: {
            for (unsigned int i = 0x702; i <= 0x70b; i++) {
                if (!IsRimCategoryLocked(i, backroom))
                    return false;
            }
            return true;
        }
        case 0x104:
            titty = static_cast<eUnlockableEntity>(0xe);
            break;
        case 0x105:
            titty = static_cast<eUnlockableEntity>(0xf);
            break;
        case 0x307:
            titty = static_cast<eUnlockableEntity>(0x11);
            break;
        case 0x201:
            if (backroom && !CanInstallJunkman(static_cast<Physics::Upgrades::Type>(4)))
                return true;
            titty = static_cast<eUnlockableEntity>(8);
            break;
        case 0x202:
            if (backroom && !CanInstallJunkman(static_cast<Physics::Upgrades::Type>(3)))
                return true;
            titty = static_cast<eUnlockableEntity>(7);
            break;
        case 0x203:
            if (backroom && !CanInstallJunkman(static_cast<Physics::Upgrades::Type>(2)))
                return true;
            titty = static_cast<eUnlockableEntity>(6);
            break;
        case 0x204:
            if (backroom && !CanInstallJunkman(static_cast<Physics::Upgrades::Type>(6)))
                return true;
            titty = static_cast<eUnlockableEntity>(10);
            break;
        case 0x205:
            if (backroom && !CanInstallJunkman(static_cast<Physics::Upgrades::Type>(0)))
                return true;
            titty = static_cast<eUnlockableEntity>(4);
            break;
        case 0x206:
            if (backroom && !CanInstallJunkman(static_cast<Physics::Upgrades::Type>(1)))
                return true;
            titty = static_cast<eUnlockableEntity>(5);
            break;
        case 0x207:
            if (backroom && !CanInstallJunkman(static_cast<Physics::Upgrades::Type>(5)))
                return true;
            titty = static_cast<eUnlockableEntity>(9);
            break;
        case 0x301:
            titty = static_cast<eUnlockableEntity>(0x17);
            break;
        case 0x302: {
            for (unsigned int i = 0x402; i <= 0x409; i++) {
                if (!IsVinylCategoryLocked(i, backroom))
                    return false;
            }
            return true;
        }
        case 0x303:
            titty = static_cast<eUnlockableEntity>(0x18);
            break;
        case 0x304:
            titty = static_cast<eUnlockableEntity>(0x12);
            break;
        case 0x305:
            if (!IsCategoryLocked(0x501, backroom))
                return false;
            if (!IsCategoryLocked(0x505, backroom))
                return false;
            if (!IsCategoryLocked(0x503, backroom))
                return false;
            return true;
        case 0x306:
            titty = static_cast<eUnlockableEntity>(0x2b);
            break;
        case 0x402:
        case 0x403:
        case 0x404:
        case 0x405:
        case 0x406:
        case 0x407:
        case 0x408:
        case 0x409:
            return IsVinylCategoryLocked(cat, backroom);
        case 0x702:
        case 0x703:
        case 0x704:
        case 0x705:
        case 0x706:
        case 0x707:
        case 0x708:
        case 0x709:
        case 0x70a:
        case 0x70b:
            return IsRimCategoryLocked(cat, backroom);
        case 0x501:
        case 0x502:
            level = 1;
            titty = static_cast<eUnlockableEntity>(0x2c);
            break;
        case 0x503:
        case 0x504:
        case 0x601:
        case 0x602:
        case 0x603:
        case 0x604:
        case 0x605:
        case 0x606:
            level = 2;
            titty = static_cast<eUnlockableEntity>(0x2e);
            break;
        case 0x505:
        case 0x506:
            level = 3;
            titty = static_cast<eUnlockableEntity>(0x30);
            break;
        default:
            return true;
    }

    if (backroom) {
        return !UnlockSystem::IsBackroomAvailable(GetUnlockFilter(), titty, level);
    } else {
        return !UnlockSystem::IsUnlockableUnlocked(GetUnlockFilter(), titty, level, 0, false);
    }
#endif
}

bool CarCustomizeManager::IsRimCategoryLocked(unsigned int cat, bool backroom) {
    unsigned int brand_name = 0;
    switch (cat) {
        case 0x702:
            brand_name = 0x352d08d1;
            break;
        case 0x703:
            brand_name = 0x9136;
            break;
        case 0x704:
            brand_name = 0x9536;
            break;
        case 0x705:
            brand_name = 0x2b77feb;
            break;
        case 0x706:
            brand_name = 0x324ac97;
            break;
        case 0x707:
            brand_name = 0x48e25793;
            break;
        case 0x708:
            brand_name = 0xdd544a02;
            break;
        case 0x709:
            brand_name = 0x648;
            break;
        case 0x70a:
            brand_name = 0x1e6a3b;
            break;
        case 0x70b:
            brand_name = 0x1c386b;
            break;
    }
    bTList<SelectablePart> list;
    GetCarPartList(0x42, list
#ifndef EA_BUILD_A124
                   ,
                   brand_name
#endif
    );
    bool locked = true;
    SelectablePart *part = list.GetHead();
    while (part != list.EndOfList()) {
        if (part->GetPart()->GetBrandNameHash() == brand_name && !IsPartLocked(part
#ifndef EA_BUILD_A124
                                                                               ,
                                                                               0
#endif
                                                                               )) {
            locked = false;
            break;
        }
        part = static_cast<SelectablePart *>(part->GetNext());
    }
    list.DeleteAllElements();
    if (backroom && !locked) {
        locked = true;
        if (TheFEMarkerManager.GetNumMarkers(FEMarkerManager::MARKER_RIMS, 0) > 0) {
            locked = false;
        }
    }
    return locked;
}

bool CarCustomizeManager::IsVinylCategoryLocked(unsigned int cat, bool backroom) {
    unsigned int group = 0;
    switch (cat) {
        case 0x402:
            group = 0;
            break;
        case 0x403:
            group = 1;
            break;
        case 0x404:
            group = 2;
            break;
        case 0x405:
            group = 3;
            break;
        case 0x406:
            group = 4;
            break;
        case 0x407:
            group = 5;
            break;
        case 0x408:
            group = 6;
            break;
        case 0x409:
            group = 7;
            break;
    }
    bTList<SelectablePart> list;
    GetCarPartList(0x4d, list
#ifndef EA_BUILD_A124
                   ,
                   group
#endif
    );
    bool locked = true;
    SelectablePart *part = list.GetHead();
    while (part != list.EndOfList()) {
        if (part->GetPart()->GetGroupNumber() == group && !IsPartLocked(part
#ifndef EA_BUILD_A124
                                                                        ,
                                                                        0
#endif
                                                                        )) {
            locked = false;
            break;
        }
        part = static_cast<SelectablePart *>(part->GetNext());
    }
    list.DeleteAllElements();
    if (backroom && !locked) {
        locked = true;
        if (TheFEMarkerManager.GetNumMarkers(FEMarkerManager::MARKER_VINYL, 0) > 0) {
            locked = false;
        }
    }
    return locked;
}

int CarCustomizeManager::GetMinInnerRadius() {
    CarTypeInfo *GetCarTypeInfo(CarType car_type);
    CarTypeInfo *info = GetCarTypeInfo(TuningCar->GetType());
    if (info) {
        return info->GetWheelInnerRadiusMin();
    }
    return 0;
}

int CarCustomizeManager::GetMaxInnerRadius() {
    CarTypeInfo *GetCarTypeInfo(CarType car_type);
    CarTypeInfo *info = GetCarTypeInfo(TuningCar->GetType());
    if (info) {
        return info->GetWheelInnerRadiusMax();
    }
    return 0;
}

void CarCustomizeManager::GetCarPartList(int car_slot, bTList<SelectablePart> &the_list
#ifndef EA_BUILD_A124
                                         ,
                                         unsigned int param
#endif
) {
    CarType car_type;
    if (gCarCustomizeManager.TuningCar) {
        car_type = gCarCustomizeManager.TuningCar->GetType();
    } else {
        car_type = static_cast<CarType>(-1);
    }
    CarPart *part = CarPartDB.NewGetFirstCarPart(car_type, car_slot, 0, -1);
#ifndef EA_BUILD_A124
    eUnlockableEntity unlock_ent = MapCarPartToUnlockable(car_slot, nullptr);
#endif
    while (part) {
        SelectablePart *sel_part;
        unsigned int status;
        switch (car_slot) {
            case 0x17: {
                int lod = 0;
                bool has_solid = false;
                int name_hash = part->GetModelNameHash(lod, 1);
                if (name_hash && StreamingSolidPackLoader.GetStreamingEntry(name_hash)) {
                    has_solid = true;
                }
                if (!has_solid) {
                    part = CarPartDB.NewGetNextCarPart(part, car_type, car_slot, 0, -1);
                    continue;
                }
                break;
            }
#ifdef EA_BUILD_A124
        }
        // A124 (oraculo GetCarPartList 0x5c2da8, banco 100 %): sin
        // MapCarPartToUnlockable, sin caso 0x42, sin vinilos del 0x4d y sin
        // niveles de desbloqueo. En backroom filtra IsCarPartUnlocked; fuera
        // de backroom descartan IsCareerMode con la marca de perfil a
        // estrenar (palabra en UserProfile+0x7498 en la epoca) y el nivel
        // maximo. Transcripcion desde el oraculo como SetupMilestones.
        if (CustomizeIsInBackRoom()) {
            if (!UnlockSystem::IsCarPartUnlocked(UNLOCK_CAREER_MODE, car_slot, part, 0, true)) {
                part = CarPartDB.NewGetNextCarPart(part, car_type, car_slot, 0, -1);
                continue;
            }
        } else if (IsCareerMode() && !FEDatabase->GetUserProfile(0)->CareerModeHasBeenCompletedAtLeastOnce &&
                   part->GetUpgradeLevel() == Physics::Upgrades::PUT_MAX) {
            part = CarPartDB.NewGetNextCarPart(part, car_type, car_slot, 0, -1);
            continue;
        }
        sel_part = new ("SelectablePart 12", 0) SelectablePart(part, car_slot, part->GetUpgradeLevel(), Physics::Upgrades::PUT_MAX, false, CPS_AVAILABLE, 0, false);
#else
            case 0x42:
                if (param != 0) {
                    if (part->GetBrandNameHash() != param) {
                        part = CarPartDB.NewGetNextCarPart(part, car_type, car_slot, 0, -1);
                        continue;
                    }
                }
                break;
            case 0x4d: {
                bool exists = true;
                bool is_collectors;
                if (!StreamingTexturePackLoader.GetStreamingEntry(GetVinylLayerHash(part, car_type, 1))) {
                    exists = false;
                }
                is_collectors = part->GetBrandNameHash() == bStringHash("CEO");
                if (!exists) {
                    goto skip_vinyl;
                }
#ifndef EA_BUILD_A124
                if ((part->GetGroupNumber() & 0x1f) != param) {
                    goto skip_vinyl;
                }
#endif
                if (!is_collectors || GetIsCollectorsEdition()) {
                    break;
                }
            skip_vinyl:
                part = CarPartDB.NewGetNextCarPart(part, car_type, car_slot, 0, -1);
                continue;
            }
        }

        switch (unlock_ent) {
            case 0x2c:
            case 0x2e:
            case 0x30: {
                int level = 0;
                switch (unlock_ent) {
                    case 0x2c:
                        level = 1;
                        break;
                    case 0x2e:
                        level = 2;
                        break;
                    case 0x30:
                        level = 3;
                        break;
                }
                if (CustomizeIsInBackRoom() && !UnlockSystem::IsUnlockableUnlocked(UNLOCK_CAREER_MODE, unlock_ent, level, 0, true)) {
                    part = CarPartDB.NewGetNextCarPart(part, car_type, car_slot, 0, -1);
                    continue;
                }
                sel_part = new ("SelectablePart 12", 0) SelectablePart(part, car_slot, level, Physics::Upgrades::PUT_MAX, false, CPS_AVAILABLE, 0, false);
                break;
            }
            default:
                if (CustomizeIsInBackRoom()) {
                    if (!UnlockSystem::IsCarPartUnlocked(UNLOCK_CAREER_MODE, car_slot, part, 0, true)) {
                        part = CarPartDB.NewGetNextCarPart(part, car_type, car_slot, 0, -1);
                        continue;
                    }
                } else if (!FEDatabase->GetCareerSettings()->HasBeatenCareer() && part->GetUpgradeLevel() == Physics::Upgrades::PUT_MAX) {
                    part = CarPartDB.NewGetNextCarPart(part, car_type, car_slot, 0, -1);
                    continue;
                }
                sel_part = new ("SelectablePart 12", 0) SelectablePart(part, car_slot, part->GetUpgradeLevel(), Physics::Upgrades::PUT_MAX, false, CPS_AVAILABLE, 0, false);
                break;
        }
#endif

        status = CPS_AVAILABLE;
        if (IsPartLocked(sel_part
#ifndef EA_BUILD_A124
                         ,
                         0
#endif
                         )) {
            status = CPS_LOCKED;
        } else if (IsPartNew(sel_part
#ifndef EA_BUILD_A124
                             ,
                             0
#endif
                             )) {
            status = CPS_NEW;
        }
        if (IsPartInstalled(sel_part)) {
            status = status | CPS_INSTALLED;
        } else if (IsPartInCart(sel_part)) {
            status = status | CPS_IN_CART;
        }
        sel_part->SetPartState(status);
        sel_part->SetPrice(gCarCustomizeManager.GetPartPrice(sel_part));
        the_list.AddTail(sel_part);
        part = CarPartDB.NewGetNextCarPart(part, car_type, car_slot, 0, -1);
    }
}

void CarCustomizeManager::GetPerformancePartsList(Physics::Upgrades::Type type, bTList<SelectablePart> &the_list) {
    int num_pkgs = Physics::Upgrades::GetMaxLevel(ThePVehicle, type);
    for (int i = 0; i < num_pkgs; i++) {
        SelectablePart *pkg = new ("SelectablePart 12", 0) SelectablePart(nullptr, 0, i + 1, type, true, CPS_AVAILABLE, 0, false);
#ifndef EA_BUILD_A124
        int unlock_level = GetMaxPackages(type) - GetNumPackages(type) + i + 1;
#endif
        unsigned int status = CPS_AVAILABLE;
        if (IsPartLocked(pkg
#ifndef EA_BUILD_A124
                         ,
                         unlock_level
#endif
                         )) {
            status = CPS_LOCKED;
        } else if (IsPartNew(pkg
#ifndef EA_BUILD_A124
                             ,
                             unlock_level
#endif
                             )) {
            status = CPS_NEW;
        }
        if (IsPartInstalled(pkg)) {
            status = status | CPS_INSTALLED;
        } else if (IsPartInCart(pkg)) {
            status = status | CPS_IN_CART;
        }
        pkg->SetPartState(status);
        pkg->SetPrice(gCarCustomizeManager.GetPartPrice(pkg));
        the_list.AddTail(pkg);
    }
}

bool CarCustomizeManager::CanInstallJunkman(Physics::Upgrades::Type type) {
    return Physics::Upgrades::CanInstallJunkman(ThePVehicle, type);
}

bool CarCustomizeManager::IsCareerMode() {
    return FEDatabase->IsCareerMode() || g_bTestCareerCustomization;
}

bool CarCustomizeManager::IsTurbo() {
    Attrib::Gen::pvehicle pveh(TuningCar->VehicleKey, 0, nullptr);
    Physics::Upgrades::SetLevel(pveh, Physics::Upgrades::PUT_INDUCTION, 1);
    return Physics::Info::InductionType(pveh) == 1;
}

float CarCustomizeManager::GetActualHeat() {
    FEPlayerCarDB *stable;
    FECareerRecord *record;
    if (!TuningCar) {
        return -1.0f;
    }
    stable = FEDatabase->GetPlayerCarStable(0);
    record = stable->GetCareerRecordByHandle(TuningCar->CareerHandle);
    if (record) {
        return record->GetVehicleHeat();
    }
    return -1.0f;
}

float CarCustomizeManager::GetPreviewHeat(SelectablePart *part) {
    if (!TuningCar) {
        return 0.0f;
    }
    FECareerRecord temp_record;
    FECareerRecord *career_record = FEDatabase->GetPlayerCarStable(0)->GetCareerRecordByHandle(TuningCar->CareerHandle);
    if (!career_record)
        return -1.0f;
    float heat = career_record->GetVehicleHeat();
    temp_record.SetVehicleHeat(heat);
    if (part && part->GetPart() != GetInstalledCarPart(part->GetSlotID())) {
        UpdateHeatOnVehicle(part, &temp_record);
    }
    ShoppingCartItem *item = ShoppingCart.GetHead();
    while (item != ShoppingCart.EndOfList()) {
        if (part) {
            if (part->GetSlotID() != item->GetBuyingPart()->GetSlotID() && item->IsActive()) {
                UpdateHeatOnVehicle(item->GetBuyingPart(), &temp_record);
            }
        } else if (item->IsActive()) {
            UpdateHeatOnVehicle(item->GetBuyingPart(), &temp_record);
        }
        item = static_cast<ShoppingCartItem *>(item->GetNext());
    }
    return temp_record.GetVehicleHeat();
}

int CarCustomizeManager::GetNumCustomizeMarkers() {
    if (g_bTestCareerCustomization) {
        return 1;
    }
    return TheFEMarkerManager.GetNumCustomizeMarkers();
}

bool CarCustomizeManager::IsCastrolCar() {
    if (TuningCar->GetType() == 0x34) {
#ifdef EA_BUILD_A124
        return true;
#else
        return gEasterEggs.IsEasterEggUnlocked(EASTER_EGG_CASTROL);
#endif
    }
    return false;
}

bool CarCustomizeManager::IsRotaryCar() {
    int type = TuningCar->GetType();
    if (type == 0x05 || type == 0x38) {
        return true;
    }
    return false;
}

bool CarCustomizeManager::IsHeroCar() {
    return TuningCar->GetType() == 0x29;
}

float CarCustomizeManager::GetCartHeat() {
    if (DoesCartHaveActiveParts()) {
        if (IsCareerMode()) {
            FECareerRecord temp_record;
            FECareerRecord *career_record = FEDatabase->GetPlayerCarStable(0)->GetCareerRecordByHandle(TuningCar->CareerHandle);
            if (!career_record)
                return -1.0f;
            float heat = career_record->GetVehicleHeat();
            temp_record.SetVehicleHeat(heat);
            ShoppingCartItem *item = ShoppingCart.GetHead();
            while (item != ShoppingCart.EndOfList()) {
                if (item->IsActive()) {
                    UpdateHeatOnVehicle(item->GetBuyingPart(), &temp_record);
                }
                item = static_cast<ShoppingCartItem *>(item->GetNext());
            }
            return temp_record.GetVehicleHeat();
        }
    }
    return GetActualHeat();
}
