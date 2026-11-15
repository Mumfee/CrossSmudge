#pragma once
#include "TestPlatform.h"
// Composition-only adapter of the header Back hit area.
namespace TouchHeaderBackButton {
inline int height(const Metrics& metrics, const MappedInputManager&) { return metrics.headerHeight; }
inline bool wasTapped(const MappedInputManager& input, const Rect& header) {
  int x, y;
  return input.hasTouch() && input.wasScreenTapped(x, y) && x >= header.x && x < header.x + 48 &&
         y >= header.y && y < header.bottom();
}
inline void draw(GfxRenderer&, const Rect&, const char*, bool, int, const char*) {}
}
