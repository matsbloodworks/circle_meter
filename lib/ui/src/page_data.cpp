#include "page_data.h"

#include <cstdio>
#include <cstring>

#include "display_policy.h"
#include "fonts/cm_fonts.h"
#include "page_common.h"
#include "theme.h"

// ページ切り替え用の外部関数（main_lcd21.cpp で定義）

namespace cm::ui {

namespace {

// 左右 2 分割レイアウト（480x480）
// 左側：EGT、AFR（DSEG7 大型）、Speed（上から順に）
// 右側：CLT、MAT、MAP、Oil P、Oil T、ISCV duty（垂直配置）
constexpr int kLeftX  = 90;    // 左側中央 X（20px右移動）
constexpr int kRightX = 310;   // 右側中央 X（さらに左へ）
constexpr int kRightLeftX = 280;   // 右側・左列 X（MAT、OIL T、ISCV用、+10px右移動）
constexpr int kRightRightX = 380;  // 右側・右列 X（MAP、OIL P、TPS用）

constexpr int kEgtLabelY   = 100;   // EGT ラベル（20px右15px上移動、5px下）
constexpr int kEgtY        = 110;   // EGT 値（15px上移動、5px下）
constexpr int kAfrLabelY   = 155;   // AFR ラベル（20px右15px上移動、5px下）
constexpr int kAfrY        = 205;   // AFR 値（15px上移動、5px下）
constexpr int kSpeedLabelY = 265;   // Speed ラベル（20px右15px上移動、5px下）
constexpr int kSpeedY      = 295;   // Speed 値（15px上移動、5px下）

constexpr int kBaroLabelY  = 345;   // BARO ラベル（20px右15px上移動、5px下）
constexpr int kBaroY       = 375;   // BARO 値（15px上移動、5px下）

constexpr int kEgtValueX   = 90;    // EGT値 X位置（右に20px）
constexpr int kSpeedValueX = 90;    // SPD値 X位置（右に20px）

constexpr int kRightLabelY = 75;    // 右側ラベル Y 開始（+25px下、5px下移動）
constexpr int kRightStep   = 70;    // 右側データの Y 間隔

constexpr int kAfrWidth    = 200;   // AFR の幅
constexpr int kEgtSpdWidth = 85;    // EGT/SPD の幅（36px DSEG7 用、見切れ対策）
constexpr int kDataWidth   = 110;   // 右側データの幅（34px DSEG7 用、見切れ対策）
constexpr int kLabelWidth  = 90;    // ラベルの幅（増加：Oil対応）

lv_obj_t* makeLabel(lv_obj_t* parent, const lv_font_t* font, uint32_t hex, int width, int centerX, int centerY) {
    lv_obj_t* l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(hex), 0);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(l, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(l, width);
    lv_obj_set_pos(l, centerX - width / 2, centerY - lv_font_get_line_height(font) / 2);
    return l;
}

/// 値があれば fmt で、無ければ "--" を書く（SYS-41 / RSK-01: 届いていない値を 0 と見せない）
void formatOr(char* buf, size_t size, bool has, const char* fmt, float value) {
    if (has) {
        std::snprintf(buf, size, fmt, value);
    } else {
        std::snprintf(buf, size, "--");
    }
}

}  // anonymous namespace

void PageData::onCreate(lv_obj_t* parent, const Config& cfg) {
    m_root = lv_obj_create(parent);
    lv_obj_remove_style_all(m_root);
    lv_obj_set_pos(m_root, 0, 0);
    lv_obj_set_size(m_root, layout::kWidth, layout::kHeight);
    lv_obj_set_style_bg_color(m_root, lv_color_hex(color::kDialBg), 0);
    lv_obj_set_style_bg_opa(m_root, LV_OPA_COVER, 0);
    lv_obj_remove_flag(m_root, static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));

