#pragma once

// PlatformIO normally supplies these through build_flags/extra_scripts. Keep
// fallbacks here so editor indexers and simulator-like tools still parse files.
#ifndef CROSSSMUDGE_VERSION
#define CROSSSMUDGE_VERSION "dev"
#endif

#ifndef CROSSINK_VERSION
#define CROSSINK_VERSION CROSSSMUDGE_VERSION
#endif

#ifndef CROSSINK_GIT_SHA
#define CROSSINK_GIT_SHA "unknown"
#endif

#ifndef CROSSINK_GIT_DIRTY
#define CROSSINK_GIT_DIRTY "unknown"
#endif

#ifndef CROSSSMUDGE_BUILD_ENV
#ifdef CROSSINK_BUILD_ENV
#define CROSSSMUDGE_BUILD_ENV CROSSINK_BUILD_ENV
#else
#define CROSSSMUDGE_BUILD_ENV "unknown"
#endif
#endif

#ifndef CROSSINK_BUILD_ENV
#define CROSSINK_BUILD_ENV CROSSSMUDGE_BUILD_ENV
#endif

#ifndef CROSSSMUDGE_FIRMWARE_DEVICE_TYPE
#ifdef CROSSINK_FIRMWARE_DEVICE_TYPE
#define CROSSSMUDGE_FIRMWARE_DEVICE_TYPE CROSSINK_FIRMWARE_DEVICE_TYPE
#else
#define CROSSSMUDGE_FIRMWARE_DEVICE_TYPE "unknown"
#endif
#endif

#ifndef CROSSINK_FIRMWARE_DEVICE_TYPE
#define CROSSINK_FIRMWARE_DEVICE_TYPE CROSSSMUDGE_FIRMWARE_DEVICE_TYPE
#endif
