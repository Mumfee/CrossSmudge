#pragma once
#include <Logging.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <atomic>

// Base class for FreeRTOS tasks with lifecycle management.
// Subclasses implement run(); owners call start()/stop()/wait().
class Task {
  std::atomic<bool> running{false};
  std::atomic<bool> stopRequested{false};

  static void trampoline(void* p) {
    auto* self = static_cast<Task*>(p);
    self->run();
    self->running.store(false);
    vTaskDelete(nullptr);
  }

 public:
  virtual ~Task() {
    stop();
    wait();
  }

  bool start(const char* name, uint32_t stackBytes, uint8_t priority) {
    stopRequested.store(false);
    running.store(true);
    if (xTaskCreate(trampoline, name, stackBytes, this, priority, nullptr) != pdPASS) {
      running.store(false);
      LOG_ERR("TASK", "Could not start %s", name);
      return false;
    }
    return true;
  }

  void stop() { stopRequested.store(true); }

  void wait() {
    while (running.load()) vTaskDelay(1);
  }

  bool isRunning() const { return running.load(); }
  bool shouldStop() const { return stopRequested.load(); }

 protected:
  virtual void run() = 0;
};
