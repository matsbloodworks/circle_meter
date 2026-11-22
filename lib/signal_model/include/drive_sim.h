// SWR-100: 実走を模したデモ用の信号生成（エンジン始動 -> 暖機 -> 加速 -> 巡航 -> 減速 -> 停車。HW 非依存）
//
// SweepSim（λ の往復スイープ）は全ゾーンの見え方と描画負荷を見るためのもの。こちらは M面の各値が
// 実車でどう動くかを、CAN を繋がずに目で確かめるために使う。
//   - 回転・車速・スロットル・MAP・AFR はキーフレームの直線補間（kKeys）
//   - 水温・油温・吸気温・EGT は一次遅れで追従させる（ループしても冷えた状態に戻らない）
//   - 噴射時間・デューティ・点火時期・ISCV・油圧・電圧は上の値から求める
// 最後まで進むと暖機後のアイドル（kLoopStartMs）へ戻って走行を繰り返す。
// 値は見た目の確認用の作り物で、特定の車両のデータではない。
#pragma once

#include <cstddef>
#include <cstdint>

namespace cm {

struct DriveOutput {
    float rpm          = 0.0f;
    float speedKmh     = 0.0f;
    float tpsPct       = 0.0f;
    float mapKpa       = 0.0f;
    float baroKpa      = 0.0f;
    float lambda       = 0.0f;  ///< wboValid のときだけ意味がある
    bool wboValid      = false; ///< O2 センサのヒーターが温まるまで false（実機の WBO と同じ）
    float egtC         = 0.0f;
    float cltC         = 0.0f;
    float oilTempC     = 0.0f;
    float iatC         = 0.0f;
    float oilPressKpa  = 0.0f;  ///< ECU と同じく kPa
    float battV        = 0.0f;
    float injPulseMs   = 0.0f;
    float injDutyPct   = 0.0f;
    float ignAdvDeg    = 0.0f;
    float iscvPct      = 0.0f;
    bool engineRunning = false;
};

class DriveSim {
public:
    /// キーフレーム。時刻 tMs の値。間は直線補間する。rpm < kRunningRpm は「エンジン停止 / クランキング」
    struct Key {
        uint32_t tMs;
        float rpm;
        float speedKmh;
        float tpsPct;
        float mapKpa;
        float afr;
    };

    static constexpr float kBaroKpa       = 101.0f;
    static constexpr float kStoichAfr     = 14.7f;
    static constexpr float kRunningRpm    = 400.0f;    ///< これ以上なら始動済み
    static constexpr float kAmbientC      = 25.0f;     ///< 冷間始動の外気温
    static constexpr uint32_t kWboHeatMs  = 8000;      ///< 始動後、λ が有効になるまで（ヒーター加熱）
    static constexpr uint32_t kLoopStartMs = 20000;    ///< ループで戻る時刻（暖機後のアイドル）

    // clang-format off
    static constexpr Key kKeys[] = {
        //  tMs    rpm   km/h  TPS%  MAP   AFR
        {     0,    0,    0,    0, 101, 14.7f},  // キー ON（エンジン停止）
        {  2000,    0,    0,    0, 101, 14.7f},
        {  2100,  220,    0,    0,  92, 14.7f},  // クランキング
        {  4000,  260,    0,    0,  90, 14.7f},
        {  4600, 1900,    0,    3,  50, 12.8f},  // 始動の吹け上がり
        {  7000, 1350,    0,    0,  42, 13.2f},  // ファストアイドル（冷間増量）
        { 20000,  900,    0,    0,  36, 14.7f},  // 暖機後のアイドル  <- ループの戻り先
        { 22000,  900,    0,    0,  36, 14.7f},
        { 23500, 1600,    6,   18,  62, 13.6f},  // 発進
        { 27000, 6800,   48,  100, 100, 12.3f},  // 1 速 全開
        { 27400, 4300,   48,    0,  28, 16.5f},  // シフト
        { 27800, 4300,   49,  100, 100, 12.5f},
        { 31500, 7000,   88,  100, 100, 12.0f},  // 2 速 全開
        { 31900, 4900,   88,    0,  28, 16.5f},  // シフト
        { 32300, 4900,   89,  100, 100, 12.3f},
        { 37000, 7100,  128,  100, 100, 11.8f},  // 3 速 全開
        { 38000, 5600,  122,    0,  20, 20.0f},  // アクセルオフ（燃料カット）
        { 41000, 3100,  100,   22,  55, 14.7f},  // 4 速 巡航
        { 55000, 2900,   96,   18,  50, 14.7f},
        { 57000, 3200,  100,   40,  75, 13.8f},  // 追い越し気味に少し踏む
        { 60000, 2900,   96,   18,  50, 14.7f},
        { 62000, 2200,   70,    0,  22, 20.0f},  // 減速（燃料カット）
        { 66000, 1100,   15,    0,  24, 18.0f},
        { 68000,  900,    0,    0,  36, 14.7f},  // 停車・アイドル
        { 80000,  900,    0,    0,  36, 14.7f},
    };
    // clang-format on
    static constexpr size_t kKeyCount = sizeof(kKeys) / sizeof(kKeys[0]);
    static constexpr uint32_t kEndMs  = kKeys[kKeyCount - 1].tMs;

