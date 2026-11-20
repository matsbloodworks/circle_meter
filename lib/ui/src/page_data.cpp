#include "page_data.h"

#include <cstdio>
#include <cstring>

#include "display_policy.h"
#include "fonts/cm_fonts.h"
#include "page_common.h"
#include "theme.h"

namespace cm::ui {

namespace {

// レイアウト
constexpr int kEgtY    = 120;  // EGT表示（上部）
constexpr int kAfrY    = 220;  // AFR表示（中央）
constexpr int kDataY   = 380;  // Speed/CLT/IAT表示（下部）

constexpr int kEgtWidth   = 60;
constexpr int kAfrWidth   = 320;  // C面と同じサイズ
constexpr int kDataWidth  = 60;

// 下部 3 つのデータ配置（横に並べる）: CLT / Speed / IAT
constexpr int kCltX   = 120;
constexpr int kSpeedX = 240;
constexpr int kIatX   = 360;

lv_obj_t* makeLabel(lv_obj_t* parent, const lv_font_t* font, uint32_t hex, int width, int centerY) {
    lv_obj_t* l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(hex), 0);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(l, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(l, width);
    lv_obj_set_pos(l, layout::kCx - width / 2, centerY - lv_font_get_line_height(font) / 2);
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

    // EGT/AFR テキストラベル
    lv_obj_t* egtLabel = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, 80, kEgtY - 20);
    lv_label_set_text_static(egtLabel, "EGT");

    lv_obj_t* afrLabel = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, 80, kAfrY - 20);
    lv_label_set_text_static(afrLabel, "AFR");

    // 数値表示
    m_egtNum.create(m_root, &cm_font_b612_egt, kEgtWidth, kEgtY);
    m_afrNum.create(m_root, &cm_font_dseg7_afr, kAfrWidth, kAfrY);
    m_afrNum.setCellScale('.', 0.6f);  // 小数点を見やすく

    // 下部テキストラベル（CLT / SPEED / IAT）
    m_speedTitle = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, 60, kDataY - 30);
    lv_obj_set_x(m_speedTitle, kSpeedX - 25);
    lv_label_set_text_static(m_speedTitle, "SPEED");

    m_cltTitle = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, 60, kDataY - 30);
    lv_obj_set_x(m_cltTitle, kCltX - 30);
    lv_label_set_text_static(m_cltTitle, "CLT");

    m_iatTitle = makeLabel(m_root, &cm_font_b612_label, color::kDialSmall, 60, kDataY - 30);
    lv_obj_set_x(m_iatTitle, kIatX - 30);
    lv_label_set_text_static(m_iatTitle, "IAT");

    // 下部 3 つの数値ラベル（横に並べる）: CLT / Speed / IAT
    m_cltLabel = makeLabel(m_root, &cm_font_b612_egt, color::kText, kDataWidth, kDataY);
    lv_obj_set_x(m_cltLabel, kCltX - kDataWidth / 2);

    m_speedLabel = makeLabel(m_root, &cm_font_b612_egt, color::kText, kDataWidth, kDataY);
    lv_obj_set_x(m_speedLabel, kSpeedX - kDataWidth / 2);

    m_iatLabel = makeLabel(m_root, &cm_font_b612_egt, color::kText, kDataWidth, kDataY);
    lv_obj_set_x(m_iatLabel, kIatX - kDataWidth / 2);

    m_diag = makeLabel(m_root, &cm_font_b612_small, color::kDialDiag, 200, 450);
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

    // EGT表示
    std::snprintf(buf, sizeof(buf), "%.0f", v.egtC);
    m_egtNum.setText(buf, egtColorHex(v, false));

    // AFR表示
    formatMain(buf, sizeof(buf), v, cfg);
    m_afrNum.setText(buf, mainColorHex(v));

    // Speed/CLT/IAT表示（専用バッファに出力）
    std::snprintf(m_speedBuf, sizeof(m_speedBuf), "%.0f", v.speedKmh);
    lv_label_set_text_static(m_speedLabel, m_speedBuf);

    std::snprintf(m_cltBuf, sizeof(m_cltBuf), "%.0f", v.cltC);
    lv_label_set_text_static(m_cltLabel, m_cltBuf);

    std::snprintf(m_iatBuf, sizeof(m_iatBuf), "%.0f", v.iatC);
    lv_label_set_text_static(m_iatLabel, m_iatBuf);
}

}  // namespace cm::ui
