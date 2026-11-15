# Battleship — local Lua adaptation

Implemented: Manual legal five-ship placement, rotation and collision checks; 10×10 board with 5/4/3/3/2 fleet; alternating shots, hits/misses, win after 17 hits; separate own/target views; saved game. Computer hunts alternating cells and follows unsunk observed hits.

Limitations: Solo only. Computer is a smaller public-observation hunt/target strategy, not a claim of original gunner parity. Nearby, original art/statistics and named sunk-ship announcements absent. Restart abandons placement only after confirmation.

Persistence uses app-relative checked two-slot journals, semantic input validation and readback; failed writes leave the previous slot available. These saves are separate from original CrossPlay and existing CrossSmudge IDs. No firmware or network hooks are installed.

Validation: `test/crossplay_ports/additions_run.py` uses the exact stock int32/float32 Lua VM at 76,800 and 2,097,152 bytes, both orientations, rule fixtures, callback/render instrumentation and journal failures. See repository `docs/crossplay-ports.md`. These are native adapter results, not e-ink screenshots or hardware measurements.

Hardware verification on Sticky v1.6.5.3: Place all five ships in both orientations, reject overlaps/edge overflow, switch own/target views, fire hit/miss, complete a game and reopen. Check serial logs for OOM/Application Error, internal free/largest heap and PSRAM. Repeat touch and buttons in both orientations. No EPUB cache reset is needed.
