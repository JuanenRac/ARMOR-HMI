# ARMOR-HMI - the fonts of the screen

LVGL's built-in fonts hold only the basic Latin letters, so the screen would draw Spanish, French, German and Italian without their accents and Japanese and Chinese not at all. The
firmware therefore carries its own fonts, generated once and kept as C files in `main/fonts/`:

| Files | What they hold |
| --- | --- |
| `hmi_font_latin_{s,m,l}.c` | Inter (medium): Latin-1, Latin Extended-A and the usual punctuation (quotes, ellipsis, bullet), at 18, 24 and 44 pixels |
| `hmi_font_ja_{s,m,l}.c` | Inter for the Latin letters and Noto Sans JP for **exactly the Japanese characters of `core/screen_text.hpp`** (the characters that text uses) |
| `hmi_font_zh_{s,m,l}.c` | The same with Noto Sans SC for Chinese |

Both families are under the SIL Open Font License 1.1, which allows embedding and redistribution with the program; their licences are those of the npm packages
`@fontsource/inter`, `@fontsource/noto-sans-jp` and `@fontsource/noto-sans-sc`.

## Regenerating them

A new text in Japanese or Chinese needs its characters in the font. The generator reads `core/screen_text.hpp`, finds which slice of the Noto family holds each character (with
`fontTools`) and runs `lv_font_conv` (4 bits per pixel, no compression) once per size and language:

```bash
npm i lv_font_conv @fontsource/inter @fontsource/noto-sans-jp @fontsource/noto-sans-sc
pip install fonttools
python tools/make_fonts.py        # writes main/fonts/*.c
```

A character of the texts that no slice holds is reported by the generator and would be drawn as a missing-glyph square on the screen.