    /// シナリオの位置（0.0 = キー ON, 1.0 = 最後）へ飛ぶ。温度などの状態はそのまま
    void seek(float pos01) {
        const float p = (pos01 < 0.0f) ? 0.0f : (pos01 > 1.0f ? 1.0f : pos01);
        m_tMs         = static_cast<uint32_t>(p * static_cast<float>(kEndMs));
    }

    uint32_t timeMs() const { return m_tMs; }

    /// @param dtMs 前回からの経過時間。0 なら進めずに現在値を返す（停止中の再配信に使う）
    DriveOutput step(uint32_t dtMs) {
        m_tMs += dtMs;
        while (m_tMs > kEndMs) {
            m_tMs = kLoopStartMs + (m_tMs - kEndMs);
        }
        const Key k = keyAt(m_tMs);
        const float dtS = static_cast<float>(dtMs) * 0.001f;

        DriveOutput o;
        o.engineRunning = k.rpm >= kRunningRpm;
        o.speedKmh      = k.speedKmh;
        o.tpsPct        = k.tpsPct;
        o.mapKpa        = k.mapKpa;
        o.baroKpa       = kBaroKpa;
        o.rpm           = k.rpm;
        if (o.engineRunning && k.tpsPct < 2.0f && k.speedKmh < 1.0f) {
            // アイドルのわずかな揺れ（決定的な値にしてテストで再現できるようにする）
            o.rpm += 15.0f * wave(m_tMs, 700) + 8.0f * wave(m_tMs, 260);
        }

        const float load    = clamp01((k.mapKpa - 20.0f) / 80.0f);       // 0 = 燃料カット相当、1 = 全開
        const float rpmFrac = clamp01(k.rpm / 7000.0f);
        const bool fuelCut  = o.engineRunning && k.afr >= 19.0f;

        // --- 温度（一次遅れ）
        if (o.engineRunning) {
            m_runMs += dtMs;
            lag(m_cltC, 86.0f + 12.0f * load * rpmFrac, 22.0f, dtS);      // 全開が続くと 95 を超える
            lag(m_oilTC, 92.0f + 25.0f * load * rpmFrac, 30.0f, dtS);
            const float egtTarget = fuelCut ? 250.0f : 280.0f + 600.0f * load * (0.3f + 0.7f * rpmFrac);
            lag(m_egtC, egtTarget, fuelCut ? 1.5f : 2.5f, dtS);
        } else {
            lag(m_egtC, m_cltC, 30.0f, dtS);
        }
        // 吸気温: 止まっているとエンジンルームの熱で上がり、走ると外気温に近づく
        const float soak = (k.speedKmh < 10.0f && o.engineRunning) ? 20.0f : 6.0f * (1.0f - k.speedKmh / 130.0f);
        lag(m_iatC, kAmbientC + 5.0f + soak, 12.0f, dtS);
        o.cltC     = m_cltC;
        o.oilTempC = m_oilTC;
        o.iatC     = m_iatC;
        o.egtC     = m_egtC;

        // --- λ（WBO はヒーターが温まるまで無効）
        o.wboValid = o.engineRunning && m_runMs >= kWboHeatMs;
        o.lambda   = k.afr / kStoichAfr;

        // --- 噴射・点火・ISCV
        const float coldness = clamp01((80.0f - m_cltC) / 55.0f);  // 1 = 冷間（25 ℃）、0 = 暖機後
        if (!o.engineRunning) {
            o.injPulseMs = (k.rpm > 0.0f) ? 12.0f : 0.0f;              // クランキングは固定の始動噴射
            o.ignAdvDeg  = (k.rpm > 0.0f) ? 8.0f : 0.0f;
            o.iscvPct    = (k.rpm > 0.0f) ? 40.0f : 0.0f;
        } else if (fuelCut) {
            o.injPulseMs = 0.0f;
            o.ignAdvDeg  = 12.0f;
            o.iscvPct    = 18.0f;  // 減速時のダッシュポット
        } else {
            o.injPulseMs = 1.0f + 9.5f * (k.mapKpa / 100.0f) * (kStoichAfr / k.afr) * (1.0f + 0.3f * coldness);
            o.ignAdvDeg  = 10.0f + 26.0f * clamp01(k.rpm / 4000.0f) * (1.0f - 0.45f * load);
            o.iscvPct    = (k.tpsPct < 2.0f) ? 18.0f + 22.0f * coldness : 12.0f;
        }
        o.injDutyPct = o.injPulseMs * o.rpm / 1200.0f;  // 4 ストローク: 1 回転あたり 1/2 回噴射 -> PW[ms] × rpm / 1200

        // --- 油圧・電圧
        const float thin = 1.0f - 0.35f * clamp01((m_oilTC - kAmbientC) / 90.0f);  // 油温が上がると下がる
        if (o.engineRunning) {
            o.oilPressKpa = (90.0f + 0.065f * o.rpm) * thin;
            o.battV       = (k.speedKmh < 1.0f && k.tpsPct < 2.0f) ? 13.8f : 14.2f;
        } else {
            o.oilPressKpa = (k.rpm > 0.0f) ? 60.0f : 0.0f;
            o.battV       = (k.rpm > 0.0f) ? 10.3f : 12.5f;  // クランキングでセルモーターが電圧を下げる
        }
        return o;
    }

private:
    static float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

