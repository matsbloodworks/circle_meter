// SWA-03 / SWD-01 実装
#include "rusefi_decoder.h"

namespace cm::rusefi {
namespace {

inline void push(DecodeResult& r, SignalId id, float v) {
    if (r.count < kMaxSignalsPerFrame) {
        r.signals[r.count].id    = id;
        r.signals[r.count].value = v;
        ++r.count;
    }
}

/// uint8 の温度信号（-40 オフセット）
inline float temp8(uint8_t raw) {
    return static_cast<float>(raw) + kTempOffsetC;
}

void decodeStatus(DecodeResult& r, const uint8_t* d) {
    // DOC-13 §3.6: byte4 にビットフラグが並ぶ。ビット位置は signal_id.h の status_bit と一致させる。
    push(r, SignalId::StatusFlags, static_cast<float>(d[4]));
    push(r, SignalId::Gear, static_cast<float>(d[5]));
}

void decodeSpeeds(DecodeResult& r, const uint8_t* d) {
    push(r, SignalId::Rpm, static_cast<float>(le16(d, 0)));
    push(r, SignalId::IgnitionTiming, static_cast<float>(le16s(d, 2)) * kTimingScaleDeg);
    push(r, SignalId::InjDuty, static_cast<float>(d[4]) * kDutyScalePct);
    push(r, SignalId::IgnDuty, static_cast<float>(d[5]) * kDutyScalePct);
    push(r, SignalId::VehicleSpeed, static_cast<float>(d[6]));
}

void decodeSensors1(DecodeResult& r, const uint8_t* d) {
    push(r, SignalId::Map, static_cast<float>(le16(d, 0)) * kMapScaleKpa);
    push(r, SignalId::Clt, temp8(d[2]));
    push(r, SignalId::Iat, temp8(d[3]));
    push(r, SignalId::FuelLevel, static_cast<float>(d[7]) * kFuelLevelScale);
}

void decodeSensors2(DecodeResult& r, const uint8_t* d) {
    // byte0-1 はパディング（DOC-13 §3.5）
    push(r, SignalId::OilPressure, static_cast<float>(le16(d, 2)) * kOilPressScaleKpa);
    push(r, SignalId::OilTemp, temp8(d[4]));
    push(r, SignalId::FuelTemp, temp8(d[5]));
    push(r, SignalId::BattVolt, static_cast<float>(le16(d, 6)) * kBattScaleV);
}

void decodeFueling(DecodeResult& r, const uint8_t* d) {
    push(r, SignalId::InjPulseMs, static_cast<float>(le16s(d, kFuelingOffPulse)) * kInjPulseScaleMs);
}

void decodeFueling3(DecodeResult& r, const uint8_t* d) {
    const float lambda1 = static_cast<float>(le16(d, 0)) * kLambdaScale;
    push(r, SignalId::Lambda1, lambda1);
    // SWR-29: rusEFI はセンサが無効のとき λ = 0 を送る（DOC-13
    // §3.1.1）。「届いたが無効」をフレームごとに記録し、 「届いていない（NO SIGNAL）」と区別する。λ
    // そのものは無効値なので受信側でストアに入れない（SYS-42）
    push(r, SignalId::Lambda1Valid, isLambdaValid(lambda1) ? 1.0f : 0.0f);
    push(r, SignalId::Lambda2, static_cast<float>(le16(d, 2)) * kLambdaScale);
}

void decodeEgts(DecodeResult& r, const uint8_t* d) {
    // rusEFI 本体が送るのは Egt1/Egt2 のみ。Egt3-8 は常に 0 のため読まない。
    push(r, SignalId::Egt1, static_cast<float>(d[0]) * kEgtScaleC);
    push(r, SignalId::Egt2, static_cast<float>(d[1]) * kEgtScaleC);
}

/// BASE+12: ECU の Lua が送る追加データ。識別子が違えば何も取り出さない（呼び出し側で判定済み）
void decodeUserExtra(DecodeResult& r, const uint8_t* d) {
    push(r, SignalId::FuelEcoInstant, static_cast<float>(le16(d, 0)) * kFuelEcoScaleKml);
    push(r, SignalId::FuelEcoAvg, static_cast<float>(le16(d, 2)) * kFuelEcoScaleKml);
    push(r, SignalId::IdlePosition, static_cast<float>(d[4]) * kIdlePosScalePct);
    // 大気圧 0 は「読めなかった」。入れなければ鮮度が上がらず、画面は "--" になる
    const float baroKpa = static_cast<float>(d[5]) * kBaroScaleKpa;
    if (baroKpa >= kBaroMinValidKpa) {
        push(r, SignalId::Baro, baroKpa);
    }
    push(r, SignalId::DwellMs, static_cast<float>(d[6]) * kDwellScaleMs);
}

}  // namespace

DecodeResult decodeFrame(uint32_t id, const uint8_t* data, uint8_t dlc, uint32_t baseId) {
    DecodeResult r;

    if (data == nullptr || dlc < kFrameDlc) {
        return r;  // SWR-06: 不正フレームは破棄
    }
    if (id < baseId || id >= baseId + kOffCount) {
        return r;  // SWR-04: ベース範囲外は無視
    }

    const uint8_t off = static_cast<uint8_t>(id - baseId);
    switch (off) {
        case kOffStatus:
            decodeStatus(r, data);
            break;
        case kOffSpeeds:
            decodeSpeeds(r, data);
            break;
        case kOffSensors1:
            decodeSensors1(r, data);
            break;
        case kOffSensors2:
            decodeSensors2(r, data);
            break;
        case kOffFueling:
            decodeFueling(r, data);
            break;
        case kOffFueling3:
            decodeFueling3(r, data);
            break;
        case kOffEgts:
            decodeEgts(r, data);
            break;
        case kOffUserExtra:
            if (data[kUserExtraOffMagic] != kUserExtraMagic) {
                return r;  // 別の機器のフレーム。受理しない
            }
            decodeUserExtra(r, data);
            break;
        case kOffUserExtra2:
            if (data[kUserExtraOffMagic] != kUserExtra2Magic) {
                return r;  // 別の機器のフレーム。受理しない
            }
            push(r, SignalId::InjTiming, static_cast<float>(le16s(data, 0)) * kInjTimingScaleDeg);
            break;
        default:
            // 範囲内だが本機が使わないフレーム（PedalTps / Fueling / Cams / Knock / Status11）。
            // 「受理したが取り出す信号はない」として accepted=true, count=0 を返す。
            // これにより診断の unknownId カウンタが無用に増えない。
            break;
    }

    r.accepted = true;
    return r;
}

DecodeResult decodeWboFrame(uint32_t id, const uint8_t* data, uint8_t dlc, uint8_t wboIndex) {
    DecodeResult r;
    if (data == nullptr || dlc < kFrameDlc) {
        return r;
    }
    const uint32_t dataId = kWboDataBaseId + 2u * wboIndex;
    if (id == dataId) {
        // StandardData。Version が違う WBO は形式が変わっている可能性があるので読まない
        if (data[kWboOffVersion] != kWboVersion) {
            return r;
        }
        r.accepted = true;
        push(r, SignalId::WboValid, (data[kWboOffValid] & 0x01u) ? 1.0f : 0.0f);
        push(r, SignalId::WboTempC, static_cast<float>(le16(data, kWboOffTempC)) * kWboTempScaleC);
        // SYS-03 / DEC-11: λ は WBO から取る。Valid = 0 の λ は 0 が入っているだけなので採用しない（SYS-42
        // の範囲も確かめる）
        const float lambda = static_cast<float>(le16(data, kWboOffLambda)) * kLambdaScale;
        if ((data[kWboOffValid] & 0x01u) != 0 && isLambdaValid(lambda)) {
            push(r, SignalId::WboLambda, lambda);
        }
    } else if (id == dataId + 1u) {
        // DiagData。Version を持たないので、状態の値の範囲だけ確かめる
        const uint8_t status = data[kWboOffStatus];
        if (status > static_cast<uint8_t>(WboStatus::SensorUnderheat)) {
            return r;
        }
        r.accepted = true;
        push(r, SignalId::WboStatus, static_cast<float>(status));
    }
    return r;
}

}  // namespace cm::rusefi
