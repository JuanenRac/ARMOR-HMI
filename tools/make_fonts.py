"""ARMOR-HMI - generate the LVGL fonts of the screen (see docs/FONTS.md). Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.

Generate the LVGL fonts of ARMOR-HMI: Inter (Latin-1 and punctuation) at three sizes, and for Japanese and Chinese only the glyphs the screen's texts use."""
import glob
import os
import re
import subprocess
import sys

from fontTools.ttLib import TTFont  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
OUT = os.path.join(ROOT, "main", "fonts")
os.makedirs(OUT, exist_ok=True)
text = open(os.path.join(ROOT, "core", "screen_text.hpp"), encoding="utf-8").read()
# the characters of the Japanese and Chinese texts that are not in the Latin font
STRING = re.compile(r'"((?:[^"\\]|\\.)*)"')
rows = [STRING.findall(line) for line in text.splitlines() if line.startswith('    {"')]
rows = [r for r in rows if len(r) == 8]
chars_by_lang = {"ja": sorted({c for r in rows for c in r[6] if ord(c) >= 0x2E80}), "zh": sorted({c for r in rows for c in r[7] if ord(c) >= 0x2E80})}
chars = sorted(set(chars_by_lang["ja"]) | set(chars_by_lang["zh"]))
print("CJK glyphs:", len(chars))
symbols = "".join(chars)
nm = os.path.join(ROOT, "node_modules")
conv = ["node", os.path.join(nm, "lv_font_conv", "lv_font_conv.js")]
inter = os.path.join(nm, "@fontsource", "inter", "files", "inter-latin-500-normal.woff")
inter_ext = os.path.join(nm, "@fontsource", "inter", "files", "inter-latin-ext-500-normal.woff")
sizes = {"s": 18, "m": 24, "l": 44}


def run(args):
    print(" ".join(args)[:200])
    subprocess.run(args, check=True)


for tag, size in sizes.items():
    # Latin-1, the Latin extended letters, the general punctuation (quotes, ellipsis) and the degree sign
    run(conv + ["--font", inter, "-r", "0x20-0xFF,0x2018-0x201F,0x2026,0x2022", "--font", inter_ext, "-r", "0x100-0x17F", "--size", str(size), "--bpp", "4", "--format", "lvgl",
         "--lv-include", "lvgl.h", "--no-compress", "-o", os.path.join(OUT, f"hmi_font_latin_{tag}.c"), "--lv-font-name", f"hmi_font_latin_{tag}"])

for lang, family in (("ja", "noto-sans-jp"), ("zh", "noto-sans-sc")):
    slices = sorted(glob.glob(os.path.join(nm, "@fontsource", family, "files", f"{family}-*-500-normal.woff")))
    assert slices, family
    # which slice of the family holds which of the glyphs: each slice is given only its own, so the command stays short
    plan, missing = [], set(chars_by_lang[lang])
    for slice_path in slices:
        cmap = TTFont(slice_path).getBestCmap()
        mine = [c for c in chars_by_lang[lang] if ord(c) in cmap and c in missing]
        if mine:
            plan.append((slice_path, "".join(mine)))
            missing -= set(mine)
    print(lang, "slices used:", len(plan), "glyphs without a font:", "".join(sorted(missing)).encode("unicode_escape").decode())
    for tag, size in sizes.items():
        args = conv + ["--font", inter, "-r", "0x20-0x7F,0x2026"]
        for slice_path, mine in plan:
            args += ["--font", slice_path, "--symbols", mine]
        args += ["--size", str(size), "--bpp", "4", "--format", "lvgl", "--lv-include", "lvgl.h", "--no-compress", "-o", os.path.join(OUT, f"hmi_font_{lang}_{tag}.c"), "--lv-font-name", f"hmi_font_{lang}_{tag}"]
        run(args)
print(sorted(os.listdir(OUT)))
