# Solitaire Sticky touch regression

Run `python3 test/solitaire_touch/run.py` from the repository. Requires Python 3,
a C compiler (`CC`, default `cc`), and libm. Set `TMPDIR` to your scratch directory
if needed. The runner builds this repository's vendored int32/float32 Lua; no
firmware build, SDK, cached binary, or other app is required.

The default 2 MiB Lua allocation ceiling models Sticky's PSRAM Lua budget.
Both 480x800 and 800x480 exercise actual main/input/engine/suits sources with
mocked drawing, device, clock, buttons, and persistent key/value storage.
Sticky receives `(x, y, event)`. x4pro/x4/x3/simulator and missing get_device
receive legacy `(event, x, y)` to verify the explicitly preserved behavior;
this does not claim their firmware dispatches in that order. Non-Sticky tests
use the same budget to isolate behavior, not to claim C3 memory compatibility.

Cases cover stock dealing, face-up cards, runs, waste, foundations, legal/illegal
and empty targets, card bounds, reset/cancel, reset edges, buttons, save reload,
and rejected callback order. Non-Sticky dispatch tests preserve legacy gap and
reset-edge behavior. Drawing APIs are mocked: these are state/hitbox tests,
not a screen renderer or physical-device validation.

`--app /path/to/extracted/solitaire` tests an installed ZIP's exact sources.
`--budget 76800 --devices sticky` reproduces the pre-existing 75 KiB allocation
failure (also present in original 1.0.0); this change does not fix that limit.

On reTerminal Sticky, install 1.0.1, open Solitaire, tap the stock, select a
face-up card/run and a legal target, and tap gaps/below cards. Tap the top-right
header to open reset confirmation: Cancel must preserve the game, Reset must
deal anew, including near button edges. Exit and reopen to verify persistence;
repeat in portrait/landscape. Existing hold-Back reset remains available. No
cache reset is required; preserve the existing saved game when upgrading.
