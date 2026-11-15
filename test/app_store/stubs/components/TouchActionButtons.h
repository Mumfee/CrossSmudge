#pragma once
#include "TestPlatform.h"
// Composition-only adapter; the real Wi-Fi touch helper has separate coverage.
namespace TouchActionButtons {
constexpr int kDefaultHeight = 56;
struct Layout { Rect container; int count; };
inline Layout vertical(Rect rect, uint8_t count) { return {rect, count}; }
inline int indexAt(const Layout& layout, int x, int y) {
  const auto& r = layout.container;
  return layout.count && x >= r.x && x < r.right() && y >= r.y && y < r.bottom() ? 0 : -1;
}
inline void draw(GfxRenderer&, const Layout&, const char* const*, int) {}
}
