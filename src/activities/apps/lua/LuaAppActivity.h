#pragma once

#include <memory>
#include <mutex>
#include <string>

#include "GfxRenderer.h"
#include "MappedInputManager.h"
#include "activities/Activity.h"
#include "activities/apps/lua/LuaRunner.h"
#include "components/UITheme.h"
#include "fontIds.h"

class LuaAppActivity : public Activity {
 public:
  LuaAppActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, const std::string& appDir,
                 const std::string& appName, const std::string& entryScript = "main.lua")
      : Activity(appName.c_str(), renderer, mappedInput),
        appDir_(appDir),
        appName_(appName),
        entryScript_(entryScript) {
    // Generate a simple appId from folder name
    size_t lastSlash = appDir_.find_last_of("/\\");
    if (lastSlash != std::string::npos && lastSlash + 1 < appDir_.length()) {
      appId_ = appDir_.substr(lastSlash + 1);
    } else {
      appId_ = appName_;
    }
  }

  void onEnter() override {
    Activity::onEnter();
    initialOrientation_ = renderer.getOrientation();
    runner_ = std::make_unique<ink::LuaRunner>(renderer, mappedInput, appDir_, appId_);

    if (!runner_->init()) {
      const std::string& err = runner_->getErrorMessage();
      renderError(err.empty() ? "Failed to initialize Lua VM" : err.c_str());
      return;
    }

    std::string scriptPath = appDir_ + "/" + entryScript_;
    if (!runner_->loadScript(scriptPath)) {
      renderError(runner_->getErrorMessage().c_str());
      return;
    }

    {
      std::lock_guard<std::mutex> lock(luaMutex_);
      runner_->onInit();
    }
    requestUpdate();
  }

  bool preventAutoSleep() override { return runner_ && runner_->preventAutoSleep(); }

  void loop() override {
    if (!runner_ || runner_->shouldExit()) {
      finish();
      return;
    }

    if (runner_->hasError()) {
      if (mappedInput.wasReleased(MappedInputManager::Button::Back) ||
          mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
        finish();
        return;
      }
      return;
    }

    bool buttonHandled = false;

    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      std::lock_guard<std::mutex> lock(luaMutex_);
      runner_->onButton("back", true);
      buttonHandled = true;
    }
    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      std::lock_guard<std::mutex> lock(luaMutex_);
      runner_->onButton("confirm", true);
      buttonHandled = true;
    }
    if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
      std::lock_guard<std::mutex> lock(luaMutex_);
      runner_->onButton("left", true);
      buttonHandled = true;
    }
    if (mappedInput.wasReleased(MappedInputManager::Button::Right)) {
      std::lock_guard<std::mutex> lock(luaMutex_);
      runner_->onButton("right", true);
      buttonHandled = true;
    }
    if (mappedInput.wasReleased(MappedInputManager::Button::Up)) {
      std::lock_guard<std::mutex> lock(luaMutex_);
      runner_->onButton("up", true);
      buttonHandled = true;
    } else if (mappedInput.wasReleased(MappedInputManager::Button::PageBack)) {
      std::lock_guard<std::mutex> lock(luaMutex_);
      runner_->onButton("page_back", true);
      buttonHandled = true;
    }
    if (mappedInput.wasReleased(MappedInputManager::Button::Down)) {
      std::lock_guard<std::mutex> lock(luaMutex_);
      runner_->onButton("down", true);
      buttonHandled = true;
    } else if (mappedInput.wasReleased(MappedInputManager::Button::PageForward)) {
      std::lock_guard<std::mutex> lock(luaMutex_);
      runner_->onButton("page_forward", true);
      buttonHandled = true;
    }

    int tx = 0, ty = 0;
    if (mappedInput.wasScreenTapped(tx, ty)) {
      std::lock_guard<std::mutex> lock(luaMutex_);
      runner_->onTap(tx, ty);
      runner_->onTouch(tx, ty, "tap");
      buttonHandled = true;
    }

    auto swipe = mappedInput.wasSwipe();
    if (swipe != MappedInputManager::SwipeDir::None) {
      const char* dirStr = "none";
      if (swipe == MappedInputManager::SwipeDir::Up)
        dirStr = "up";
      else if (swipe == MappedInputManager::SwipeDir::Down)
        dirStr = "down";
      else if (swipe == MappedInputManager::SwipeDir::Left)
        dirStr = "left";
      else if (swipe == MappedInputManager::SwipeDir::Right)
        dirStr = "right";
      std::lock_guard<std::mutex> lock(luaMutex_);
      runner_->onSwipe(dirStr);
      buttonHandled = true;
    }

    {
      std::lock_guard<std::mutex> lock(luaMutex_);
      runner_->onUpdate();
    }

    if (buttonHandled || (runner_ && runner_->checkAndClearRedrawRequested())) {
      if (runner_->shouldExit()) {
        finish();
        return;
      }
      requestUpdate();
    }
  }

  void render(RenderLock&& lock) override {
    if (!runner_) return;
    if (runner_->hasError()) {
      renderError(runner_->getErrorMessage().c_str());
      return;
    }
    {
      std::lock_guard<std::mutex> lk(luaMutex_);
      runner_->onDraw();
    }
    if (runner_->hasError()) {
      renderError(runner_->getErrorMessage().c_str());
      return;
    }
    HalDisplay::RefreshMode refreshMode = (runner_ && runner_->checkAndClearFullRefreshRequested())
                                              ? HalDisplay::RefreshMode::FULL_REFRESH
                                              : HalDisplay::RefreshMode::FAST_REFRESH;
    renderer.displayBuffer(refreshMode);
  }

  void onExit() override {
    {
      std::lock_guard<std::mutex> lock(luaMutex_);
      if (runner_) {
        runner_->onExit();
        runner_->shutdown();
        runner_.reset();
      }
    }
    renderer.setOrientation(initialOrientation_);
    Activity::onExit();
  }

 private:
  std::string appDir_;
  std::string appName_;
  std::string entryScript_;
  std::string appId_;
  std::unique_ptr<ink::LuaRunner> runner_;
  std::mutex luaMutex_;
  GfxRenderer::Orientation initialOrientation_ = GfxRenderer::Orientation::Portrait;

  void renderError(const char* msg) {
    renderer.setOrientation(initialOrientation_);
    int w = renderer.getScreenWidth();
    int h = renderer.getScreenHeight();
    const auto& m = UITheme::getInstance().getMetrics();

    renderer.clearScreen();
    GUI.drawHeader(renderer, Rect{0, m.topPadding, w, m.headerHeight}, appName_.c_str(), "Script Error");

    renderer.drawCenteredText(UI_12_FONT_ID, h / 2 - 40, "Application Error", true, EpdFontFamily::BOLD);
    if (msg) {
      auto lines = renderer.wrappedText(SMALL_FONT_ID, msg, w - 64, 5);
      int y = h / 2 - 10;
      for (const auto& line : lines) {
        renderer.drawCenteredText(SMALL_FONT_ID, y, line.c_str(), true);
        y += renderer.getLineHeight(SMALL_FONT_ID) + 2;
      }
    }

    const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), "", "");
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
    renderer.displayBuffer();
  }
};
