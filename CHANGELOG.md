## [v1.6.5.3] - 2026-10-06

### Added

- Added *Stopwatch* (`apps/stopwatch/`): a retro pixel cartoon stopwatch application featuring a charming handheld stopwatch character with animated mechanical crown plunger, blinking/workout facial expressions, digital LCD window (`MM:SS.cc`), real-time second sweep indicator, and a full lap split recording card beneath displaying lap numbers, split times, cumulative elapsed times, and best lap highlights with persistent timing data.
- Added *Solitaire* (`apps/solitaire/`): classic 7-column Klondike Solitaire card game optimized for e-paper with draw-1 and draw-3 modes, auto-move logic, touch tap-to-move, and full button navigation.
- Added *Water Tracker* (`apps/watertracker/`) to the on-device App Store catalog (`apps/catalog.json`): a responsive, cute daily hydration tracking application featuring animated pixel cup water levels, cup facial expressions, 14-day history archive with daily completion badges, configurable volume units (`ml`, `cups`, `oz`, `gallons`, `L`), automatic banner timeouts, and multi-device display scaling across ESP32-C3 and ESP32-S3 e-paper readers.
- Added native auto-sleep prevention APIs to the Lua engine: `smudge.prevent_sleep([enabled])` (aliased as `smudge.keep_awake([enabled])`) and `smudge.is_prevent_sleep()` / `smudge.is_keep_awake()`, alongside declarative `"prevent_sleep": true` manifest support in `manifest.json`. Enables continuous desk clock, timer, and dashboard displays without triggering the system's inactivity sleep timer; physical Power button sleep remains immediate, and regular auto-sleep automatically resumes when the app exits.
- Enabled auto-sleep prevention across desktop productivity apps: Desk Stand (`apps/dashboard/`), Hourglass Timer (`apps/hourglass/`), and Stopwatch (`apps/stopwatch/`), allowing timers and desk displays to stay visible on desks indefinitely.
- Added native orientation control APIs to the Lua engine: `smudge.set_orientation("portrait" | "landscape" | "portrait_inverted" | "landscape_ccw")` and `smudge.get_orientation()`, with automatic orientation restoration on app exit.
- Added Lua API versioning and feature detection infrastructure:
  - `smudge.api_version` / `smudge.get_api_version()` returning integer API level (current: `2`).
  - `smudge.version()` / `smudge.get_version()` returning firmware version string.
  - `smudge.has_feature(name)` for programmatic capability checks (`"prevent_sleep"`, `"orientation"`, `"full_refresh"`, `"touch"`, `"version"`).
  - Manifest `"min_api"` specification in `manifest.json` with pre-flight check in `LuaRunner::init()`, displaying a helpful update notification instead of crashing on outdated firmware.

### Changed

- Overhauled **Desk Stand** (`apps/dashboard/`) from a single clock style into a multi-layout desk companion featuring 7 full-app styles with distinct element placements, dynamic layout scaling, and persistent settings (`dashboard.dat`):
  - **Classic Station**: Balanced vertical portrait station with top pill bar, flip clock, daylight progress bar, monthly calendar, and 3-column Year Overview card (Quarter, Week number, Days remaining, Season).
  - **Big Clock Focus**: Clock-dominant layout featuring giant 124px digital numerals, daylight bar, and bottom monthly calendar.
  - **Calendar Planner**: Calendar-dominant layout with a compact digital status bar, full-height monthly grid, and month progress card.
  - **Executive Dark**: High-contrast inverted dark theme for portrait desk display.
  - **Minimal Studio**: Frameless Swiss typography layout with hairline rules, battery/power stats, and centered calendar grid.
  - **Landscape Desk Stand**: Rotates the display to landscape (800x480) for side-by-side placement on a desk stand (clock & year stats on the left, full calendar on the right).
  - **Landscape Dark**: High-contrast inverted dark mode for landscape desk stand placement.
- Enhanced **Hourglass** (`apps/hourglass/`): eliminated rigid flat sand cutoffs across the glass by adding a natural parabolic funnel crater in the top bulb, flowing sand completely through the neck channel with a tapered conical nozzle tip, slenderizing the neck waist (`neck_w = 22`, `neck_h = 10`), rendering a continuous falling sand stream that stays visible during pauses, and building a conical bottom sand mound with animated landing bounce particles, all while maintaining the cute facial expressions and zero-allocation performance on ESP32-C3.
- Replaced the quotes section in Desk Stand with bi-directional style cycling buttons (`[Style -]` and `[Style +]`) alongside `[12-Hour]` / `[24-Hour]` time format toggling.
- Added a 5-second auto-hide timer to button hints in Desk Stand (`apps/dashboard/`) for a clean, ambient, distraction-free display. Touching the screen anywhere or pressing any hardware button reveals the buttons for 5 seconds. Kept the exact appearance of standard system button tabs (`smudge.button_hints`), and added native landscape button hint rendering to `LuaRunner` so button tabs align with the active theme in both portrait and landscape.
- Eliminated redundant and duplicated information across Desk Stand layouts: removed duplicate digital time text from Minimal Studio, removed duplicated clock time and day-of-year counters from landscape styles, and refined vertical progress bar spacing to prevent text-outline collisions.
- Upgraded **Wordle** (`apps/wordle/`) to **v1.1.0** in the App Store catalog (`apps/catalog.json`):
  - Expanded secret word bank to a full 3,103 5-letter binary word dictionary (`words.bin`) generated from the complete word list.
  - Implemented random-access offset reading support in `smudge.read_file(filename, [len], [offset])` in `LuaRunner`, allowing zero-RAM secret word lookups directly from flash/SD.
  - Implemented full game state persistence across app exits and device sleep (`wordle_state.dat`), saving secret word, all past guesses with tile evaluations, in-progress typed letters, and keyboard cursor position so players can resume where they left off.
  - Fixed duplicate letter evaluation in `evaluate_guess` where surplus letters in a guess would erroneously mark valid secret letters as absent on the keyboard.

### Fixed

- Fixed out-of-memory error on ESP32-C3 hardware when launching Desk Stand (`apps/dashboard/`) after updating from the App Store:
  - Eliminated runtime `dofile("styles.lua")` compilation peak by merging all layouts back into a single streamlined `main.lua`, using `local` static tables and functions to avoid global environment churn and redundant AST allocations.
  - Released Wi-Fi stack memory (`WiFi.mode(WIFI_OFF)`) in `AppStoreActivity::onExit()` so all ~50 KB of network DRAM is immediately restored to the system heap before launching applications.
  - Released Desk Stand v1.0.2 in the on-device App Store catalog (`apps/catalog.json`).
- Fixed full screen refresh flickering during style transitions in Desk Stand and Lua applications: updated `LuaRunner` (`l_fullRefresh`) to defer full e-ink display refreshes until after `on_draw` renders the new layout into the framebuffer, eliminating stale-buffer flashes.
- Fixed Desk Stand landscape layout clipping and overlap on physical devices by switching the display renderer to 800x480 (`smudge.set_orientation("landscape")`), providing full two-column side-by-side cards with clean left tab insets and automatic portrait restoration on exit.
- Fixed Wordle word bank containing the 6-letter word "VALLEY" and duplicate "RIVER" in `apps/wordle/`, which caused impossible games where the 5-letter board could never match the secret word. Replaced with valid 5-letter words ("VITAL", "SOLAR"), expanded the curated 5-letter word bank to 221 words, added runtime length filter safeguards, and released Wordle v1.0.1 in the App Store catalog (`apps/catalog.json`).
- Fixed calendar day highlight and clock 24H/AM/PM badge clipping in Desk Stand: replaced undersized square boxes with properly padded rounded pills centered around the glyph baseline to eliminate digit truncation.

### Removed

- Removed the quote card section and unneeded nested tables in Desk Stand, compressing digit segments into bitmask arrays to cut Lua compilation heap usage from ~70 KB down to ~25 KB and strictly adhere to ESP32-C3's 75 KB DRAM ceiling (`SMUDGE_X3_CONSTRAINTS=1`).

## [v1.6.5.2] - 2026-10-06

### Fixed

- Fixed 8-pixel horizontal right offset bug in the Applications launcher for SD card package icons (`src/activities/apps/ApplicationsActivity.h`), ensuring all dynamically loaded App Store apps with `icon.raw` align with built-in system applications (`metrics.contentSidePadding + 16`).
- Fixed date query in Lua applications on physical hardware to query the battery-backed RTC chip and timezone offset via `smudge.get_date()` / `smudge.date()`, preventing un-synchronized POSIX `"Jan 1"` displays on devices without active Wi-Fi.

### Removed

- Decoupled downloadable applications from the core firmware: removed hardcoded application IDs and baked-in icons from the launcher (`src/activities/apps/ApplicationsActivity.h`) so that all App Store and SD card applications dynamically load their own 32x32 `icon.raw`.
- Removed obsolete, dead C++ application activities and large baked-in asset headers (`WordleActivity.h`, `TetrisActivity.h`, `SudokuActivity.h`, `TwoZeroFourEightActivity.h`, `BlackjackActivity.h`, `CodexActivity.h`, `DailyOfficeActivity.h`, `DiceSimActivity.h`, `LifeCounterActivity.h`, `RosaryActivity.h`, `assets/`, `codex/`), reducing firmware flash footprint and eliminating duplicated logic with App Store packages.
- Removed unused application icon enums and flash assets (`Dice`, `Wordle`, `LifeCounter`, `Rosary`, `Sudoku`, `TwoZeroFourEight`, `Blackjack`, `Tetris`, `DailyOffice`, `Codex`) from `UIIcon` in `src/components/themes/BaseTheme.h` and `src/components/themes/lyra/LyraTheme.cpp`.

### Added

- Added comprehensive icon converter utility (`scripts/convert_icon.py`) with terminal ASCII `view`, `raw2png`, `png2raw`, and `header` generation commands, along with complete format and design specifications in `apps/README.md`.

## [v1.6.5.1] - 2026-10-03

### Fixed

- Hardened on-device App Store application downloads and error reporting (`src/activities/apps/AppStoreActivity.h`):
  - Added Wi-Fi connection check before starting app installations to prevent silent timeouts on dropped connections.
  - Added pre-download validation of application target directories on SD card storage.
  - Added automatic 3-stage retry loop with transport switching (`WOLFSSL` -> `ESP_HTTP` -> `WOLFSSL`) and 500ms backoff per file.
  - Differentiated between `Storage Error` (SD card write-lock, full card, or filesystem failure) and `Network Error` (Wi-Fi disconnect or TLS handshake timeout).
  - Enhanced the error screen's "Retry" action to automatically retry the failed application installation instead of resetting to catalog fetch.

