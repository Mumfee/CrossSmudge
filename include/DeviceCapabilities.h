#pragma once

#include <HalGPIO.h>

#include "AppCapabilities.h"

inline bool deviceHasEdgeSideButtons(const HalGPIO& gpio) {
#ifdef SIMULATOR
  return gpio.deviceIsX3();
#else
  return gpio.hasEdgeSideButtons();
#endif
}

inline bool deviceUsesSideButtonHintGutters(const HalGPIO& gpio) {
  if (!deviceHasEdgeSideButtons(gpio)) return false;
#if CROSSSMUDGE_APP_CAP_TOUCH
  return !gpio.hasTouch();
#else
  return true;
#endif
}
