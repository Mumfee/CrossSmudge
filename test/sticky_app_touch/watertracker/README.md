# watertracker: Sticky touch regression

Run independently from the repository root:

```sh
TMPDIR=/root/.hermes/cache/scratch python3 test/sticky_app_touch/watertracker/run.py --baseline-ref 2c82f306ff504c886843421846e4ff3b0c8e315c
```

Requires Python 3 and a C compiler. Builds the unchanged bundled Lua with asserted
32-bit integers/floats and a 2 MiB allocator cap. No downloads or firmware build.
The harness records drawing/API calls and in-memory saves; it does not render pixels.
Read-only debug inspection checks actual app closure state. Input is delivered directly
without adapter-side normalization. Tests cover Sticky stock-order tap/click, ignored
non-tap events, button controls, saves/reload, and app-specific targets. On x4pro,
x4, x3, simulator, and missing get_device, legacy calls still work and stock-order
calls remain ignored. The baseline option requires Sticky to fail before the fix and
compares complete non-Sticky drawing/API/save traces byte for byte.

The cap includes test instrumentation and is not a measurement of firmware heap use
or a C3 memory qualification. No 75 KiB compatibility claim is made.
