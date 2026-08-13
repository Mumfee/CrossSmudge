#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <atomic>

class DictionaryLookupController;

// A single, static dictionary worker avoids allocating a 4 KB task stack for
// every lookup. Controllers still own all lookup state and must wait for the
// worker before they are destroyed.
class DictionaryLookupWorker {
 public:
  static DictionaryLookupWorker& instance();

  // Starts a lookup for owner. Returns false when the static task could not be
  // created or another controller still owns the worker.
  bool start(DictionaryLookupController& owner);

  // Waits until owner is no longer running on the worker. The owner must set
  // its cancellation flag before calling this method.
  void waitForOwner(const DictionaryLookupController& owner);

 private:
  static constexpr size_t kStackBytes = 4096;
  // ESP-IDF's xTaskCreateStatic takes its depth in bytes. Keep the storage
  // expressed in StackType_t units so the backing buffer is sized correctly.
  static constexpr size_t kStackWords = (kStackBytes + sizeof(StackType_t) - 1) / sizeof(StackType_t);

  static void taskEntry(void* context);
  void run();

  StaticTask_t taskStorage_ = {};
  StackType_t stack_[kStackWords] = {};
  TaskHandle_t taskHandle_ = nullptr;
  std::atomic<DictionaryLookupController*> owner_{nullptr};
};
