# Checkers — local Lua adaptation

Implemented: English draughts; compulsory complete capture chains including nine jumps and returning king cycles; crowning ends a man’s turn; 80 idle-ply draw; deterministic original two-ply material evaluation; local shared-device and solo modes.

Limitations: Nearby and original artwork/statistics absent. Full legal move generation has no original 48-move list truncation. Pieces use o/x and capital kings; selecting a final square chooses the first legal chain to that square.

Persistence uses app-relative checked two-slot journals, semantic input validation and readback; failed writes leave the previous slot available. These saves are separate from original CrossPlay and existing CrossSmudge IDs. No firmware or network hooks are installed.

Validation: `test/crossplay_ports/additions_run.py` uses the exact stock int32/float32 Lua VM at 76,800 and 2,097,152 bytes, both orientations, rule fixtures, callback/render instrumentation and journal failures. See repository `docs/crossplay-ports.md`. These are native adapter results, not e-ink screenshots or hardware measurements.

Hardware verification on Sticky v1.6.5.3: Test a mandatory jump, a nine-jump king chain, a king returning to its origin, promotion, solo response, two-player play and exit/reopen. Check serial logs for OOM/Application Error, internal free/largest heap and PSRAM. Repeat touch and buttons in both orientations. No EPUB cache reset is needed.
