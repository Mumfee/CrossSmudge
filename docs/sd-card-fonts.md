---
title: SD Card Fonts
nav_order: 4
---

# SD Card Fonts

CrossSmudge supports loading additional fonts from the SD card, including fonts
with extended Unicode coverage (CJK, Cyrillic, Greek, etc.).

## Installing Fonts

There are three ways to install fonts:

### Option 1: Download from device

1. Connect your CrossSmudge reader to Wi-Fi
2. Go to **Settings > Reader > Font Options > Manage Fonts**
3. Browse available font families and select to download
4. Downloaded fonts appear immediately in **Settings > Reader > Font Options > Font Family**

**Note**: To change the font sizes that are downloaded, change the option for `Download Font Size Range` _before_ downloading.

### Option 2: Upload via web browser

1. Start **File Transfer** and connect through **Join Network** or **Create Hotspot**
2. Open the web interface URL shown on the reader
3. Navigate to the **Fonts** tab
4. Upload `.cpfont` files using the upload form

### Option 3: Manual SD card copy (Fastest)

1.  Download font files from the
    [CrossSmudge Fonts](https://github.com/uxjulia/crosssmudge-fonts/tree/main/cpfonts) repository.
    - Click the `.zip` file for the font you want then click on the download icon to download the raw file.
2.  Copy font family folders to one of two locations on your SD card:
    - `/.fonts/` — hidden directory (preferred; keeps the SD root tidy
      when mounted on a desktop)
    - `/fonts/` — visible directory (use this if your OS hides dot-files
      and you'd rather see the folder in your file manager)

    Both roots are always scanned at boot and the results are merged: a
    family installed in `/fonts/` shows up even when `/.fonts/` also
    exists, and vice versa. The two roots only collide if the same family
    name appears in both — in that case the copy in `/.fonts/` wins and
    the duplicate in `/fonts/` is ignored.

        SD Card Root/
        ├── .fonts/                     ← Hidden root (preferred)
        │   └── Literata/
        │       ├── Literata_12.cpfont
        │       ├── Literata_14.cpfont
        │       ├── Literata_16.cpfont
        │       └── Literata_18.cpfont
        └── fonts/                      ← Visible root (equally valid)
            └── Merriweather/
                ├── Merriweather_12.cpfont
                └── ...

3.  Insert the SD card and power on your CrossSmudge device

## Dictionary Fonts

EPUB books can use a different installed SD-card family for dictionary definitions.
This can be set globally or per-book via `Font Options`. If a
saved point size is no longer available, CrossSmudge chooses the closest file from
the dictionary family. If the device experiences low available RAM, you may see the
dictionary font fall back to your reader font. This is normal.

### Generating dictionary font families

Use the dictionary-specific builder to generate the complete family catalog with
the extra coverage used by dictionary definitions:

    python3 -m pip install -r lib/EpdFont/scripts/requirements.txt
    python3 lib/EpdFont/scripts/build-dictionary-fonts.py \
      --output-dir ./generated-dictionary-fonts \
      --clean \
      --jobs 2

The dictionary build includes the `reading` ranges and the built-in ranges, plus
IPA and phonetic-extension characters (`U+0250–U+02FF` and `U+1D00–U+1DBF`) and
combining-mark ranges (`U+1DC0–U+1DFF`, `U+20D0–U+20FF`, and
`U+FE20–U+FE2F`).

The default output is `../crosssmudge-fonts/dictionary-fonts`. Use a separate
`--output-dir` for personal builds, because `--clean` removes the selected output
directory before generating the fonts. The output contains family folders and ZIP
archives; copy a family folder or unzip its archive into `/.fonts/` or `/fonts/`
on the SD card. Use `--only FamilyA,FamilyB` to generate selected families.

## Available Pre-Built Fonts

You can view pre-built fonts available for download at [Inky](https://inky.crosssmudge.dev/#downloads).

## Converting Custom Fonts with CrossPoint's Font Builder

To convert your own TrueType/OpenType fonts use CrossPoint's [Font Builder](https://crosspointreader.com/fonts)

## Converting Custom Fonts with Python

### Prerequisites

    pip install freetype-py fonttools

### Single font (one style)

    python3 lib/EpdFont/scripts/fontconvert_sdcard.py \
      MyFont-Regular.ttf \
      --intervals latin-ext \
      --sizes 12,14,16,18 \
      --style regular \
      --name MyFont \
      --output-dir ./MyFont/

### Multi-style font

    python3 lib/EpdFont/scripts/fontconvert_sdcard.py \
      --regular MyFont-Regular.ttf \
      --bold MyFont-Bold.ttf \
      --italic MyFont-Italic.ttf \
      --bolditalic MyFont-BoldItalic.ttf \
      --intervals latin-ext \
      --sizes 12,14,16,18 \
      --name MyFont \
      --output-dir ./MyFont/

### Available Unicode interval presets

| Preset        | Coverage                                                                                                             |
| ------------- | -------------------------------------------------------------------------------------------------------------------- |
| `ascii`       | U+0020–U+007E (Basic Latin)                                                                                          |
| `latin1`      | U+0080–U+00FF (Latin-1 Supplement)                                                                                   |
| `latin-ext`   | European languages (Latin + Extended-A/B + punctuation + ligatures)                                                  |
| `greek`       | Greek + Extended Greek                                                                                               |
| `cyrillic`    | Cyrillic + Supplement                                                                                                |
| `hebrew`      | Hebrew + Alphabetic Presentation Forms                                                                               |
| `georgian`    | Georgian + Georgian Supplement                                                                                       |
| `armenian`    | Armenian                                                                                                             |
| `ethiopic`    | Ethiopic + Extended                                                                                                  |
| `vietnamese`  | Vietnamese subset (ơ/ư and combining marks)                                                                          |
| `punctuation` | General punctuation (U+2000–U+206F)                                                                                  |
| `cjk`         | CJK Unified Ideographs + Hiragana + Katakana + Fullwidth                                                             |
| `hangul`      | Korean Hangul syllables + Jamo + Compatibility Jamo                                                                  |
| `cherokee`    | Cherokee (historic + supplement block)                                                                               |
| `tifinagh`    | Tifinagh                                                                                                             |
| `symbols`     | Math, currency, arrows, box-drawing, misc symbols, dingbats                                                          |
| `reading`     | Literary fiction coverage: Latin, Greek, Cyrillic, math/symbol blocks, supplemental punctuation, and CJK quote marks |
| `builtin`     | Matches the firmware's built-in font conversion intervals                                                            |

Combine presets with commas: `--intervals latin-ext,greek,cyrillic`

You can also specify arbitrary Unicode ranges directly:
`--intervals latin-ext,(0x2100-0x214F)`

To list all presets with codepoint counts:

    python3 lib/EpdFont/scripts/fontconvert_sdcard.py --list-presets

### Additional options

`--force-autohint` — force FreeType's auto-hinter instead of the font's native hinting (useful when a font's built-in hints produce poor results at small sizes).

Install custom fonts via the web interface or manual SD card copy.
