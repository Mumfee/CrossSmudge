# Hex — local Lua adaptation

Implemented: 11×11 connection rules, no swap/no draws, local shared-device play; incremental shortest-path opponent checks immediate wins and threats; saved boards and input.

Limitations: Opponent is a smaller deterministic shortest-path strategy, NOT original RAVE Monte Carlo, bridge policy, tree reuse or difficulty levels. Board is a square coordinate grid; documented diagonal adjacency is upper-right/lower-left. Nearby and original art/statistics absent.

Persistence uses app-relative checked two-slot journals, semantic input validation and readback; failed writes leave the previous slot available. These saves are separate from original CrossPlay and existing CrossSmudge IDs. No firmware or network hooks are installed.

Validation: `test/crossplay_ports/additions_run.py` uses the exact stock int32/float32 Lua VM at 76,800 and 2,097,152 bytes, both orientations, rule fixtures, callback/render instrumentation and journal failures. See repository `docs/crossplay-ports.md`. These are native adapter results, not e-ink screenshots or hardware measurements.

Hardware verification on Sticky v1.6.5.3: Connect opposite edges, test the two diagonal neighbors, let solo search finish, cancel search with Back, exit/reopen and verify board. Check serial logs for OOM/Application Error, internal free/largest heap and PSRAM. Repeat touch and buttons in both orientations. No EPUB cache reset is needed.
