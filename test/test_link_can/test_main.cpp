// SWE.4 ユニット検証: Link G4X の Generic Dash デコーダ
// UT-26 (docs/30_test_strategy.md §2) / DOC-14
#include <unity.h>

#include "link_decoder.h"
#include "signal_id.h"

using namespace cm;
using namespace cm::link;

namespace {

constexpr uint32_t kId = kDefaultCanId;  // 1000

bool find(const DecodeResult& r, SignalId id, float& out) {
    for (uint8_t i = 0; i < r.count; ++i) {
        if (r.signals[i].id == id) {
            out = r.signals[i].value;
            return true;
        }
    }
    return false;
}

/// フレーム番号と 3 つの生値からフレームを作る（DOC-14 §2）
void frame(uint8_t* d, uint8_t no, uint16_t a, uint16_t b, uint16_t c) {
    d[0] = no;
    d[1] = 0;
    d[2] = static_cast<uint8_t>(a & 0xFF);
    d[3] = static_cast<uint8_t>(a >> 8);
    d[4] = static_cast<uint8_t>(b & 0xFF);
    d[5] = static_cast<uint8_t>(b >> 8);
    d[6] = static_cast<uint8_t>(c & 0xFF);
    d[7] = static_cast<uint8_t>(c >> 8);
}

void expect(const DecodeResult& r, SignalId id, float want, float tol = 1e-3f) {
    float v = 0.0f;
    TEST_ASSERT_TRUE_MESSAGE(find(r, id, v), "signal missing");
    TEST_ASSERT_FLOAT_WITHIN(tol, want, v);
}

}  // namespace

void setUp() {}
void tearDown() {}

// 受理の条件: ID・DLC・番号
void test_UT26_accept_conditions() {
    LinkDecoder dec;
    uint8_t d[8];
    frame(d, kFrmRpmMap, 3000, 100, 200);

    TEST_ASSERT_TRUE(dec.decode(kId, d, 8, kId).accepted);
    TEST_ASSERT_FALSE(dec.decode(kId + 1, d, 8, kId).accepted);  // ID 違い
    TEST_ASSERT_FALSE(dec.decode(kId, d, 7, kId).accepted);      // DLC 不足
    TEST_ASSERT_FALSE(dec.decode(kId, nullptr, 8, kId).accepted);

    frame(d, kFrameNoCount, 1, 2, 3);  // 番号 16
    TEST_ASSERT_FALSE(dec.decode(kId, d, 8, kId).accepted);

    frame(d, 5, 1, 2, 3);  // カム位置: 受理するが信号なし
    const DecodeResult r = dec.decode(kId, d, 8, kId);
    TEST_ASSERT_TRUE(r.accepted);
    TEST_ASSERT_EQUAL_UINT8(0, r.count);

    // ID は設定で変えられる
    frame(d, kFrmRpmMap, 3000, 100, 200);
    TEST_ASSERT_TRUE(dec.decode(0x500, d, 8, 0x500).accepted);
}

// DOC-14 §3 のスケーリング
void test_UT26_scaling() {
    LinkDecoder dec;
    uint8_t d[8];

    frame(d, kFrmRpmMap, 6500, 95, 195);
    DecodeResult r = dec.decode(kId, d, 8, kId);
    expect(r, SignalId::Rpm, 6500.0f);
    expect(r, SignalId::Map, 95.0f);

    frame(d, kFrmBapTpsDuty, 1013, 500, 456);
    r = dec.decode(kId, d, 8, kId);
    expect(r, SignalId::Baro, 101.3f);
    expect(r, SignalId::InjDuty, 45.6f);

    frame(d, kFrmPwEct, 0, 3250, 132);  // PW 3.25 ms、ECT 82 °C
    r = dec.decode(kId, d, 8, kId);
    expect(r, SignalId::InjPulseMs, 3.25f);
    expect(r, SignalId::Clt, 82.0f);

    frame(d, kFrmIatBatt, 40, 1385, 0);  // IAT -10 °C、13.85 V
    r = dec.decode(kId, d, 8, kId);
    expect(r, SignalId::Iat, -10.0f);
    expect(r, SignalId::BattVolt, 13.85f);

    frame(d, kFrmGearTiming, 3, 350, 1000 + 255);  // 点火 25.5 deg
    r = dec.decode(kId, d, 8, kId);
    expect(r, SignalId::Gear, 3.0f);
    expect(r, SignalId::InjTiming, 350.0f);
    expect(r, SignalId::IgnitionTiming, 25.5f);

    frame(d, kFrmGearTiming, 0, 0, 1000 - 50);  // 点火 -5 deg
    r = dec.decode(kId, d, 8, kId);
    expect(r, SignalId::IgnitionTiming, -5.0f);

    frame(d, kFrmOilLf, 140, 350, 0);
    r = dec.decode(kId, d, 8, kId);
    expect(r, SignalId::OilTemp, 90.0f);
    expect(r, SignalId::OilPressure, 350.0f);

    // 大気圧 0 は入れない
    frame(d, kFrmBapTpsDuty, 0, 0, 0);
    r = dec.decode(kId, d, 8, kId);
    float v = 0.0f;
    TEST_ASSERT_FALSE(find(r, SignalId::Baro, v));
}