### Removed

- Obsolete release asset `daily_office_sd_package.zip`: Divine Worship Daily Office is now distributed and installed as a standalone modular application directly via the on-device App Store (`apps/dailyoffice/`).

## [v1.6.5] - 2026-10-03

### Added

- Web Portal **Applications** Manager tab (`/applications`), allowing users to browse installed SD card applications with live 32x32 pixel logos, download or move applications to their PC as `.zip` packages, delete applications, and drag-and-drop or select folders and `.zip` archives for instant on-device installation.
- Native streaming section reader `smudge.find_section(path, tag, [endPrefix], [maxBytes])` (aliased as `smudge.find_file_section`) in `LuaRunner` with 512-byte stack chunk buffer via `HalFile`, enabling memory-constrained devices (ESP32-C3) to search and extract individual liturgical propers, appointed psalms, and scripture lessons directly from SD card storage with zero heap fragmentation.
- Developer-oriented extensions to the `smudge.*` Lua standard library:
  - Full e-paper waveform refresh control via `smudge.full_refresh()` and `smudge.refresh([full])` to eliminate ghosting during high-contrast transitions.
  - Pixel buffer inversion operations `smudge.invert_rect(x, y, w, h)` and `smudge.invert()` / `smudge.invert_screen()` for instant visual feedback and inverted banners.
  - Geometry and hit-testing helper `smudge.in_rect(px, py, rx, ry, rw, rh)` / `smudge.point_in_rect` for streamlined touch handling.
  - Triangle drawing primitive `smudge.triangle(x1, y1, x2, y2, x3, y3, [filled], [thickness], [color])` with outline stroke and scanline fill rasterization.
  - Hardware and capability queries: `smudge.has_touch()`, `smudge.get_battery()`, and `smudge.get_device()`.
  - Sandboxed file I/O operations for applications: `smudge.write_file(path, content, [append])`, `smudge.list_files([dir])`, and `smudge.delete_file(path)`.
  - Native modal popup dialog helper `smudge.popup(message)` / `smudge.draw_popup`.
- Classical engraved coin artwork for *Dice Roller* featuring a Roman laureate emperor bust face (Heads) and heraldic Roman eagle with laurel wreath backside (Tails) rendered via `smudge.draw_sprite()`.
- Overlapping card layout in *Blackjack* ensuring hands with 5+ cards automatically step and fit cleanly on-screen without clipping.
- Online App Store application (`AppStoreActivity.h`) connecting over Wi-Fi directly to the CrossSmudge GitHub repository (`apps/catalog.json`), featuring a two-level catalog and detail flow:
  - Catalog list view with 32x32 1-bit icons, app name, author, version tags, and status pills (`[ Installed ]`, `[ Update ]`, `[ Install ]`).
  - App detail view featuring a 2x-scaled 64x64 hero icon in an embossed badge frame, developer attribution, version tags, installation status, full multi-line description, and package details.
  - One-click Install, Upgrade, Reinstall, and Uninstall with recursive directory deletion (`Storage.removeDir`) and automatic subdirectory creation during GitHub downloads, with full physical button and touchscreen hit-testing.
- Converted all 11 applications (*2048*, *Blackjack*, *Codex: Ink & Iron*, *Dice Roller*, *Divine Worship: Daily Office*, *Holy Rosary*, *Life Counter*, *Sudoku*, *Tally Counter*, *Tetris*, and *Wordle*) into standalone modular Lua packages with custom 32x32 1-bit icons, dynamic discovery from SD card storage, and zero hardcoded C++ apps in firmware, freeing over 246 KB of flash headroom on ESP32-C3 devices.

- 15-page Chapter 1 progression in *Codex: Ink & Iron* (renaming floors to pages and acts to chapters), introducing the Scriptorium Shop on Pages 5 and 10 with an illustrated merchant, card purchases, relic offerings, healing draughts, and parchment scraping card removal.
- 5 new monsters and encounters in *Codex: Ink & Iron* featuring 128x128 woodcut illustrations (Quill Hound, Crypt Warden, Spine Horror, The Iron Scriptor elite, and The Arch-Heretic boss) with distinct multi-turn tactical AI patterns.
- Expanded relic collection in *Codex: Ink & Iron* with 12 relics, all equipped with custom 48x48 1-bit woodcut illustrations and framed miniature gallery presentation (Silver Quill, Iron Sigil, Vampiric Seal, Scholar's Ring, Whetstone, Monk's Rosary, Golden Bookmark, Obsidian Inkwell, Barbed Bookmark, Hourglass of Sand, Censer of Cleansing, Alchemical Flask).
- 7 new cards expanding the *Codex: Ink & Iron* catalog to 30 verses, introducing *Sever*, *Parry & Thrust*, *Divine Aegis*, *Rebound Ward*, *Ink Siphon*, *Illumination: Transcendence*, and *Transcribe*.
- *Codex: Ink & Iron* roguelike deckbuilder application tailored for 480x800 e-paper displays, featuring poetic Meter Couplet synergies (Blade, Ward, Script), a tactical Blot Out system, embedded woodcut artwork, and zero-allocation combat engine for ESP32-C3.
- Multi-choice random chamber encounters in *Codex: Ink & Iron* occurring after every 2 completed combats (floors 2, 4, 6, and 8), offering 9 thematic medieval events (The Cursed Reliquary, The Scriptorium, The Alchemist's Crucible, Corridor Ambush, The Whispering Lectern, The Wandering Peddler, The Scribe's Blood Altar, The Torture Vaults, and Fountain of Shimmering Ink) with consequential decisions granting permanent strength bonuses, base ink font capacity, combat ward, relics, cards, and gold, or inflicting hazardous trap damage and mortal wounds.
- Authentic medieval illuminated manuscript aesthetic for *Codex: Ink & Iron*, including deckle edge fiber margins, leather spine stitch markings, illuminated dividers (`── • ❖ ◈ ❖ • ──`), embossed woodcut miniature frames with rosettes, corner filigrees, and double-line card rules.
- Split side-by-side combat encounter layout for *Codex: Ink & Iron*, featuring 128x128 Dürer-style woodcut engravings (Ink Imp, Stone Gargoyle, Cursed Scribe, Parchment Ghoul, Book Golem, Marginalia Fiend, Grand Inquisitor) on the right, with monster name, HP progress bar, status effects, intent, and rhythm on the left, plus full touch target hit-testing for hand cards and bottom action hints.
- Four-tab grimoire system in *Codex: Ink & Iron* (`[Deck]`, `[Discard]`, `[Relics]`, `[Menu]`) accessible via Xteink X3 top buttons (`BTN_UP`/`BTN_DOWN`) or header touch, featuring dedicated views for collected relics and an in-game delve menu with options to Resume, Save & Exit to Title, Save & Exit to Applications, or Abandon Delve.
- Visual grey banner shading (`Color::LightGray`) for 1-use Power cards (`Illumination: Flow`, `Illumination: Bastion`, `Illumination: Fury`) in combat hand and card inspector.
- Run state persistence for *Codex: Ink & Iron* (`/.smudge/codex/run.state`), saving automatically on deep sleep or when exiting to Title/Applications, with zero SD card writes during combat turns to avoid flash wear.
- Deck and Discard pile viewers in *Codex: Ink & Iron* mapped to Xteink X3 top-left (`BTN_UP`) and top-right (`BTN_DOWN`) buttons with floor/grimoire/discard counts in the combat header.
- Visible combat status effect indicators (`[Str +X]`, `[Weak X]`, `[Vuln X]`, `[Poison X]`) for player and enemies in *Codex: Ink & Iron*, with clean ASCII Couplet labels, simplified Rhythm indicator, and streamlined reward screens.
- Divine Worship: Daily Office application for the Personal Ordinariate (North American Edition) with all 7 traditional hours, 100% offline standalone support with complete liturgical propers and calendar (397 days) bundled in firmware flash and auto-installed to SD card (/.smudge/daily_office/), automatic day-to-day liturgical propers from device clock, and physical button list navigation.
- Complete 150-Psalm Coverdale Psalter (including Psalm 119 sections) compressed in firmware flash and auto-unpacked to SD storage for daily psalmody.
- Invitatory Antiphons before and after the Venite for seasons and commons, Benedictus and Magnificat Antiphons, and appointed Office Hymns (including *Iste Confessor*) with Versicle and Response.
- Full Scripture lesson rendering for First and Second Lessons with support for offline lesson text bundles on SD card.
- Offline package script `scripts/build_daily_office_package.py` to generate complete SD card distribution zip for Divine Worship texts, propers, and lessons.
- EPUBs with stable page numbers can jump directly to a specific stable page from the reader menu.
- Hidden folders can be created using the web file manager now when prefixed with a dot.
- Choose whole numbers, one decimal, or two decimals for the book progress percentage in status bar settings.
- Two-finger Screen Rotation can be turned off in Settings > Controls > Taps & Gestures on multi-touch devices.
- Go to % and Go to Stable Page use a numeric keypad for typing an exact destination, including decimal percentages. Touch devices use the keypad exclusively; button-only devices keep the slider by default and hold Confirm/Select to switch to the keypad.
- Files can be renamed from the File Browser action menu while keeping reading progress, bookmarks, clippings, and recent-book entries linked to the new name.
- Firmware builds can include only selected UI languages to reduce flash usage while preserving English fallback.

### Changed

- Renamed the Lua application API global and namespaces from `ink.*` to `smudge.*` (e.g. `smudge.clear()`, `smudge.text()`, `smudge.button_hints()`) across all 10 applications, runtime bindings, and developer documentation to match CrossSmudge branding, while maintaining `ink.*` as a backwards-compatible alias for existing scripts.
- Added a generic 1-bit sprite drawing function `smudge.draw_sprite(x, y, w, h, data, [inverted], [dither])` (aliased as `smudge.sprite`), supporting on-demand streaming from SD card raw bitmap files, binary byte strings, or Lua byte arrays, with support for dark mode inversion and light-gray checkerboard e-paper dithering.
- Added customizable line `thickness` (stroke width in pixels) to all 2D shape drawing primitives (`smudge.line`, `smudge.rect`, `smudge.rounded_rect`, `smudge.circle`), fixing thin coin borders in *Dice Roller* and allowing bold vector outlines across all applications.
- Decoupled all application-specific graphics rendering (playing card suits, tetromino patterns, crucifix woodcuts, illuminated manuscript frames, and monster/relic plates) from the core C++ firmware engine into individual application packages, reducing firmware flash footprint and making apps fully standalone.
- Streamlined App Settings by removing per-app show/hide toggles so all installed applications on SD card storage are immediately visible in the launcher, focusing the settings screen exclusively on selecting the menu sort order (`Alphabetical (A - Z)` vs. `Most Frequently Used`).