    // === M B W ラベル（真ん中上部、グレー色） ===
    m_mbwLabel = lv_label_create(m_root);
    lv_obj_set_style_text_font(m_mbwLabel, &cm_font_b612_label, 0);
    lv_obj_set_style_text_color(m_mbwLabel, lv_color_hex(0x888888), 0);  // グレー色
    lv_label_set_text_static(m_mbwLabel, "M B W");
    lv_obj_set_style_text_align(m_mbwLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(m_mbwLabel, 100);
    lv_obj_set_pos(m_mbwLabel, layout::kCx - 50, 20);  // 真ん中上部

    // === M B W の下にバー（画面を横断） ===
    m_topBar = lv_obj_create(m_root);
    lv_obj_remove_style_all(m_topBar);
    lv_obj_set_size(m_topBar, layout::kWidth, 2);  // 幅: 480, 高さ: 2px
    lv_obj_set_pos(m_topBar, 0, 50);  // M B W の下
    lv_obj_set_style_bg_color(m_topBar, lv_color_hex(0x888888), 0);  // グレー色
    lv_obj_set_style_bg_opa(m_topBar, LV_OPA_COVER, 0);
    lv_obj_remove_flag(m_topBar, static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));

    // === 左側：EGT、AFR（DSEG7 大型）、Speed（上から順に） ===
    // テキストラベル（右へシフト）
    m_cltTitle = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, kLabelWidth, kLeftX + 30, kEgtLabelY - 30);
    lv_label_set_text_static(m_cltTitle, "EGT");

    // EGT値（DSEG7 36px、左に20px移動）
    m_egtNum.create(m_root, &cm_font_dseg7_egt_spd, kEgtSpdWidth, kEgtY);
    lv_obj_set_x(m_egtNum.obj(), kEgtValueX);  // 左に20px移動
    lv_obj_set_style_text_align(m_egtNum.obj(), LV_TEXT_ALIGN_CENTER, 0);

    // AFRテキスト（右へシフト、下に移動、上に3px移動）
    lv_obj_t* afrLabel = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, kLabelWidth, kLeftX + 30, kAfrLabelY - 3);
    lv_label_set_text_static(afrLabel, "AFR");

    // AFR値（左へシフト、EGT値と同じX位置）
    m_afrNum.create(m_root, &cm_font_dseg7_afr, kAfrWidth, kAfrY);
    m_afrNum.setCellScale('.', 0.6f);  // 小数点を見やすく
    lv_obj_set_x(m_afrNum.obj(), 20);  // 左へ移動（EGT値と同じX位置）

    // SPDテキスト（右へシフト）
    m_speedTitle = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, kLabelWidth, kLeftX + 30, kSpeedLabelY);
    lv_label_set_text_static(m_speedTitle, "SPD");

    // SPD値（DSEG7 36px、左に20px移動、下に3px移動）
    m_speedNum.create(m_root, &cm_font_dseg7_egt_spd, kEgtSpdWidth, kSpeedY + 3);
    lv_obj_set_x(m_speedNum.obj(), kSpeedValueX);
    lv_obj_set_style_text_align(m_speedNum.obj(), LV_TEXT_ALIGN_CENTER, 0);

    // BARO ラベル
    lv_obj_t* baroLabel = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, kLabelWidth, kLeftX + 30, kBaroLabelY);
    lv_label_set_text_static(baroLabel, "BARO");

    // BARO値（DSEG7 36px）
    m_baroNum.create(m_root, &cm_font_dseg7_egt_spd, kEgtSpdWidth, kBaroY);
    lv_obj_set_x(m_baroNum.obj(), kEgtValueX);
    lv_obj_set_style_text_align(m_baroNum.obj(), LV_TEXT_ALIGN_CENTER, 0);

    // === 右側：2列x5行 ===
    // Row 1: TPS（左） | ISCV（右）
    m_tpsTitleLabel = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, kLabelWidth, kRightLeftX, kRightLabelY);
    lv_label_set_text_static(m_tpsTitleLabel, "TPS");
    m_tpsNum.create(m_root, &cm_font_dseg7_data, kDataWidth, kRightLabelY + 25);
    lv_obj_set_x(m_tpsNum.obj(), kRightLeftX - kDataWidth / 2);
    lv_obj_set_style_text_align(m_tpsNum.obj(), LV_TEXT_ALIGN_CENTER, 0);

    m_iscvTitleLabel = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, kLabelWidth, kRightRightX, kRightLabelY);
    lv_label_set_text_static(m_iscvTitleLabel, "ISCV");
    m_iscvNum.create(m_root, &cm_font_dseg7_data, kDataWidth, kRightLabelY + 25);
    lv_obj_set_x(m_iscvNum.obj(), kRightRightX - kDataWidth / 2);
    lv_obj_set_style_text_align(m_iscvNum.obj(), LV_TEXT_ALIGN_CENTER, 0);

    // Row 2: MAP（左） | MAT（右）
    m_mapTitleLabel = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, kLabelWidth, kRightLeftX, kRightLabelY + kRightStep);
    lv_label_set_text_static(m_mapTitleLabel, "MAP");
    m_mapNum.create(m_root, &cm_font_dseg7_data, kDataWidth, kRightLabelY + kRightStep + 25);
    lv_obj_set_x(m_mapNum.obj(), kRightLeftX - kDataWidth / 2);
    lv_obj_set_style_text_align(m_mapNum.obj(), LV_TEXT_ALIGN_CENTER, 0);

    m_matTitleLabel = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, kLabelWidth, kRightRightX, kRightLabelY + kRightStep);
    lv_label_set_text_static(m_matTitleLabel, "MAT");
    m_matNum.create(m_root, &cm_font_dseg7_data, kDataWidth, kRightLabelY + kRightStep + 25);
    lv_obj_set_x(m_matNum.obj(), kRightRightX - kDataWidth / 2);
    lv_obj_set_style_text_align(m_matNum.obj(), LV_TEXT_ALIGN_CENTER, 0);

    // Row 3: OIL-P（左） | OIL-T（右）
    m_oilPTitleLabel = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, kLabelWidth, kRightLeftX, kRightLabelY + kRightStep * 2);
    lv_label_set_text_static(m_oilPTitleLabel, "OIL-P");
    m_oilPNum.create(m_root, &cm_font_dseg7_data, kDataWidth, kRightLabelY + kRightStep * 2 + 25);
    m_oilPNum.setCellScale('.', 0.6f);  // 小数点を見やすく
    lv_obj_set_x(m_oilPNum.obj(), kRightLeftX - kDataWidth / 2);
    lv_obj_set_style_text_align(m_oilPNum.obj(), LV_TEXT_ALIGN_RIGHT, 0);

    m_oilTTitleLabel = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, kLabelWidth, kRightRightX, kRightLabelY + kRightStep * 2);
    lv_label_set_text_static(m_oilTTitleLabel, "OIL-T");
    m_oilTNum.create(m_root, &cm_font_dseg7_data, kDataWidth, kRightLabelY + kRightStep * 2 + 25);
    lv_obj_set_x(m_oilTNum.obj(), kRightRightX - kDataWidth / 2);
    lv_obj_set_style_text_align(m_oilTNum.obj(), LV_TEXT_ALIGN_CENTER, 0);

    // Row 4: PW（左） | ADV（右）
    m_pwTitleLabel = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, kLabelWidth, kRightLeftX, kRightLabelY + kRightStep * 3);
    lv_label_set_text_static(m_pwTitleLabel, "PW");
    m_pwNum.create(m_root, &cm_font_dseg7_data, kDataWidth, kRightLabelY + kRightStep * 3 + 25);
    m_pwNum.setCellScale('.', 0.6f);  // 小数点を見やすく
    lv_obj_set_x(m_pwNum.obj(), kRightLeftX - kDataWidth / 2);
    lv_obj_set_style_text_align(m_pwNum.obj(), LV_TEXT_ALIGN_RIGHT, 0);

    m_advTitleLabel = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, kLabelWidth, kRightRightX, kRightLabelY + kRightStep * 3);
    lv_label_set_text_static(m_advTitleLabel, "ADV");
    m_advNum.create(m_root, &cm_font_dseg7_data, kDataWidth, kRightLabelY + kRightStep * 3 + 25);
    lv_obj_set_x(m_advNum.obj(), kRightRightX - kDataWidth / 2);
    lv_obj_set_style_text_align(m_advNum.obj(), LV_TEXT_ALIGN_CENTER, 0);

    // Row 5: BAT（左） | CLT（右）
    m_vBattTitleLabel = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, kLabelWidth, kRightLeftX, kRightLabelY + kRightStep * 4);
    lv_label_set_text_static(m_vBattTitleLabel, "BAT");
    m_vBattNum.create(m_root, &cm_font_dseg7_data, kDataWidth, kRightLabelY + kRightStep * 4 + 25);
    m_vBattNum.setCellScale('.', 0.6f);  // 小数点を見やすく
    lv_obj_set_x(m_vBattNum.obj(), kRightLeftX - kDataWidth / 2 - 6);  // 左に6px移動（左8px後、右2px移動）
    lv_obj_set_style_text_align(m_vBattNum.obj(), LV_TEXT_ALIGN_CENTER, 0);

    m_cltTitleLabel = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, kLabelWidth, kRightRightX, kRightLabelY + kRightStep * 4);
    lv_label_set_text_static(m_cltTitleLabel, "CLT");
    m_cltNum.create(m_root, &cm_font_dseg7_data, kDataWidth, kRightLabelY + kRightStep * 4 + 25);
    lv_obj_set_x(m_cltNum.obj(), kRightRightX - kDataWidth / 2);
    lv_obj_set_style_text_align(m_cltNum.obj(), LV_TEXT_ALIGN_CENTER, 0);

    // SPDラベル
    m_spdTitleLabel = m_speedTitle;

    m_diag = makeLabel(m_root, &cm_font_b612_small, color::kDialDiag, 200, layout::kCx, 450);
    lv_label_set_text_static(m_diag, m_lastDiag);

    // === rusEFI テキスト（右端に90度傾けて配置）===
    m_rusEfiLabel = lv_label_create(m_root);
    lv_obj_set_style_text_font(m_rusEfiLabel, &cm_font_b612_label, 0);
    lv_obj_set_style_text_color(m_rusEfiLabel, lv_color_hex(0xFF8800), 0);  // オレンジ
    // SWR-104: 受信している ECU の名前を出す
    lv_label_set_text_static(m_rusEfiLabel,
                             cfg.ecuType == static_cast<uint8_t>(EcuType::Link) ? "LINK" : "RUSEFI");
    lv_label_set_long_mode(m_rusEfiLabel, LV_LABEL_LONG_CLIP);  // 折り返し無効
    lv_obj_set_pos(m_rusEfiLabel, 375, 190);  // 右に10px移動
    lv_obj_set_width(m_rusEfiLabel, 150);  // 幅を広くして1行に表示
    lv_obj_set_style_transform_angle(m_rusEfiLabel, 2700, 0);  // 90度回転（2700 = 270 * 10）
    lv_obj_set_style_transform_pivot_x(m_rusEfiLabel, 75, 0);
    lv_obj_set_style_transform_pivot_y(m_rusEfiLabel, 10, 0);
}