    static void lag(float& state, float target, float tauS, float dtS) {
        if (dtS <= 0.0f) {
            return;
        }
        const float a = dtS / (tauS + dtS);
        state += (target - state) * a;
    }

    /// 周期 periodMs の三角波（-1..1）。sin を使わずに済ませる（HW 非依存・決定的）
    static float wave(uint32_t tMs, uint32_t periodMs) {
        const float ph = static_cast<float>(tMs % periodMs) / static_cast<float>(periodMs);
        return ph < 0.5f ? (4.0f * ph - 1.0f) : (3.0f - 4.0f * ph);
    }

    static Key keyAt(uint32_t tMs) {
        for (size_t i = 1; i < kKeyCount; ++i) {
            if (tMs <= kKeys[i].tMs) {
                const Key& a  = kKeys[i - 1];
                const Key& b  = kKeys[i];
                const float f = (b.tMs == a.tMs) ? 1.0f
                                                 : static_cast<float>(tMs - a.tMs) / static_cast<float>(b.tMs - a.tMs);
                return Key{tMs,
                           a.rpm + (b.rpm - a.rpm) * f,
                           a.speedKmh + (b.speedKmh - a.speedKmh) * f,
                           a.tpsPct + (b.tpsPct - a.tpsPct) * f,
                           a.mapKpa + (b.mapKpa - a.mapKpa) * f,
                           a.afr + (b.afr - a.afr) * f};
            }
        }
        return kKeys[kKeyCount - 1];
    }

    uint32_t m_tMs  = 0;
    uint32_t m_runMs = 0;  ///< 始動してからの累計（WBO の加熱判定）
    float m_cltC    = kAmbientC;
    float m_oilTC   = kAmbientC;
    float m_iatC    = kAmbientC;
    float m_egtC    = kAmbientC;
};

}  // namespace cm