### Fixed

- Resolved App Store catalog fetching hangs, TLS out-of-memory errors, and render freezes on ESP32-C3 devices (`src/activities/apps/AppStoreActivity.h`):
  - Implemented the FreeRTOS `void render(RenderLock&& lock) override` contract, routing all screen states (`CHECK_WIFI`, `FETCHING_CATALOG`, `CATALOG_READY`, `APP_DETAIL`, `DOWNLOADING`, `ERROR`) through `ActivityManager` and eliminating illegal direct `displayBuffer()` calls from the main loop thread.
  - Added release of resident SD card fonts (`sdFontSystem.releaseForNetwork(renderer)`) before network initialization and restored them on exit (`sdFontSystem.ensureLoaded(renderer); sdFontSystem.releaseRegistry()`), freeing 20–30 KB of critical internal DRAM required for TLS handshakes on ESP32-C3 devices.
  - Streamed GitHub `catalog.json` downloads directly to an SD temp cache file (`/.crosssmudge/cache/catalog.tmp`) via lightweight `Transport::WOLFSSL` (with fallback to `Transport::ESP_HTTP`), parsing JSON from the file stream and eliminating RAM buffering during TLS.
  - Removed 10 sequential blocking HTTPS icon downloads from `fetchCatalog()`, loading only locally installed or cached icons during catalog fetch and deferring icon downloads to individual app detail views, reducing initial catalog load time from 30+ seconds down to 1–2 seconds.
  - Added full interactive Retry and Back handling for physical buttons and touch in the error view (`renderError()`), ensuring clear diagnostic feedback if Wi-Fi or GitHub transfers fail.
  - Fixed physical front button and on-screen button hint navigation in the App Store catalog list by mapping `Button::Left` (Button 3 / "Up") to previous and `Button::Right` (Button 4 / "Down") to next, and added swipe gestures and expanded touch tolerance for bottom hints.

- Enhanced *Blackjack* gameplay controls, insurance, and splitting rules:
  - Added an interactive Insurance modal prompt when the dealer's face-up card is an Ace, paying 2:1 on dealer natural blackjack.
  - Added Double Down functionality mapped to the Select/Confirm button after initial deal.
  - Added pair splitting mapped to the Up/Down physical front buttons with a dedicated button hint prompt when identical card ranks are dealt.
  - Fixed pair splitting logic to strictly require matching card ranks (e.g. 8-8, K-K), preventing illegal splits on different 10-value cards (e.g. King and Queen).
  - Fixed split hand layout and typography so active hand highlights and button hints remain legible and unclipped on 480px screens.
- Enhanced *Codex: Ink & Iron* chamber events, navigation, and rewards:
  - Formatted all narrative lines across all 9 chamber events to $\le 44$ characters and trimmed choice descriptions, preventing text clipping and overflowing off the screen borders on 480px e-paper displays.
  - Removed the Back bypass button during chamber events to force players to make a choice, and routed side buttons to the Grimoire (`Deck`) and Delve (`Menu`) tabs with `eventId` persistence, allowing players to inspect cards/relics and save during events.
  - Mapped chamber event option cycling to the Left and Right front buttons with matching `<` and `>` button hints.
  - Updated bottom button hints in the Delve Menu tab (`deck_view.lua`) to display "Up" and "Down" instead of "<" and ">", and corrected tab navigation so the top button cycles to the previous tab and the bottom button cycles to the next tab.
  - Removed the redundant 4th "Skip Verse" card option from the victory reward screen in favor of direct skipping via the dedicated "Skip" front button.
- Hardened Lua runtime memory management and eliminated `abort()` panics (`src/activities/apps/lua/LuaRunner.cpp`):
  - Replaced unbounded `std::string` heap accumulation in `smudge.find_section()` with 512-byte stack buffers and `luaL_Buffer`, eliminating bare `operator new` allocations that called `abort()` / `std::terminate` (`PC 0x422aad1d`) when free contiguous DRAM dropped below allocation request sizes on ESP32-C3 devices.
  - Added line-by-line callback streaming to `smudge.find_section(path, tag, [endPrefix], callback)` (and `smudge.read_lines(path, callback)`), streaming sections and files from SD storage directly into Lua closures with zero C++ heap footprint.
  - Added live DRAM headroom bounds-checking for string-return mode on ESP32-C3, safely capping `maxBytes` to remaining free heap.
  - Switched garbage collection mode to aggressive incremental GC (`LUA_GCINC`, pause 105%, step multiplier 250%) to collect memory continuously during allocations rather than waiting for heap to double, preventing heap exhaustion on ESP32-C3 devices.
  - Tuned system safety cushion to 3 KB (`kMinSystemSafetyBytes = 3 * 1024`), preventing premature allocation rejections when free heap drops to 10-15 KB during valid peak loads.
  - Added proactive full GC collection prior to compiling scripts in `smudge.dofile()`, ensuring memory from previous screens is cleanly reclaimed before parsing new Lua chunks.
  - Optimized `smudge.wrapped_text()` with single-line bypass for text fitting within max bounds, eliminating temporary C++ vector allocations.
- Added exact ESP32-C3 hardware DRAM simulation in host builds (`LuaRunner.cpp`):
  - Setting environment variable `SMUDGE_X3_CONSTRAINTS=1` (or building with `-DSIMULATOR_DEVICE_X3`) enforces the strict 75 KB hardware DRAM ceiling on host simulator runs, enabling deterministic testing and profiling of embedded memory constraints without hardware guesswork.
- Resolved out-of-memory errors in *Divine Worship: Daily Office* on ESP32-C3 devices when opening Mattins and long liturgical hours:
  - Replaced in-memory page storage with a direct zero-RAM-page disk cache architecture (`cache_hour.txt`). Formatted pages stream directly to disk as they are compiled without allocating intermediate page tables, strings, or line vectors.
  - Reader displays pages on-demand directly from `cache_hour.txt` via `smudge.find_section()`, keeping reader memory flat (~59–60 KB) regardless of how many pages an hour has.
  - Migrated large office hymns (`ISTE_CONFESSOR`, `DEUS_TUORUM_MILITUM`, `ECCE_JAM_NOCTIS`, `LUCIS_CREATOR_OPTIME`), antiphons, and the 30-day psalter schedule out of Lua code into on-demand SD text file `hours/hymns.txt`.
  - Unified menu controller and reader into an ultra-lean, disciplined script, reducing peak memory from 85+ KB down to 59–71 KB across all 7 hours and 35+ pages, safely within the 75 KB hardware limit.
- Fixed out-of-memory errors in *Codex: Ink & Iron* when clicking "New Delve" and navigating combat, shop, and scriptorium screens on ESP32-C3 devices:
  - Modularized combat and victory reward flow into separate screen controllers (`screens/combat.lua` and `screens/reward.lua`), and decoupled narrative chamber events from consequence outcomes (`screens/scriptorium.lua` and `screens/outcome.lua`).
  - Enforced strict token ($\le 128$ identifiers) and bytecode instruction ($\le 256$ instructions per function) budgets across all 10 game screens, preventing Lua 5.4's compiler from doubling lexer hash tables (`ls->h`, a 6 KB contiguous DRAM allocation) and proto instruction buffers.
  - Converted card and relic instance caches in `cards.lua` and `relics.lua` to weak-valued metatables (`{ __mode = "v" }`), allowing temporary card instances to be reclaimed automatically by garbage collection.
  - Lazy-loaded relic definitions on-demand in shop and deck view rendering rather than preloading them into baseline memory at boot, maintaining a lean baseline DRAM footprint of ~40.9 KB.
  - Replaced line-wrapping table allocations in narrative chambers with direct centered text rendering, eliminating frame-by-frame heap churn.
  - Reduced peak memory consumption across an entire 15-floor delve (Title -> Delve -> Combat -> Cards -> Reward -> Scriptorium -> Outcome -> Shop -> Victory) from $>85$ KB down to **74.3 KB**, successfully executing under the 75 KB ESP32-C3 hardware DRAM limit without a single allocation failure.
- Resolved out-of-memory errors and completed casino rules in *Blackjack* (`apps/blackjack/main.lua`):
  - Replaced 416 separate Lua table allocations (`{rank = r, suit = s}`) for the shoe with integer-encoded cards (`(suit - 1) * 13 + rank`), preallocated in-place shoe shuffling, and reusable hand tables, keeping peak DRAM consumption under **67.9 KB** across 100+ rounds of gameplay.
  - Added an interactive **Insurance Modal Dialog** when dealer upcard is an Ace, allowing players to purchase 2:1 insurance against dealer Blackjack.
  - Added **Double Down** functionality on the `Confirm` button (with on-screen `"Double"` button hint) during player turn when holding two cards, doubling bet, drawing one card, and standing.
  - Added **Split** functionality for matching pairs on top/bottom buttons (`Up`/`Down`/`PageBack`/`PageForward`) and tap, displaying an on-screen `[SPLIT (Side Button / Tap)]` indicator, with multi-hand play and review.
- Fixed card split rules and button layout in *Blackjack* (`apps/blackjack/main.lua`):
  - Fixed rank comparison in `can_split_hand` (`card_rank` instead of `card_value`), strictly restricting splits to identical card ranks (e.g. K-K, Q-Q, 8-8) and preventing different 10-value cards (e.g. King and Queen, Jack and 10) from being split.
  - Redesigned the Split indicator button into a crisp, high-contrast rounded pill (`SPLIT (Side Button / Tap)`) with clean padding and standard typography, eliminating Unicode glyph corruption (`?` characters) and text clipping.
  - Corrected section header spacing and font sizing (`UI_12_FONT_ID`) for dealer and player sections, eliminating card overlap against header text and preventing text overflow in the insurance modal dialog.
  - Verified across multiple simulator scenarios under the exact 75 KB ESP32-C3 hardware DRAM limit (`SMUDGE_X3_CONSTRAINTS=1`).
