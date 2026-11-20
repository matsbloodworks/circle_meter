// SWA-21: λ ページの全デザインで共通の表示規則（DOC-23 §3.3 / §4）
//
// 数値の書式・色と EGT の警告色を 1 か所に置き、デザインによって規則が食い違わないようにする。
#pragma once

#include <cstddef>
#include <cstdint>

#include "config.h"
#include "display_filter.h"
#include "display_text.h"  // formatSensorStatus（両機種で共有。SWR-68）

namespace cm::ui {

/// 主数値の文字列。AFR は小数 1 桁、λ は小数 3 桁、信号喪失なら "--"。
void formatMain(char* buf, size_t size, const DisplayValues& v, const Config& cfg);

/// 主数値の色。通常は白、LEAN_HEAVY は赤、Stale / Lost は灰。
uint32_t mainColorHex(const DisplayValues& v);

/// AFR値の色。13以上で黄色、14.5以上で赤。
uint32_t afrColorHex(const DisplayValues& v, const Config& cfg);

/// EGT の文字列（"845°C" / "--°C"）。
void formatEgt(char* buf, size_t size, const DisplayValues& v);

/// EGT の色。警告は黄、危険は赤の 2 Hz 点滅（SWR-46）、Stale / Lost は灰。
uint32_t egtColorHex(const DisplayValues& v, bool blinkOn);

/// formatSensorStatus の文字列の色。
uint32_t sensorStatusColorHex(const LambdaSensorInfo& info);

/// EGT 危険の警告帯を今このフレームで点灯させるか。
bool egtAlarmOn(const DisplayValues& v, bool blinkOn);

/// SPD値の色。80以上で黄色、120以上で赤。
uint32_t spdColorHex(const DisplayValues& v);

/// CLT値の色。60以下で青、100以上で赤。
uint32_t cltColorHex(const DisplayValues& v);

/// VB値の色。12以下で黄色、11.5以下で赤。
uint32_t vbColorHex(const DisplayValues& v);

/// OIL_T値の色。105以上で黄色、115以上で赤。
uint32_t oilTempColorHex(const DisplayValues& v);

}  // namespace cm::ui
