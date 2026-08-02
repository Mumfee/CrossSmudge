#pragma once

#include "activities/Activity.h"
#include "activities/apps/DiceSimActivity.h"
#include "activities/apps/WordleActivity.h"
#include "activities/apps/LifeCounterActivity.h"
#include "activities/apps/RosaryActivity.h"
#include "activities/apps/TwoZeroFourEightActivity.h"
#include "activities/apps/SudokuActivity.h"
#include "activities/apps/AppSettingsActivity.h"
#include "SmudgeSettings.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class ApplicationsActivity : public Activity {
public:
  ApplicationsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Applications", renderer, mappedInput) {}

  void onEnter() override {
    Activity::onEnter();
    refreshMenuList();
    requestUpdate();
  }

  void loop() override {
    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      finish();
      return;
    }

    int appCount = static_cast<int>(visibleApps.size());
    if (appCount == 0) return;

    if (mappedInput.wasReleased(MappedInputManager::Button::Right) || 
        mappedInput.wasReleased(MappedInputManager::Button::Down)) {
      selectedIndex = (selectedIndex + 1) % appCount;
      requestUpdate();
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Left) || 
               mappedInput.wasReleased(MappedInputManager::Button::Up)) {
      selectedIndex = (selectedIndex - 1 + appCount) % appCount;
      requestUpdate();
    }

    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      const auto& app = visibleApps[selectedIndex];

      if (app.name != "App Settings") {
        SmudgeSettings::getInstance().recordAppLaunch(app.name);
      }

      startActivityForResult(
          app.factory(),
          [this](const ActivityResult&) {
            refreshMenuList();
            requestUpdate();
          }
      );
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

    GUI.drawHeader(renderer, Rect{0, headerY, pageWidth, headerH}, "Applications");

    int appCount = static_cast<int>(visibleApps.size());
    GUI.drawButtonMenu(
        renderer,
        Rect{0, startY, pageWidth, menuHeight},
        appCount,
        selectedIndex,
        [this](int index) { return visibleApps[index].name.c_str(); },
        [this](int index) { return visibleApps[index].icon; }
    );

    const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

    renderer.displayBuffer();
  }

private:
  struct AppEntry {
    std::string name;
    UIIcon icon;
    std::function<std::unique_ptr<Activity>()> factory;
  };

  int selectedIndex = 0;
  std::vector<AppEntry> visibleApps;

  void refreshMenuList() {
    auto& settings = SmudgeSettings::getInstance();

    std::vector<AppEntry> allApps = {
      { "Dice", UIIcon::Dice, [this]() { return std::make_unique<DiceSimActivity>(renderer, mappedInput); } },
      { "Wordle", UIIcon::Wordle, [this]() { return std::make_unique<WordleActivity>(renderer, mappedInput); } },
      { "Life Counter", UIIcon::LifeCounter, [this]() { return std::make_unique<LifeCounterActivity>(renderer, mappedInput); } },
      { "Rosary", UIIcon::Rosary, [this]() { return std::make_unique<RosaryActivity>(renderer, mappedInput); } },
      { "2048", UIIcon::TwoZeroFourEight, [this]() { return std::make_unique<TwoZeroFourEightActivity>(renderer, mappedInput); } },
      { "Sudoku", UIIcon::Sudoku, [this]() { return std::make_unique<SudokuActivity>(renderer, mappedInput); } }
    };

    std::vector<std::string> allNames;
    for (const auto& a : allApps) {
      allNames.push_back(a.name);
    }
    settings.registerKnownApps(allNames);

    visibleApps.clear();
    for (const auto& app : allApps) {
      if (settings.isAppVisible(app.name)) {
        visibleApps.push_back(app);
      }
    }

    if (settings.sortMode == MenuSortMode::Alphabetical) {
      std::sort(visibleApps.begin(), visibleApps.end(), [](const AppEntry& a, const AppEntry& b) {
        return a.name < b.name;
      });
    } else if (settings.sortMode == MenuSortMode::MostUsed) {
      std::sort(visibleApps.begin(), visibleApps.end(), [&](const AppEntry& a, const AppEntry& b) {
        auto itA = std::find_if(settings.apps.begin(), settings.apps.end(), [&](const AppUsageData& d) { return d.appName == a.name; });
        auto itB = std::find_if(settings.apps.begin(), settings.apps.end(), [&](const AppUsageData& d) { return d.appName == b.name; });
        float scoreA = (itA != settings.apps.end()) ? itA->usageScore : 0.0f;
        float scoreB = (itB != settings.apps.end()) ? itB->usageScore : 0.0f;
        return scoreA > scoreB;
      });
    }

    // Always append App Settings at the bottom
    visibleApps.push_back({
      "Settings", UIIcon::Settings, [this]() {
        return std::make_unique<AppSettingsActivity>(renderer, mappedInput);
      }
    });

    if (selectedIndex >= static_cast<int>(visibleApps.size())) {
      selectedIndex = std::max(0, static_cast<int>(visibleApps.size()) - 1);
    }
  }
};