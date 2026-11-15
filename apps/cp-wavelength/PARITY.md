# Wavelength — local Lua adaptation

Implemented: All 245 original spectrum pairs; nonrepeat chosen pairs with one/two choices; private target across all 20 slots; co-op and team end-call scoring; zero-point practice round; session total and saved state.

Limitations: Original art and lifetime histogram/statistics absent. Team mode keeps the original shared session scoring formula, not invented separate team totals. Hidden target reopens behind a reveal gate. English only.

Persistence uses app-relative checked two-slot journals, semantic input validation and readback; failed writes leave the previous slot available. These saves are separate from original CrossPlay and existing CrossSmudge IDs. No firmware or network hooks are installed.

Validation: `test/crossplay_ports/additions_run.py` uses the exact stock int32/float32 Lua VM at 76,800 and 2,097,152 bytes, both orientations, rule fixtures, callback/render instrumentation and journal failures. See repository `docs/crossplay-ports.md`. These are native adapter results, not e-ink screenshots or hardware measurements.

Hardware verification on Sticky v1.6.5.3: Pick a spectrum privately, reveal and hide target, guess, exercise both end-call directions, verify zero practice points and exact/adjacent scores, exhaust deck and reopen. Check serial logs for OOM/Application Error, internal free/largest heap and PSRAM. Repeat touch and buttons in both orientations. No EPUB cache reset is needed.
