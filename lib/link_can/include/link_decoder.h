// SWR-103 / SWR-105: Link G4X の Generic Dash 形式のデコード（DOC-14）
//
// HW 非依存（DEC-06 / SWR-10）。結果の型は rusEFI のデコーダと共通にし、受信タスクの後段
// （ストアへの書き込み・無効値の除外）をそのまま使う。
// 車速（4 輪の最大値。DOC-14 §4.3）のために直近の車輪速度だけを状態として持つ。
// 受信タスクだけが呼ぶ（1 タスクからしか使わないので排他は不要）。
#pragma once

#include <cstdint>

#include "link_can_spec.h"
#include "rusefi_decoder.h"

namespace cm::link {

using DecodeResult = cm::rusefi::DecodeResult;

class LinkDecoder {
public:
    /// フレームをデコードする。
    /// ID が canId と違う、data が nullptr、DLC が 8 未満、番号が 16 以上なら accepted = false。
    /// 番号が 0–15 で本機が使わないものは accepted = true、count = 0。
    DecodeResult decode(uint32_t id, const uint8_t* data, uint8_t dlc, uint32_t canId);

private:
    enum Wheel : uint8_t { kLf = 0, kLr, kRf, kRr, kWheelCount };
    float m_wheelKmh[kWheelCount] = {0.0f, 0.0f, 0.0f, 0.0f};

    float maxWheelKmh() const;
};

}  // namespace cm::link
