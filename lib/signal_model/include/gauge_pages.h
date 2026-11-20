// M5Dial の複数ページ（数値を並べるだけのページ）の定義
//
// HW 非依存（DEC-06）。どの信号をどのページに、どの名前・単位・小数桁で出すかだけを決める。
// 描画は src/main.cpp（M5GFX）が行う。ページの入れ替え・追加はこの表を書き換える。
//
// 文字は ASCII のみ（M5GFX の内蔵フォントに合わせる）。
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "signal_store.h"

namespace cm {

constexpr size_t kGaugeMaxItems = 4;  ///< 1 ページに並べる項目の上限（240 px の円に読める大きさで入る数）

struct GaugeItem {
    SignalId id;
    const char* label;  ///< 項目名（左側に小さく出す）
    const char* unit;   ///< 単位（値の右に小さく出す）
    uint8_t decimals;   ///< 小数の桁数
};

struct GaugePage {
    const char* title;
    uint8_t count;
    GaugeItem items[kGaugeMaxItems];
};

// 過渡を追う必要が薄い項目を、内容の近いものどうしでまとめる
inline constexpr GaugePage kGaugePages[] = {
    {"TEMP / PRESS",
     4,
     {{SignalId::Iat, "IAT", "C", 0},
      {SignalId::Clt, "CLT", "C", 0},
      {SignalId::Map, "MAP", "kPa", 1},
      {SignalId::Baro, "BARO", "kPa", 1}}},
    {"IGN / INJ",
     4,
     {{SignalId::IgnitionTiming, "ADV", "deg", 1},
      {SignalId::DwellMs, "DWELL", "ms", 2},
      {SignalId::InjDuty, "INJ", "%", 1},
      {SignalId::InjTiming, "INJ T", "deg", 0}}},
    {"IDLE",
     4,
     {{SignalId::IdlePosition, "ISCV", "%", 1},
      {SignalId::Iat, "IAT", "C", 0},
      {SignalId::BattVolt, "BATT", "V", 1},
      {SignalId::Clt, "CLT", "C", 0}}},
    {"FUEL",
     4,
     {{SignalId::Iat, "IAT", "C", 0},
      {SignalId::InjPulseMs, "PW", "ms", 2},
      {SignalId::FuelEcoInstant, "INST", "km/L", 1},
      {SignalId::FuelEcoAvg, "AVG", "km/L", 1}}},
};

constexpr int kGaugePageCount = static_cast<int>(sizeof(kGaugePages) / sizeof(kGaugePages[0]));

/// 項目の値を文字列にする。
/// 信号が Lost のときは "--" を書いて false を返す（古い値を出さない。RSK-01）。
inline bool formatGaugeValue(char* buf, size_t size, const Snapshot& snap, const GaugeItem& item) {
    float v = 0.0f;
    if (!snap.get(item.id, v)) {
        snprintf(buf, size, "--");
        return false;
    }
    snprintf(buf, size, "%.*f", static_cast<int>(item.decimals), static_cast<double>(v));
    return true;
}

/// ページ番号を delta だけ進め、0..count-1 に巻き戻す（delta は負でもよい）。
constexpr int wrapPage(int page, int delta, int count) {
    return (count <= 0) ? 0 : (((page + delta) % count) + count) % count;
}

}  // namespace cm
