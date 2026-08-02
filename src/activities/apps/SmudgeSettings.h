#pragma once

#include "FsHelpers.h"
#include <Serialization.h>

#include <algorithm>
#include <string>
#include <vector>

enum class MenuSortMode : uint8_t {
  Alphabetical = 0,
  MostUsed = 1
};

struct AppUsageData {
  std::string appName;
  bool visible = true;
  float usageScore = 0.0f;
};

class SmudgeSettings {
public:
  static SmudgeSettings& getInstance() {
    static SmudgeSettings instance;
    return instance;
  }

  MenuSortMode sortMode = MenuSortMode::Alphabetical;
  std::vector<AppUsageData> apps;

  static constexpr uint32_t SETTINGS_MAGIC = 0x534D4447; // 'SMDG'
  static constexpr char settingsFile[] = "/.smudge/settings.bin";

  void ensureDirectoriesExist() {
    if (!Storage.exists("/.smudge")) {
      Storage.mkdir("/.smudge");
    }
  }

  void recordAppLaunch(const std::string& name) {
    for (auto& app : apps) {
      app.usageScore *= 0.85f;
    }

    auto it = std::find_if(apps.begin(), apps.end(), [&](const AppUsageData& a) {
      return a.appName == name;
    });

    if (it != apps.end()) {
      it->usageScore += 1.0f;
    } else {
      apps.push_back({name, true, 1.0f});
    }

    save();
  }

  bool isAppVisible(const std::string& name) {
    auto it = std::find_if(apps.begin(), apps.end(), [&](const AppUsageData& a) {
      return a.appName == name;
    });
    if (it != apps.end()) {
      return it->visible;
    }
    return true;
  }

  void save() {
    ensureDirectoriesExist();

    HalFile file;
    if (!Storage.openFileForWrite("Settings", settingsFile, file)) {
      return;
    }

    serialization::writePod(file, SETTINGS_MAGIC);
    serialization::writePod(file, static_cast<uint8_t>(sortMode));

    uint32_t appCount = static_cast<uint32_t>(apps.size());
    serialization::writePod(file, appCount);

    for (const auto& app : apps) {
      serialization::writeString(file, app.appName);
      serialization::writePod(file, static_cast<uint8_t>(app.visible ? 1 : 0));
      serialization::writePod(file, app.usageScore);
    }

    file.close();
  }

  bool load() {
    if (!Storage.exists(settingsFile)) {
      return false;
    }

    HalFile file;
    if (!Storage.openFileForRead("Settings", settingsFile, file)) {
      return false;
    }

    uint32_t magic = 0;
    if (!serialization::tryReadPod(file, magic) || magic != SETTINGS_MAGIC) {
      file.close();
      Storage.remove(settingsFile);
      return false;
    }

    uint8_t rawSort = 0;
    uint32_t appCount = 0;

    if (!serialization::tryReadPod(file, rawSort) ||
        !serialization::tryReadPod(file, appCount)) {
      file.close();
      Storage.remove(settingsFile);
      return false;
    }

    sortMode = static_cast<MenuSortMode>(rawSort);
    apps.clear();

    for (uint32_t i = 0; i < appCount; ++i) {
      AppUsageData data;
      uint8_t rawVisible = 1;

      if (!serialization::tryReadString(file, data.appName) ||
          !serialization::tryReadPod(file, rawVisible) ||
          !serialization::tryReadPod(file, data.usageScore)) {
        apps.clear();
        file.close();
        Storage.remove(settingsFile);
        return false;
      }
      data.visible = (rawVisible != 0);
      apps.push_back(data);
    }

    file.close();
    return true;
  }

  void registerKnownApps(const std::vector<std::string>& knownNames) {
    for (const auto& name : knownNames) {
      if (name.empty()) continue;
      auto it = std::find_if(apps.begin(), apps.end(), [&](const AppUsageData& a) {
        return a.appName == name;
      });
      if (it == apps.end()) {
        apps.push_back({name, true, 0.0f});
      }
    }
  }

private:
  SmudgeSettings() { load(); }
};