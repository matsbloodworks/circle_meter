// SWR-103 / SWR-105 実装（DOC-14）
#include "link_decoder.h"

namespace cm::link {
namespace {

using cm::rusefi::kMaxSignalsPerFrame;

inline void push(DecodeResult& r, SignalId id, float v) {
    if (r.count < kMaxSignalsPerFrame) {
        r.signals[r.count].id    = id;
        r.signals[r.count].value = v;
        ++r.count;
    }
}

inline float raw(const uint8_t* d, uint8_t off) {
    return static_cast<float>(le16(d, off));
}

inline float temp(const uint8_t* d, uint8_t off) {
    return raw(d, off) - kTempOffsetC;
}

void decodeLambda(DecodeResult& r, const uint8_t* d) {
    // SWR-105: 表示は rusEFI のときと同じ WboLambda / WboValid の経路を使う。
    // Link はセンサの状態を送らないので、範囲外の λ は「無効」とだけ伝える（DOC-14 §4.1）
    const float lambda1 = raw(d, kOffB) * kLambdaScale;
    const bool valid    = cm::rusefi::isLambdaValid(lambda1);
    push(r, SignalId::WboValid, valid ? 1.0f : 0.0f);
    push(r, SignalId::Lambda1Valid, valid ? 1.0f : 0.0f);
    if (valid) {
        push(r, SignalId::WboLambda, lambda1);
        push(r, SignalId::Lambda1, lambda1);
    }
    // Lambda2 は受信タスクが範囲外を除外する（SYS-42）
    push(r, SignalId::Lambda2, raw(d, kOffC) * kLambdaScale);
}

}  // namespace

float LinkDecoder::maxWheelKmh() const {
    float m = m_wheelKmh[0];
    for (uint8_t i = 1; i < kWheelCount; ++i) {
        if (m_wheelKmh[i] > m) {
            m = m_wheelKmh[i];
        }
    }
    return m;
}

DecodeResult LinkDecoder::decode(uint32_t id, const uint8_t* d, uint8_t dlc, uint32_t canId) {
    DecodeResult r;
    if (d == nullptr || dlc < kFrameDlc || id != canId) {
        return r;
    }
    const uint8_t frameNo = d[kOffFrameNo];
    if (frameNo >= kFrameNoCount) {
        return r;  // 別の機器のフレームの可能性がある。受理しない
    }

    switch (frameNo) {
        case kFrmRpmMap:
            push(r, SignalId::Rpm, raw(d, kOffA));
            push(r, SignalId::Map, raw(d, kOffB));
            break;
        case kFrmBapTpsDuty: {
            const float baroKpa = raw(d, kOffA) * kBaroScaleKpa;
            if (baroKpa >= kBaroMinValidKpa) {
                push(r, SignalId::Baro, baroKpa);
            }
            push(r, SignalId::InjDuty, raw(d, kOffC) * kDutyScalePct);
            break;
        }
        case kFrmPwEct:
            push(r, SignalId::InjPulseMs, raw(d, kOffB) * kInjPwScaleMs);
            push(r, SignalId::Clt, temp(d, kOffC));
            break;
        case kFrmIatBatt:
            push(r, SignalId::Iat, temp(d, kOffA));
            push(r, SignalId::BattVolt, raw(d, kOffB) * kBattScaleV);
            break;
        case kFrmGearTiming:
            push(r, SignalId::Gear, raw(d, kOffA));
            push(r, SignalId::InjTiming, raw(d, kOffB));
            push(r, SignalId::IgnitionTiming, (raw(d, kOffC) - kIgnOffsetRaw) * kIgnScaleDeg);
            break;
        case kFrmLambda:
            decodeLambda(r, d);
            break;
        case kFrmFault: {
            // DOC-14 §4.2: Fault Codes が 0 以外なら CEL。ほかの状態ビットは Link からは取らない
            const uint32_t flags = (le16(d, kOffB) != 0) ? status_bit::kCheckEngine : 0u;
            push(r, SignalId::StatusFlags, static_cast<float>(flags));
            break;
        }
        case kFrmOilLf:
            push(r, SignalId::OilTemp, temp(d, kOffA));
            push(r, SignalId::OilPressure, raw(d, kOffB));
            m_wheelKmh[kLf] = raw(d, kOffC) * kWheelSpeedScaleKmh;
            push(r, SignalId::VehicleSpeed, maxWheelKmh());
            break;
        case kFrmWheels:
            m_wheelKmh[kLr] = raw(d, kOffA) * kWheelSpeedScaleKmh;
            m_wheelKmh[kRf] = raw(d, kOffB) * kWheelSpeedScaleKmh;
            m_wheelKmh[kRr] = raw(d, kOffC) * kWheelSpeedScaleKmh;
            push(r, SignalId::VehicleSpeed, maxWheelKmh());
            break;
        case kFrmExtraEgt:
            // EGT の 0 以下は受信タスクが除外する（RSK-09）
            push(r, SignalId::Egt1, raw(d, kOffA));
            push(r, SignalId::Egt2, raw(d, kOffB));
            push(r, SignalId::FuelLevel, raw(d, kOffC) * kFuelLevelScalePct);
            break;
        case kFrmExtraFuel:
            push(r, SignalId::FuelTemp, temp(d, kOffA));
            push(r, SignalId::DwellMs, raw(d, kOffB) * kDwellScaleMs);
            push(r, SignalId::IdlePosition, raw(d, kOffC) * kIscScalePct);
            break;
        default:
            // 番号 0–15 のうち本機が使わないもの（カム位置・ノック・APS など）
            break;
    }

    r.accepted = true;
    return r;
}

}  // namespace cm::link