void PageData::onShow() {
    lv_obj_remove_flag(m_root, LV_OBJ_FLAG_HIDDEN);
}

void PageData::onHide() {
    lv_obj_add_flag(m_root, LV_OBJ_FLAG_HIDDEN);
}

void PageData::setLabel(lv_obj_t* label, char* last, size_t size, const char* text) {
    if (std::strncmp(last, text, size) == 0) {
        return;
    }
    std::snprintf(last, size, "%s", text);
    lv_label_set_text_static(label, last);
}

void PageData::setDiagText(const char* text) {
    setLabel(m_diag, m_lastDiag, sizeof(m_lastDiag), text != nullptr ? text : "");
}

void PageData::onUpdate(const Snapshot& snap, const Config& cfg) {
    const DisplayValues v = m_filter.update(snap, cfg);
    char buf[16];

    // AFR表示（DSEG7）13.5以上で黄色、14.7以上で赤
    formatMain(buf, sizeof(buf), v, cfg);
    uint32_t afrColor = color::kText;
    float afr = 0.0f;
    if (v.hasLambda) {
        afr = cfg.showAfr ? lambdaToAfr(v.lambda, cfg.stoich) : v.lambda;
        if (cfg.showAfr && afr >= 14.7f) {
            afrColor = 0xFF3333;  // 赤
        } else if (cfg.showAfr && afr >= 13.5f) {
            afrColor = 0xFFDD00;  // 黄色
        }
    }
    m_afrNum.setText(buf, afrColor);

    // EGT表示（DSEG7 36px）400以上で黄色、500以上で赤
    formatOr(buf, sizeof(buf), v.hasEgt, "%.0f", v.egtC);
    uint32_t egtColor = color::kText;
    if (!v.hasEgt) {
        // 値なし: 白のまま
    } else if (v.egtC >= 500.0f) {
        egtColor = 0xFF3333;  // 赤
    } else if (v.egtC >= 400.0f) {
        egtColor = 0xFFDD00;  // 黄色
    }
    m_egtNum.setText(buf, egtColor);

    // Speed表示（DSEG7 36px）80以上で黄色、120以上で赤
    formatOr(m_speedBuf, sizeof(m_speedBuf), v.hasSpeed, "%.0f", v.speedKmh);
    uint32_t spdColor = color::kText;
    if (v.speedKmh >= 120.0f) {
        spdColor = 0xFF3333;  // 赤
    } else if (v.speedKmh >= 80.0f) {
        spdColor = 0xFFDD00;  // 黄色
    }
    m_speedNum.setText(m_speedBuf, spdColor);

    // TPS表示（DSEG7）
    formatOr(m_tpsBuf, sizeof(m_tpsBuf), v.hasTps, "%.0f", v.tpsPercent);
    m_tpsNum.setText(m_tpsBuf, color::kText);

    // CLT表示（DSEG7）60以下で青、100以上で黄色、105以上で赤で点滅
    formatOr(m_cltBuf, sizeof(m_cltBuf), v.hasClt, "%.0f", v.cltC);
    uint32_t cltColor = color::kText;
    if (!v.hasClt) {
        // 値なし: 白のまま（0 を「冷えている」の青にしない）
    } else if (v.cltC >= 105.0f) {
        // 点滅処理
        if (m_blinkTimer % 1000 < 500) {
            cltColor = 0xFF3333;  // 赤
        } else {
            cltColor = 0x000000;  // 黒（非表示のような）
        }
    } else if (v.cltC >= 100.0f) {
        cltColor = 0xFFDD00;  // 黄色
    } else if (v.cltC <= 60.0f) {
        cltColor = 0x33BBFF;  // 青
    }
    m_cltNum.setText(m_cltBuf, cltColor);

    // OIL P表示（DSEG7、小数点1桁、kgf/cm2。ECU は kPa で送るので DisplayFilter で換算済み）
    formatOr(m_oilPressureBuf, sizeof(m_oilPressureBuf), v.hasOilP, "%.1f", v.oilPressKgfCm2);
    m_oilPNum.setText(m_oilPressureBuf, color::kText);

    // OIL T表示（DSEG7）105以上で黄色、115以上で赤で点滅
    formatOr(m_oilTempBuf, sizeof(m_oilTempBuf), v.hasOilT, "%.0f", v.oilTempC);
    uint32_t oilTColor = color::kText;
    if (v.oilTempC >= 115.0f) {
        // 点滅処理
        if (m_blinkTimer % 1000 < 500) {
            oilTColor = 0xFF3333;  // 赤
        } else {
            oilTColor = 0x000000;  // 黒（非表示のような）
        }
    } else if (v.oilTempC >= 105.0f) {
        oilTColor = 0xFFDD00;  // 黄色
    }
    m_oilTNum.setText(m_oilTempBuf, oilTColor);

    // MAP表示（DSEG7）
    formatOr(m_mapBuf, sizeof(m_mapBuf), v.hasMap, "%.0f", v.mapKpa);
    m_mapNum.setText(m_mapBuf, color::kText);

    // BARO表示（DSEG7 36px）
    formatOr(m_baroBuf, sizeof(m_baroBuf), v.hasBaro, "%.0f", v.baroKpa);
    m_baroNum.setText(m_baroBuf, color::kText);

    // MAT表示（DSEG7）40以上で黄色、60以上で赤
    formatOr(m_matBuf, sizeof(m_matBuf), v.hasMat, "%.0f", v.matC);
    uint32_t matColor = color::kText;
    if (v.matC >= 60.0f) {
        matColor = 0xFF3333;  // 赤
    } else if (v.matC >= 40.0f) {
        matColor = 0xFFDD00;  // 黄色
    }
    m_matNum.setText(m_matBuf, matColor);

    // ISCV Duty表示（DSEG7）
    formatOr(m_iscvDutyBuf, sizeof(m_iscvDutyBuf), v.hasIscv, "%.0f", v.iscvDuty);
    m_iscvNum.setText(m_iscvDutyBuf, color::kText);

    // vBatt表示（DSEG7、小数点1桁）12以下で黄色、11.5以下で赤で点滅
    formatOr(m_vBattBuf, sizeof(m_vBattBuf), v.hasBatt, "%.1f", v.battVolt);
    uint32_t vbColor = color::kText;
    if (!v.hasBatt) {
        // 値なし: 白のまま（0 V を「電圧低下」の赤点滅にしない）
    } else if (v.battVolt <= 11.5f) {
        // 点滅処理
        if (m_blinkTimer % 1000 < 500) {
            vbColor = 0xFF3333;  // 赤
        } else {
            vbColor = 0x000000;  // 黒（非表示のような）
        }
    } else if (v.battVolt <= 12.0f) {
        vbColor = 0xFFDD00;  // 黄色
    }
    m_vBattNum.setText(m_vBattBuf, vbColor);

    // PW（インジェクターパルス幅）表示（DSEG7、小数点以下1桁）
    // 注：色はINJ duty に基づいて変更（80以上で黄色、95以上で赤）
    formatOr(m_pwBuf, sizeof(m_pwBuf), v.hasPw, "%.1f", v.pulseWidth);
    // INJ duty を snap から取得
    float injDuty = 0.0f;
    snap.get(SignalId::InjDuty, injDuty);
    uint32_t pwColor = color::kText;
    if (injDuty >= 95.0f) {
        pwColor = 0xFF3333;  // 赤
    } else if (injDuty >= 80.0f) {
        pwColor = 0xFFDD00;  // 黄色
    }
    m_pwNum.setText(m_pwBuf, pwColor);

    // ADV（点火進角）表示（DSEG7）
    formatOr(m_advBuf, sizeof(m_advBuf), v.hasAdv, "%.0f", v.advanceDeg);
    m_advNum.setText(m_advBuf, color::kText);

    // === ラベルと値の色変更ロジック ===
    // EGT: 400以上で黄色、500以上で赤
    if (v.egtC >= 500.0f) {
        lv_obj_set_style_text_color(m_cltTitle, lv_color_hex(0xFF3333), 0);  // 赤
    } else if (v.egtC >= 400.0f) {
        lv_obj_set_style_text_color(m_cltTitle, lv_color_hex(0xFFDD00), 0);  // 黄色
    } else {
        lv_obj_set_style_text_color(m_cltTitle, lv_color_hex(color::kDialSmall), 0);  // デフォルト
    }

    // SPD: 80以上で黄色、120以上で赤
    if (v.speedKmh >= 120.0f) {
        lv_obj_set_style_text_color(m_spdTitleLabel, lv_color_hex(0xFF3333), 0);  // 赤
    } else if (v.speedKmh >= 80.0f) {
        lv_obj_set_style_text_color(m_spdTitleLabel, lv_color_hex(0xFFDD00), 0);  // 黄色
    } else {
        lv_obj_set_style_text_color(m_spdTitleLabel, lv_color_hex(color::kDialSmall), 0);  // デフォルト
    }

    // CLT: 60以下で青、100以上で黄色、105以上で赤で点滅
    if (!v.hasClt) {
        lv_obj_set_style_text_color(m_cltTitleLabel, lv_color_hex(color::kDialSmall), 0);  // デフォルト
    } else if (v.cltC >= 105.0f) {
        // 点滅処理
        if (m_blinkTimer % 1000 < 500) {
            lv_obj_set_style_text_color(m_cltTitleLabel, lv_color_hex(0xFF3333), 0);  // 赤
        } else {
            lv_obj_set_style_text_color(m_cltTitleLabel, lv_color_hex(0x000000), 0);  // 黒（非表示のような）
        }
    } else if (v.cltC >= 100.0f) {
        lv_obj_set_style_text_color(m_cltTitleLabel, lv_color_hex(0xFFDD00), 0);  // 黄色
    } else if (v.cltC <= 60.0f) {
        lv_obj_set_style_text_color(m_cltTitleLabel, lv_color_hex(0x33BBFF), 0);  // 青
    } else {
        lv_obj_set_style_text_color(m_cltTitleLabel, lv_color_hex(color::kDialSmall), 0);  // デフォルト
    }

    // MAT: 40以上で黄色、60以上で赤
    if (v.matC >= 60.0f) {
        lv_obj_set_style_text_color(m_matTitleLabel, lv_color_hex(0xFF3333), 0);  // 赤
    } else if (v.matC >= 40.0f) {
        lv_obj_set_style_text_color(m_matTitleLabel, lv_color_hex(0xFFDD00), 0);  // 黄色
    } else {
        lv_obj_set_style_text_color(m_matTitleLabel, lv_color_hex(color::kDialSmall), 0);  // デフォルト
    }

    // VB: 12以下で黄色、11.5以下で赤で点滅
    if (!v.hasBatt) {
        lv_obj_set_style_text_color(m_vBattTitleLabel, lv_color_hex(color::kDialSmall), 0);  // デフォルト
    } else if (v.battVolt <= 11.5f) {
        // 点滅処理
        if (m_blinkTimer % 1000 < 500) {
            lv_obj_set_style_text_color(m_vBattTitleLabel, lv_color_hex(0xFF3333), 0);  // 赤
        } else {
            lv_obj_set_style_text_color(m_vBattTitleLabel, lv_color_hex(0x000000), 0);  // 黒（非表示のような）
        }
    } else if (v.battVolt <= 12.0f) {
        lv_obj_set_style_text_color(m_vBattTitleLabel, lv_color_hex(0xFFDD00), 0);  // 黄色
    } else {
        lv_obj_set_style_text_color(m_vBattTitleLabel, lv_color_hex(color::kDialSmall), 0);  // デフォルト
    }

    // OIL_T: 105以上で黄色、115以上で赤で点滅
    if (v.oilTempC >= 115.0f) {
        // 点滅処理
        if (m_blinkTimer % 1000 < 500) {
            lv_obj_set_style_text_color(m_oilTTitleLabel, lv_color_hex(0xFF3333), 0);  // 赤
        } else {
            lv_obj_set_style_text_color(m_oilTTitleLabel, lv_color_hex(0x000000), 0);  // 黒（非表示のような）
        }
    } else if (v.oilTempC >= 105.0f) {
        lv_obj_set_style_text_color(m_oilTTitleLabel, lv_color_hex(0xFFDD00), 0);  // 黄色
    } else {
        lv_obj_set_style_text_color(m_oilTTitleLabel, lv_color_hex(color::kDialSmall), 0);  // デフォルト
    }

    // PW（INJ duty）: 80以上で黄色、95以上で赤
    if (injDuty >= 95.0f) {
        lv_obj_set_style_text_color(m_pwTitleLabel, lv_color_hex(0xFF3333), 0);  // 赤
    } else if (injDuty >= 80.0f) {
        lv_obj_set_style_text_color(m_pwTitleLabel, lv_color_hex(0xFFDD00), 0);  // 黄色
    } else {
        lv_obj_set_style_text_color(m_pwTitleLabel, lv_color_hex(color::kDialSmall), 0);  // デフォルト
    }

    // rusEFI点滅ロジック（CLT>110 または OIL-T>115）
    const bool needsBlink = (v.cltC > 110.0f) || (v.oilTempC > 115.0f);
    m_blinkTimer += 50;  // 50ms更新と仮定

    if (needsBlink) {
        if (m_blinkTimer >= 500) {  // 500msごとに点滅
            m_blinkState = !m_blinkState;
            m_blinkTimer = 0;
        }
        if (m_blinkState) {
            lv_obj_remove_flag(m_rusEfiLabel, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(m_rusEfiLabel, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        lv_obj_remove_flag(m_rusEfiLabel, LV_OBJ_FLAG_HIDDEN);
        m_blinkTimer = 0;
        m_blinkState = true;
    }
}

bool PageData::onEvent(InputEvent ev) {
    // 2026-11-22: M面だけを使う（C面へは切り替えない）。タップは何もしない
    (void)ev;
    return false;
}

}  // namespace cm::ui
