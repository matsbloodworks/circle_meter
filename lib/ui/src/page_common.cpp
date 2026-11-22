#include "page_common.h"

#include <cstdio>

#include "theme.h"

namespace cm::ui {

// UTF-8 の度記号。"\xB0C" と続けると 16 進エスケープが "B0C" と読まれるので、リテラルを分ける
#define CM_DEG "\xC2\xB0"

void formatMain(char* buf, size_t size, const DisplayValues& v, const Config& cfg) {
    if (!v.hasLambda) {
        std::snprintf(buf, size, "--");
    } else if (cfg.showAfr) {
        std::snprintf(buf, size, "%.1f", lambdaToAfr(v.lambda, cfg.stoich));
    } else {
        std::snprintf(buf, size, "%.3f", v.lambda);
    }
}

uint32_t mainColorHex(const DisplayValues& v) {
    if (!v.hasLambda || v.lambdaStale) {
        return color::kDisabled;
    }
    if (v.zone == LambdaZone::LeanHeavy) {
        return color::kLeanHeavy;  // DOC-23 §3.3: 白。ただし LEAN_HEAVY のときはゾーン色
    }
    return color::kText;
}

uint32_t afrColorHex(const DisplayValues& v, const Config& cfg) {
    if (!v.hasLambda || v.lambdaStale) {
        return color::kDisabled;
    }
    float afr = lambdaToAfr(v.lambda, cfg.stoich);
    if (afr >= 14.5f) {
        return 0xFF3333;  // 赤
    }
    if (afr >= 13.0f) {
        return 0xFFDD00;  // 黄色
    }
    return color::kText;  // 白
}

void formatEgt(char* buf, size_t size, const DisplayValues& v) {
    if (v.hasEgt) {
        std::snprintf(buf, size, "%d" CM_DEG "C", static_cast<int>(v.egtC + 0.5f));
    } else {
        std::snprintf(buf, size, "--" CM_DEG "C");
    }
}

uint32_t egtColorHex(const DisplayValues& v, bool blinkOn) {
    if (!v.hasEgt || v.egtStale) {
        return color::kDisabled;
    }
    // 温度値ベースの色変化: 500以上で赤、400以上で黄色
    if (v.egtC >= 500.0f) {
        return 0xFF3333;  // 赤
    }
    if (v.egtC >= 400.0f) {
        return 0xFFDD00;  // 黄色
    }
    return color::kText;  // 白
}

uint32_t sensorStatusColorHex(const LambdaSensorInfo& info) {
    switch (info.state) {
        case LambdaSensorState::SensorOff:
            return color::kTextSub;
        case LambdaSensorState::WarmingUp:
        case LambdaSensorState::Check:
            return color::kWarn;
        default:
            return color::kDanger;  // 故障・通信なし
    }
}

bool egtAlarmOn(const DisplayValues& v, bool blinkOn) {
    return v.hasEgt && !v.egtStale && v.egtLevel == EgtLevel::Danger && blinkOn;
}

uint32_t spdColorHex(const DisplayValues& v) {
    if (v.speedKmh >= 120.0f) {
        return 0xFF3333;  // 赤
    }
    if (v.speedKmh >= 80.0f) {
        return 0xFFDD00;  // 黄色
    }
    return color::kText;  // 白
}

uint32_t cltColorHex(const DisplayValues& v) {
    if (v.cltC >= 105.0f) {
        return 0xFF3333;  // 赤
    }
    if (v.cltC >= 95.0f) {
        return 0xFFDD00;  // 黄色
    }
    if (v.cltC <= 60.0f) {
        return 0x33BBFF;  // 青
    }
    return color::kText;  // 白
}

uint32_t vbColorHex(const DisplayValues& v) {
    if (v.battVolt <= 11.5f) {
        return 0xFF3333;  // 赤
    }
    if (v.battVolt <= 12.0f) {
        return 0xFFDD00;  // 黄色
    }
    return color::kText;  // 白
}

uint32_t oilTempColorHex(const DisplayValues& v) {
    if (v.oilTempC >= 115.0f) {
        return 0xFF3333;  // 赤
    }
    if (v.oilTempC >= 105.0f) {
        return 0xFFDD00;  // 黄色
    }
    if (v.oilTempC <= 60.0f) {
        return 0x33BBFF;  // 青
    }
    return color::kText;  // 白
}

}  // namespace cm::ui
