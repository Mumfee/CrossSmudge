# Go local adaptation

Captures before suicide, simple ko and source eight-position situational repetition window, source 5*N*N move limit, 7.5 komi (0.5 for handicap), 2-5 handicap stones on 9x9, two passes, group dead marks, area scoring and two-sided acceptance; disagreement resumes play. 9/13/19 shared-device boards.

## Differences

NOT full-history superko: matches the source bounded window and move-limit house rule. Optional 9x9 one-ply capture/liberty opponent, not Michi/search/difficulty parity. Human adjudicates dead groups and both score acceptances, including solo. No Nearby, original art, automatic life/death, resign button or original statistics. Coordinate-row UI replaces a graphical board; handicap UI is 9x9 only although core supports all three sizes.

## Controls and saves

Up/down (or page buttons) scroll the action list; Confirm selects; taps select visible rows. Back saves/exits. Inspect screens return on selecting a row; use navigation buttons to scroll. New-game/case selection is explicit and cancellable. App-relative checksummed two-slot journals validate decoded state, read back writes and fall back from torn/invalid latest slots. Autosaves are debounced to five seconds on actions; Back/on_exit always attempts saving. Sudden power loss can lose recent actions. No global firmware state is written. English only.

## Verification boundary

Stock vendored Lua int32/float32, serial native adapter runs at 76,800 and 2,097,152 allocator bytes, both orientations, complete games/cases, journal failures and independent rule review. Source-oracle full-game traces use 2 MiB to include instrumentation; separate smaller full-game regressions run at 75 KiB. This does not measure C/HAL allocations or prove physical-device headroom. See repository docs/crossplay-ports.md and external five-runtime-results.json/five-oracle-results.json for exact evidence.

On Sticky v1.6.5.3, install this new folder without overwriting saves. Test buttons/taps in portrait/landscape, every inspection/setup menu, a full match/case, exit/reopen and sleep/wake. Inspect Application Error/OOM logs, internal/PSRAM free and largest blocks. No EPUB cache reset. No physical hardware or visual-fidelity test is claimed.