- Restored difficulty selection modal and procedural board generation in *Sudoku* matching original C++ `SudokuActivity`:
  - Added "SELECT DIFFICULTY" modal dialog with options for Easy, Medium, and Hard on initial startup, when activating the "New Game" button, and on game completion.
  - Replaced the single static hardcoded puzzle with a randomized procedural board generator module (`generator.lua`) using independent diagonal 3x3 block pre-filling, randomized backtracking, and difficulty-based clue carving (Easy: 35 removed, Medium: 45 removed, Hard: 52 removed), generating unique boards in under 2ms under **68.5 KB** peak DRAM.
  - Added run state persistence (`smudge.save("state", ...)`) to restore ongoing games across reboots, and restored full physical button and touch navigation for the difficulty picker.
- Fixed keyboard layout and button sizing in *Wordle*:
  - Corrected the `ok` and `clear` button widths in row 2 to include the missing inter-key spacing (`keyPad`), ensuring both buttons together span the exact width of 3 keys (`3 * keyW + 2 * keyPad`) and align seamlessly with the rows above.
  - Updated touch hit-testing in `on_tap` to accurately map touches across the full bounding boxes of both `ok` and `clear`.
- Hardened the Lua runtime allocator and memory management in firmware (`LuaRunner.cpp`):
  - Eliminated illegal re-entrant `lua_gc` calls inside `customLuaAlloc` that violated Lua VM GC invariants and caused CPU `Load access fault` panics (`MTVAL 0x000000BC` at `lgc.c`). `customLuaAlloc` now cleanly returns `nullptr` on memory limit exhaustion, delegating safely to Lua's native emergency GC (`gcemergency = 1`) and raising catchable `LUA_ERRMEM` without hardware crash.
  - Optimized `smudge.read_file()` using Lua's native `luaL_buffinitsize()` buffer, eliminating intermediate C++ `String` heap churn and out-of-memory errors on large text files.
  - Added `smudge.get_memory()` / `smudge.memory()` API returning real-time Lua memory allocation and ESP32 system DRAM heap status (`{lua_kb, lua_max_kb, free_heap, max_alloc}`).
  - Added incremental GC pacing (`lua_gc(L, LUA_GCSTEP, 50)`) after drawing and button/touch events to avoid GC stalls.

- Fixed sideways orientation of installed application icons in the Web Portal Applications manager (`/applications`), properly rotating 90° clockwise from the pre-rotated hardware e-ink bitmap format so icons render upright on the HTML canvas.
- Restored dynamic liturgical content in *Divine Worship: Daily Office* (`apps/dailyoffice/main.lua`):
  - Appointed 30-Day Coverdale Psalms dynamically loaded from `psalter.txt` for Mattins and Evensong, with *Gloria Patri* after every psalm and section dividers.
  - Appointed Scripture readings and lesson citations (First and Second Lessons) dynamically loaded from `propers.txt` and `lessons/<date>.txt` with proper versicle & response dialogs ("Here endeth the First/Second Lesson." / "Thanks be to God.").
  - Daily Collect of the Day dynamically loaded from `propers.txt` replacing placeholder rubrics, plus minor hour collects for Terce, Sext, and None.
  - Seasonal and common Invitatory Antiphons before and after the Venite, Benedictus Antiphons, Magnificat Antiphons, and appointed Office Hymns (*Iste Confessor*, *Deus Tuorum Militum*, *Aeterna Christi Munera*) with versicle and response pairs.
- Restored card detail inspector and stat/cost/synergy breakdown banner in the *Codex: Ink & Iron* victory card reward picker (`apps/codex/screens/reward.lua`), including dedicated miniature frame presentation when "Skip Card Reward" is selected.
- Fixed hardware button navigation in the *Codex: Ink & Iron* Grimoire (`apps/codex/screens/deck_view.lua`): Top and Bottom side buttons (`up`/`down` and `page_back`/`page_forward`) continuously cycle through all 4 tabs (`Deck` -> `Discard` -> `Relics` -> `Menu`), while Left and Right front buttons navigate pages within lists and options in the Menu.
- Corrected button movement mapping in *2048* so the physical front buttons (labeled "Up" and "Down" on the bottom bar) move tiles up and down, and top side buttons move tiles left and right, matching on-screen button hints.
- Overhauled *Tetris* and `LuaAppActivity` to faithfully match the original C++ implementation and eliminate input lag:
  - Migrated `LuaAppActivity` screen refresh to the FreeRTOS background `renderTaskLoop()` via `Activity::render(RenderLock&&)` protected by `luaMutex_`, preventing synchronous 150-250ms main-loop display freezes and eliminating dropped or delayed button presses during real-time game loops.
  - Fixed duplicate button release dispatching on side buttons (`BTN_UP` firing both `up` and `page_back`, and `BTN_DOWN` firing both `down` and `page_forward`), preventing double-rotation (180°) and dropped inputs.
  - Restored authentic visual styling from the original C++ `TetrisActivity`: light gray dither background surrounding the solid white board, 2px outer black outline, center dot grid on unoccupied cells, and distinct custom fill styles for all 7 tetrominoes (Solid Black, Solid White, Segmented White, Solid Light Gray, Segmented Light Gray, Solid Dark Gray, Segmented Dark Gray).
  - Restored full original game mechanics: natural fit and left/right wall-kick rotation on `Up` / `PageBack`, "RESET GAME?" confirmation modal on `Down` / `PageForward`, continuous 50ms hold-to-drop on `Confirm`, 150ms solid-black line clear flash animation before rows collapse, rounded Next piece preview box, and persistent game state saving on exit and restoring on launch.
  - Integrated hardware RTC (`HalClock`) with user-configured UTC time zone offset (`smudge.get_date()`), replacing the Jan 1, 1970 fallback date.
  - Refactored text parser and paginator to use flat string prefix encoding and 1D page arrays, reducing Lua heap allocation by over 70% and preventing memory crashes when reading long liturgical offices (Mattins, Compline, Evensong).
- Resolved sprite rotation issue in `smudge.draw_sprite()` / `smudge.sprite()` where external 1-bit `.raw` bitmaps were rendered sideways by 90° clockwise; raw bitmaps are now drawn in direct row-major logical orientation, restoring proper upright orientation for card suits (Spades and Clubs) in *Blackjack* and all monster/relic artwork in *Codex: Ink & Iron*.
- Resolved Out of Memory (`[Application Error not enough memory]`) failure when launching *Codex: Ink & Iron* on memory-constrained ESP32-C3 devices (Xteink X3/X4):
  - Architected a dynamic screen-overlay loader (`set_screen`) with eager garbage collection between screen transitions, ensuring only the active screen (`screens/title.lua`, `screens/combat.lua`, `screens/reward.lua`, `screens/shop.lua`, `screens/scriptorium.lua`, `screens/deck_view.lua`) resides in memory at any given time.
  - Converted card and relic catalogs to compact array-backed tuples with shared metatables, separated monster definitions into combat-local storage, and isolated narrative event chambers, reducing peak startup heap consumption from 155 KB down to under 50 KB.
  - Optimized the C++ firmware Lua runner (`LuaRunner`): trimmed loaded modules to essential standard libraries (`base`, `table`, `string`, `math`, `os`), immediately freeing 12+ KB of DRAM on every app launch; added an automatic emergency double-GC sweep and memory reallocation retry before reporting out-of-memory errors.
  - Synchronized both `.crosssmudge/applications/codex/` and `fs_/.crosssmudge/applications/codex/` package distributions.
- Fixed *Codex: Ink & Iron* combat screen stability and layout:
  - Removed redundant total values in parentheses (e.g. `(9 tot)`, `(8 tot)`) from couplet descriptions, preventing text overflow across card borders and improving readability in the hand cards and card inspector.
  - Corrected Floor 1 enemy identity to **Ink Imp** (goblin perched atop an inkwell, 22 HP) matching the woodcut artwork, restoring the intended 12-monster catalog progression.
  - Fixed combat freeze/crash caused by nested table unpacking in text wrapping (`smudge.wrapped_text()`).
  - Added white background card fill to `draw_card_item()` and tuned combat spacing using dynamic line heights to prevent overlapping text and dividers.
- Corrected directional movement mapping in *Sudoku* so Up/Left and Down/Right match physical button expectations while preserving display button labels.
- Enlarged *Blackjack* playing cards from 64x90 to 84x120 for clearer visibility and balanced vertical layout on 480x800 e-paper displays.
- Fixed button width and woodcut plate spacing on the *Codex: Ink & Iron* title screen, eliminating duplicate frame borders and preventing "Continue Delve" text from overflowing the button boundary.
- Restored authentic native app icons for all modular packages in the Applications menu, matching native high-resolution 32x32 graphics with dithered selection backgrounds.
- Permanently removed the extraneous Tally Counter application from firmware and package catalogs.
- Restored authentic visual designs, layouts, and exact button mappings across all applications:
  - **Blackjack**: Restored 32x32 playing card suit bitmaps (Spades, Clubs, dithered Hearts/Diamonds), card outlines, and exact phase button mappings (`Back`, `Deal`, `- Bet`, `+ Bet` in betting; `Back`, ``, `Hit`, `Stand` in player turn; `Back`, `Next`, ``, `` on round over).
  - **Dice Roller**: Restored polyhedral wireframes (Coin, D4, D6, D8, D10, D12, D20 with inner wireframe), die name below shape, wrapped roll history, and buttons `Back`, `Reset`, `Dice`, `Roll`.
  - **Divine Worship: Daily Office**: Restored 7-hour liturgical menu list with active date banner and feast liturgical color, formatted prayer reader with versicles, responses, and rubrics, and buttons `Back`, `Select`, `Up`, `Down` and `Back`, ``, `Prev`, `Next`.
  - **Holy Rosary**: Restored 5-decade loop with connecting lines, drop strand, active bead highlight, 64x64 crucifix icon at base, and buttons `Back`, `Reset`, `Prev`, `Next`.
  - **Sudoku**: Restored hardware button labels to `Back`, `Select`, `Up`, `Down`, and corrected directional movement mapping so the bottom "Up" button moves the cursor up, bottom "Down" button moves the cursor down, and top buttons navigate left and right columns.
  - **Tetris**: Restored real-time gravity game loop in `on_update()` using `ink.millis()`, hold-to-drop on Confirm, 7 distinct tetromino fill styles, next piece preview, and buttons `Back`, `Drop`, `Left`, `Right`.
  - **Wordle**: Added on-screen `reset` button below keyboard with modal confirmation dialog, auto-advance cursor to `ok` upon entering 5 letters, and restored buttons to `Back`, `Select`, `Up`, `Down`.
  - **Codex: Ink & Iron**: Restored full illuminated manuscript deckbuilder, 128x128 monster woodcut plates, intent, rhythm, illuminated card inspector with hatching, authentic cards in hand with manuscript borders, cost/meter badges, couplet bonuses, Grimoire tabs, Scriptorium shop, 12 relics, 30 cards, and combat buttons `End Turn`, `Play`, `< Card`, `Card >`.
