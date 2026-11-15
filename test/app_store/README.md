# App Store production behavior tests

This small native executable includes the real `src/activities/apps/AppStoreActivity.h` and uses an existing ArduinoJson dependency to parse catalog fixtures. It executes the activity's loading, caching, rendering, loop scheduling and input handling. It does not copy the implementation into a test model.

The host adapters in `stubs/` provide a recording renderer, in-memory storage with injected failures, a deterministic network fake, a controllable clock and input snapshots. Input queries do not consume events, matching production; `Fixture::tick()` clears them between frames. Font measurements and safe-area policy are adapters, not production rasterization. These tests are geometry/behavior evidence, not screenshots, firmware compilation, actual TLS/network validation, concurrency stress, or on-glass verification.

Coverage includes:

- Only visible-page icons load, at most one per idle loop, once per catalog session; rendering never fetches. Back/input takes priority. Cached icons work offline without implying installation.
- Exact 128-byte local/remote icons; independent installed/legacy/cache fallback; malformed and unsafe IDs; truncated, oversized, failed and corrupt reads; write-open, short-write, sync, close and network failures. Invalid downloads never reach storage; failed cache readback never publishes pixels.
- Storage opens occur under the render lock; streaming occurs outside it. The icon transport remains `ESP_HTTP` even with `FREEINK_NET_WOLFSSL` defined.
- Real catalog JSON parsing, installed-state distinction, portrait/landscape capacity, all four safe-area orientations with asymmetric bezel margins, larger font metrics, long text, UTF-8 boundaries, navigation/wrapping, last pages, exact footer boundaries and touch Back on an empty list.

## Run without downloads

Use an existing ArduinoJson `src` directory. The script builds in `/tmp`, retains both binaries, and runs normal and ASan/UBSan variants. The sanitizer variant additionally defines `FREEINK_NET_WOLFSSL`.

```sh
ARDUINOJSON_INCLUDE_DIR=/path/to/ArduinoJson/src \
  test/app_store/run_tests.sh
```

`APP_STORE_TEST_BUILD_DIR` selects an existing output directory; `CXX` selects the compiler. Sanitizers halt on errors. This session's execution environment uses ptrace, which prevents LeakSanitizer from running; the recorded runs use `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1`. ASan and UBSan remain enabled. The script defaults to enabling leak detection elsewhere.

The target also registers as `app_store_behavior` in the normal `test/CMakeLists.txt`. It can be configured standalone:

```sh
cmake -S test/app_store -B /tmp/app-store-cmake \
  -DARDUINOJSON_INCLUDE_DIR=/path/to/ArduinoJson/src \
  -DAPP_STORE_SANITIZERS=ON
cmake --build /tmp/app-store-cmake
ctest --test-dir /tmp/app-store-cmake --output-on-failure
```

CMake was unavailable in the task environment, so validation used the direct compiler script. No firmware/SDK build or dependency download is needed.

## Wi-Fi composition

After applying only this change's AppStore header delta to a private temporary copy of the owned Wi-Fi fix header, run:

```sh
APP_STORE_ACTIVITY_HEADER=/tmp/composed/AppStoreActivity.h \
ARDUINOJSON_INCLUDE_DIR=/path/to/ArduinoJson/src \
  test/app_store/run_tests.sh
```

This runs the same suite plus a composition scenario: touch Connect starts the picker, its result defers catalog loading to the normal loop, then catalog icons load. `TouchActionButtons` and `TouchHeaderBackButton` are explicitly labeled host adapters for this scenario; their renderer implementation is outside this test. No other worktree is modified.

The final suite also compiles against the original header with `APP_STORE_BASELINE` and `APP_STORE_ACTIVITY_HEADER` defined directly in the compiler command. `APP_STORE_BASELINE` skips only new-footer-helper tests that cannot compile against the old header. The original pre-change run and the expanded baseline run are retained in the task handoff as RED evidence; the expanded baseline produced 204 failed assertions.
