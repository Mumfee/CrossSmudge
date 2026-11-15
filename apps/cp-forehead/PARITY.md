# Forehead — local Lua adaptation

Implemented: All 2,474 entries and 17 categories; 30/60/90/120-second rounds; got/missed/unanswered marks; maximum 128-card history; no within-round repeats across deck laps; cumulative, recent and per-category records stored.

Limitations: Original artwork and graphical record charts are absent. List-based result review. English only. Closed time is paused. Category selection replaces the last completed result display; persisted records remain.

Persistence uses app-relative checked two-slot journals, semantic input validation and readback; failed writes leave the previous slot available. These saves are separate from original CrossPlay and existing CrossSmudge IDs. No firmware or network hooks are installed.

Validation: `test/crossplay_ports/additions_run.py` uses the exact stock int32/float32 Lua VM at 76,800 and 2,097,152 bytes, both orientations, rule fixtures, callback/render instrumentation and journal failures. See repository `docs/crossplay-ports.md`. These are native adapter results, not e-ink screenshots or hardware measurements.

Hardware verification on Sticky v1.6.5.3: Choose each category/length, start with screen outward, mark Got/Missed, pause, expire, inspect results, exit/reopen a full round. Check serial logs for OOM/Application Error, internal free/largest heap and PSRAM. Repeat touch and buttons in both orientations. No EPUB cache reset is needed.
