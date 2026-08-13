#pragma once

// PlatformIO normally supplies these through build_flags/extra_scripts. Keep
// fallbacks here so editor indexers and simulator-like tools still parse files.
#ifndef CROSSSMUDGE_VERSION
#define CROSSSMUDGE_VERSION "dev"
#endif

#ifndef CROSSSMUDGE_BUILD_ENV
#define CROSSSMUDGE_BUILD_ENV "unknown"
#endif

#ifndef CROSSSMUDGE_FIRMWARE_DEVICE_TYPE
#define CROSSSMUDGE_FIRMWARE_DEVICE_TYPE "unknown"
#endif
