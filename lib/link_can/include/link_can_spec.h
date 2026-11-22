// SWR-103: Link G4X の Generic Dash 形式のフレーム定義とスケーリング定数
//
// 一次情報源: Link G4X の車両（junna6ce）の PCLink（G4X）の CAN Setup → Streams → Stream 1（2026-11-22 確認）。
// 仕様の詳細は docs/14_ICD_link_can.md を参照。PCLink の Stream 1 を変えたら本ヘッダと DOC-14 を同時に更新する。
// スケーリング値をコード中に直書きしないこと（SWD-01 と同じ規則）。
//
// PCLink は「生値 = 表示値 × Multiplier ÷ Divider + Offset」を送る。本機は
// 「表示値 = (生値 − Offset) × kXxxScale」で戻す（kXxxScale = Divider ÷ Multiplier）。
#pragma once

#include <cstdint>

namespace cm::link {

// ---------------------------------------------------------------- フレーム
/// PCLink の Channel の CAN ID の既定値（1000 = 0x3E8）。USER_LINK_CAN_ID で変更可能（SWR-104）
constexpr uint32_t kDefaultCanId = 1000;
constexpr uint32_t kMaxStdId     = 0x7FF;

constexpr uint8_t kFrameDlc     = 8;
constexpr uint8_t kOffFrameNo   = 0;  ///< 0 バイト目がフレーム番号（PCLink の ID Position = 0）
constexpr uint8_t kOffA         = 2;  ///< 値 A（uint16 LE）
constexpr uint8_t kOffB         = 4;  ///< 値 B
constexpr uint8_t kOffC         = 6;  ///< 値 C

/// フレーム番号（PCLink の Frame の ID (decimal)。Frame 1 が 0）
enum FrameNo : uint8_t {
    kFrmRpmMap       = 0,   ///< Engine Speed / MAP / MGP
    kFrmBapTpsDuty   = 1,   ///< BAP / TPS / Injector Duty
    kFrmPwEct        = 2,   ///< Inj Duty (Sec) / Injection Actual PW / ECT
    kFrmIatBatt      = 3,   ///< IAT / Batt Voltage / MAF
    kFrmGearTiming   = 4,   ///< Gear / Injection Timing / Ignition Angle
    kFrmLambda       = 6,   ///< Exhaust Bank 2 Position / Lambda 1 / Lambda 2
    kFrmFault        = 7,   ///< Trig1 Err / Fault Codes / Fuel Pressure
    kFrmOilLf        = 8,   ///< Oil Temp / Oil Pressure / LF Wheel Speed
    kFrmWheels       = 9,   ///< LR / RF / RR Wheel Speed
    kFrmExtraEgt     = 14,  ///< 本機が定義する追加フレーム: EGT1 / EGT2 / Fuel Level（DOC-14 §3.1）
    kFrmExtraFuel    = 15,  ///< 本機が定義する追加フレーム: Fuel Temp / Dwell / ISC
    kFrameNoCount    = 16   ///< 0..15 を受理する
};

// ---------------------------------------------------------------- スケーリング
constexpr float kTempOffsetC        = 50.0f;     ///< ECT / IAT / Oil Temp / Fuel Temp の Offset
constexpr float kBaroScaleKpa       = 0.1f;      ///< BAP ×10
constexpr float kDutyScalePct       = 0.1f;      ///< Injector Duty ×10
constexpr float kInjPwScaleMs       = 0.001f;    ///< Injection Actual PW ×1000
constexpr float kBattScaleV         = 0.01f;     ///< Batt Voltage ×100
constexpr float kIgnOffsetRaw       = 1000.0f;   ///< Ignition Angle の Offset
constexpr float kIgnScaleDeg        = 0.1f;      ///< Ignition Angle ×10
constexpr float kLambdaScale        = 0.001f;    ///< Lambda ×1000
constexpr float kWheelSpeedScaleKmh = 0.1f;      ///< Wheel Speed ×10
constexpr float kFuelLevelScalePct  = 0.1f;      ///< Fuel Level ×10（追加フレーム）
constexpr float kDwellScaleMs       = 0.01f;     ///< Ignition Dwell ×100（追加フレーム）
constexpr float kIscScalePct        = 0.1f;      ///< ISC Duty ×10（追加フレーム）
constexpr float kBaroMinValidKpa    = 50.0f;     ///< 0 は「読めない」。rusEFI のときと同じ下限

constexpr uint16_t le16(const uint8_t* d, uint8_t off) {
    return static_cast<uint16_t>(d[off] | (static_cast<uint16_t>(d[off + 1]) << 8));
}

}  // namespace cm::link
