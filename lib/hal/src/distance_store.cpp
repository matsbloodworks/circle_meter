// SWR-106: 走行距離の保存（Arduino の Preferences = ESP-IDF の NVS。書き込みは NVS がウェアレベリングする）
#include "distance_store.h"

#include <Preferences.h>

namespace cm {

namespace {
// デモ版は別の領域を使う（SWR-106: シミュレータの距離を実車のオドに混ぜない）
#ifdef CM_ENABLE_CAN_SIM
constexpr const char* kNamespace = "cm_dist_sim";
#else
constexpr const char* kNamespace = "cm_dist";
#endif
constexpr const char* kKeyOdo  = "odo_m";
constexpr const char* kKeySeed = "seed_km";
}  // namespace

DistanceRecord DistanceStore::load() {
    DistanceRecord r;
    Preferences p;
    if (!p.begin(kNamespace, true)) {  // 読み取り専用。初回は領域が無いので失敗する -> 既定値
        return r;
    }
    r.odoM   = p.getULong(kKeyOdo, r.odoM);
    r.seedKm = p.getULong(kKeySeed, r.seedKm);
    p.end();
    return r;
}

bool DistanceStore::save(const DistanceRecord& r) {
    Preferences p;
    if (!p.begin(kNamespace, false)) {
        return false;
    }
    // 値が同じキーは NVS が書き込みを省く（無駄な書き込みを増やさない）
    const bool ok = p.putULong(kKeyOdo, r.odoM) == sizeof(uint32_t) &&
                    p.putULong(kKeySeed, r.seedKm) == sizeof(uint32_t);
    p.end();
    return ok;
}

}  // namespace cm
