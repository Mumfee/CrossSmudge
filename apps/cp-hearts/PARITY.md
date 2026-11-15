# Hearts — local Lua adaptation

Implemented: Four 13-card hands; left/right/across/hold passing; opening two of clubs, follow-suit, first-trick penalty and unbroken-heart restrictions; visible complete tricks; shooting the moon; 100-point match ending and tied low winners; saved matches.

Limitations: Opponent is a new explicit public-information risk heuristic, not the original HeartsBrain. Opponent turns advance with a labeled action. Card list UI replaces original artwork; lifetime statistics absent. No hidden-hand peeking.

Persistence uses app-relative checked two-slot journals, semantic input validation and readback; failed writes leave the previous slot available. These saves are separate from original CrossPlay and existing CrossSmudge IDs. No firmware or network hooks are installed.

Validation: `test/crossplay_ports/additions_run.py` uses the exact stock int32/float32 Lua VM at 76,800 and 2,097,152 bytes, both orientations, rule fixtures, callback/render instrumentation and journal failures. See repository `docs/crossplay-ports.md`. These are native adapter results, not e-ink screenshots or hardware measurements.

Hardware verification on Sticky v1.6.5.3: Select three passes, inspect each complete trick, verify follow-suit and heart restrictions, finish a hand and a match, exit/reopen during passing and mid-trick. Check serial logs for OOM/Application Error, internal free/largest heap and PSRAM. Repeat touch and buttons in both orientations. No EPUB cache reset is needed.
