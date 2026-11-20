// tools/make_vlw_font.py が生成（手で編集しない）
#pragma once

#include <cstddef>
#include <cstdint>

/// 数値（例 101.5 / --）（DSEG7Classic-Bold.ttf 30 px、13 文字）
extern const uint8_t cm_vlw_value[];
extern const size_t cm_vlw_value_len;
/// 項目名・単位（Orbitron-Variable.ttf 13 px、95 文字）
extern const uint8_t cm_vlw_label[];
extern const size_t cm_vlw_label_len;
/// 見出し（Orbitron-Variable.ttf 10 px、95 文字）
extern const uint8_t cm_vlw_title[];
extern const size_t cm_vlw_title_len;
