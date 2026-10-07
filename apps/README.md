# CrossSmudge Application Developer Guide

Welcome to the **CrossSmudge Application Platform**! CrossSmudge features a lightweight, sandboxed **Lua 5.4** application engine that enables anyone to build, share, and install custom e-paper applications directly from SD card storage—**no firmware compilation or flashing required**.

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Application Directory Structure](#application-directory-structure)
3. [Manifest Specification (`manifest.json`)](#manifest-specification-manifestjson)
4. [Application Icon (`icon.raw`)](#application-icon-iconraw)
5. [Application Lifecycle Callbacks](#application-lifecycle-callbacks)
6. [Hardware Button & Touch Mapping](#hardware-button--touch-mapping)
7. [The `smudge.*` API Reference](#the-smudge-api-reference)
   - [Display & Refresh Control](#display--refresh-control)
   - [Typography & Text Wrapping](#typography--text-wrapping)
   - [2D Graphics Primitives & Shapes](#2d-graphics-primitives--shapes)
   - [Hardware Queries & Utilities](#hardware-queries--utilities)
   - [Persistence & File I/O](#persistence--file-io)
8. [Multi-File Projects & Assets](#multi-file-projects--assets)
9. [Memory Discipline & E-Paper Best Practices](#memory-discipline--e-paper-best-practices)
10. [Complete "Hello World" Example](#complete-hello-world-example)
11. [Testing & Installing Apps](#testing--installing-apps)
12. [Submitting to the App Store](#submitting-to-the-app-store)
13. [Official Applications](#official-applications)

---

## Architecture Overview

- **Engine:** Sandboxed Lua 5.4 runtime embedded within CrossSmudge firmware.
- **Location on Device:** `/.crosssmudge/applications/<app_id>/` (fallback: `/.smudge/applications/<app_id>/`).
- **Dynamic Discovery:** CrossSmudge discovers all installed applications on SD card startup and populates the **Applications** menu automatically.
- **Zero-Flash Iteration:** Changes made to Lua scripts or assets on the SD card take effect immediately the next time the app is launched.

---

## Application Directory Structure

Every application lives in its own subdirectory inside `apps/` (or on device in `/.crosssmudge/applications/<app_id>/`):

```text
apps/
└── my_app/
    ├── manifest.json       # Required: Metadata, name, author, entry script
    ├── main.lua            # Required: Main Lua entry script
    ├── icon.raw            # Optional: 32x32 1-bit raw bitmap icon (128 bytes)
    ├── helper.lua          # Optional: Secondary Lua modules
    └── data.txt            # Optional: Text or read-only assets
```

---

## Manifest Specification (`manifest.json`)

The `manifest.json` file describes your application to CrossSmudge, the App Store, and the Web Manager:

```json
{
  "id": "my_app",
  "name": "My Custom App",
  "version": "1.0.0",
  "author": "YourName",
  "description": "A clean e-paper utility or game for CrossSmudge.",
  "entry": "main.lua"
}
```

### Manifest Fields

| Field | Type | Description |
| :--- | :--- | :--- |
| `id` | string | Unique identifier matching folder name (`[a-z0-9_-]+`). |
| `name` | string | User-facing display title shown in the Applications menu. |
| `version` | string | Semantic version (e.g. `"1.0.0"`). |
| `author` | string | Creator name or handle. |
| `description`| string | One-sentence summary displayed in the App Store / Web Viewer. |
| `entry` | string | Entry point script filename (default: `"main.lua"`). |
| `min_api` | integer | Optional: Minimum required CrossSmudge Lua API version (e.g. `2` for apps utilizing `prevent_sleep`, `set_orientation`, or `has_feature`). Firmware older than this version will prevent launching and show a clear update prompt rather than crashing. |
| `prevent_sleep` | boolean | Optional: If `true`, disables the device's automatic inactivity sleep timer while this app is running (e.g. for desk clocks, timers, dashboards). Standard auto-sleep resumes immediately when the app exits. |

---

## Application Icon (`icon.raw`)

Each application can provide an authentic 32x32 monochrome icon:

- **Format:** Raw 1-bit monochrome uncompressed bitmap, 32 pixels wide by 32 pixels high.
- **File Size:** Exactly **128 bytes** (`32 * 32 / 8 = 128`).
- **Bit Convention:** 
  - `0` bit = **Drawn / Black pixel**
  - `1` bit = **White / Transparent background**
- **Hardware Coordinate Alignment:** Because CrossSmudge operates in portrait mode on landscape e-ink hardware panels, the icon pixel stream is stored pre-rotated 90° counter-clockwise so the ESP32 can blit it directly into the screen framebuffer with zero rotation overhead.

### Managing and Editing `icon.raw` with `scripts/convert_icon.py`

The repository includes a helper utility at `scripts/convert_icon.py`:

```bash
# 1. View an icon directly in your terminal (ASCII art preview):
python scripts/convert_icon.py view apps/watertracker/icon.raw

# 2. Convert icon.raw to an editable PNG:
python scripts/convert_icon.py raw2png apps/watertracker/icon.raw icon.png

# 3. Convert an edited PNG/SVG back to icon.raw:
python scripts/convert_icon.py png2raw icon.png apps/watertracker/icon.raw

# 4. Generate a C header (for built-in system firmware icons):
python scripts/convert_icon.py header icon.png my_icon
```

### Editing Workflow

1. Export the icon to PNG: `python scripts/convert_icon.py raw2png apps/myapp/icon.raw my_icon.png`
2. Open `my_icon.png` in any image editor (Aseprite, Photoshop, GIMP, MS Paint, Piskel).
3. Draw your 32x32 black and white pixel art.
4. Convert back to `icon.raw`: `python scripts/convert_icon.py png2raw my_icon.png apps/myapp/icon.raw`

---

## Application Lifecycle Callbacks

CrossSmudge calls global functions defined in your `main.lua` at specific events:

### `on_init()`
Called once when your app is launched. Initialize your state, seed random numbers, and load saved preferences here:
```lua
function on_init()
    math.randomseed(os.time())
    score = tonumber(smudge.load("high_score", "0")) or 0
end
```

### `on_draw()`
Called whenever the screen needs to be rendered. Clear the buffer and draw all UI elements here:
```lua
function on_draw()
    smudge.clear()
    smudge.header("My App", "v1.0")
    smudge.centered_text(300, "Press Select to play!", "ui12", "bold", true)
    smudge.button_hints("Back", "Select", "", "")
end
```

### `on_button(btn, pressed)`
Called on hardware button state changes. `btn` is a string identifier, and `pressed` is a boolean (`true` on button down, `false` on button release):
```lua
function on_button(btn, pressed)
    if not pressed then return end -- Act on release/press

    if btn == "back" then
        smudge.exit()
    elseif btn == "confirm" then
        score = score + 1
        smudge.request_update()
    end
end
```

### `on_touch(event, x, y)` *(Touch-Enabled Devices)*
Called on touch events on supported hardware (e.g. Seeed reTerminal Sticky, Xteink X4 Pro). `event` is `"down"`, `"move"`, or `"up"`:
```lua
function on_touch(event, x, y)
    if event == "up" then
        -- Check if touch landed inside a button rectangle
    end
end
```

### `on_update()` *(Optional Real-Time Loop)*
For games or animations requiring real-time gravity or ticking (e.g. *Tetris* piece falling):
```lua
local last_tick = 0
function on_update()
    local now = smudge.millis()
    if now - last_tick > 500 then
        last_tick = now
        drop_piece()
        smudge.request_update()
    end
end
```

### `on_exit()` *(Optional)*
Called right before the app terminates. Save final state or flush data:
```lua
function on_exit()
    smudge.save("high_score", tostring(score))
end
```

---

## Hardware Button & Touch Mapping

CrossSmudge standardizes button names across all hardware variations:

| Button String | Action / Convention | Physical Button (Xteink X3/X4) |
| :--- | :--- | :--- |
| `"back"` | Exit / Cancel / Previous menu | Bottom Left Button |
| `"confirm"` | Select / Enter / OK / Action | Bottom Center-Left Button |
| `"left"` | Navigate Left / Decrease | Bottom Center-Right Button |
| `"right"` | Navigate Right / Increase | Bottom Right Button |
| `"up"` | Navigate Up / Menu | Top Left Button |
| `"down"` | Navigate Down / Submenu | Top Right Button |
| `"page_back"` | Page Previous | Top Left Button |
| `"page_forward"` | Page Next | Top Right Button |

### Menu Navigation Convention
In vertical menu lists (settings menus, grimoires, selection menus):
- **Bottom Left Button (`"left"`)**: Navigates **Up** (previous menu item).
- **Bottom Right Button (`"right"`)**: Navigates **Down** (next menu item).
- **Top Buttons (`"up"` / `"page_back"`, `"down"` / `"page_forward"`)**: Adjust values **Left** and **Right** (e.g. decrease/increase values, cycle options).
- **Confirm Button (`"confirm"`)**: Selects or cycles the highlighted row.
- **Back Button (`"back"`)**: Saves changes and exits/returns.
- **Button Hints Convention**: In menu screens, use `smudge.button_hints("Back", "Select", "Up", "Down")`.

---

## The `smudge.*` API Reference

### Display & Refresh Control

- **`smudge.get_bounds()`**  
  Returns `w, h` integers for current screen dimensions (typically `480, 800`).
- **`smudge.get_metrics()`**  
  Returns system layout metrics table:
  - `top_padding` (default ~4px)
  - `header_height` (standard top header height)
  - `button_hints_height` (bottom action bar height)
  - `content_side_padding` (bezel-safe margin)
- **`smudge.clear([color])`**  
  Clears the screen buffer. Default is white (`color=false`).
- **`smudge.request_update()`** *(or `smudge.redraw()`)*  
  Schedules an e-paper screen refresh. Call this whenever state changes.
- **`smudge.refresh([full])`**  
  Triggers a display refresh. If `full` is `true` or `"full"`, performs a full e-paper flashing refresh to clear ghosting.
- **`smudge.full_refresh()`**  
  Forces a full hardware e-paper flashing refresh (`HalDisplay::RefreshMode::FULL_REFRESH`) immediately.
- **`smudge.invert_rect(x, y, w, h)`**  
  Inverts the 1-bit pixel buffer inside the specified rectangular region (turns white pixels black and black pixels white).
- **`smudge.invert()`** *(or `smudge.invert_screen()`)*  
  Inverts the entire screen buffer.
- **`smudge.set_orientation(mode)`**  
  Dynamically changes the display rotation at runtime. `mode` can be:
  - `"portrait"` (default, 0°)
  - `"landscape"` / `"landscape_cw"` (90° clockwise)
  - `"portrait_inverted"` (180° upside down)
  - `"landscape_ccw"` (270° counter-clockwise)  
  *Note:* When the application exits, CrossSmudge automatically restores the system's prior orientation.
- **`smudge.get_orientation()`**  
  Returns the current display orientation string (`"portrait"`, `"landscape"`, `"portrait_inverted"`, or `"landscape_ccw"`).
- **`smudge.popup(message)`** *(or `smudge.draw_popup(message)`)*  
  Renders a standard system modal popup box centered on screen with the given message.
- **`smudge.millis()`** *(or `smudge.get_time_ms()`)*  
  Returns millisecond uptime counter.
- **`smudge.time()`**  
  Returns UNIX epoch timestamp (seconds).
- **`smudge.random(min, max)`**  
  Returns a random integer between `min` and `max`.
- **`smudge.exit()`**  
  Closes the application and returns to the Applications menu.
- **`smudge.log(message)`**  
  Outputs a debug log message to serial monitor (`[INF] [LUA] ...`).

---

### Typography & Text Wrapping

CrossSmudge includes high-quality embedded anti-aliased bitmap fonts:

- **Supported Fonts:**
  - `"small"`: 8pt compact UI font (rubrics, metadata)
  - `"ui10"`: 10pt standard UI font
  - `"ui12"`: 12pt prominent font (headings, cards, menu items)
  - `"bitter14"`: 14pt serif reader font
  - `"reader"`: Default user-selected reader font
- **Styles:** `"regular"`, `"bold"`

#### Functions:
- **`smudge.text(x, y, text, [font], [style], [align], [color])`**  
  Renders text at `(x, y)`. `align` can be `"left"` (default), `"center"`, or `"right"`. `color` is boolean (`true` = black, `false` = white).
- **`smudge.centered_text(y, text, [font], [style], [color])`**  
  Draws horizontally centered text across the entire display width at vertical position `y`.
- **`smudge.text_width(text, [font], [style])`**  
  Returns the measured pixel width of the text.
- **`smudge.line_height([font])`**  
  Returns the font's standard line height in pixels.
- **`smudge.wrapped_text(text, maxWidth, [font], [style], [maxLines])`**  
  Performs pixel-perfect word wrapping. Returns a table:
  ```lua
  local res = smudge.wrapped_text("A long sentence that wraps cleanly.", 400, "ui12", "regular")
  -- res.lines is an array of wrapped strings
  -- res.total_height is the total pixel height
  for i, line in ipairs(res.lines) do
      smudge.text(20, y + (i - 1) * smudge.line_height("ui12"), line, "ui12", "regular")
  end
  ```

---

### 2D Graphics Primitives & Shapes

All shape drawing functions support a customizable line **thickness** (stroke width in pixels):

- **`smudge.pixel(x, y, [color])`**  
  Draws a single pixel.
- **`smudge.line(x1, y1, x2, y2, [thickness], [color])`**  
  Draws a line between `(x1, y1)` and `(x2, y2)` with optional pixel thickness (default 1).
- **`smudge.thick_line(x1, y1, x2, y2, thickness, [color])`**  
  Draws a line with specified pixel thickness.
- **`smudge.rect(x, y, w, h, [filled], [thickness], [color])`**  
  Draws an outline or solid rectangle. For outlines, `thickness` specifies the border stroke width in pixels.
- **`smudge.rounded_rect(x, y, w, h, radius, [filled], [thickness], [color])`**  
  Draws a rounded rectangle with corner radius. For outlines, `thickness` specifies border stroke width in pixels.
- **`smudge.circle(x, y, radius, [filled], [thickness], [color])`**  
  Draws an outline or solid circle. For outlines, `thickness` specifies border stroke width in pixels.
- **`smudge.triangle(x1, y1, x2, y2, x3, y3, [filled], [thickness], [color])`**  
  Draws an outline or filled triangle between three vertices `(x1, y1)`, `(x2, y2)`, and `(x3, y3)`.
- **`smudge.rect_dither(x, y, w, h, [is_dark])`**  
  Fills a rectangle with an authentic e-paper dither pattern (checkerboard shade).
- **`smudge.heart(x, y, size, [filled])`**  
  Draws a vector heart icon.
- **`smudge.draw_sprite(x, y, w, h, data, [inverted], [dither])`** *(or `smudge.sprite(...)`)*  
  Renders a 1-bit monochrome bitmap or sprite to the display:
  - `x, y`: Screen coordinates.
  - `w, h`: Pixel dimensions of the sprite.
  - `data`: Can be a relative file path (e.g. `"icon.raw"`, `"sprites/coin_heads.raw"`), a raw binary byte string (MSB-first, 1 bit per pixel, `0` = black, `1` = white/transparent), or a Lua table of byte values (`{ 0x00, 0xFF, ... }`).
  - `inverted`: Optional boolean (`true` renders white pixels on dark backgrounds).
  - `dither`: Optional boolean (`true` renders black pixels with e-paper light-gray checkerboard dithering).
- **`smudge.in_rect(px, py, rx, ry, rw, rh)`** *(or `smudge.point_in_rect(...)`)*  
  Hit-testing helper: returns `true` if coordinate `(px, py)` falls inside rectangle `(rx, ry, rw, rh)`.
- **`smudge.header(title, [right_text])`**  
  Draws standard CrossSmudge system header with title, battery indicator, and divider.
- **`smudge.button_hints(btn1, btn2, btn3, btn4)`**  
  Draws bottom navigation pills corresponding to hardware buttons.

---

### Hardware Queries & Utilities

- **`smudge.has_touch()`** *(or `smudge.has_touchscreen()`)*  
  Returns `true` if the current device has hardware touchscreen support (e.g. Seeed reTerminal Sticky, Xteink X4 Pro), or `false` on button-only devices (e.g. Xteink X3/X4).
- **`smudge.get_battery()`** *(or `smudge.battery()`)*  
  Returns the current battery level as an integer percentage (`0`–`100`).
- **`smudge.get_device()`**  
  Returns a string identifying the hardware platform (`"Xteink X3"`, `"Seeed reTerminal Sticky"`, `"Xteink X4 Pro"`, or `"Simulator"`).
- **`smudge.get_date()`** *(or `smudge.date()`)*  
  Returns a calendar date/time table from the device's hardware RTC clock:
  ```lua
  local d = smudge.get_date()
  -- d.year (e.g. 2026), d.month (1-12), d.day (1-31),
  -- d.wday (1=Sun .. 7=Sat), d.hour (0-23), d.min (0-59), d.sec (0-59)
  ```
- **`smudge.get_memory()`** *(or `smudge.memory()`)*  
  Returns real-time heap metrics table for profiling embedded memory usage:
  - `lua_kb`: Kilobytes currently allocated by the Lua runtime.
  - `lua_max_kb`: Maximum allowed Lua heap ceiling (e.g. 75 KB on ESP32-C3, 2048 KB on S3).
  - `free_heap`: Total free ESP32 internal DRAM (in bytes).
  - `max_alloc`: Largest contiguous allocatable block in DRAM (in bytes).
- **`smudge.is_button_down(btn)`**  
  Returns `true` if the specified hardware button (`"back"`, `"confirm"`, `"up"`, `"down"`, `"left"`, `"right"`) is currently pressed/held down.
- **`smudge.prevent_sleep([enabled])`** *(or `smudge.keep_awake([enabled])`)*  
  Controls the device's automatic inactivity sleep timer while the app is active. Pass `true` (default if omitted) to keep the display awake indefinitely (ideal for desk stands, live clocks, timers, and dashboards). Pass `false` to re-enable normal auto-sleep timeout. The device's physical Power button continues to enter deep sleep immediately on demand, and regular auto-sleep is automatically restored as soon as the user exits the app.
- **`smudge.is_prevent_sleep()`** *(or `smudge.is_keep_awake()`)*  
  Returns `true` if auto-sleep prevention is currently active, or `false` otherwise.
- **`smudge.version()`** *(or `smudge.get_version()`)*  
  Returns the current CrossSmudge firmware version string (e.g. `"1.6.5.3"`).
- **`smudge.api_version`** *(or `smudge.get_api_version()`)*  
  Returns the integer API level (currently `2`). API `1` was the initial app engine release; API `2` adds sleep prevention, dynamic orientation switching, full-refresh requests, and version/feature introspection.
- **`smudge.has_feature(name)`**  
  Returns `true` if a specific feature is supported (e.g. `"prevent_sleep"`, `"orientation"`, `"full_refresh"`, `"touch"`).

---

### Persistence & File I/O

CrossSmudge provides key-value persistence, full sandboxed file operations, and high-performance streaming file section readers:

- **`smudge.save(key, value_string)`**  
  Persists a key-value string to the app's persistent storage on SD card (`/.crosspoint/data/<app_id>_<key>.dat`).
- **`smudge.load(key, [default_string])`**  
  Reads a previously saved string. Returns `default_string` if not found.
- **`smudge.file_exists(relative_path)`**  
  Returns `true` if a file exists inside the application's directory.
- **`smudge.read_file(relative_path)`**  
  Reads and returns the complete contents of a file inside the app folder as a string.
- **`smudge.write_file(relative_path, content_string, [append])`**  
  Writes `content_string` to a file in the app directory. If `append` is `true`, appends to existing content; otherwise overwrites or creates the file. Returns `true` on success.
- **`smudge.list_files([dir_relative_path])`**  
  Returns a Lua array of filenames present in the given subfolder (or app root directory if omitted).
- **`smudge.delete_file(relative_path)`**  
  Deletes the specified file inside the app directory. Returns `true` on success.
- **`smudge.find_section(path, tag, [endPrefix], [maxBytes], [callback])`**  
  *(Aliases: `smudge.find_file_section`, `smudge.read_lines`)*  
  High-performance section parser that reads directly from SD card storage using 512-byte stack buffers with **zero dynamic C++ heap allocation**:
  
  **Mode 1 — String Return:**
  ```lua
  -- Extracts text between "[SECTION_A]" and the next "[" marker:
  local text = smudge.find_section("data.txt", "[SECTION_A]", "[", 4096)
  ```
  
  **Mode 2 — Zero-Allocation Line Streaming Callback (Recommended for Large Files):**
  ```lua
  -- Streams matching lines one-by-one directly into a Lua closure.
  -- Uses virtually zero Lua heap; does not load the file into memory!
  smudge.find_section("psalter.txt", "[PSALM 23]", "[", function(line)
      -- Process line (e.g. draw on screen or paginate)
      -- Return false to stop reading early
  end)

  -- If tag is "", reads from line 1 of the file to end (or until callback returns false):
  smudge.find_section("chapter1.txt", "", "", function(line)
      print(line)
  end)
  ```

---

## Multi-File Projects & Assets

To keep your codebase organized, you can split your logic into multiple Lua files and load them using `dofile()` with relative paths:

```lua
-- In main.lua:
dofile("constants.lua")
dofile("game_engine.lua")
```

Any extra assets (like `.txt`, `.json`, `.raw` sprites, or level definitions) placed in your app folder can be read using `smudge.read_file("levels/level1.txt")` or streamed using `smudge.find_section()`.

---

## Memory Discipline & E-Paper Best Practices

### The 75 KB Hardware DRAM Ceiling (ESP32-C3)
Devices like the Xteink X3 and X4 run on a single-core ESP32-C3 microcontroller with **no PSRAM** and only **~75 KB total usable internal DRAM** for Lua applications. Any application intended for the App Store must run comfortably within this ceiling.

Follow these proven patterns to ensure your app is rock-solid:

1. **Beware of Compiler Memory Spikes with `dofile()`:**  
   Compiling a Lua script from source text at runtime takes 3x–4x the script's source file size in temporary compiler memory. If `main.lua` is already using 35 KB, calling `dofile("parser.lua")` (15 KB of source) will allocate ~40 KB of compiler memory, pushing peak memory to ~75 KB+ and causing an out-of-memory error.  
   *Rule of thumb:* Keep `main.lua` lean, or load modules only once at startup rather than repeatedly inside functions.

2. **The Zero-RAM-Page Disk Cache Pattern:**  
   If your app displays long multi-page text (e.g. books, daily office prayers, scripture, rules, logs):  
   - **Do NOT** store all formatted pages in an array of Lua strings in RAM (`pages = {"...", "..."}`). 30 pages of text can consume 20+ KB of precious heap.  
   - **Instead**, stream formatted lines directly to a cache file (`smudge.write_file("cache.txt", ...)`).  
   - When displaying a page, use `smudge.find_section("cache.txt", "[PAGE " .. cur .. "]", "[PAGE ", callback)` to stream only the active page's lines to screen. This keeps memory usage flat (~55–60 KB) regardless of whether the document has 5 pages or 100 pages!

3. **Use Top-Level Functions Instead of Deeply Nested Closures:**  
   Functions defined inside other functions create new closure and upvalue table objects every time the outer function runs. Define helpers (`emit`, `wrap`, `divider`, `draw_ui`) as file-level `local function`s so their prototypes are allocated only once.

4. **Offload Heavy Tables to SD Text Files:**  
   Static data (calendars, hymn stanzas, psalter schedules, monster stats, dialog trees) defined as large Lua tables consumes permanent Lua heap. Store them in `.txt` files in your app folder and stream them on-demand using `smudge.find_section()`.

5. **Pace Garbage Collection:**  
   Call `collectgarbage("collect")` prior to starting heavy operations and immediately after freeing large tables. CrossSmudge configures an aggressive incremental garbage collector (`LUA_GCINC`), but explicit collection before page transitions ensures peak heap headroom.

6. **Avoid Allocations in `on_draw()`:**  
   Do not allocate tables or format heavy strings inside `on_draw()` or `on_update()`. Mutate existing tables in place.

7. **E-Paper Refresh Rate:**  
   E-ink screens have physical refresh latency (~200ms–400ms). Only call `smudge.request_update()` when visual state actually changes—never call it on every millisecond tick.

8. **Debounce File Writes:**  
   Flash memory has wear limits. Call `smudge.save()` when a game finishes or when exiting in `on_exit()`, rather than after every individual tap or score increment.

9. **Version Compatibility & Defensive Programming:**  
   As new firmware updates introduce newer Lua APIs (such as `smudge.prevent_sleep`, `smudge.set_orientation`, or `smudge.full_refresh`), apps in the wild might be loaded on devices running older firmware.
   - **How Older Firmware Behaves:**  
     In Lua, accessing an unassigned table key (e.g. `smudge.prevent_sleep`) safely evaluates to `nil` without raising an error. However, directly invoking `nil` as a function (`smudge.prevent_sleep(true)`) throws a fatal Lua runtime exception (`attempt to call a nil value`), halting the app and displaying the e-ink "Application Error" crash screen.
   - **Best Practice — Feature Detection:**  
     Always guard newly added APIs with idiomatic Lua feature checks:
     ```lua
     if smudge and smudge.prevent_sleep then
         smudge.prevent_sleep(true)
     end
     ```
   - **Declaring Minimum API Level in Manifest:**  
     If your application fundamentally depends on a newer capability to function (e.g. dynamic screen rotation), declare `"min_api": 2` in your `manifest.json`. CrossSmudge validates `min_api` before launching the script; if the device firmware is older, it gracefully displays an informative "Firmware update required" screen instead of executing and crashing.
   - **Querying API Version at Runtime:**  
     You can programmatically inspect the environment at runtime:
     ```lua
     local api_lvl = smudge.api_version or 1
     if smudge.has_feature and smudge.has_feature("prevent_sleep") then
         smudge.prevent_sleep(true)
     end
     ```

---

## Complete "Hello World" Example

Here is a complete, working counter application:

### `manifest.json`
```json
{
  "id": "counter",
  "name": "Clicker Counter",
  "version": "1.0.0",
  "author": "Mumfee",
  "description": "Simple demonstration counter app for CrossSmudge.",
  "entry": "main.lua"
}
```

### `main.lua`
```lua
local count = 0
local w, h = 480, 800

function on_init()
    w, h = smudge.get_bounds()
    count = tonumber(smudge.load("count", "0")) or 0
end

function on_draw()
    smudge.clear()
    smudge.header("Counter", string.format("Value: %d", count))

    -- Draw centered card box
    local cardW, cardH = 260, 160
    local cardX = math.floor((w - cardW) / 2)
    local cardY = 280
    smudge.rounded_rect(cardX, cardY, cardW, cardH, 8, false, 2)

    -- Number readout
    smudge.centered_text(cardY + 50, tostring(count), "ui12", "bold", true)
    smudge.centered_text(cardY + 100, "Press Select to Count", "small", "regular", true)

    -- Bottom action buttons
    smudge.button_hints("Back", "+ 1", "Reset", "")
end

function on_button(btn, pressed)
    if not pressed then return end

    if btn == "back" then
        smudge.save("count", tostring(count))
        smudge.exit()
    elseif btn == "confirm" then
        count = count + 1
        smudge.request_update()
    elseif btn == "left" then
        count = 0
        smudge.request_update()
    end
end

function on_exit()
    smudge.save("count", tostring(count))
end
```

---

## Testing & Installing Apps

### Method 1: Web Viewer (Drag-and-Drop)
1. On your device, open **Web Transfer** (or enable Wi-Fi).
2. On your PC, navigate to `http://<device-ip>/applications`.
3. Drag and drop your application folder or `.zip` package directly onto the browser window.
4. Your application will appear immediately in the installed applications list!

### Method 2: Direct SD Card Copy / USB Drive Mode
1. Connect your device via USB Drive mode or insert the SD card into your PC.
2. Copy your application folder into `/.crosssmudge/applications/<app_id>/`.
3. Eject the drive. Open **Applications** on your device to launch!

### Method 3: In the Native Simulator
1. Place your application folder inside `apps/<app_id>/` in the repository (and mirror it to `fs_/.crosssmudge/applications/<app_id>/`).
2. Build and launch the simulator:
   ```bash
   pio run -e simulator
   .pio/build/simulator/program
   ```
3. **MANDATORY FOR APP DEVELOPERS — Test Under Exact X3 Hardware Constraints:**
   To guarantee your application runs on physical ESP32-C3 hardware (such as the Xteink X3 and X4) without crashing or running out of memory, launch the simulator with the `SMUDGE_X3_CONSTRAINTS=1` environment variable:
   ```bash
   SMUDGE_X3_CONSTRAINTS=1 .pio/build/simulator/program
   ```
   This clamps the host Lua heap to the exact **75 KB hardware DRAM limit** of the ESP32-C3. If your script or assets exceed 75 KB, the simulator will raise an out-of-memory error immediately, allowing you to catch and fix memory bloat before flashing to physical devices.
4. Open **Applications** from the simulator home menu to test keyboard inputs, button mappings, touch gestures, and rendering instantly.

---

## Submitting to the App Store

CrossSmudge features an on-device **App Store** accessible directly over Wi-Fi. It queries the community catalog hosted at `https://raw.githubusercontent.com/Mumfee/CrossSmudge/main/apps/catalog.json`. Users can browse descriptions, see icons, install, update, and uninstall apps with a single click.

To publish your application to the official CrossSmudge App Store so any user can discover and install it:

### Step 1: Prepare Your App Directory
Ensure your application is located in `apps/<app_id>/` with all required files:
- `manifest.json` (Valid JSON with `id`, `name`, `version`, `author`, `description`, `entry`)
- `main.lua` (Clean, working code)
- `icon.raw` (Optional but highly recommended 32x32 1-bit icon, exactly 128 bytes)
- Any supplementary modules or asset files

### Step 2: Validate Against Hardware Constraints
Before submitting, you **must** test your application in the simulator with the 75 KB RAM constraint:
```bash
SMUDGE_X3_CONSTRAINTS=1 .pio/build/simulator/program
```
Verify:
1. The app launches without out-of-memory errors.
2. Navigating through all menus, screens, and gameplay loops does not leak memory.
3. Exiting back to the Applications menu works smoothly.

### Step 3: Add to `apps/catalog.json`
Open [`apps/catalog.json`](./catalog.json) and add an entry under the `"apps"` array:

```json
{
  "id": "my_app",
  "name": "My Custom App",
  "version": "1.0.0",
  "author": "YourGitHubHandle",
  "description": "Short, catchy summary of what your application does.",
  "files": [
    "manifest.json",
    "main.lua",
    "icon.raw"
  ]
}
```
*Note: Make sure all files required by your app are listed in `files`. The on-device App Store downloads each file in this list.*

### Step 4: Open a GitHub Pull Request
1. Fork [https://github.com/Mumfee/CrossSmudge](https://github.com/Mumfee/CrossSmudge).
2. Commit your new application folder (`apps/<app_id>/...`) and your edit to `apps/catalog.json`.
3. Submit a Pull Request with title `feat(apps): add <app_name>`.
4. Once reviewed and merged into `main`, your application is immediately available in the on-device App Store across all CrossSmudge devices worldwide!

---

## Official Applications

The following applications are included in the repository and available through the on-device App Store:

| App ID | Name | Category | Description |
|---|---|---|---|
| `dashboard` | **Desk Stand** | Productivity | Multi-style ambient desk clock with calendar, year progress, 12h/24h toggle, auto-sleep prevention, and 800×480 landscape stand. |
| `hourglass` | **Hourglass** | Productivity | Expressive countdown timer with dynamic parabolic sand funnel physics, full neck passage, bottom sand accumulation mound, and auto-sleep prevention. |
| `stopwatch` | **Stopwatch** | Productivity | Pixel cartoon stopwatch character with animated mechanical crown plunger, facial expressions, digital LCD window (`MM:SS.cc`), lap split recording, and auto-sleep prevention. |
| `solitaire` | **Solitaire** | Games | Classic 7-column Klondike Solitaire card game optimized for e-paper with draw-1 and draw-3 modes. |
| `watertracker` | **Water Tracker** | Health | Cute daily hydration companion with animated pixel cup expressions, 14-day history archive, and customizable volume units (`ml`, `cups`, `oz`, `gallons`, `L`). |
| `rosary` | **Holy Rosary** | Devotion | Interactive Holy Rosary prayer guide with bead tracking, mystery reflections, and authentic pixel crucifix art. |
| `dailyoffice` | **Divine Worship** | Devotion | Daily Office liturgical prayer book (Morning, Evening & Compline) featuring zero-allocation SD card section streaming for propers and psalter. |
| `codex` | **Codex: Ink & Iron** | Games | Medieval illuminated manuscript roguelike deckbuilder with Couplet synergy, relic system, and page progression. |
| `wordle` | **Wordle** | Games | 5-letter word deduction puzzle with on-screen keyboard, guess evaluation, and local statistics. |
| `sudoku` | **Sudoku** | Games | Classic 9×9 Sudoku logic puzzle with on-device puzzle generator and conflict validation. |
| `tetris` | **Tetris** | Games | Classic falling block puzzle with ghost piece guide and e-paper optimized refresh scheduling. |
| `2048` | **2048** | Games | Classic 4×4 sliding tile number puzzle with high scores and smooth e-paper rendering. |
| `blackjack` | **Blackjack** | Games | Classic 21 casino card game with betting chips, hit, stand, double down, and dealer AI. |
| `dice` | **Dice Roller** | Utilities | Multi-polyhedral tabletop dice simulator (d4, d6, d8, d10, d12, d20, d100) with coin flipper and roll history. |
| `lifecounter` | **Life Counter** | Utilities | Magic: The Gathering & card game life tracker supporting multiplayer and commander damage tracking. |

