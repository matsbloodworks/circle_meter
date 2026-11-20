"""M5Dial の数値ページ用フォントを TTF から生成する（M5GFX の VLW 形式を C の配列にする）。

    python tools/make_vlw_font.py     assets/m5dial/fonts/cm_vlw_fonts.{h,cpp} を生成

M5GFX の内蔵フォントは種類が少ないので、数字は DSEG7（7 セグメント風）、項目名・単位・見出しは
Orbitron を使う。どちらも SIL Open Font License 1.1（assets/fonts/ のライセンス文）。

VLW 形式（M5GFX の lgfx::VLWfont::loadFont が読む形。数値はすべて 32 bit ビッグエンディアン）:
    ヘッダ 24 バイト : 文字数, 版(11), 文字サイズ, 0, ascent, descent
    文字ごと 28 バイト: 文字コード, 高さ, 幅, 送り幅, dY（ベースラインから上端まで）, dX, 0
    ビットマップ     : 文字ごとに 幅 x 高さ バイト（濃さ 0-255）を上の順に並べる
M5GFX は行の高さを ascent + descent として扱い、文字列の「中央揃え」はその箱の中央になる。
数字のフォントは descent を 0 にして、数字の上下中央がそのまま揃うようにしてある。
"""

import os
import struct
import sys

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    sys.exit("Pillow が必要です: python -m pip install --user pillow")

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
FONT_DIR = os.path.join(ROOT, "assets", "fonts")
OUT_DIR = os.path.join(ROOT, "assets", "m5dial", "fonts")

DSEG7 = os.path.join(FONT_DIR, "DSEG7Classic-Bold.ttf")
ORBITRON = os.path.join(FONT_DIR, "Orbitron-Variable.ttf")
ASCII = "".join(chr(c) for c in range(0x20, 0x7F))

# (シンボル名, TTF, 可変フォントの太さ, サイズ px, 収録文字, 高さの基準にする文字, 用途)
# 値のサイズは、4 分割の 1 マス（幅 98 px）に "101.5" が収まる最大
FONTS = [
    ("cm_vlw_value", DSEG7, None, 30, " -.0123456789", "8", "数値（例 101.5 / --）"),
    ("cm_vlw_label", ORBITRON, "Bold", 13, ASCII, "H", "項目名・単位"),
    ("cm_vlw_title", ORBITRON, "Medium", 10, ASCII, "H", "見出し"),
]


def load(path, size, weight):
    font = ImageFont.truetype(path, size)
    if weight:
        font.set_variation_by_name(weight)
    return font


def render(font, ch):
    """1 文字を描き、(幅, 高さ, 送り幅, dY, dX, 濃さの bytes) を返す。"""
    size = font.size
    ox, oy = size * 2, size * 3  # 大きなキャンバス上のベースライン原点
    img = Image.new("L", (size * 6, size * 5), 0)
    ImageDraw.Draw(img).text((ox, oy), ch, font=font, fill=255, anchor="ls")
    advance = int(round(font.getlength(ch)))
    box = img.getbbox()
    if box is None:  # 空白
        return 0, 0, advance, 0, 0, b""
    left, top, right, bottom = box
    return right - left, bottom - top, advance, oy - top, left - ox, img.crop(box).tobytes()


def build(path, weight, size, chars, ref):
    font = load(path, size, weight)
    glyphs = [(ord(ch),) + render(font, ch) for ch in sorted(set(chars))]
    ref_h, ref_dy = render(font, ref)[1], render(font, ref)[3]
    ascent = ref_dy               # 基準文字の上端
    descent = ref_h - ref_dy      # 基準文字の下端（数字やアルファベット大文字なら 0 付近）
    out = struct.pack(">6i", len(glyphs), 11, size, 0, ascent, max(descent, 0))
    for code, w, h, adv, dy, dx, _ in glyphs:
        if w > 255 or adv > 255 or not -128 <= dx <= 127:
            sys.exit("文字 U+%04X が大きすぎます（幅・送り幅は 255 まで）" % code)
        out += struct.pack(">7i", code, h, w, adv, dy, dx, 0)
    for g in glyphs:
        out += g[6]
    return out, font


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    header = ["// tools/make_vlw_font.py が生成（手で編集しない）", "#pragma once", "",
              "#include <cstddef>", "#include <cstdint>", ""]
    source = ["// tools/make_vlw_font.py が生成（手で編集しない）", '#include "cm_vlw_fonts.h"', ""]
    for name, path, weight, size, chars, ref, usage in FONTS:
        data, font = build(path, weight, size, chars, ref)
        header.append("/// %s（%s %d px、%d 文字）" % (usage, os.path.basename(path), size, len(set(chars))))
        header.append("extern const uint8_t %s[];" % name)
        header.append("extern const size_t %s_len;" % name)
        source.append("const uint8_t %s[] = {" % name)
        for i in range(0, len(data), 20):
            source.append("    " + ",".join("0x%02X" % b for b in data[i:i + 20]) + ",")
        source.append("};")
        source.append("const size_t %s_len = sizeof(%s);" % (name, name))
        source.append("")
        sample = "101.5" if ref == "8" else "km/L"
        print("%-14s %6d bytes  '%s' = %d px wide" % (name, len(data), sample, round(font.getlength(sample))))
    with open(os.path.join(OUT_DIR, "cm_vlw_fonts.h"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(header) + "\n")
    with open(os.path.join(OUT_DIR, "cm_vlw_fonts.cpp"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(source) + "\n")
    print("generated:", OUT_DIR)


if __name__ == "__main__":
    main()
