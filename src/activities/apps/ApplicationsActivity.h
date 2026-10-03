#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "SmudgeSettings.h"
#include "activities/Activity.h"
#include "activities/apps/AppSettingsActivity.h"
#include "activities/apps/AppStoreActivity.h"
#include "activities/apps/lua/AppPackage.h"
#include "activities/apps/lua/LuaAppActivity.h"

class ApplicationsActivity : public Activity {
 public:
  ApplicationsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Applications", renderer, mappedInput) {}

  void onEnter() override {
    Activity::onEnter();
    refreshMenuList();
    requestUpdate();
  }

  void onExit() override {
    visibleApps.clear();
    visibleApps.shrink_to_fit();
    Activity::onExit();
  }

  void loop() override {
    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      finish();
      return;
    }

    int appCount = static_cast<int>(visibleApps.size());
    if (appCount == 0) return;

    auto launchSelectedApp = [this]() {
      const auto& app = visibleApps[selectedIndex];
      bool isSetting = (app.name == "App Settings" || app.name == "Settings" || app.name == "App Store");
      if (!isSetting) {
        SmudgeSettings::getInstance().recordAppLaunch(app.name);
      }
      startActivityForResult(app.factory(), [this, isSetting](const ActivityResult&) {
        if (!isSetting) {
          renderer.clearScreen();
          renderer.displayBuffer(HalDisplay::RefreshMode::FULL_REFRESH);
        }
        refreshMenuList();
        requestUpdate();
      });
    };

    int touchedIndex = -1;
    if (mappedInput.wasItemTapped(touchedIndex) && touchedIndex >= 0 && touchedIndex < appCount) {
      selectedIndex = touchedIndex;
      launchSelectedApp();
      return;
    }

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
      launchSelectedApp();
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

    GUI.drawHeader(renderer, Rect{0, headerY, pageWidth, headerH}, "Applications");

    int appCount = static_cast<int>(visibleApps.size());
    GUI.drawButtonMenu(
        renderer, Rect{0, startY, pageWidth, menuHeight}, appCount, selectedIndex,
        [this](int index) { return visibleApps[index].name.c_str(); },
        [this](int index) { return visibleApps[index].icon; });

    // Overlay custom 32x32 raw icons ONLY for packages with UIIcon::None
    const int rowStep = metrics.menuRowHeight + metrics.menuSpacing;
    const int pageItems = std::max(1, (menuHeight + metrics.menuSpacing) / rowStep);
    const int pageStartIndex = (selectedIndex / pageItems) * pageItems;

    for (int i = pageStartIndex; i < appCount && i < pageStartIndex + pageItems; ++i) {
      if (visibleApps[i].icon == UIIcon::None && visibleApps[i].hasIcon) {
        const int displayIndex = i - pageStartIndex;
        const int tileY = startY + displayIndex * rowStep;
        const int lineHeight = renderer.getLineHeight(UI_12_FONT_ID);
        const int textY = tileY + (metrics.menuRowHeight - lineHeight) / 2;
        const int iconY = textY + 3;
        const int iconX = metrics.contentSidePadding + 24;
        renderer.drawIcon(visibleApps[i].iconData, iconX, iconY, 32, 32);
      }
    }

    const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

    renderer.displayBuffer();
  }

 private:
  struct AppEntry {
    std::string name;
    UIIcon icon;
    std::function<std::unique_ptr<Activity>()> factory;
    bool hasIcon = false;
    uint8_t iconData[128] = {0};
  };

  int selectedIndex = 0;
  std::vector<AppEntry> visibleApps;

  void refreshMenuList() {
    auto& settings = SmudgeSettings::getInstance();

    std::vector<AppEntry> allApps;

    // Dynamically discover and load SD card packages (both .crosssmudge and .smudge)
    auto installedPackages = AppPackage::scanApplications();
    for (const auto& pkg : installedPackages) {
      std::string dir = pkg.path;
      std::string name = pkg.name;
      std::string entry = pkg.entryScript;
      AppEntry app;
      app.name = pkg.name;

      std::string id = pkg.id;
      for (auto& c : id) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));

      if (id == "2048")
        app.icon = UIIcon::TwoZeroFourEight;
      else if (id == "blackjack")
        app.icon = UIIcon::Blackjack;
      else if (id == "codex")
        app.icon = UIIcon::Codex;
      else if (id == "dailyoffice")
        app.icon = UIIcon::DailyOffice;
      else if (id == "dice")
        app.icon = UIIcon::Dice;
      else if (id == "lifecounter")
        app.icon = UIIcon::LifeCounter;
      else if (id == "rosary")
        app.icon = UIIcon::Rosary;
      else if (id == "sudoku")
        app.icon = UIIcon::Sudoku;
      else if (id == "tetris")
        app.icon = UIIcon::Tetris;
      else if (id == "wordle")
        app.icon = UIIcon::Wordle;
      else
        app.icon = pkg.hasIcon ? UIIcon::None : UIIcon::Applications;

      app.factory = [this, dir, name, entry]() {
        return std::make_unique<LuaAppActivity>(renderer, mappedInput, dir, name, entry);
      };
      app.hasIcon = pkg.hasIcon;
      if (pkg.hasIcon) {
        std::memcpy(app.iconData, pkg.iconData, sizeof(pkg.iconData));
      }
      allApps.push_back(std::move(app));
    }

    std::vector<std::string> allNames;
    for (const auto& a : allApps) {
      allNames.push_back(a.name);
    }
    settings.registerKnownApps(allNames);

    visibleApps = allApps;

    if (settings.sortMode == MenuSortMode::Alphabetical) {
      std::sort(visibleApps.begin(), visibleApps.end(),
                [](const AppEntry& a, const AppEntry& b) { return a.name < b.name; });
    } else if (settings.sortMode == MenuSortMode::MostUsed) {
      std::sort(visibleApps.begin(), visibleApps.end(), [&](const AppEntry& a, const AppEntry& b) {
        auto itA = std::find_if(settings.apps.begin(), settings.apps.end(),
                                [&](const AppUsageData& d) { return d.appName == a.name; });
        auto itB = std::find_if(settings.apps.begin(), settings.apps.end(),
                                [&](const AppUsageData& d) { return d.appName == b.name; });
        float scoreA = (itA != settings.apps.end()) ? itA->usageScore : 0.0f;
        float scoreB = (itB != settings.apps.end()) ? itB->usageScore : 0.0f;
        return scoreA > scoreB;
      });
    }

    // Always append App Store and App Settings at the bottom
    visibleApps.push_back(
        {"App Store", UIIcon::Library, [this]() { return std::make_unique<AppStoreActivity>(renderer, mappedInput); }});
    visibleApps.push_back({"Settings", UIIcon::Settings,
                           [this]() { return std::make_unique<AppSettingsActivity>(renderer, mappedInput); }});

    if (selectedIndex >= static_cast<int>(visibleApps.size())) {
      selectedIndex = std::max(0, static_cast<int>(visibleApps.size()) - 1);
    }
  }
};