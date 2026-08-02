#pragma once

// PlatformIO normally supplies these through build_flags/extra_scripts. Keep
// fallbacks here so editor indexers and simulator-like tools still parse files.
#ifndef CROSSINKY_VERSION
#define CROSSINKY_VERSION "dev"
#endif

#ifndef CROSSINKY_BUILD_ENV
#define CROSSINKY_BUILD_ENV "unknown"
#endif

#ifndef CROSSINKY_FIRMWARE_VARIANT
#ifdef CROSSPOINT_FIRMWARE_VARIANT
#define CROSSINKY_FIRMWARE_VARIANT CROSSPOINT_FIRMWARE_VARIANT
#else
#define CROSSINKY_FIRMWARE_VARIANT "unknown"
#endif
#endif
