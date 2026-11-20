#include "page_data.h"

#include <cstdio>
#include <cstring>

#include "display_policy.h"
#include "fonts/cm_fonts.h"
#include "page_common.h"
#include "theme.h"

namespace cm::ui {

namespace {

// 左右 2 分割レイアウト（480x480）
// 左側：EGT、AFR（DSEG7 大型）、Speed（上から順に）
// 右側：CLT、MAT、MAP、Oil P、Oil T、ISCV duty（垂直配置）
constexpr int kLeftX  = 70;    // 左側中央 X（さらに左へ）
constexpr int kRightX = 310;   // 右側中央 X（さらに左へ）
constexpr int kRightLeftX = 270;   // 右側・左列 X（MAT、OIL T、ISCV用）
constexpr int kRightRightX = 350;  // 右側・右列 X（MAP、OIL P、TPS用）

constexpr int kEgtLabelY   = 140;   // EGT ラベル（+40px下）
constexpr int kEgtY        = 170;   // EGT 値
constexpr int kAfrLabelY   = 190;   // AFR ラベル（+30px下）
constexpr int kAfrY        = 240;   // AFR 値（+30px下）
constexpr int kSpeedLabelY = 300;   // Speed ラベル（+10px下）
constexpr int kSpeedY      = 330;   // Speed 値（+10px下）

constexpr int kEgtValueX   = 110;   // EGT値 X位置（左に50px）
constexpr int kSpeedValueX = 140;   // SPD値 X位置（左に50px）

constexpr int kRightLabelY = 70;    // 右側ラベル Y 開始（+20px下）
constexpr int kRightStep   = 70;    // 右側データの Y 間隔

constexpr int kAfrWidth    = 200;   // AFR の幅
constexpr int kDataWidth   = 60;    // 右側データの幅
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

}  // anonymous namespace

void PageData::onCreate(lv_obj_t* parent, const Config& cfg) {
    m_root = lv_obj_create(parent);
    lv_obj_remove_style_all(m_root);
    lv_obj_set_pos(m_root, 0, 0);
    lv_obj_set_size(m_root, layout::kWidth, layout::kHeight);
    lv_obj_set_style_bg_color(m_root, lv_color_hex(color::kDialBg), 0);
    lv_obj_set_style_bg_opa(m_root, LV_OPA_COVER, 0);
    lv_obj_remove_flag(m_root, static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));

