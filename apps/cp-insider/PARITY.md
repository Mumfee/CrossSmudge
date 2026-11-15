# Insider — local Lua adaptation

Implemented: 4–8 seats; discarded-role variant; private reveal/pass; 300-second questions, pause/resume, voting including Nobody; complete 560-word nonrepeat deck; cumulative and recent records.

Limitations: Original artwork and ornamental record display are absent. English deck only. Timer resumes paused after closing; no wall-clock time is charged while closed.

Persistence uses app-relative checked two-slot journals, semantic input validation and readback; failed writes leave the previous slot available. These saves are separate from original CrossPlay and existing CrossSmudge IDs. No firmware or network hooks are installed.

Validation: `test/crossplay_ports/additions_run.py` uses the exact stock int32/float32 Lua VM at 76,800 and 2,097,152 bytes, both orientations, rule fixtures, callback/render instrumentation and journal failures. See repository `docs/crossplay-ports.md`. These are native adapter results, not e-ink screenshots or hardware measurements.

Hardware verification on Sticky v1.6.5.3: Deal to all seats, hide each reveal before passing, find the word, vote Nobody or a seat, expire a timer, close on a reveal and reopen privately. Check serial logs for OOM/Application Error, internal free/largest heap and PSRAM. Repeat touch and buttons in both orientations. No EPUB cache reset is needed.