- Fixed floating-point conversion crash in Lua drawing bridge (`number has no integer representation`) by safely rounding coordinates and dimensions for all graphics primitives.
- Resolved Lua heap limit exhaustion warnings during gameplay by expanding dynamic sandbox limits (up to 192 KB on ESP32-C3 and 2 MB on PSRAM/Simulator), implementing incremental garbage collection in the game loop, and refactoring *2048* to use preallocated scratch arrays with debounced state saving on exit.


- Redesigned The Grand Inquisitor boss artwork in *Codex: Ink & Iron* as an ominous witch-hunter judge with an inquisitor's broad-brimmed peaked hat, dark hooded mantle, iron gorget, and executioner blade, removing the previous papal mitre and clerical robes.
- PNG, XTC, and image-dithering scratch buffers use fewer heap allocations to reduce fragmentation.
- The shared settings catalog keeps its initial allocation instead of retaining unused vector capacity.
- SPI SD-card transfers are batched through the ESP32 hardware FIFO for faster reads.
- SD-card font prewarming releases temporary lookup buffers before allocating large glyph bitmaps.
- UC8179 grayscale images use a slightly longer waveform for stronger midtone separation.
- EPUB image preparation writes extracted data in chunks and reuses two cached images on PSRAM readers.
- Font menus and the web portal use a persistent catalog that loads one family's details at a time, preventing crashes with larger font collections.
- Web portal pages reuse browser-cached content after checking for firmware updates.
- Rapid queued EPUB page turns defer text anti-aliasing and image loading until the final page, making intermediate turns faster.
- Grayscale sleep screen images use the panel's direct grayscale waveform where supported, which folds the base frame into the grayscale pass instead of refreshing the screen separately first.

### Fixed

- Replaced solid black title rectangle on 1-use Power cards (`Illumination: Flow`, `Illumination: Bastion`, `Illumination: Fury`) in *Codex: Ink & Iron* with medieval woodcut diagonal hatching (`///`) and a 1px white knockout text halo, ensuring complete readability on 1-bit monochrome displays.
- Inverted 1-bit coordinate orientation and bitmap packing for the *Codex: Ink & Iron* launcher icon and monster sprites, restoring clean black ink rendering on 1-bit e-paper displays.
- Card draw turn bonus from `Illumination: Flow` in *Codex: Ink & Iron*, proper exhaustion of 1-use Power cards upon play instead of being recycled into the discard pile, and run state saving on sleep transitions.
- Divine Worship: Daily Office reader memory optimization using a contiguous text pool and compact line offsets, preventing heap fragmentation and out-of-memory crashes on ESP32-C3 devices when rendering long offices such as Mattins.
- Divine Worship: Daily Office prayer reader typography, section heading rules, rubric indentations, right-edge text margins, page-height budgeting to prevent button overlaps, and app icon portrait orientation.
- Divine Worship: Daily Office Little Hours (Prime, Terce, Sext, None) and Compline now correctly preserve their fixed traditional psalms, Little Chapters, and collects instead of being overridden by the 30-day psalter, with optional Collect of the Day recitation added to Terce, Sext, and None.
- The web EPUB optimizer now accepts books that use standard Adobe or IDPF font obfuscation, while leaving DRM-protected books unchanged.
- Frontlight schedule time pickers now use the compact number keypad from Go To screens.
- X4 Classic's left/right tilt direction labels now match the physical page-turn direction.
- Touch keyboards no longer show button-only hold and navigation hints.
- The web settings page no longer offers the Up + Down shortcut on devices that cannot use it.
- OPDS Wi-Fi selection and search entry stay awake while the user is actively choosing or typing.
- USB Drive exits cleanly when a connected host is unplugged without ejecting first.
- EPUB ordered lists show numbers, respect marker-free styles, and retain their container indentation.
- EPUB chapter layout releases rebuildable font caches first, reducing low-memory failures on X3/X4.
- KOReader Sync uploads retain exact text-node positions, including zero offsets and UTF-8 text.
- Saved clipping highlights now retain Focus Reading's custom-font glyphs instead of showing replacement characters.
- EPUB dictionary lookup can select an individual part of a hyphenated word.
- Short Power-button frontlight and touchscreen shortcuts in EPUB books no longer run the configured long-press action.
- Silent restarts now preserve the frontlight state instead of applying wake or schedule settings.
- The Home button now returns from Customize Status Bar to the previous menu instead of leaving the reader.
- OPDS book downloads can follow secure redirects without sharing catalog credentials with the download host.
- Larger EPUB stylesheets work on PSRAM readers, including rules that hide duplicate images.
- JPEG-heavy EPUBs can use PSRAM for decoding on supported readers, leaving internal memory available for reading.
- Importing CrossPoint settings preserves tap and swipe modes without carrying over a stale reader touchscreen lock.
- Saved clippings no longer highlight unrelated single words at page boundaries when matching text after a layout change.
- Quick Lock sleep now respects the configured short Power-button wake behavior.
- Quick Lock now clears when the device wakes after an automatic sleep timeout.
- EPUB content marked with the HTML hidden attribute no longer appears in the reader.
- EPUB paragraphs without source indentation no longer gain a synthetic first-line indent.
- End-of-book selection remains consistent during concurrent redraws.
- Image dithering reports low-memory failures instead of aborting during buffer allocation.
- The debugging monitor plots CrossInk heap and PSRAM logs separately; ZIP failures identify the affected EPUB entry.
- Many progressive JPEG images that store brightness and color in separate scans now render instead of appearing blank.
- PNG sleep overlays preserve four evenly spaced grayscale levels on supported displays.
- Exiting Calibre Wireless on X4 now returns Home with one clean screen refresh instead of repeated blank flashes.
- Manage Fonts no longer crashes after Wi-Fi connects on ESP32-S3 readers.
- Editing font settings from the top drawer's global settings within a book now applies those changes when no per-book font settings exist.
- Per-book reading stats now write to a backup file first.
- Paragraph-alignment previews remain available on text-heavy pages instead of disappearing when the preview sample is full.
- Quick Actions assignments stay visible in button-combo settings, and X4 Classic can use the Up + Down shortcut.
- Sync Progress from the reader menu opens KOReader setup when credentials have not been configured.
- Button-combo settings no longer offer Sleep because the same combo cannot wake the reader.
- EPUB variation selectors no longer appear as missing-glyph boxes after otherwise supported symbols.
- Cancelling Word Spacing on button readers no longer briefly changes the slider value.
- The File Browser now displays decomposed Hangul and accented filenames copied from macOS correctly.

## [v1.5.1] - 2026-09-10

### Added

- Xteink X4 Pro and X4 Classic support, including device-specific firmware and USB Drive access; X4 Pro also supports direct USB file transfers.
- The built-in EPUB optimizer can keep cover art in color while still resizing it to a reader-safe baseline JPEG.
- Custom BMP boot screens, selected in the File Browser or rotated from `/bootscreen` or `/.bootscreen`; sleep screens can also be selected from any folder.
- Quick Lock, assignable button combinations, and shortcuts for Previous Page and Nearby Position Sync. Quick Actions can also be assigned to Power + Up and X4 Pro Home-button gestures.
- Configurable touch page-turn gestures, pinch-to-resize text, two-finger rotation and swipe actions, and a tap-to-hide reader status bar.
- Selectable keyboard layouts, switchable from the keyboard's language key.
- Clippings from dictionary lookups on touch devices, plus selection of text inside EPUB tables.

### Changed

- Touch EPUB readers use a half-height, five-tab menu. Sticky opens the menu with a swipe up and book details with a swipe down; X4 Pro frontlight controls include reading stats and reader shortcuts.
- Screen margins have separate Top/Bottom and Left/Right controls, adjustable up to 200 pixels.
- Night Mode applies system-wide on ESP32-S3 devices; frontlit readers can disable periodic full-screen refreshes.
- Waking keeps the sleep screen visible until the reader or Home is ready, unless a custom boot screen is enabled.
- Font choices show available point sizes, Download Fonts replaces the font-manager label, and Wi-Fi passwords are visible during entry.
- Reader controls, shortcut pickers, touch targets, and File Browser settings are easier to reach; Book Options is last in the button reader menu.
- EPUB indexing, image decoding, fonts, and reading-state updates use fewer resources; the web optimizer prefers natural boundaries when splitting chapters.

### Removed

- The undocumented X4 Pro power-button double-click frontlight toggle.
- Built-in reader-font emoticons and hand gestures; SD-card fonts retain emoji fallback support.

### Fixed

- Clipping highlights stay aligned after font changes, retain multi-paragraph text, and remain readable in Dark Mode. Selection stays on its final page, and browsing saved clippings responds reliably.
- Dictionary lookup respects landscape controls and selected fonts, handles repeated lookups more reliably, and returns to the reader cleanly when dismissed.
- EPUB tables retain column widths and wrap long labels; mixed-direction text, Arabic/Persian shaping, ruby annotations, and footnote styling render correctly.
- EPUB contents links, split-chapter navigation, footnote resumes, and end-of-book exits preserve the intended reading position.
- Large EPUBs, image pages, and SD-font preparation recover more safely from limited memory and SD read errors.
- XTC/XTCH page turns no longer overlap, tables of contents show all entries, and covers retain their grayscale detail. TXT font-size controls and Home progress work reliably.
- Quick Resume, custom sleep images, transparent overlays, and X3/X4 wake refreshes avoid blank screens, grid artifacts, and lingering images.
- Long-press shortcuts no longer trigger an extra action on release; Quick Actions, Quick Lock, and Dark Mode shortcuts respond consistently.
- Touch scrolling, page gestures, font-download cancellation, and reader settings behave reliably across orientations and UI scales.
- KOReader Sync preserves orientation and settings, handles missing remote positions, and avoids repeated screen flashes during network transitions.
- Nearby sync, OPDS search, file listings, and image actions handle input and errors more reliably; Calibre Wireless shows the full IP address.
- S3 sleep, charger detection, and power-button wake behavior are more reliable. USB Drive recovers from storage failures, USB transfers avoid watchdog errors, and updates reject firmware for a different board.
- Book-specific settings stay separate from global defaults, and Recent Books and KOReader credentials survive network restarts.

