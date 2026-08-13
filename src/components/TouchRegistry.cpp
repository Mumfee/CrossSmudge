#include "TouchRegistry.h"

#if CROSSSMUDGE_APP_CAP_TOUCH

namespace {
bool contains(const Rect& rect, const int x, const int y) {
  return x >= rect.x && x < rect.x + rect.width && y >= rect.y && y < rect.y + rect.height;
}
}  // namespace

TouchRegistry& TouchRegistry::getInstance() {
  static TouchRegistry instance;
  return instance;
}

void TouchRegistry::beginFrame() {
  if (!enabled_) return;
  counts_[backIndex()] = 0;
}

void TouchRegistry::add(const Rect& rect, const int id, const Kind kind) {
  if (!enabled_) return;
  const uint8_t bufferIndex = backIndex();
  size_t& count = counts_[bufferIndex];
  if (count >= CAPACITY) return;
  buffers_[bufferIndex][count] = Target{rect, static_cast<int16_t>(id), static_cast<uint8_t>(kind)};
  ++count;
}

void TouchRegistry::publish() {
  if (!enabled_) return;
  live_.store(backIndex(), std::memory_order_release);
}

void TouchRegistry::clear() {
  counts_[0] = 0;
  counts_[1] = 0;
  live_.store(0, std::memory_order_release);
}

bool TouchRegistry::hitTest(const int x, const int y, const Kind kind, int& outId) const {
  if (!enabled_) return false;
  const uint8_t bufferIndex = live_.load(std::memory_order_acquire);
  const size_t count = counts_[bufferIndex];
  const auto& buffer = buffers_[bufferIndex];
  for (size_t i = count; i-- > 0;) {
    if (buffer[i].kind == kind && contains(buffer[i].rect, x, y)) {
      outId = buffer[i].id;
      return true;
    }
  }
  return false;
}
#endif
