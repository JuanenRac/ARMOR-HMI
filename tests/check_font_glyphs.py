#!/usr/bin/env python3
"""Check that the screen's fonts hold every character the screen can draw.

Usage:  python tests/check_font_glyphs.py

The fonts are generated for exactly the characters the texts use (tools/make_fonts.py). A text added after the fonts were made has no glyph and is drawn as nothing or as a square, so this
reads the code points the generated files list (the `U+XXXX` comments) and checks, language by language, every character of core/screen_text.hpp, of the texts the firmware writes itself
(main/ui.cpp) and of the font of that language: Japanese and Chinese have their own font, the other six use the Latin one.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LANGUAGES = ["en", "es", "de", "fr", "it", "ja", "zh", "pt"]
STRING = re.compile(r'"((?:[^"\\]|\\.)*)"')


def glyphs_of(path: Path) -> set:
    return {int(m.group(1), 16) for m in re.finditer(r"/\* U\+([0-9A-Fa-f]{4,6}) ", path.read_text(encoding="utf-8", errors="replace"))}


def unescape(text: str) -> str:
    return text.replace('\\"', '"').replace("\\\\", "\\").replace("\\n", "\n")


def main() -> int:
    fonts = {}
    for family in ("latin", "ja", "zh"):
        sizes = [glyphs_of(ROOT / "main" / "fonts" / f"hmi_font_{family}_{tag}.c") for tag in ("s", "m", "l")]
        fonts[family] = sizes
    source = (ROOT / "core" / "screen_text.hpp").read_text(encoding="utf-8")
    rows = [STRING.findall(line) for line in source.splitlines() if line.startswith('    {"')]
    rows = [r for r in rows if len(r) == 1 + len(LANGUAGES)]
    problems = []
    if len(rows) < 20:
        problems.append(f"only {len(rows)} rows of {1 + len(LANGUAGES)} strings found in screen_text.hpp")
    ui = (ROOT / "main" / "ui.cpp").read_text(encoding="utf-8")
    ui_literals = [unescape(s) for s in STRING.findall(ui) if any(ord(c) > 0x7F for c in s)]
    for lang_index, lang in enumerate(LANGUAGES):
        family = lang if lang in ("ja", "zh") else "latin"
        needed = set()
        for row in rows:
            needed |= {c for c in unescape(row[1 + lang_index]) if c not in "\n\r"}
        for literal in ui_literals:
            needed |= set(literal)
        for size, have in zip("sml", fonts[family]):
            missing = sorted(ord(c) for c in needed if ord(c) >= 0x20 and ord(c) not in have)
            if missing:
                problems.append(f"{lang}: the font {family}_{size} has no glyph for " + " ".join(f"U+{c:04X}" for c in missing))
    if problems:
        print("FONT_GLYPHS=FAIL")
        for problem in problems:
            print("  " + problem)
        return 1
    print(f"FONT_GLYPHS=PASS {len(rows)} rows in {len(LANGUAGES)} languages")
    return 0


if __name__ == "__main__":
    sys.exit(main())
