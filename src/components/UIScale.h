#pragma once
#include "CrossPointSettings.h"
#include "fontIds.h"

// Maps the uiScale setting to the FreeInkUI font slots. Small and body use the
// same font so list labels and their values have the same visible size. Row
// heights, header height, and touch sizes are derived from the body font's line
// height by FreeInkApp.
struct UIScaleSpec {
  int smallFontId;
  int bodyFontId;
  int titleFontId;
};

inline UIScaleSpec uiScaleSpec() {
  UIScaleSpec spec{};
  switch (SETTINGS.uiScale) {
    case CrossPointSettings::UI_SCALE_SMALL:
      spec.bodyFontId = UI_10_FONT_ID;
      spec.titleFontId = UI_12_FONT_ID;
      break;
    case CrossPointSettings::UI_SCALE_LARGE:
      spec.bodyFontId = UI_12_FONT_ID;
      spec.titleFontId = UI_12_FONT_ID;
      break;
    default:
      spec.bodyFontId = UI_12_FONT_ID;
      spec.titleFontId = UI_12_FONT_ID;
      break;
  }
  spec.smallFontId = spec.bodyFontId;
  return spec;
}