## [v1.5.0] - 2026-08-08

### Added

- X4 Pro readers can lock the Home button while reading, with a Power-button shortcut to toggle it.
- End-of-book suggestions can now be opened directly by tapping their rows on touch devices.
- Quick Actions lets readers assign up to five favorite reader commands to one Power, Back, or Menu shortcut.

### Fixed

- EPUB tables now lay out a row at a time in both Incremental and Full Section indexing, keeping regular tables readable without whole-table buffering.
- Touch support for Seeed Studio Sticky
- Nearby File Transfer can send EPUB, TXT, XTC, XTCH, PNG, and BMP files directly between two CrossSmudge devices without a Wi-Fi network.
- Recent Books and image-file long-press actions can send files directly to a nearby CrossSmudge device.
- Dictionary lookup and lookup history
- EPUB books can use a dedicated SD-card dictionary font while keeping a different reader font.
- EPUB books can set a dedicated dictionary font size independently of the reader font size.
- Dictionary font and size defaults can be set globally from Settings > Reader > Font Options, with per-book choices still taking precedence.
- Reusable dictionary SD-font builder with IPA coverage and per-family ZIP packaging
- RTC-enabled devices can now choose the date format and numeric separator shown in headers from Settings > System > Device.
- The web EPUB optimizer now splits oversized chapters into memory-friendlier sections before sending them to the reader.
- Reader indexing can now use `Incremental` or `Full Section` mode globally or per book; changing modes keeps the current chapter readable and applies when the next chapter needs indexing.
- Look Up Word can now be assigned to short- and long-press Power button shortcuts.
- EPUB readers can now choose from five word-spacing levels, from normal through extra-wide.
- EPUB inline-image pages on X3 now use the grayscale-aware display base before the image grayscale overlay, reducing the moment where images appear too dark before settling.
- EPUB publisher small-caps styling now renders ASCII lowercase text as smaller capital letters without needing extra font files.
- When incremental EPUB indexing runs out of memory at the first unindexed page, the reader now silently restarts once and resumes the book with a fresh heap.

### Changed

- PSRAM-equipped readers now keep EPUB grayscale and image-cache working buffers in external memory, preserving more internal RAM for layout and reducing repeated SD reads on image pages.
- Reader font sizes now persist as actual point sizes, keeping the closest matching size when font families or installed files change.
- SD-card fonts now include the built-in reader fallback stack for common symbols, emoji, and selected CJK glyphs while retaining Noto Sans fallback coverage.
- Downloadable SD-card fonts are now rendered with the same darker anti-aliasing as the built-in reading fonts.
- Full-section EPUB indexing now prepares one-page chapters and direct jumps to a chapter's last page, while avoiding repeated checks after the next chapter is ready.
- EPUB grayscale rendering now reuses its 8 KB strip buffer across stable pages, reducing repeated heap allocation and release during long reading sessions.
- Reading progress is now saved in batches during ordinary page turns, immediately after layout changes, and when leaving a book, reducing repeated SD-card writes without carrying stale pagination into the next session.
- SD-card font discovery now waits until a custom font is selected or font settings are opened, reducing SD-card work during normal startup with built-in fonts.
- EPUB page turns using SD-card fonts now prepare the next page's glyphs while the reader is idle.
- Dictionary lookups now reuse open index files for stem matching, reducing repeated SD-card work after a miss.
- The web file manager now batches directory listings into fewer network packets, improving large-folder response time.
- Firmware releases now identify the supported device type: X3/X4 or Seeed Sticky.
- Image-heavy EPUB chapters now index by reading image headers first and extract each full image only when its page is shown.
- EPUB books with repeated byte-identical stylesheets now parse each unique stylesheet only once when building caches.
- SD-card fonts now reuse their page-sized glyph buffers, reducing heap fragmentation during long reading sessions.
- Firmware builds now prioritize usable heap over oversized system timer stacks and maximum WiFi throughput, leaving more memory for reading and network operations.
- Downloaded-font size range options now show their actual point-size ranges instead of firmware build names.
- KOReader Sync and authentication, OTA updates, and OPDS browsing now restart into a lightweight network mode that leaves reader and Home data unloaded, providing more contiguous memory for WiFi and secure connections.
- The web file manager can now delete non-empty folders recursively and, when hidden files are shown, remove hidden or system-managed SD card items after confirmation.
- SD-font, OPDS catalogs, and other unneeded settings now stay out of memory while reading unless their settings are open.
- EPUB books can now keep more saved clippings without loading every clipping's text into memory while reading.

### Removed

- The font download manager no longer offers a Download All action; fonts can still be downloaded individually or updated together.

### Fixed

- Book menu tab navigation, popup scrolling, customized Reading Stats hints, and short button presses after low-power mode now work reliably.
- Sleep screens now honor the current orientation, avoid X4 transition flashes, fall back to a valid wallpaper when needed, and handle low-memory image decoding without rebooting.
- Choosing Set Cover uses the selected image in place, and Home no longer repeatedly generates missing EPUB covers.
- Finished-book suggestions are now collected before an EPUB is moved to `/Read`.
- Manage Fonts now opens and scans large catalogs more safely on X3/X4, reports low-memory failures instead of restarting, and returns to Font Options when cancelled.
- Network screens refresh cleanly on X4; long errors wrap correctly; saved Wi-Fi networks and KOReader connections recover more reliably after restart or a missing address.
- Translated Wi-Fi and clock labels no longer truncate text or time values, and clock sync no longer risks a reboot while saving settings on memory-constrained X3/X4 devices.
- KOReader Sync no longer crashes during time setup, re-triggers while connecting, or loses precise EPUB positions; CrossPoint-only data stays on the official CrossPoint Sync server.
- Firmware updates reject images for the wrong chip family, and saved Wi-Fi settings safely handle concurrent access and corrupted values.
- EPUB opening, reflow, and background indexing now handle fragmented memory more safely, retry recoverable work, remain responsive to input and setting changes, and show useful errors instead of rebooting or silently returning Home.
- Low-memory EPUB grayscale and sleep rendering now fall back safely without leaving stale display content.
- Full-section indexing preserves more memory for large chapters and cancels speculative work on page turns, keeping the reader responsive.
- SD-card font and clipping work now release temporary data at the right time, preserving memory for reflow, dictionary use, covers, and thumbnails on X3/X4.
- EPUBs with book-specific built-in fonts no longer load an unnecessary global SD-card font, and custom fonts retain ligatures.
- EPUB styling choices apply before style caches load; CSS-heavy books use less temporary memory; and disabling Embedded Style consistently skips unused stylesheet work.
- EPUB layout now keeps CJK ruby and spaces, Russian paragraph continuations, Focus Reading, underline/strikethrough runs, and right-to-left text correct.
- EPUBs with flowing `<br>` elements, image-led or decorative chapter headings, unsupported images, and dense final pages now lay out without excess gaps, clipping, dropped images, or misleading low-memory warnings.
- EPUB footnote and cross-reference previews now show complete notes, including targets in the middle of a paragraph.
- Saved EPUB positions, clipping highlights, and selections now stay accurate after font, orientation, or indexing changes; selections also remain readable in dark mode and on memory-tight pages.
- Dictionary misses can switch dictionaries without leaving the reader, and dictionary read failures now report an error instead of a false “not found.”
- Reader popups, KOReader Wi-Fi labels, Lyra battery headers, and the sleep message now remain correctly oriented and positioned.
- Manual refreshes preserve EPUB and TXT text anti-aliasing; XTC and XTCH status bars show the configured time-left estimate.
- Watchdog panics with captured diagnostics open crash reporting, while reset-only events return normally; power-button wake timing no longer depends on SD-card startup.
- The web file manager and uploads now handle simulator/device ports and stalled connections safely; unsupported settings stay hidden, and the optimizer removes empty chapter stubs without breaking table-of-contents links.

## [v1.4.0.1.1] - 2026-08-01

### Added
- CrossSmudge custom branding and ink-spatter splash graphics.
- Applications menu 
- Wordle
- Dice sim
- 2048
- Rosary
- Sudoku
- Life Counter
- App setting to re order, or hide/show apps
- Custom OTA update endpoint pointing to CrossSmudge GitHub releases.

## [v1.4.0.1] - 2026-07-28

### Added

- Updates to support Xteink device detection so the correct display panel driver is used.

## [v1.4.0] - 2026-07-10

### Added

- Dashboard UI theme for the Home screen, showing the current book cover and reading stats.
- Nearby Position Sync for sending or applying the current EPUB position between two CrossSmudge devices over ESP-NOW.
- Web EPUB optimizer support for CrossSmudge location metadata, so optimized EPUBs can keep better progress and stable page numbers.
- Reading Stats support for XTC and XTCH books, including reader menus, Home and sleep screen stats, mark finished, delete stats, and preserving stats when clearing book caches.
- Web file manager image previews, so PNG, JPEG, BMP, GIF, and WebP files can be viewed inline before downloading.

### Changed

- Large EPUBs, SD-card font-heavy books, and cover thumbnails now open, index, and generate more reliably under low-memory conditions.
- Home and sleep screens now load more cover and thumbnail data only when needed, reducing reader startup work and reusing cached cover data where possible.
- Built-in reader font choices have been reduced to Lexend Deca and Bitter, reducing firmware size while keeping fallback glyph coverage.

### Removed

- Teensy firmware builds are no longer produced for releases or release candidates.

### Fixed

