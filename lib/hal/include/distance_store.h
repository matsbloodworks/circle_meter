// SWR-106: オドメーターをフラッシュ（NVS）に保存・復元する（ESP32 依存）。トリップは保存しない
#pragma once

#include <cstdint>

namespace cm {

struct DistanceRecord {
    uint32_t odoM   = 0;
    uint32_t seedKm = 0xFFFFFFFFu;  ///< 前回反映した USER_ODO_START_KM（未反映は全ビット 1）
};

class DistanceStore {
public:
    /// 保存値を読む。無ければ既定値（0 km、初期値は未反映）
    DistanceRecord load();
    /// 保存する。失敗したら false
    bool save(const DistanceRecord& r);
};

}  // namespace cm
