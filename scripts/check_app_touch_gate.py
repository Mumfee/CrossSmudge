"""Reject app touch symbols from firmware configured without touch UI."""

import os
import subprocess


FORBIDDEN_SYMBOLS = (
    "TouchRegistry",
    "HalGPIO::hasTouch",
    "HalGPIO::wasTouch",
    "HalGPIO::isTouch",
    "HalGPIO::lastTouch",
    "HalGPIO::wasSwipe",
    "MappedInputManager::hasTouch",
    "MappedInputManager::wasScreen",
    "MappedInputManager::isScreenTouch",
    "MappedInputManager::wasItemTap",
    "MappedInputManager::wasTabTap",
    "MappedInputManager::wasCoverTap",
    "MappedInputManager::wasSwipe",
    "MappedInputManager::wasLeftEdgeGesture",
    "MappedInputManager::wasHomeGesture",
    "MappedInputManager::wasMenuGesture",
    "ReaderUtils::detectTouchPageTurn",
)


def _capability_enabled(env):
    for define in env.get("CPPDEFINES", []):
        if isinstance(define, (list, tuple)):
            name, value = define[0], define[1]
        else:
            name, value = define, None
        if name == "CROSSSMUDGE_APP_CAP_TOUCH":
            return str(value) == "1"
    raise RuntimeError("CROSSSMUDGE_APP_CAP_TOUCH is missing from CPPDEFINES")


def audit_app_touch_gate(target, source, env):
    del source
    if _capability_enabled(env):
        return

    toolchain_dir = env.PioPlatform().get_package_dir("toolchain-riscv32-esp")
    nm = os.path.join(toolchain_dir, "bin", "riscv32-esp-elf-nm")
    output = subprocess.check_output([nm, "--demangle", "--defined-only", str(target[0])], text=True)
    leaked = [line for line in output.splitlines() if any(symbol in line for symbol in FORBIDDEN_SYMBOLS)]
    if leaked:
        print("Non-touch firmware retained app touch symbols:")
        print("\n".join(leaked))
        env.Exit(1)

    print("App touch gate audit passed: no app touch symbols in non-touch firmware")


Import("env")
env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", audit_app_touch_gate)