// SWR-105: λ の有効・無効
void test_UT26_lambda() {
    LinkDecoder dec;
    uint8_t d[8];
    float v = 0.0f;

    frame(d, kFrmLambda, 0, 987, 1000);
    DecodeResult r = dec.decode(kId, d, 8, kId);
    expect(r, SignalId::WboLambda, 0.987f);
    expect(r, SignalId::WboValid, 1.0f);
    expect(r, SignalId::Lambda1, 0.987f);

    frame(d, kFrmLambda, 0, 0, 0);  // 0 = 無効
    r = dec.decode(kId, d, 8, kId);
    TEST_ASSERT_FALSE(find(r, SignalId::WboLambda, v));
    TEST_ASSERT_FALSE(find(r, SignalId::Lambda1, v));
    expect(r, SignalId::WboValid, 0.0f);
    expect(r, SignalId::Lambda1Valid, 0.0f);
}

// DOC-14 §4.2: Fault Codes -> CEL
void test_UT26_fault_to_cel() {
    LinkDecoder dec;
    uint8_t d[8];
    frame(d, kFrmFault, 0, 2, 300);
    DecodeResult r = dec.decode(kId, d, 8, kId);
    expect(r, SignalId::StatusFlags, static_cast<float>(status_bit::kCheckEngine));

    frame(d, kFrmFault, 0, 0, 300);
    r = dec.decode(kId, d, 8, kId);
    expect(r, SignalId::StatusFlags, 0.0f);
}

// DOC-14 §4.3: 車速は 4 輪の最大値
void test_UT26_vehicle_speed_max_wheel() {
    LinkDecoder dec;
    uint8_t d[8];

    frame(d, kFrmWheels, 0, 0, 0);
    DecodeResult r = dec.decode(kId, d, 8, kId);
    expect(r, SignalId::VehicleSpeed, 0.0f);

    // センサが LR に割り当てられている車両
    frame(d, kFrmWheels, 625, 0, 0);
    r = dec.decode(kId, d, 8, kId);
    expect(r, SignalId::VehicleSpeed, 62.5f);

    // LF は 0 のまま。番号 8 を受けても LR の値が車速
    frame(d, kFrmOilLf, 140, 300, 0);
    r = dec.decode(kId, d, 8, kId);
    expect(r, SignalId::VehicleSpeed, 62.5f);

    // LF の車両
    LinkDecoder dec2;
    frame(d, kFrmOilLf, 140, 300, 1003);
    r = dec2.decode(kId, d, 8, kId);
    expect(r, SignalId::VehicleSpeed, 100.3f);
}

// DOC-14 §3.1: 追加フレーム
void test_UT26_extra_frames() {
    LinkDecoder dec;
    uint8_t d[8];

    frame(d, kFrmExtraEgt, 850, 830, 555);
    DecodeResult r = dec.decode(kId, d, 8, kId);
    expect(r, SignalId::Egt1, 850.0f);
    expect(r, SignalId::Egt2, 830.0f);
    expect(r, SignalId::FuelLevel, 55.5f);

    frame(d, kFrmExtraFuel, 85, 320, 284);
    r = dec.decode(kId, d, 8, kId);
    expect(r, SignalId::FuelTemp, 35.0f);
    expect(r, SignalId::DwellMs, 3.2f);
    expect(r, SignalId::IdlePosition, 28.4f);

    // 追加フレームが無い間、ほかのフレームは EGT を出さない
    float v = 0.0f;
    for (uint8_t no = 0; no < 14; ++no) {
        frame(d, no, 100, 100, 100);
        r = dec.decode(kId, d, 8, kId);
        TEST_ASSERT_FALSE(find(r, SignalId::Egt1, v));
    }
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_UT26_accept_conditions);
    RUN_TEST(test_UT26_scaling);
    RUN_TEST(test_UT26_lambda);
    RUN_TEST(test_UT26_fault_to_cel);
    RUN_TEST(test_UT26_vehicle_speed_max_wheel);
    RUN_TEST(test_UT26_extra_frames);
    return UNITY_END();
}