- EPUB render-mode and Safe Mode toast messages now clear reliably, even when the reader is low on memory.
- EPUB Reading Stats no longer drops unsaved page-turn counts after viewing the stats screen mid-session.
- KOSync is more reliable with many SD-card fonts installed, reducing low-memory failures during secure sync requests and uploads.
- Web file manager actions now handle filenames with special characters safely and reject unsafe rename characters before saving.
- Auto Turn interval settings and related action prompts opened from long-press shortcuts now stay open after releasing the shortcut button.
- EPUB footnote previews no longer show clipped status-bar labels or misleading reader progress indicators, and clipping selection now works from footnote previews.
- Font selection no longer reopens the font preview after choosing a font.
- EPUB chapters with stale publisher style data now rebuild it instead of opening without the book's styling.
- Large SD-card font EPUBs no longer overlap characters after font or line-spacing changes, and clipping selection can fall back to a built-in UI font when needed.
- EPUB cover and thumbnail generation is more reliable with custom SD-card fonts selected and optimized books under low-memory conditions.
- Web EPUB optimizer now preserves more PNG and SVG artwork on-device, including transparent PNGs, dividers, and images in malformed or XML-declared chapters.
- Unsupported SVG images in EPUB chapters are now skipped silently instead of triggering low-memory image warnings.
- Nearby Position Sync now silently restarts back into the reader after using ESP-NOW, matching other WiFi sync flows and reducing post-sync memory fragmentation.
- EPUB page cache loading now uses fewer small heap allocations, reducing fragmentation-related reader failures.
- EPUB grayscale page turns on X3 now use the grayscale-aware display base, reducing the moment where new text appears too dark before the anti-aliased overlay finishes.
- EPUB chapters with many inline anchors, footnote links, malformed XHTML, large publisher styles, or SD-card fonts are less likely to fail or get stuck on the indexing screen.
- EPUB opening and image rendering now recover from more low-memory conditions instead of rebooting, including landscape image pages and books that need lighter render modes.
- EPUB clipping selection now follows right-to-left line order when selecting Hebrew and other RTL text.
- Lyra Carousel no longer shows a blank carousel after returning from WiFi-related File Transfer screens and moving between the menu row and book row.
- Generated SD-card font packages now include the same core glyph coverage as built-in reader fonts.
- Manage Fonts no longer crashes while loading or reloading large SD-card font lists.
- Minimal Home no longer swaps to another recent book when returning from Settings when Back button is mapped to the first button.
- Cancelling a font download now stops on the first Cancel button press instead of needing several presses.
- The `Inverted` sleep cover filter now keeps book covers unchanged on Minimal and Dashboard sleep screens while switching the background to white.
- Rare EPUB open or thumbnail crashes during ZIP decompression are fixed.

## [v1.3.4] - 2026-06-24

### Added

- File Browser now indexes large SD-card folders so directories with many books can be browsed without loading every filename into memory at once.
- EPUB text clipping with saved highlights, clipping lists, and Kindle-style `/My Clippings.txt` export.
- `Create Clipping` is now available as a reader shortcut for short/long Power, long-press Menu, and long-press Back actions.
- Per-book EPUB options for font, layout, styling, reading aids, and render modes, including `CrossSmudge Default`, `Balanced`, and `Light` modes for difficult books.
- Arena allocator (`lib/Memory/Arena.h`) for burst-then-discard allocation patterns - reduces heap fragmentation during EPUB parsing and page layout over long reading sessions.
- Optimized EPUBs now store location metadata at `META-INF/x-locations.json`.
- X3 SD-card writes now use the RTC for file timestamps when the clock is available.

### Changed

- The EPUB reader menu now splits the growing menu into 3 screens, labels per-book settings as `Book Options`, and avoids showing duplicate `Orientation` controls.
- The `Inverted` sleep cover filter now flips Minimal and Reading Stats sleep screens to black text on a white background.

### Fixed

- Quick Resume no longer shows a blank page after EPUB next-chapter indexing.
- Calibre Wireless transfer status no longer stacks the last received-file message on top of the upload percentage.
- X3 Tilt Direction now labels left/right choices as `Left-Right` and `Right-Left`, with existing left/right preferences migrated to keep the same physical tilt behavior.
- EPUB layout now honors publisher page-break CSS, avoids stretching justified spaces before closing punctuation, and keeps large CSS rule sets in a smaller disk-backed lookup cache.
- EPUB first-open conversion now uses more compact OPF manifest lookups and streams cover-wrapper parsing to avoid large temporary heap buffers on books with huge manifests.
- EPUB chapters that run out of memory now retry with `Balanced`, `Light`, and final `Safe Mode` rendering before showing an error, apply the same fallbacks during next-chapter pre-indexing, and let book action menus reset a book's reader settings if Safe Mode still cannot open it.
- EPUB reader font-size changes now restore the current chapter position by content instead of jumping far backward after re-indexing.
- Reading Stats now use the reader's last live book time-left estimate instead of showing a separate fallback estimate.
- Per-book reading stats now migrate compatible legacy `stats.bin` files into the `stats_v5.bin` flow instead of resetting when only the old filename exists.
- Lyra Carousel Home menu rendering now avoids extra label allocations that could crash builds under low memory.
- Lyra Carousel Home cover refresh no longer risks a reboot when memory is tight after returning to or selecting a recent book.
- EPUB image-heavy chapters no longer risk a reboot while saving their reading cache under low memory.
- TXT readers now stay open when pressing a page-turn button at the end of the file.
- Long-press reader shortcuts that open another screen no longer close or confirm it again when releasing the shortcut button.
- RoundedRaff's header battery icon and percentage now sit lower to avoid clipping at the top edge.
- Lyra Carousel now keeps the Home header current when rendering the menu or restoring cached carousel frames, preventing stale battery and clock values while navigating between books.
- Web file manager multi-delete now handles larger selections without failing after a small batch.
- Portuguese EPUBs now use Portuguese hyphenation rules instead of leaving long words unhyphenated when Hyphenation is enabled.
- Progressive JPEG EPUB covers now render more smoothly in generated cover and thumbnail BMP assets.
- EPUB section layout now flushes long text runs earlier when Focus Reading or Guide Dots are enabled, reducing low-memory failures on difficult books.
- Footnotes in EPUBs with very large shared notes sections no longer cause long stalls when opened.
- Firmware updates now follow GitHub asset redirects before streaming the install.
- Tiled grayscale rendering now serializes display transfers on the shared SPI bus to avoid display glitches during SD activity.

## [v1.3.3] - 2026-06-13

### Added

- `File Browser Display` in `Settings > System > Files & Cache` for choosing one-line or two-line file browser rows across all themes, while preserving Minimal users' existing two-line display on upgrade.
- `Hide File Extension` in `Settings > System > Files & Cache` for expanding file-browser filenames by hiding the right-side extension label.
- Device Name in Settings > System > Device for customizing the KOReader Sync and Nearby Stats Sync device label.
- Additional shortcut options and new ability to add custom shortcuts for Long-press Back Action.
- Delete Reading Stats actions in the EPUB reader and book action menus for clearing one book's stats without deleting its cache.

### Changed

- CrossSmudge settings now save to `/.crosspoint/crosssmudge-settings.json`, with a one-time fallback migration from `/.crosspoint/settings.json`, so switching between firmware builds is less likely to reset preferences.
- The X3 clock visibility setting is now phrased as `Hide Clock`, with existing `Show Clock` preferences migrated to the matching hide behavior.

### Fixed

- RoundedRaff's date shown in settings now sits lower on X3 devices instead of overlapping the battery.
- Clear Bookmark List now asks for confirmation before deleting a book's bookmarks.
- Clear Reading Cache now preserves per-book reading stats while continuing to leave all-time reading stats untouched.
- Moving finished EPUBs to `/Read` now consistently preserves reading progress, per-book stats, bookmarks, and resume state.
- Book settings option lists now return to the submenu they were opened from when pressing Back.
- Lyra Carousel now refreshes its cached Home icon row after OPDS, Reading Stats, or Bookmarks icons appear or disappear.
- KOReader Sync failure screens now wrap long error messages and shut down WiFi cleanly before returning to the book.
- Sleep Screen > Cover now generates the current book cover on demand instead of falling back to the dark sleep screen when the setting is changed after opening a book.
- File Browser now previews PNG images instead of trying to open them as EPUBs, and hides common macOS and Windows metadata files.
- File Browser now refreshes immediately after falling back to the root folder from a stale saved path.
- File Browser now stops loading oversized folders before low memory can crash the device and shows a memory error instead.
- TXT reader long-press Power page turns now work when Long Power Button is set to Page Turn.
- SD-card font read failures no longer risk a reboot while cleaning up the failed file read.
- Page Overlay sleep screens no longer force EPUB chapters to re-index after waking.
- Page Overlay sleep screens now use the current screen as the overlay background outside the reader instead of trying to rebuild a stale book page.

## [v1.3.2] - 2026-06-10

### Added

- Current date in the top-right Settings header on X3 devices.
- Dark Reader Mode for EPUB and TXT reading screens, plus shortcut actions for the power button and front-button long press.
- File Browser long-press folder action for choosing a custom sleep-image folder instead of only `/.sleep` or `/sleep`.
- Expanded X3 Reading Stats, including streaks, time charts, editable dates, all-time backups, reset controls, an idle-time threshold, and the `Minimal Stats` sleep screen.
- `Reset Reading Pace` in the EPUB reader menu when Time Left is enabled, for clearing only the time-left pace estimate while keeping book reading stats.

### Changed

- Display, Reader, and Controls settings now open list menus instead of cycling through options one by one.
- The X3 clock visibility setting is now phrased as `Hide Clock`, with existing `Show Clock` preferences migrated to the matching hide behavior.
- Reading time and time-left pace tracking now ignore page intervals longer than the configured idle-time threshold.
- Web portal pages now use shared templates, stylesheet, and logo assets, reducing on-device page size and improving browser caching.
- Already-cached EPUBs now open directly to the first page without an extra book-loading popup refresh.
- Reader font-size choices now show point sizes like `10 pt` instead of names like `Tiny`.

### Fixed

