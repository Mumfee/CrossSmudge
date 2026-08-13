#include "DictionaryLookupWorker.h"

#include <Logging.h>

#include "DictionaryLookupController.h"

DictionaryLookupWorker& DictionaryLookupWorker::instance() {
  static DictionaryLookupWorker worker;
  return worker;
}

bool DictionaryLookupWorker::start(DictionaryLookupController& owner) {
  if (taskHandle_ == nullptr) {
    taskHandle_ = xTaskCreateStatic(taskEntry, "DictLookup", kStackBytes, this, 1, stack_, &taskStorage_);
    if (taskHandle_ == nullptr) {
      LOG_ERR("DICT", "Could not start static dictionary lookup worker");
      return false;
    }
  }

  DictionaryLookupController* expected = nullptr;
  if (!owner_.compare_exchange_strong(expected, &owner, std::memory_order_release, std::memory_order_relaxed)) {
    LOG_ERR("DICT", "Dictionary lookup worker is busy");
    return false;
  }

  xTaskNotify(taskHandle_, 1, eIncrement);
  return true;
}

void DictionaryLookupWorker::waitForOwner(const DictionaryLookupController& owner) {
  while (owner_.load(std::memory_order_acquire) == &owner) vTaskDelay(1);
}

void DictionaryLookupWorker::taskEntry(void* context) { static_cast<DictionaryLookupWorker*>(context)->run(); }

void DictionaryLookupWorker::run() {
  while (true) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    auto* owner = owner_.load(std::memory_order_acquire);
    if (owner == nullptr) continue;
    owner->runLookup();
    owner_.store(nullptr, std::memory_order_release);
  }
}
