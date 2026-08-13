#pragma once

// The app deliberately has a build-time touch capability, separate from a
// runtime probe. Button-only images must not retain the touch UI merely because
// the shared source tree also builds for Sticky.
#ifndef CROSSSMUDGE_APP_CAP_TOUCH
#error "Define CROSSSMUDGE_APP_CAP_TOUCH as 0 or 1 in the PlatformIO environment"
#endif

#if CROSSSMUDGE_APP_CAP_TOUCH != 0 && CROSSSMUDGE_APP_CAP_TOUCH != 1
#error "CROSSSMUDGE_APP_CAP_TOUCH must be 0 or 1"
#endif

// Native simulator BoardConfig deliberately has no FREEINK_CAP_TOUCH macro.
// Firmware builds must keep the app and SDK capability selections in lockstep.
#if !defined(SIMULATOR)
#include <BoardConfig.h>
#if CROSSSMUDGE_APP_CAP_TOUCH != FREEINK_CAP_TOUCH
#error "CROSSSMUDGE_APP_CAP_TOUCH must match FREEINK_CAP_TOUCH"
#endif
#endif