- Inverted reader menus now honor orientation-aware side-button navigation.
- EPUB book time-left estimates now wait for more session pace samples and use a progress-based floor after pace data exists, reducing swings from unusually short or long pages.
- Deleting an EPUB book cache now preserves that book's reading stats and pace data.
- X3 clock settings now have clearer UTC offset editing, and `Sync Date/Time` can use saved WiFi networks automatically.
- Home, Lyra Carousel, WiFi setup, and SD-card font flows now release memory more aggressively to avoid freezes or crashes on constrained builds.
- Vietnamese settings labels no longer show replacement diamonds after generated translation offsets shifted.
- KOReader Sync now lands correctly at chapter starts and shows more specific connection guidance.
- EPUB bookmarks saved under the old unstable path hash now show up again, including for books moved to `/Read`.
- SD-card font downloads now use versioned direct S3-hosted HTTP endpoints with CRC validation, avoiding GitHub release redirects and ESP32-C3 TLS stalls when loading the font catalog.
- EPUB text blocks now keep the book's alignment style when an inline image appears before the text.

## [v1.3.1] - 2026-05-28

### Added

- EPUB reading-position improvements, including bookmark anchors, bookmark preview snippets, and optional chapter/book time-left estimates.
- Nearby Reading Stats sync with separate totals for this device and all synced CrossSmudge readers.
- Per-server OPDS filename settings so downloaded books can use either Author - Title or Title - Author.
- EPUB render heap diagnostics that include the largest allocatable block, not just total free heap.

### Changed

- Moved the X3 reader clock into a new top-centered status bar and moved clock settings to Settings > System > Device.
- Reworked Display, Reader, Controls, in-reader options, and larger System settings groups so related options open as submenus.
- Improved OPDS and font download responsiveness by reducing progress-update overhead and temporarily disabling WiFi power saving during transfers.
- Book selection now shows a loading popup before EPUB indexing or cache loading begins.
- Delayed the automatic finished-book prompt until the reader leaves the chapter where they reach 99%.

### Fixed

- WiFi settings screen now keeps the displayed MAC address consistent with the router-visible WiFi address.
- Reader UI issues with inverted menu button hints, Lyra Carousel popups, and Auto Page Turn interval persistence.
- Web uploads and KOReader Sync progress saves now preserve progress, stats, settings, and valid resume data for refreshed book files.
- OPDS low-memory handling now shows a specific parser-buffer memory message and releases SD-card fonts before catalog loading.
- EPUB cache, CSS, table, SD-card font, and allocation failure paths now recover, retry, or stop cleanly under low memory instead of opening unstyled pages, failing unnecessarily, or risking a reboot.
- EPUB text with invisible word-joiner characters no longer shows replacement diamonds for missing font glyphs.
- Clarified the low-memory EPUB image warning so it says some or all images may be missing.

## [v1.3.0] - 2026-05-21

### Added

- Back/Cancel support while downloading books from OPDS catalogs.
- Recent Books long-press menu in both List and Grid views with delete, cache delete, completion, and remove-from-recents actions.
- Minimal sleep screen option that shows the current book cover and reading progress on a dark background.
- More detailed WiFi connection debug logs for scans, selected networks, status changes, disconnect reasons, and timeouts.
- 9pt `Itty Bitty` reader font size, plus build flags for omitting Itty Bitty and Large reader font assets in size-constrained firmware variants.
- In-reader confirmation message when a shortcut turns tilt-to-turn on or off.

### Fixed

- WiFi and OPDS connection-flow edge cases: manual Settings connections now show the connected status before continuing, copied or corrupted saved-password files are rejected before use, OPDS retries show loading before requests, and large OPDS feeds fail safely under low memory instead of rebooting.
- Reader and Home UI polish issues, including landscape status-bar settings, missing Vietnamese labels, File Browser and Lyra Carousel icon alignment, cover thumbnail artifacts, and duplicate Home progress/stat loading.
- EPUB cache and low-memory handling now use stable cache folder keys, migrate older cache folders where possible, rebuild stale section caches, lay out very long text blocks earlier, stream table fallback content when heap is tight, and clarify the warning text.
- Sleep-entry, network, and SD-card font download reliability improvements: cached sleep-screen assets are reused, OPDS pages idle normally after load, the X3 tilt sensor sleeps outside the reader, WiFi power saving is disabled during transfers, WebDAV stack usage is lower, longer stalls are tolerated, interrupted font files are retried, and active reader fonts are freed when needed.
- Remaining reader service edge cases, including an XTC chapter selector crash on memory-constrained builds, SD-card font size selection, SD-card font-size shortcuts skipping manually installed sizes, and KOReader Sync login compatibility with self-hosted servers that return valid JSON on success.

### Changed

- Modified upstream "page-as-sleep" behavior into a new `Sleep Screen > Quick Resume` option, which also keeps `Quick Resume on Timeout` on, and renamed the timeout-only toggle.
- Improved reader and browser menu behavior by moving the Footnotes shortcut above Select Chapter, wrapping long book titles in action menus, and reducing progress-screen repaint work during OPDS and SD font downloads.

## [v1.2.11.1] - 2026-05-15

### Changed

- Removed Medium font size from `xlarge` build to get it below the size limit

### Fixed

- Lyra Carousel is now included by activating the build flag `DCROSSSMUDGE_ENABLE_LYRA_CAROUSEL=1`

---

## [v1.2.11] - 2026-05-14

### Added

- New personal theme: "Minimal"
- Custom sleep timer picker so `Time to Sleep` can be set from 1 to 30 minutes instead of cycling fixed presets.
- In-reader Controls shortcut for customizing buttons without leaving the book.
- Bookmark cleanup shortcuts: hold Select on a bookmark to delete it, or hold Open on a book in Bookmarks to clear that book's bookmark list.
- Confirmation message after deleting a book's cache from the reader or File Browser.
- File Browser long-press action for deleting an EPUB or XTC book's cache.
- Downloaded-font size range setting so SD-card fonts can use compact, default, or large point-size sets.
- File Browser long-press action for marking EPUB books as finished or unfinished.

### Changed

- Hardened deep sleep entry by shutting WiFi down before waiting for the power button to be released.
- Raised the web file-transfer filename limit from 100 to 150 bytes so longer uploaded filenames are preserved.
- Made the in-reader Reader Options menu include the same Reader settings and actions as Settings > Reader.
- Split SD-card font descriptions and supported languages into separate lines in the font download screen.

### Fixed

- Inline EPUB images no longer disappear in landscape when their bottom edge slightly overlaps the screen margin.
- Reduced unnecessary low-memory image suppression for JPEG-heavy EPUB chapters and added CSS heap diagnostics during chapter rebuilds.
- Allowed wider inline JPEG images in EPUBs to render when they still fit the total pixel and heap safety limits.
- SD-card font picker no longer reopens immediately after selecting a font from Settings > Reader > Font Family.
- In-reader font-size changes now work for SD-card fonts.
- In-reader SD-card font changes now rebuild the current EPUB page layout consistently.

## [v1.2.10] - 2026-05-11

### Added

- `Recent Books View` setting so the dedicated Recent Books screen can switch between the classic list and a 3x3 cover grid.
- More flexible reader controls, including orientation-aware front/side button settings, nav-only or all-button front inversion, tilt page turn shortcuts, and side-button long-press rotation actions.
- Per-session auto page turn interval picker with values from 5 to 120 seconds.
- File Browser Home/Back long-press action for toggling hidden files and folders.
- EPUB rendering and diagnostics improvements, including visible `<hr>` separators and heap logs around section rebuilds, image extraction, page serialization, and sleep-cache rebuilds.
- Reader font coverage for block redactions, black-square ornaments, Greek category letters, and turned-comma punctuation (PR #104).
- Simulator tools for testing sleep/wake behavior and smoke-testing common screens and EPUB reader menus.

### Changed

- Reduced Controls settings section spacing so the grouped controls fit better on X3 screens.
- Made front reader long-press actions trigger when the hold delay is reached while normal page turns still trigger on release.
- Used the fast EPUB spine/TOC indexing path for books with 300+ spine entries so heavily split books build `book.bin` faster on first open.
- Allowed the web file manager and WebDAV to browse dot-prefixed hidden files when hidden files are enabled, matching the device file browser.

### Fixed

- Reader button and shortcut behavior, including X3 power-button wake filtering, folder delete long-press timing, and WiFi scan/connect screens that could not be exited while work was in progress.
- RoundedRaff home-menu, keyboard, and button-hint rendering issues so Settings remains reachable and compact labels no longer overlap or disappear.
- Font and glyph handling now reduces persistent SD-card font advance-cache memory, releases optional font caches before image extraction only when heap is tight, and shows a visible replacement symbol when compact UI fonts lack `U+FFFD`.
- KOReader Sync authentication diagnostics and an in-reader sync crash, including clearer handling when a server or proxy returns non-JSON content.
- EPUB text rendering for redactions, whitespace-only XHTML text nodes, simple black CSS span backgrounds, list bullets in `<li><p>...</p></li>` items, and very long base64-like text runs.
- EPUB image, thumbnail, and section-rebuild stability so image-heavy chapters use less temporary memory, scale images more reliably, avoid stale dimensions, and suppress optional image work earlier under heap pressure.
- EPUB low-memory and cache safety now skips optional next-chapter indexing and sleep-page cache rebuilds when heap is tight, fails safely with a malformed-book warning and Home exit path, rebuilds incompatible fork-written caches, and handles low-memory CSS parsing, truncated SD writes, invalid serialized strings, and failed temp-cache promotion.
- Home no longer crashes after clearing reading cache when the source EPUB cache is missing.
- Reader prewarm behavior now skips image decoding, keeps mixed-style font glyphs cached together, and avoids section rebuilds for render-quality-only option changes.
- Concurrent render/storage crashes are avoided by serializing `GfxRenderer` scratch-buffer access, shared SPI bus access, and failed SPI lock cleanup.
- Recent Books, EPUB/XTC thumbnail caches, deleted-folder metadata, and XTC cover scaling now keep cached book data in sync and grid covers fill their slots correctly.
- Simulator build configuration now lets SDL2 and simulator-provided network/OTA shims compile cleanly.

---

## [v1.2.9.1] - 2026-05-03

### Changed

- Cleaned up EPUB table rendering by removing synthetic row/cell labels and defaulting table cells to readable left alignment
- Allow simple EPUB tables with full-width note rows so a single `colspan` cell spanning the whole table no longer forces the entire table back to paragraph fallback

### Fixed

- Power-button shortcut conflicts outside the reader so reader-only actions fall back to `Confirm` while Sleep, Refresh, Screenshot, Sync Progress, and File Transfer remain real power actions.
- Potential crash when using `Go to %` in EPUBs.
- Potential crash when entering sleep with Page Overlay enabled if the cached EPUB page data is invalid.
