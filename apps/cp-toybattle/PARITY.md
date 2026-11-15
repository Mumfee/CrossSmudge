# Toy Battle local adaptation

All nine source playable map topologies, HQ/medal/stuck endings, 24-troop shuffles/four set aside, rack limit, joker covers, reachability, buried stacks, all eight troop kinds, Capn chains, all seven base effects/restrictions, frozen troop expiry and atomic invalid-move rejection. Hidden Proving Ground is included for rule testing but excluded from map picker.

## Differences

New public-board/rack-size heuristic, not ToyBattleBrain; it does not plan Capn chains or special-base effects. Human can configure all effects and chains with the placement builder (0 declines, 1 uses; zero target means none). Solo only, no Nearby or original art. Map inspector gives numbered adjacency/regions/effects; this is a text board, not graphical parity.

## Controls and saves

Up/down (or page buttons) scroll the action list; Confirm selects; taps select visible rows. Back saves/exits. Inspect screens return on selecting a row; use navigation buttons to scroll. New-game/case selection is explicit and cancellable. App-relative checksummed two-slot journals validate decoded state, read back writes and fall back from torn/invalid latest slots. Autosaves are debounced to five seconds on actions; Back/on_exit always attempts saving. Sudden power loss can lose recent actions. No global firmware state is written. English only.

## Verification boundary

Stock vendored Lua int32/float32, serial native adapter runs at 76,800 and 2,097,152 allocator bytes, both orientations, complete games/cases, journal failures and independent rule review. Source-oracle full-game traces use 2 MiB to include instrumentation; separate smaller full-game regressions run at 75 KiB. This does not measure C/HAL allocations or prove physical-device headroom. See repository docs/crossplay-ports.md and external five-runtime-results.json/five-oracle-results.json for exact evidence.

On Sticky v1.6.5.3, install this new folder without overwriting saves. Test buttons/taps in portrait/landscape, every inspection/setup menu, a full match/case, exit/reopen and sleep/wake. Inspect Application Error/OOM logs, internal/PSRAM free and largest blocks. No EPUB cache reset. No physical hardware or visual-fidelity test is claimed.
