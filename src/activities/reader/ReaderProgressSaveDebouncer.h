#pragma once

#include <Arduino.h>

#include <cstdint>

// Allocation-free save policy adapted from Sichroteph/YACP commit
// 3f3c5fc42e794c021edb9832856ef98c2d2065b9 (MIT). Callers retain ownership
// of their persistence format and explicitly flush pending state on exit.
class ReaderProgressSaveDebouncer {
  static constexpr uint8_t PAGE_CHANGE_INTERVAL = 10;
  static constexpr unsigned long MAX_SAVE_INTERVAL_MS = 5UL * 60UL * 1000UL;

  uint32_t lastPositionKey_ = 0;
  uint32_t lastMetadataKey_ = 0;
  unsigned long lastPersistedAtMs_ = 0;
  uint8_t pendingPageChanges_ = 0;
  bool initialized_ = false;
  bool pending_ = false;

 public:
  bool observe(const uint32_t positionKey) { return observe(positionKey, 0); }

  bool observe(const uint32_t positionKey, const uint32_t metadataKey) {
    const unsigned long now = millis();
    if (!initialized_) {
      initialized_ = true;
      lastPositionKey_ = positionKey;
      lastMetadataKey_ = metadataKey;
      lastPersistedAtMs_ = now;
      pendingPageChanges_ = 1;
      pending_ = true;
      return false;
    }

    if (positionKey != lastPositionKey_) {
      lastPositionKey_ = positionKey;
      pending_ = true;
      if (pendingPageChanges_ < UINT8_MAX) {
        ++pendingPageChanges_;
      }
    }
    if (metadataKey != lastMetadataKey_) {
      lastMetadataKey_ = metadataKey;
      pending_ = true;
    }

    return pending_ &&
           (pendingPageChanges_ >= PAGE_CHANGE_INTERVAL || now - lastPersistedAtMs_ >= MAX_SAVE_INTERVAL_MS);
  }

  bool hasPending() const { return pending_; }
  uint32_t lastObservedPosition() const { return lastPositionKey_; }
  uint32_t lastObservedMetadata() const { return lastMetadataKey_; }

  void markPersisted(const uint32_t positionKey) { markPersisted(positionKey, 0); }

  void markPersisted(const uint32_t positionKey, const uint32_t metadataKey) {
    if (!initialized_) {
      initialized_ = true;
      lastPositionKey_ = positionKey;
      lastMetadataKey_ = metadataKey;
    } else if (positionKey != lastPositionKey_ || metadataKey != lastMetadataKey_) {
      return;
    }

    pending_ = false;
    pendingPageChanges_ = 0;
    lastPersistedAtMs_ = millis();
  }
};
