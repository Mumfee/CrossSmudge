# CrossSmudge ✒️

**CrossSmudge** is a custom firmware fork of **[CrossInk](https://github.com/uxjulia/crossink)** (which is derived from **[CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader)**). 

This fork maintains the high-legibility typography, custom reader features, and e-ink optimizations from CrossInk while layering in custom branding, expanded applications (games & utilities), app management, and a dedicated OTA release pipeline.

---

### Supported Devices

- Xteink X3
- Xteink X4
- Xteink X4 Pro
- Xteink X4 Classic
- Seeed Studio Sticky

---

## What's New in CrossSmudge 🎮✨

Beyond all standard reading features and font optimizations, **CrossSmudge** adds a whole suite of built-in applications, customization tools, and system tweaks:

### 🚀 Applications & Utilities
* **Applications Menu:** A dedicated launcher for quickly accessing on-device tools and mini-games.
* **Divine Worship: Daily Office:** Complete offline prayer book and psalter for the Personal Ordinariate with automatic calendar-based propers, psalms, and scripture lessons.
* **Tetris:** Classic falling-block puzzle adapted for low-latency e-ink play.
* **Wordle:** Full-featured daily word-guessing game adapted for low-power e-ink screens.
* **2048:** Classic sliding-tile puzzle with persistent state saving and confirmation reset modals.
* **Sudoku:** Grid-based logic puzzle system optimized for e-ink contrast.
* **Rosary App:** Dedicated step-by-step prayer companion featuring high-contrast bead tracking and custom scaled bitmap graphics.
* **Dice Simulator:** Instant multi-dice roller utility for tabletop gaming and quick decisions.
* **Life Counter:** Digital tracker for Magic: The Gathering and other tabletop games.

### ⚙️ System & Customization
* **CrossSmudge Branding:** Custom splash graphics, ink-spatter visuals, and tailored UI themes.
* **App Settings Manager:** Easily re-order, hide, or reveal applications in your main menu.
* **Custom OTA Updates:** Built-in Over-The-Air firmware update manager pointed directly to [CrossSmudge GitHub Releases](https://github.com/Mumfee/CrossSmudge/releases).

---

## What's different in this fork (CrossInk Section)

My goal with this fork was to maintain the core Crosspoint firmware while integrating my preferred typography and some lightweight reading statistics. I’ve focused on keeping the underlying system stable while layering in a few "nice-to-have" features and UI refinements along the way.

<table>
  <tr>
    <td align="center">
      <img src="./docs/images/bitter-small-15-margin.jpg" alt="Font: Bitter, Size: 12 pt, Margin: 15" /><br/>
      <em>Font: Bitter, Size: 12 pt, Margin: 15</em>
    </td>
    <td align="center">
      <img src="./docs/images/reading-stats.jpg" alt="Reading Stats with custom front button mapping shown" /><br/>
      <em>Reading Stats with custom front button mapping shown</em>
    </td>
  </tr>
</table>

### Highlights

- New reader fonts: Lexend Deca and Bitter.
- Music notation and selected supplemental Unicode glyph support to be able to render Project Hail Mary accurately.
- Added a custom `Minimal` theme and sleep screen option for the minimalists out there.
- Added a custom `Dashboard` theme and sleep screen option for reading stats enthusiasts.
- Reader font sizes: 10 pt, 12 pt, 14 pt, and 16 pt.
- Added ~~strikethrough~~ support.
- Made <u>underlines</u> thicker for better visibility.
- Added support for `<hr>` section breaks.
- Added support for "redaction" style rendering.
- Added improved support for tables with simple markup.
- Added ability to add bookmarks.
- Added ability to remap front buttons that only applies in the reader.
- Added Focus Reading and Guide Dots as optional reader modes.
- Added Force Paragraph Indents for books that render as one giant wall of text.
- Added ability to pin a sleep image as a favorite. The favorited image will always be displayed when your sleep settings are set to `Custom` or `Cover + Custom` (when no cover is available).
- Added more in-reader control remapping options for side buttons, short power button clicks, and long-press menu actions, and more.
- Added ability to mark a book as finished from the in-book menu. A pop-up will also display once 99% of the book is reached. This status allows tracking of total books read.
- Added ability to move finished books to "Read" folder.
- In-book menu to quickly adjust reader options without having to exit the book.
- Reading stats: total books read, total reading time, number of sessions, pages turned, average session time, pages turned per minute. You can also set your reading stats as your sleep screen.
- All-time reading stats [syncing](./docs/reading-stats-sync.md) between two CrossSmudge devices.
- Reading [progress sync](./docs/nearby-position-sync.md) between two CrossSmudge devices.
- Added customizable Auto Page Turn Interval (anything between 5-120 seconds).
- Added ability to view Recent Books as a 3x3 grid view.
- To view a more detailed list for each version, visit the [releases](https://github.com/uxjulia/CrossSmudge/releases) page to read release notes.

---

### Reader Fonts

The default fonts have been replaced with Lexend Deca and Bitter. These fonts have been chosen specifically to improve reading fluency and e-ink performance. These 'sturdier' typefaces feature uniform stroke weights and open geometries, allowing the X4/X3 to render crisp, high-contrast text with font-aliasing on while significantly reducing ghosting and artifacts.

- [Lexend Deca](https://fonts.google.com/specimen/Lexend+Deca) - A research-backed sans-serif typeface designed to improve reading fluency. Lexend was engineered based on the theory that reading issues are often a design problem (visual crowding) rather than a cognitive one.
- [Bitter](https://fonts.google.com/specimen/Bitter) - A "contemporary" slab serif typeface for text, it is specially designed for comfortably reading on digital screens. The consistent stroke weight of Bitter helps it render particularly well on e-ink devices. The medium weight has been chosen specifically for improved rendering on the X4/X3.

The UI now uses [Inter](https://fonts.google.com/specimen/Inter) as the display font which has improved readability at smaller sizes.

### Music and Supplemental Glyphs

- Built-in reader fonts include music notation, selected Cyrillic glyphs, and the Project Hail Mary CJK fallback ranges. Additional SD-card fonts retain emoji fallback support.

---

### Font Sizes

CrossSmudge includes 10 pt, 12 pt, 14 pt, and 16 pt built-in reader font sizes.

See [SD Card Fonts](./docs/sd-card-fonts.md) for installing additional font families and size ranges.

---

### Reader features

Reader Options, Focus Reading, Guide Dots, Force Paragraph Indents, reading stats, and finished-book behavior are documented in [Reader Features](./docs/reader-features.md).

### Custom button actions

CrossSmudge adds configurable button shortcuts.

See [Controls](./docs/controls.md) for the full action list and defaults.

---

## Tips for the best reading experience

CrossSmudge runs on an ESP32-C3 with limited RAM, so very large folders or complex EPUBs can be slower than they would be on a phone, tablet, or desktop app.

- Keep folders under about 200 files. For the smoothest browsing, aim for 50-100 files per folder.
- Having 1000+ books on the SD card is fine if they are split into smaller folders, such as by author, series, genre, or read/unread status.
- Avoid putting every book in the SD card root. The file browser has to scan and sort the current folder before it can show it.
- Text-first EPUBs are the best fit. Large image-heavy EPUBs, scanned books, comics, and omnibus files with thousands of sections may load slowly or fail under memory pressure.
- As a rough target, EPUBs under 20 MB tend to work the best. Files over 50 MB may still work, but they are more likely to be slow or memory-sensitive, especially if they contain many large images.
- If an EPUB is unusually slow, try [optimizing](./docs/webserver.md#epub-optimization) it with the built-in web optimizer (via File Transfer) before copying it to the SD card: remove unused high-resolution images, split very large omnibus files, and avoid embedding multiple full font families when possible.
- Use a reliable SD card and leave some free space. CrossSmudge stores settings, reading progress, cache files, stats, and generated book data on the card.

## Development Device Simulator

The [device simulator](https://github.com/uxjulia/crosssmudge-simulator) renders the e-ink display in an SDL2 window so firmware changes can be sanity-checked without flashing hardware.

See [Simulator](./docs/simulator.md) for setup, platform notes, keyboard controls, and cache tips.

---

## Installation

The fastest way to install Crosssmudge is by using Inky, Crosssmudge's web companion app: https://inky.crosssmudge.dev/#flash-tools

Download a `firmware-*.bin` from the [releases page](https://github.com/uxjulia/CrossSmudge/releases), then flash it with the web installer or command line.

See [Installation](./docs/installation.md) for step-by-step flashing and revert instructions.

---

## Guides & Documentation

Visit [https://www.crossink.dev](https://www.crossink.dev) for more user guides and additional documentation.

---

## Development quick start

CrossSmudge uses PlatformIO for building and flashing firmware. See [Getting Started](./docs/development/getting-started.md) for prerequisites, clone setup, and validation commands.

### Nix/NixOS

Nix/NixOS users can enter the development shell with either `nix develop` (flakes) or `nix-shell`:

```bash
nix develop -f nix
# or
nix-shell nix
```

To flash a connected ESP32-C3 device, enable PlatformIO's udev rules in your NixOS configuration:

```nix
services.udev.packages = with pkgs; [ platformio-core.udev ];
```

After rebuilding the system configuration, reconnect the device or reload udev rules.

### Build / flash / monitor

Connect your device to your computer via a USB cable. Before the first build, initialize the repository's submodules (including `freeink-sdk`):

```sh
git submodule update --init --recursive
```

Then flash the firmware using the correct environment for the device. The `default` environment is for the X3/X4 devices. ESP32-S3 devices have their own named environments.

```sh
pio run -e default --target upload
```

If PlatformIO reports `PackageException: Can not create a symbolic link for freeink-sdk/libs/hardware/BatteryMonitor, not a directory`, the `freeink-sdk` submodule is not initialized. Run the submodule command above and retry.

See [Testing and Debugging](./docs/development/testing-debugging.md) for serial logging, simulator checks, static analysis, and bug-report guidance.

---

## Repository layout

-  - app orchestration, settings/state, and activity implementations (home, reader, settings, network, boot/sleep)
-  - supporting libraries: EPUB parsing/layout, fonts, i18n, filesystem helpers, HAL wrappers, and more
-  - hardware SDK submodule for display, input, storage, and battery (docs: https://freeink.org/docs)
-  - web portal sources (, , ); compiled by  into 
-  - user and developer documentation, published via the  Astro site
-  - Astro project that builds  into the CrossSmudge documentation website
-  - unit tests and EPUB test fixtures
-  - build, codegen, and release tooling (i18n generation, web asset building, hyphenation tries, release packaging, etc.)
-  - helper scripts for formatting () and CI checks
-  - sample SD card contents (books, sleep images, themes) used by the simulator
-  - Nix/NixOS development shell definitions
-  - ESP-IDF managed component dependencies, fetched automatically during build
- [](./SCOPE.md), [](./GOVERNANCE.md), [](./CHANGELOG.md) - project scope, community principles, and release history

## Internals

The ESP32-C3 has about 380 KB of usable RAM, so CrossSmudge stores reusable book and device data on the SD card instead of rebuilding everything in memory.

See [Data Cache](./docs/data-cache.md) for the  layout and [File Formats](./docs/file-formats.md) for binary cache details.

## Notice on Contributions

This repository does not accept pull requests. Feature requests may be opened in [discussions](https://github.com/uxjulia/CrossSmudge/discussions), but major features requiring ongoing support should be directed upstream to [CrossPoint](https://github.com/crosspoint-reader/crosspoint-reader).

---

If you'd like to show some love and support ongoing development, please consider supporting me on Ko-fi.

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/Q5Q01M6S7)
