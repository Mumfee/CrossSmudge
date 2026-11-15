#!/usr/bin/env bash
set -euo pipefail
# No downloads. Supply an existing ArduinoJson checkout/dependency cache.
: "${ARDUINOJSON_INCLUDE_DIR:?Set ARDUINOJSON_INCLUDE_DIR to ArduinoJson/src}"
repo_root=$(cd "$(dirname "$0")/../.." && pwd)
build_dir=${APP_STORE_TEST_BUILD_DIR:-$(mktemp -d /tmp/app-store-tests.XXXXXX)}
mkdir -p "$build_dir"
common=(-std=c++17 -Wall -Wextra -Wno-unused-variable -I"$repo_root/test/app_store/stubs" -I"$repo_root/src" -I"$ARDUINOJSON_INCLUDE_DIR")
if [[ -n "${APP_STORE_ACTIVITY_HEADER:-}" ]]; then
  common+=('-DAPP_STORE_WIFI_COMPOSITION' "-DAPP_STORE_ACTIVITY_HEADER=\"$APP_STORE_ACTIVITY_HEADER\"")
fi
"${CXX:-g++}" "${common[@]}" "$repo_root/test/app_store/app_store_test.cpp" -o "$build_dir/app_store_test"
"$build_dir/app_store_test"
"${CXX:-g++}" "${common[@]}" -DFREEINK_NET_WOLFSSL -fsanitize=address,undefined -fno-omit-frame-pointer -g "$repo_root/test/app_store/app_store_test.cpp" -o "$build_dir/app_store_test_sanitized"
# Override ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 only in ptrace runners.
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=1:halt_on_error=1}" UBSAN_OPTIONS="${UBSAN_OPTIONS:-halt_on_error=1}" "$build_dir/app_store_test_sanitized"
printf 'Binaries retained in %s\n' "$build_dir"
