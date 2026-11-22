// SWR-106: 走行距離（オドメーター・トリップメーター）の積算と保存の判定（HW 非依存）
//
// Link G4X は距離を送らないので、車速を時間で積算する（SYS-64）。
// 保存（NVS への書き込み）そのものは HAL 側が行い、本クラスは「いつ保存するか」だけを決める。
#pragma once

#include <cstdint>

namespace cm {

class DistanceMeter {
public:
    static constexpr uint32_t kMaxStepMs       = 1000;  ///< 1 回に積算する最大の時間（停止後の跳ね防止）
    static constexpr uint32_t kSaveEveryM      = 500;   ///< 未保存がこれに達したら保存する
    static constexpr float kStoppedKmh         = 1.0f;  ///< これ未満を停車とみなす
    static constexpr uint32_t kStopSaveAfterMs = 2000;  ///< 停車がこれだけ続いたら保存する

    /// 起動時に保存値を入れる（積算の途中の端数と未保存の量は 0 に戻す）。トリップは起動時に 0 を渡す
    void restore(uint32_t odoM, uint32_t tripM) {
        m_odoM     = odoM;
        m_tripM    = tripM;
        m_fracM    = 0.0f;
        m_unsavedM = 0;
        m_stopMs   = 0;
    }

    /// 車速を積算する。hasSpeed = false（Lost）なら積算しない（RSK-01: 古い速度で距離を増やさない）
    void update(bool hasSpeed, float speedKmh, uint32_t dtMs) {
        if (dtMs > kMaxStepMs) {
            dtMs = kMaxStepMs;
        }
        if (!hasSpeed || speedKmh < 0.0f) {
            m_stopMs = 0;  // 車速が分からない間は「停車」と決めない
            return;
        }
        if (speedKmh < kStoppedKmh) {
            m_stopMs += dtMs;
            return;
        }
        m_stopMs = 0;
        m_fracM += speedKmh / 3.6f * static_cast<float>(dtMs) * 0.001f;
        if (m_fracM >= 1.0f) {
            const uint32_t whole = static_cast<uint32_t>(m_fracM);
            m_fracM -= static_cast<float>(whole);
            m_odoM += whole;
            m_tripM += whole;
            m_unsavedM += whole;
        }
    }

    /// トリップを 0 に戻す。トリップは保存しない（電源を入れるたびに 0 から。SWR-106）ので保存の判定は変えない
    void resetTrip() { m_tripM = 0; }

    /// オドの初期値（USER_ODO_START_KM）を、前回反映した値と違うときだけ入れる。入れたら true
    bool applySeed(uint32_t seedKm, uint32_t lastAppliedSeedKm) {
        if (seedKm == lastAppliedSeedKm) {
            return false;
        }
        m_odoM  = seedKm * 1000u;
        m_fracM = 0.0f;
        m_dirty = true;
        return true;
    }

    /// 今保存すべきか（SWR-106: 未保存 500 m、または停車 2 秒で未保存あり、または値を直接変えた）
    bool shouldSave() const {
        if (m_dirty) {
            return true;
        }
        if (m_unsavedM >= kSaveEveryM) {
            return true;
        }
        return m_unsavedM > 0 && m_stopMs >= kStopSaveAfterMs;
    }

    /// HAL が保存し終えたら呼ぶ
    void markSaved() {
        m_unsavedM = 0;
        m_dirty    = false;
    }

    uint32_t odoM() const { return m_odoM; }
    uint32_t tripM() const { return m_tripM; }
    float odoKm() const { return static_cast<float>(m_odoM) * 0.001f; }
    float tripKm() const { return static_cast<float>(m_tripM) * 0.001f; }

private:
    uint32_t m_odoM     = 0;
    uint32_t m_tripM    = 0;
    float m_fracM       = 0.0f;  ///< 1 m 未満の端数（整数の距離に足すまで貯める）
    uint32_t m_unsavedM = 0;
    uint32_t m_stopMs   = 0;
    bool m_dirty        = false;
};

}  // namespace cm
