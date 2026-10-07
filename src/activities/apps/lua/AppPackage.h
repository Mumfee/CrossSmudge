#pragma once

#include <ArduinoJson.h>
#include <HalStorage.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <vector>

#include "Logging.h"

struct AppPackage {
  std::string id;           // Directory name, e.g. "counter"
  std::string name;         // Display name, e.g. "Tally Counter"
  std::string version;      // e.g. "1.0.0"
  std::string author;       // e.g. "Community"
  std::string description;  // e.g. "Simple e-ink tally counter"
  std::string entryScript;  // default "main.lua"
  std::string path;         // e.g. "/.crosssmudge/applications/counter"

  // 32x32 1-bit icon (32 * 32 / 8 = 128 bytes)
  bool hasIcon = false;
  uint8_t iconData[128] = {0};
  bool preventSleep = false;
  int minApi = 0;

  static std::vector<AppPackage> scanApplications() {
    std::vector<AppPackage> apps;
    const char* searchDirs[] = {"/.crosssmudge/applications", "/applications", "/apps", "/.smudge/applications",
                                "/.crosspoint/applications"};

    for (const char* baseDir : searchDirs) {
      if (!Storage.exists(baseDir)) {
        continue;
      }

      HalFile dir = Storage.open(baseDir);
      if (!dir || !dir.isDirectory()) {
        if (dir) dir.close();
        continue;
      }

      char folderName[128];
      for (HalFile entry = dir.openNextFile(); entry; entry = dir.openNextFile()) {
        folderName[0] = '\0';
        entry.getName(folderName, sizeof(folderName));
        bool isDir = entry.isDirectory();
        // Close entry handle immediately so SdFat allows child file operations
        entry.close();

        // Strip trailing slashes or backslashes
        size_t len = strlen(folderName);
        while (len > 0 && (folderName[len - 1] == '/' || folderName[len - 1] == '\\')) {
          folderName[--len] = '\0';
        }

        if (folderName[0] == '\0' || strcmp(folderName, ".") == 0 || strcmp(folderName, "..") == 0) {
          continue;
        }

        // Check if already found in an earlier search dir
        bool alreadyFound = false;
        for (const auto& existing : apps) {
          if (existing.id == folderName) {
            alreadyFound = true;
            break;
          }
        }
        if (alreadyFound) continue;

        std::string appDir = std::string(baseDir) + "/" + folderName;
        std::string manifestPath = appDir + "/manifest.json";
        std::string mainLuaPath = appDir + "/main.lua";

        bool hasManifest = Storage.exists(manifestPath.c_str());
        bool hasMainLua = Storage.exists(mainLuaPath.c_str());

        // An application folder must contain either manifest.json or main.lua
        if (!hasManifest && !hasMainLua) {
          continue;
        }

        AppPackage pkg;
        pkg.id = folderName;
        pkg.name = folderName;
        pkg.version = "1.0.0";
        pkg.author = "Community";
        pkg.description = "";
        pkg.entryScript = "main.lua";
        pkg.path = appDir;

        if (hasManifest) {
          String jsonStr = Storage.readFile(manifestPath.c_str());
          JsonDocument doc;
          DeserializationError err = deserializeJson(doc, jsonStr);
          if (!err) {
            if (doc["id"].is<const char*>()) pkg.id = doc["id"].as<const char*>();
            if (doc["name"].is<const char*>()) pkg.name = doc["name"].as<const char*>();
            if (doc["version"].is<const char*>()) pkg.version = doc["version"].as<const char*>();
            if (doc["author"].is<const char*>()) pkg.author = doc["author"].as<const char*>();
            if (doc["description"].is<const char*>()) pkg.description = doc["description"].as<const char*>();
            if (doc["entry"].is<const char*>()) pkg.entryScript = doc["entry"].as<const char*>();
            if (doc["prevent_sleep"].is<bool>()) pkg.preventSleep = doc["prevent_sleep"].as<bool>();
            if (doc["min_api"].is<int>()) pkg.minApi = doc["min_api"].as<int>();
          } else {
            LOG_ERR("APP", "Failed to parse manifest for %s: %s", folderName, err.c_str());
          }
        }

        // Check for 32x32 raw icon
        std::string iconPath = appDir + "/icon.raw";
        if (Storage.exists(iconPath.c_str())) {
          HalFile iconFile;
          if (Storage.openFileForRead("LUA", iconPath.c_str(), iconFile)) {
            if (iconFile.read(pkg.iconData, sizeof(pkg.iconData)) == sizeof(pkg.iconData)) {
              pkg.hasIcon = true;
            }
            iconFile.close();
          }
        }

        LOG_INF("APP", "Discovered package: %s (%s) at %s", pkg.name.c_str(), pkg.id.c_str(), appDir.c_str());
        apps.push_back(std::move(pkg));
        yield();
      }
      dir.close();
    }
    return apps;
  }
};
