# Add eighteen scoped local CrossPlay Lua adaptations

Adds installable `cp-*` packages and catalog entries for Minesweeper, Connect Four, Knucklebones, Yahtzee, Calculator, Notes, Insider, Forehead, Checkers, Wavelength, Hearts, Hex, Battleship, Jaipur, Sea Salt, Toy Battle, Murdle and Go. Each includes local UI, checked saves, MIT attribution and explicit parity limits. Four existing equivalents remain unchanged. Eleven original titles remain unimplemented because of the recorded runtime/content/permission gaps; this is not a full 33-title port.

New board-game opponents are labeled where they replace original engines. Murdle supplies 64 source-generated, uniqueness/deduction-checked cases. Go preserves the source eight-position repetition window and move-limit house rule. No firmware API changes, network workarounds or restricted content banks are included.

Validation uses stock int32/float32 Lua at 75 KiB and 2 MiB, complete-game regressions, independent code/rule review, source-core comparisons, catalog-only installs and exact extracted-package byte readback. See `docs/crossplay-ports.md` and each `PARITY.md` for per-title limits. No physical-device or renderer fidelity claim. Fork asset readback remains a parent publication step; stock firmware sees these entries only after upstream merge.
