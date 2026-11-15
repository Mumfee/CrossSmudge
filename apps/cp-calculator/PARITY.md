# Calculator: stock v1.6.5.3 local port

Status: installable local implementation, **not a claim of full CrossPlay feature parity**.

Target firmware source: `bae8322b7933fc46007682d2b2d13db7c157f729`.
Original rules: `CrossPlay/src/apps_local/calculator/CalcEngine.h and CalcFormat.h`. Original code is MIT; notices are included.

Supported: Immediate decimal execution; 16 working/10 display digits; +/-; contextual percent; operator replacement; repeated equals; CE/AC/backspace; error lock; original display subnormal quantum and exponent limits tested against decNumber. App-local resume is added.

Gaps and changes: Original custom fonts and graphical styles are not ported. This is a new decimal-string implementation, not the native decNumber library. 491 differential key sequences passed; that is substantial evidence, not exhaustive proof over every decimal input.

Controls: Arrow keys select a keypad cell; Confirm presses it. Touch a key to press it. Menu then Confirm saves and exits. A labeled Menu touch target is near the top right in both orientations. Physical Back also opens the menu. Page buttons are mapped to Up/Down where the firmware sends those names.

Persistence: fixed app-relative two-slot text journals with sequence number, bounded read, checksum, semantic validation and write/readback check. Saves occur on menu/exit paths, including stock activity exit before deep sleep. No claim of surviving power removal before a save. A failed save is shown; menu exit stays available for retry. Corrupted newest saves fall back to the older valid slot. Original apps, books and imports are never written. Journal files are not part of the package.

Validation: vendored **stock Lua 5.4 with int32/float32**, all 71 stock API keys represented, real compilation and logic under a hard 76,800-byte Lua allocation cap, plus 2 MiB runs; native adapter input/draw-call/file tests in 480x800 and 800x480; repeated VM launch/close checks. Drawing is instrumented, **not the production renderer**. Neither a physical Sticky test nor a simulator/HAL run was possible here: PlatformIO is absent and the pinned SDK submodule is uninitialized. Lua allocation peaks exclude native renderer/HAL/system memory.

Install: extract the per-app ZIP into SD `/.crosssmudge/applications/`, producing `cp-calculator/manifest.json` and `main.lua`, or upload that individual ZIP in stock Web Transfer /applications. If that app ID already exists, stop and back it up; do not overwrite its journals. Do not upload the combined collection ZIP to the one-app web uploader.

Hardware check: on the Sticky running the delivered v1.6.5.3 firmware, launch CP Calculator; test both touch and buttons, Menu/Cancel/New as applicable, rotate using firmware settings and relaunch, save/exit/reopen, then sleep/wake and reopen. Expect the same saved state, no Application Error, and no changes to other app saves. No EPUB cache reset is required. Physical e-ink redraw latency, long AI turns and heap/PSRAM availability still need observation.
