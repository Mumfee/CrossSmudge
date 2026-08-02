#pragma once

#include "activities/Activity.h"
#include "components/UITheme.h"
#include "SmudgeSettings.h"

#include <algorithm>
#include <cstdio>
#include <string>

class AppSettingsActivity : public Activity {
public:
  AppSettingsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("AppSettings", renderer, mappedInput) {}

  ~AppSettingsActivity() override {
    SmudgeSettings::getInstance().save();
  }

  void onEnter() override {
    Activity::onEnter();
    requestUpdate();
  }

  void loop() override {
    auto& settings = SmudgeSettings::getInstance();
    int totalItems = 1 + static_cast<int>(settings.apps.size());

    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      settings.save();
      finish();
      return;
    }

    if (mappedInput.wasReleased(MappedInputManager::Button::Right) ||
        mappedInput.wasReleased(MappedInputManager::Button::Down)) {
      cursorIndex = (cursorIndex + 1) % std::max(1, totalItems);
      requestUpdate();
    }
    else if (mappedInput.wasReleased(MappedInputManager::Button::Left) ||
             mappedInput.wasReleased(MappedInputManager::Button::Up)) {
      cursorIndex = (cursorIndex == 0) ? std::max(0, totalItems - 1) : (cursorIndex - 1);
      requestUpdate();
    }
    else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      if (cursorIndex == 0) {
        settings.sortMode = (settings.sortMode == MenuSortMode::Alphabetical)
                                ? MenuSortMode::MostUsed
                                : MenuSortMode::Alphabetical;
      } else {
        size_t appIdx = static_cast<size_t>(cursorIndex - 1);
        if (appIdx < settings.apps.size()) {
          settings.apps[appIdx].visible = !settings.apps[appIdx].visible;
        }
      }
      settings.save();
      requestUpdate();
    }
  }

  void render(RenderLock&& lock) override {
    renderer.clearScreen();

    const int pageWidth = renderer.getScreenWidth();
    const int pageHeight = renderer.getScreenHeight();
    const auto& metrics = UITheme::getInstance().getMetrics();

    const int headerY = metrics.topPadding;
    const int headerH = metrics.headerHeight;
    const int startY  = headerY + headerH + metrics.verticalSpacing; 
    const int menuHeight = pageHeight - startY - metrics.buttonHintsHeight - metrics.verticalSpacing;

    auto& settings = SmudgeSettings::getInstance();
    int totalItems = 1 + static_cast<int>(settings.apps.size());

    // 1. Draw Header
    GUI.drawHeader(renderer, Rect{0, headerY, pageWidth, headerH}, "App Settings");

    // 2. Draw Menu with App-Specific Icons
    GUI.drawButtonMenu(
        renderer,
        Rect{0, startY, pageWidth, menuHeight},
        totalItems,
        cursorIndex,
        [&settings](int index) -> const char* {
          static char itemBuf[64];
          if (index == 0) {
            return (settings.sortMode == MenuSortMode::Alphabetical)
                       ? "Menu Order: Alphabetical"
                       : "Menu Order: Most Used";
          }
          size_t appIdx = static_cast<size_t>(index - 1);
          if (appIdx < settings.apps.size()) {
            const auto& app = settings.apps[appIdx];
            snprintf(itemBuf, sizeof(itemBuf), "%s: %s", 
                     app.appName.c_str(), 
                     app.visible ? "SHOW" : "HIDE");
            return itemBuf;
          }
          return "";
        },
        [&settings](int index) {
          if (index == 0) {
            return UIIcon::Applications;
          }
          size_t appIdx = static_cast<size_t>(index - 1);
          if (appIdx < settings.apps.size()) {
            const std::string& name = settings.apps[appIdx].appName;
            if (name == "Dice") return UIIcon::Dice;
            if (name == "Wordle") return UIIcon::Wordle;
            if (name == "Life Counter") return UIIcon::LifeCounter;
            if (name == "Rosary") return UIIcon::Rosary;
            if (name == "2048") return UIIcon::TwoZeroFourEight;
            if (name == "Sudoku") return UIIcon::Sudoku;
          }
          return UIIcon::Applications;
        }
    );

    // 3. Draw Footer Hints
    const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

    renderer.displayBuffer();
  }

private:
  int cursorIndex = 0;
};