    // === 左側：EGT、AFR（DSEG7 大型）、Speed（上から順に） ===
    // テキストラベル（右へシフト）
    m_cltTitle = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, kLabelWidth, kLeftX + 30, kEgtLabelY - 30);
    lv_label_set_text_static(m_cltTitle, "EGT");

    // EGT値（左に20px移動）
    m_egtNum.create(m_root, &cm_font_b612_egt, kDataWidth, kEgtY);
    lv_obj_set_x(m_egtNum.obj(), kEgtValueX);  // 左に20px移動

    // AFRテキスト（右へシフト、下に移動）
    lv_obj_t* afrLabel = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, kLabelWidth, kLeftX + 30, kAfrLabelY);
    lv_label_set_text_static(afrLabel, "AFR");

    // AFR値（左へシフト、EGT値と同じX位置）
    m_afrNum.create(m_root, &cm_font_dseg7_afr, kAfrWidth, kAfrY);
    m_afrNum.setCellScale('.', 0.6f);  // 小数点を見やすく
    lv_obj_set_x(m_afrNum.obj(), 20);  // 左へ移動（EGT値と同じX位置）

    // SPDテキスト（右へシフト）
    m_speedTitle = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, kLabelWidth, kLeftX + 30, kSpeedLabelY);
    lv_label_set_text_static(m_speedTitle, "SPD");

    // SPD値（左に20px移動）
    m_speedLabel = makeLabel(m_root, &cm_font_b612_egt, color::kText, kDataWidth, kSpeedValueX, kSpeedY);

    // === 右側：CLT / MAT|MAP / OIL T|OIL P / ISCV / vBatt ===
    // CLT（中央揃え）
    lv_obj_t* cltTitleLabel = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, kLabelWidth, kRightX, kRightLabelY);
    lv_label_set_text_static(cltTitleLabel, "CLT");

    m_cltLabel = makeLabel(m_root, &cm_font_b612_egt, color::kText, kDataWidth, kRightX, kRightLabelY + 25);

    // MAT（左） | MAP（右）
    lv_obj_t* matTitleLabel = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, kLabelWidth, kRightLeftX, kRightLabelY + kRightStep);
    lv_label_set_text_static(matTitleLabel, "MAT");

    m_matLabel = makeLabel(m_root, &cm_font_b612_egt, color::kText, kDataWidth, kRightLeftX, kRightLabelY + kRightStep + 25);

    lv_obj_t* mapTitleLabel = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, kLabelWidth, kRightRightX, kRightLabelY + kRightStep);
    lv_label_set_text_static(mapTitleLabel, "MAP");

    m_mapLabel = makeLabel(m_root, &cm_font_b612_egt, color::kText, kDataWidth, kRightRightX, kRightLabelY + kRightStep + 25);

    // OIL T（左） | OIL P（右） ※位置入れ替え
    lv_obj_t* oilTTitleLabel = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, kLabelWidth, kRightLeftX, kRightLabelY + kRightStep * 2);
    lv_label_set_text_static(oilTTitleLabel, "OIL T");

    m_oilTempLabel = makeLabel(m_root, &cm_font_b612_egt, color::kText, kDataWidth, kRightLeftX, kRightLabelY + kRightStep * 2 + 25);

    lv_obj_t* oilPTitleLabel = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, kLabelWidth, kRightRightX, kRightLabelY + kRightStep * 2);
    lv_label_set_text_static(oilPTitleLabel, "OIL P");

    m_oilPressureLabel = makeLabel(m_root, &cm_font_b612_egt, color::kText, kDataWidth, kRightRightX, kRightLabelY + kRightStep * 2 + 25);

    // ISCV（左） | TPS（右）
    lv_obj_t* iscvTitleLabel = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, kLabelWidth, kRightLeftX, kRightLabelY + kRightStep * 3);
    lv_label_set_text_static(iscvTitleLabel, "ISCV");

    m_iscvDutyLabel = makeLabel(m_root, &cm_font_b612_egt, color::kText, kDataWidth, kRightLeftX, kRightLabelY + kRightStep * 3 + 25);

    lv_obj_t* tpsTitleLabel = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, kLabelWidth, kRightRightX, kRightLabelY + kRightStep * 3);
    lv_label_set_text_static(tpsTitleLabel, "TPS");

    m_tpsLabel = makeLabel(m_root, &cm_font_b612_egt, color::kText, kDataWidth, kRightRightX, kRightLabelY + kRightStep * 3 + 25);

    // vBatt（中央揃え）
    lv_obj_t* vBattTitleLabel = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, kLabelWidth, kRightX, kRightLabelY + kRightStep * 4);
    lv_label_set_text_static(vBattTitleLabel, "VB");

    m_vBattLabel = makeLabel(m_root, &cm_font_b612_egt, color::kText, kDataWidth, kRightX, kRightLabelY + kRightStep * 4 + 25);

    m_diag = makeLabel(m_root, &cm_font_b612_small, color::kDialDiag, 200, layout::kCx, 450);
    lv_label_set_text_static(m_diag, m_lastDiag);
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

    // AFR表示（DSEG7）
    formatMain(buf, sizeof(buf), v, cfg);
    m_afrNum.setText(buf, afrColorHex(v, cfg));

    // EGT表示
    std::snprintf(buf, sizeof(buf), "%.0f", v.egtC);
    m_egtNum.setText(buf, egtColorHex(v, false));

    // Speed表示
    std::snprintf(m_speedBuf, sizeof(m_speedBuf), "%.0f", v.speedKmh);
    lv_label_set_text_static(m_speedLabel, m_speedBuf);
    lv_obj_set_style_text_color(m_speedLabel, lv_color_hex(spdColorHex(v)), 0);

    // CLT表示
    std::snprintf(m_cltBuf, sizeof(m_cltBuf), "%.0f", v.cltC);
    lv_label_set_text_static(m_cltLabel, m_cltBuf);
    lv_obj_set_style_text_color(m_cltLabel, lv_color_hex(cltColorHex(v)), 0);

    // MAT表示
    std::snprintf(m_matBuf, sizeof(m_matBuf), "%.0f", v.matC);
    lv_label_set_text_static(m_matLabel, m_matBuf);

    // MAP表示
    std::snprintf(m_mapBuf, sizeof(m_mapBuf), "%.0f", v.mapKpa);
    lv_label_set_text_static(m_mapLabel, m_mapBuf);

    // Oil Pressure表示
    std::snprintf(m_oilPressureBuf, sizeof(m_oilPressureBuf), "%.0f", v.oilPressure);
    lv_label_set_text_static(m_oilPressureLabel, m_oilPressureBuf);

    // Oil Temp表示
    std::snprintf(m_oilTempBuf, sizeof(m_oilTempBuf), "%.0f", v.oilTempC);
    lv_label_set_text_static(m_oilTempLabel, m_oilTempBuf);
    lv_obj_set_style_text_color(m_oilTempLabel, lv_color_hex(oilTempColorHex(v)), 0);

    // ISCV Duty表示
    std::snprintf(m_iscvDutyBuf, sizeof(m_iscvDutyBuf), "%.0f", v.iscvDuty);
    lv_label_set_text_static(m_iscvDutyLabel, m_iscvDutyBuf);

    // TPS表示
    std::snprintf(m_tpsBuf, sizeof(m_tpsBuf), "%.0f", v.tpsPercent);
    lv_label_set_text_static(m_tpsLabel, m_tpsBuf);

    // vBatt表示（小数点第1位を表示）
    std::snprintf(m_vBattBuf, sizeof(m_vBattBuf), "%.1f", v.battVolt);
    lv_label_set_text_static(m_vBattLabel, m_vBattBuf);
    lv_obj_set_style_text_color(m_vBattLabel, lv_color_hex(vbColorHex(v)), 0);
}

}  // namespace cm::ui
