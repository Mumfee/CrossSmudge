#pragma once

#include <algorithm>
#include <cstdio>
#include <string>

#include "SmudgeSettings.h"
#include "activities/Activity.h"
#include "components/UITheme.h"

class AppSettingsActivity : public Activity {
 public:
  AppSettingsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("AppSettings", renderer, mappedInput) {}

  ~AppSettingsActivity() override { SmudgeSettings::getInstance().save(); }

  void onEnter() override {
    Activity::onEnter();
    auto& settings = SmudgeSettings::getInstance();
    cursorIndex = (settings.sortMode == MenuSortMode::Alphabetical) ? 0 : 1;
    requestUpdate();
  }

  void loop() override {
    auto& settings = SmudgeSettings::getInstance();
    constexpr int totalItems = 2;

    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      settings.save();
      finish();
      return;
    }

    int touchedIndex = -1;
    if (mappedInput.wasItemTapped(touchedIndex) && touchedIndex >= 0 && touchedIndex < totalItems) {
      cursorIndex = touchedIndex;
      applySelection();
      return;
    }

    if (mappedInput.wasReleased(MappedInputManager::Button::Right) ||
        mappedInput.wasReleased(MappedInputManager::Button::Down)) {
      cursorIndex = (cursorIndex + 1) % totalItems;
      requestUpdate();
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Left) ||
               mappedInput.wasReleased(MappedInputManager::Button::Up)) {
      cursorIndex = (cursorIndex - 1 + totalItems) % totalItems;
      requestUpdate();
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      applySelection();
    }
  }

  void render(RenderLock&& lock) override {
    renderer.clearScreen();

    const int pageWidth = renderer.getScreenWidth();
    const int pageHeight = renderer.getScreenHeight();
    const auto& metrics = UITheme::getInstance().getMetrics();

    const int headerY = metrics.topPadding;
    const int headerH = metrics.headerHeight;
    const int startY = headerY + headerH + metrics.verticalSpacing;
    const int menuHeight = pageHeight - startY - metrics.buttonHintsHeight - metrics.verticalSpacing;

    auto& settings = SmudgeSettings::getInstance();
    constexpr int totalItems = 2;

    // 1. Draw Header
    GUI.drawHeader(renderer, Rect{0, headerY, pageWidth, headerH}, "App Settings");

    // 2. Draw Menu with Sort Options
    GUI.drawButtonMenu(
        renderer, Rect{0, startY, pageWidth, menuHeight}, totalItems, cursorIndex,
        [&settings](int index) -> const char* {
          if (index == 0) {
            return (settings.sortMode == MenuSortMode::Alphabetical) ? "Alphabetical (A - Z)  [Active]"
                                                                     : "Alphabetical (A - Z)";
          } else if (index == 1) {
            return (settings.sortMode == MenuSortMode::MostUsed) ? "Most Frequently Used  [Active]"
                                                                 : "Most Frequently Used";
          }
          return "";
        },
        [](int index) { return (index == 0) ? UIIcon::Applications : UIIcon::Recent; });

    // 3. Draw Footer Hints
    const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

    renderer.displayBuffer();
  }

 private:
  int cursorIndex = 0;

  void applySelection() {
    auto& settings = SmudgeSettings::getInstance();
    if (cursorIndex == 0) {
      settings.sortMode = MenuSortMode::Alphabetical;
    } else {
      settings.sortMode = MenuSortMode::MostUsed;
    }
    settings.save();
    requestUpdate();
  }
};