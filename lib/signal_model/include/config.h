// SWA-06 / SWD-04: 設定構造体
//
// HW 非依存（DEC-06 / SWR-10）。NVS への保存は SWA-09 NvsStore が行う。
#pragma once

#include <cstdint>
#include "units.h"

namespace cm {

/// 構造体レイアウトを変えたら必ずインクリメントすること。
/// 不一致の保存データは既定値で上書きされる（SWR-81）。
constexpr uint16_t kConfigVersion = 3;  // 2: dialStyle を追加（2026-10-04）/ 3: ecuType・linkCanId（2026-11-22）

/// SYS-21: λ ページの表示デザイン。値を変えない（NVS に保存される）。追加は Count の直前へ。
enum class DialStyle : uint8_t { Ring = 0, NeedleA = 1, Count };

/// SYS-63 / SWR-104: 受信する ECU の種類。値を変えない。追加は Count の直前へ。
enum class EcuType : uint8_t { Rusefi = 0, Link = 1, Count };

struct Config {
    uint16_t version = kConfigVersion;

    // ---- 表示 ----
    bool showAfr = true;             ///< true=AFR, false=λ (SYS-14)
    float stoich = kStoichGasoline;  ///< 5.0 - 20.0
#ifdef USER_RING_LOW
    float ringLo = USER_RING_LOW;    ///< リング下端 λ (ユーザー設定)
#else
    float ringLo = 0.68f;            ///< リング下端 λ (AFR 10.0 相当)
#endif
#ifdef USER_RING_HIGH
    float ringHi = USER_RING_HIGH;   ///< リング上端 λ (ユーザー設定)
#else
    float ringHi = 1.36f;            ///< リング上端 λ (AFR 20.0 相当)
#endif

    LambdaZoneConfig zones{0.75f, 0.85f, 1.03f, 1.10f};  ///< DOC-23 §3.2
    EgtConfig egt{850, 920};                             ///< DOC-23 §4

    uint8_t brightness = 4;                                         ///< 1-5 (DOC-23 §10)
    bool buzzerEnabled = true;                                      ///< EGT 危険時のブザー
    uint8_t lastPage   = 0;                                         ///< SYS-35
    uint8_t dialStyle  = static_cast<uint8_t>(DialStyle::NeedleA);  ///< SYS-21。既定は指針式 A

    // ---- CAN ----
    uint16_t canBaseId      = 0x200;  ///< SYS-02
    uint16_t canBitrateKbps = 500;    ///< 250 / 500 / 1000
    bool canExtendedId      = false;  ///< rusefiVerbose29b
    uint8_t ecuType         = static_cast<uint8_t>(EcuType::Rusefi);  ///< SWR-104
    uint16_t linkCanId      = 1000;   ///< SWR-104: Link G4X の Generic Dash の ID（DOC-14 §1）

    uint32_t crc32 = 0;  ///< 上記全フィールドのチェックサム（本フィールドは対象外）
};

Config defaultConfig();
uint32_t computeCrc(const Config& c);

/// 範囲・単調性・CRC を検証する。
/// @param checkCrc false にすると CRC を検査しない（メニューでの編集中の検証に使う）
bool validate(const Config& c, bool checkCrc = true);

/// ゾーン境界とリング範囲の単調性のみを検証する（SWD-04）。
///   ringLo < richHeavyMax < richMax < optimalMax < leanMax < ringHi
bool validateMonotonic(const Config& c);

}  // namespace cm
