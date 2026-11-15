#pragma once

#include <ArduinoJson.h>
#include <HalStorage.h>

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "Logging.h"
#include "SdCardFontSystem.h"
#include "activities/Activity.h"
#include "activities/network/WifiSelectionActivity.h"
#include "components/UITheme.h"
#include "fontIds.h"
#include "network/HttpDownloader.h"

#if !defined(SIMULATOR)
#include <WiFi.h>
#endif

class AppStoreActivity : public Activity {
 public:
  AppStoreActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("App Store", renderer, mappedInput) {}

  void onEnter() override {
    Activity::onEnter();
    sdFontSystem.releaseForNetwork(renderer);
    catalogUrl_ = "https://raw.githubusercontent.com/Mumfee/CrossSmudge/main/apps/catalog.json";
    baseUrl_ = "https://raw.githubusercontent.com/Mumfee/CrossSmudge/main/apps/";
    selectedIndex_ = 0;
    lastIconWorkMs_ = millis();
    errorTitle_ = "Network Error";
    errorMessage_.clear();
    lastFailedAppIndex_ = -1;
    if (isWifiConnected()) {
      state_ = State::FETCHING_CATALOG;
    } else {
      state_ = State::CHECK_WIFI;
    }
    requestUpdate();
  }

  void onExit() override {
    apps_.clear();
    apps_.shrink_to_fit();
#if !defined(SIMULATOR)
    if (WiFi.getMode() != WIFI_MODE_NULL) {
      WiFi.disconnect(false);
      delay(30);
      WiFi.mode(WIFI_OFF);
    }
#endif
    sdFontSystem.ensureLoaded(renderer);
    sdFontSystem.releaseRegistry();
    Activity::onExit();
  }

  void render(RenderLock&&) override {
    switch (state_) {
      case State::CHECK_WIFI:
        if (!isWifiConnected()) {
          renderNoWifi();
        } else {
          renderLoading("Connecting to App Catalog...");
        }
        break;

      case State::FETCHING_CATALOG:
        renderLoading("Fetching App Catalog from GitHub...");
        break;

      case State::CATALOG_READY:
        renderCatalog();
        break;

      case State::APP_DETAIL:
        renderAppDetail();
        break;

      case State::DOWNLOADING: {
        const char* appName = (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(apps_.size()))
                                  ? apps_[selectedIndex_].name.c_str()
                                  : "";
        renderDownloadProgress(appName);
        break;
      }

      case State::ERROR:
        renderError();
        break;
    }
    renderer.displayBuffer();
  }

  void loop() override {
    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      if (state_ == State::DOWNLOADING) {
        cancelDownload_ = true;
        return;
      }
      if (state_ == State::APP_DETAIL) {
        state_ = State::CATALOG_READY;
        requestUpdate();
        return;
      }
      if (state_ == State::ERROR) {
        if (!apps_.empty()) {
          state_ = State::CATALOG_READY;
          requestUpdate();
          return;
        }
      }
      finish();
      return;
    }

    switch (state_) {
      case State::CHECK_WIFI: {
        if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
          startActivityForResult(std::make_unique<WifiSelectionActivity>(renderer, mappedInput),
                                 [this](const ActivityResult&) {
                                   if (isWifiConnected()) {
                                     state_ = State::FETCHING_CATALOG;
                                     requestUpdateAndWait();
                                     fetchCatalog();
                                   } else {
                                     state_ = State::CHECK_WIFI;
                                     requestUpdate();
                                   }
                                 });
          return;
        }
        int tx = 0, ty = 0;
        if (mappedInput.wasScreenTapped(tx, ty)) {
          int w = renderer.getScreenWidth();
          int h = renderer.getScreenHeight();
          const auto& m = UITheme::getInstance().getMetrics();
          if (ty > h - m.buttonHintsHeight) {
            if (tx < w / 2) {
              finish();
              return;
            } else {
              startActivityForResult(std::make_unique<WifiSelectionActivity>(renderer, mappedInput),
                                     [this](const ActivityResult&) {
                                       if (isWifiConnected()) {
                                         state_ = State::FETCHING_CATALOG;
                                         requestUpdateAndWait();
                                         fetchCatalog();
                                       } else {
                                         state_ = State::CHECK_WIFI;
                                         requestUpdate();
                                       }
                                     });
              return;
            }
          }
        }
        if (isWifiConnected()) {
          state_ = State::FETCHING_CATALOG;
          requestUpdateAndWait();
          fetchCatalog();
        }
        break;
      }

      case State::FETCHING_CATALOG:
        fetchCatalog();
        break;

      case State::CATALOG_READY:
        if (handleCatalogInput()) {
          lastIconWorkMs_ = millis();
        } else {
          fetchVisibleIcon();
        }
        break;

      case State::APP_DETAIL:
        handleDetailInput();
        break;

      case State::DOWNLOADING:
        // Handled during download operation
        break;

      case State::ERROR: {
        if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
          if (isWifiConnected()) {
            if (lastFailedAppIndex_ >= 0 && lastFailedAppIndex_ < static_cast<int>(apps_.size())) {
              installApp(apps_[lastFailedAppIndex_]);
            } else {
              state_ = State::FETCHING_CATALOG;
              requestUpdateAndWait();
              fetchCatalog();
            }
          } else {
            state_ = State::CHECK_WIFI;
            requestUpdate();
          }
          return;
        }
        int tx = 0, ty = 0;
        if (mappedInput.wasScreenTapped(tx, ty)) {
          int w = renderer.getScreenWidth();
          int h = renderer.getScreenHeight();
          const auto& m = UITheme::getInstance().getMetrics();
          if (ty > h - m.buttonHintsHeight) {
            if (tx < w / 2) {
              if (!apps_.empty()) {
                state_ = State::CATALOG_READY;
                requestUpdate();
              } else {
                finish();
              }
              return;
            } else {
              if (isWifiConnected()) {
                if (lastFailedAppIndex_ >= 0 && lastFailedAppIndex_ < static_cast<int>(apps_.size())) {
                  installApp(apps_[lastFailedAppIndex_]);
                } else {
                  state_ = State::FETCHING_CATALOG;
                  requestUpdateAndWait();
                  fetchCatalog();
                }
              } else {
                state_ = State::CHECK_WIFI;
                requestUpdate();
              }
              return;
            }
          }
        }
        break;
      }
    }
  }

 private:
  enum class State { CHECK_WIFI, FETCHING_CATALOG, CATALOG_READY, APP_DETAIL, DOWNLOADING, ERROR };

  struct CatalogApp {
    std::string id;
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    std::vector<std::string> files;
    bool isInstalled = false;
    std::string installedVersion;
    bool hasIcon = false;
    bool iconAttempted = false;
    uint8_t iconData[128] = {0};
  };

  State state_ = State::CHECK_WIFI;
  std::string catalogUrl_;
  std::string baseUrl_;
  std::string errorTitle_ = "Network Error";
  std::string errorMessage_;
  int lastFailedAppIndex_ = -1;
  std::vector<CatalogApp> apps_;
  int selectedIndex_ = 0;
  bool cancelDownload_ = false;
  unsigned long lastIconWorkMs_ = 0;

  std::string currentDownloadingFile_;
  int currentFileNum_ = 0;
  int totalFiles_ = 0;

  static void expand32To64(const uint8_t src[128], uint8_t dst[512]) {
    for (int y = 0; y < 32; ++y) {
      int srcRowOffset = y * 4;
      int dstRow0 = (y * 2) * 8;
      int dstRow1 = (y * 2 + 1) * 8;
      for (int xByte = 0; xByte < 4; ++xByte) {
        uint8_t b = src[srcRowOffset + xByte];
        uint8_t hi = 0;
        uint8_t lo = 0;
        for (int bit = 0; bit < 4; ++bit) {
          if ((b >> (7 - bit)) & 1) {
            hi |= (3 << (6 - bit * 2));
          }
          if ((b >> (3 - bit)) & 1) {
            lo |= (3 << (6 - bit * 2));
          }
        }
        dst[dstRow0 + xByte * 2] = hi;
        dst[dstRow0 + xByte * 2 + 1] = lo;
        dst[dstRow1 + xByte * 2] = hi;
        dst[dstRow1 + xByte * 2 + 1] = lo;
      }
    }
  }

  bool isWifiConnected() {
#if defined(SIMULATOR)
    return true;
#else
    return (WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0));
#endif
  }

  static bool safeAppId(const std::string& id) {
    if (id.empty() || id.size() > 64) return false;
    for (unsigned char c : id) {
      if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
            c == '-' || c == '_')) {
        return false;
      }
    }
    return true;
  }

  bool readAppIcon(CatalogApp& app, const std::string& path, const uint8_t* expected = nullptr) {
    // Storage shares SPI with the display. Publish only complete, validated pixels.
    RenderLock lock(*this);
    HalFile file;
    if (!Storage.openFileForRead("STORE", path.c_str(), file)) return false;
    uint8_t pixels[128];
    const bool valid = file.size() == sizeof(pixels) && file.read(pixels, sizeof(pixels)) == sizeof(pixels);
    const bool closed = file.close();
    if (!valid || !closed || (expected && memcmp(expected, pixels, sizeof(pixels)) != 0)) {
      LOG_ERR("STORE", "Invalid icon: %s", path.c_str());
      return false;
    }
    memcpy(app.iconData, pixels, sizeof(pixels));
    app.hasIcon = true;
    return true;
  }

  void loadAppIcon(CatalogApp& app, bool allowDownload = false) {
    if (!safeAppId(app.id)) {
      LOG_ERR("STORE", "Invalid catalog app ID");
      return;
    }
    {
      RenderLock lock(*this);
      app.hasIcon = false;
    }
    // Paths/URL are bounded by the 64-byte ID limit; allocated only on this cold
    // path, never during render. Reuse the existing 128-byte per-app pixel store.
    if (readAppIcon(app, "/.crosssmudge/applications/" + app.id + "/icon.raw") ||
        readAppIcon(app, "/.smudge/applications/" + app.id + "/icon.raw")) return;
    const std::string cachePath = "/.crosssmudge/cache/icons/" + app.id + ".raw";
    if (readAppIcon(app, cachePath)) return;

#if defined(SIMULATOR)
    const std::string hostPath = "apps/" + app.id + "/icon.raw";
    FILE* hf = fopen(hostPath.c_str(), "rb");
    if (!hf) hf = fopen(("../" + hostPath).c_str(), "rb");
    if (hf) {
      uint8_t pixels[128];
      const bool valid = fread(pixels, 1, sizeof(pixels), hf) == sizeof(pixels) && fgetc(hf) == EOF && !ferror(hf);
      fclose(hf);
      if (valid) {
        RenderLock lock(*this);
        memcpy(app.iconData, pixels, sizeof(pixels));
        app.hasIcon = true;
        return;
      }
    }
#endif

    if (!allowDownload || app.iconAttempted || !isWifiConnected()) return;
    app.iconAttempted = true;  // At most one network attempt per app per catalog session.
    // Keep network I/O outside the render lock. The bounded stack response avoids
    // an unbounded download/file and a separate heap allocation for tiny icons.
    uint8_t pixels[128];
    size_t received = 0;
    HttpDownloader::DownloadOptions options;
    options.bufferSize = sizeof(pixels);
    // ESP_HTTP attaches the CA bundle. Do not use the insecure WolfSSL fallback.
    options.transport = HttpDownloader::Transport::ESP_HTTP;
    const auto err = HttpDownloader::streamUrl(
        baseUrl_ + app.id + "/icon.raw",
        [&](const uint8_t* data, size_t size) {
          if (size > sizeof(pixels) - received) return false;
          memcpy(pixels + received, data, size);
          received += size;
          return true;
        }, nullptr, "", "", options);
    if (err != HttpDownloader::OK || received != sizeof(pixels)) {
      LOG_ERR("STORE", "Icon download failed: %s (%d, %zu bytes)", app.id.c_str(), err, received);
      return;
    }
    {
      RenderLock lock(*this);
      if (!Storage.ensureDirectoryExists("/.crosssmudge") || !Storage.ensureDirectoryExists("/.crosssmudge/cache") ||
          !Storage.ensureDirectoryExists("/.crosssmudge/cache/icons")) {
        LOG_ERR("STORE", "Could not create icon cache");
        return;
      }
      HalFile file;
      if (!Storage.openFileForWrite("STORE", cachePath.c_str(), file)) {
        LOG_ERR("STORE", "Could not open icon cache");
        return;
      }
      const bool written = file.write(pixels, sizeof(pixels)) == sizeof(pixels);
      const bool synced = file.sync();
      const bool closed = file.close();
      if (!written || !synced || !closed) {
        LOG_ERR("STORE", "Could not write icon cache");
        Storage.remove(cachePath.c_str());
        return;
      }
    }
    if (!readAppIcon(app, cachePath, pixels)) {
      LOG_ERR("STORE", "Could not verify icon cache");
      RenderLock lock(*this);
      Storage.remove(cachePath.c_str());
    }
  }

  void ensureAppIcon(CatalogApp& app) {
    if (!app.hasIcon && !app.iconAttempted) loadAppIcon(app, true);
  }

  void fetchVisibleIcon() {
    // Input always gets a loop between requests. Synchronous connect/read can still
    // block until the transport timeout; input is not polled inside that transport.
    if (state_ != State::CATALOG_READY || !isWifiConnected() || millis() - lastIconWorkMs_ < 250 ||
        mappedInput.isPressed(MappedInputManager::Button::Back)) return;
    const auto layout = catalogLayout();
    const int start = selectedIndex_ / layout.rowsPerPage * layout.rowsPerPage;
    const int end = std::min(static_cast<int>(apps_.size()), start + layout.rowsPerPage);
    for (int i = start; i < end; ++i) {
      auto& app = apps_[i];
      if (app.hasIcon || app.iconAttempted) continue;
      ensureAppIcon(app);
      lastIconWorkMs_ = millis();
      if (app.hasIcon) requestUpdate();
      return;
    }
  }

  void fetchCatalog() {
    sdFontSystem.releaseForNetwork(renderer);

    std::string tmpCatalog = "/.crosssmudge/cache/catalog.tmp";
    Storage.ensureDirectoryExists("/.crosssmudge");
    Storage.ensureDirectoryExists("/.crosssmudge/cache");
    Storage.remove(tmpCatalog.c_str());

    HttpDownloader::DownloadOptions options;
#if defined(FREEINK_NET_WOLFSSL)
    options.transport = HttpDownloader::Transport::WOLFSSL;
#endif

    HttpDownloader::DownloadError err =
        HttpDownloader::downloadToFile(catalogUrl_, tmpCatalog, nullptr, nullptr, "", "", options);

#if defined(FREEINK_NET_WOLFSSL)
    if (err != HttpDownloader::OK) {
      LOG_DBG("STORE", "WolfSSL catalog fetch failed (%d), trying ESP_HTTP", err);
      options.transport = HttpDownloader::Transport::ESP_HTTP;
      err = HttpDownloader::downloadToFile(catalogUrl_, tmpCatalog, nullptr, nullptr, "", "", options);
    }
#endif

#if defined(SIMULATOR)
    if (err != HttpDownloader::OK) {
      FILE* f = fopen("apps/catalog.json", "rb");
      if (!f) f = fopen("../apps/catalog.json", "rb");
      if (!f) f = fopen("/var/home/brady/C/crosssmudge/apps/catalog.json", "rb");
      if (f) {
        HalFile outF;
        if (Storage.openFileForWrite("STORE", tmpCatalog.c_str(), outF)) {
          uint8_t buf[256];
          size_t bytes;
          while ((bytes = fread(buf, 1, sizeof(buf), f)) > 0) {
            outF.write(buf, bytes);
          }
          outF.close();
          err = HttpDownloader::OK;
        }
        fclose(f);
      }
    }
#endif

    if (err != HttpDownloader::OK) {
      Storage.remove(tmpCatalog.c_str());
      state_ = State::ERROR;
      lastFailedAppIndex_ = -1;
      errorTitle_ = "Network Error";
      errorMessage_ = "Failed to download catalog.json.\nCheck network connection.";
      requestUpdate();
      return;
    }

    FsFile catalogFile;
    if (!Storage.openFileForRead("STORE", tmpCatalog.c_str(), catalogFile)) {
      Storage.remove(tmpCatalog.c_str());
      state_ = State::ERROR;
      lastFailedAppIndex_ = -1;
      errorTitle_ = "Storage Error";
      errorMessage_ = "Failed to read catalog.tmp.\nCheck SD card.";
      requestUpdate();
      return;
    }

    JsonDocument doc;
    DeserializationError jsonErr = deserializeJson(doc, catalogFile);
    catalogFile.close();
    Storage.remove(tmpCatalog.c_str());

    if (jsonErr) {
      state_ = State::ERROR;
      lastFailedAppIndex_ = -1;
      errorTitle_ = "Catalog Error";
      errorMessage_ = std::string("JSON Parse Error: ") + jsonErr.c_str();
      requestUpdate();
      return;
    }

    apps_.clear();
    JsonArray appList = doc["apps"].as<JsonArray>();
    apps_.reserve(appList.size());
    for (JsonObject obj : appList) {
      CatalogApp app;
      app.id = obj["id"] | "";
      if (!safeAppId(app.id)) {
        LOG_ERR("STORE", "Skipping invalid catalog app ID");
        continue;
      }
      app.name = obj["name"] | app.id.c_str();
      app.version = obj["version"] | "1.0.0";
      app.author = obj["author"] | "Unknown";
      app.description = obj["description"] | "";

      JsonArray files = obj["files"].as<JsonArray>();
      app.files.reserve(files.size());
      for (const char* f : files) {
        if (f) app.files.push_back(f);
      }
      if (app.files.empty()) {
        app.files = {"manifest.json", "main.lua", "icon.raw"};
      }

      checkInstalledStatus(app);
      loadAppIcon(app, false);
      apps_.push_back(std::move(app));
    }

    state_ = State::CATALOG_READY;
    selectedIndex_ = 0;
    lastIconWorkMs_ = millis();
    requestUpdate();
  }

  void checkInstalledStatus(CatalogApp& app) {
    std::string path1 = "/.crosssmudge/applications/" + app.id + "/manifest.json";
    std::string path2 = "/.smudge/applications/" + app.id + "/manifest.json";

    std::string manifestPath;
    if (Storage.exists(path1.c_str())) {
      manifestPath = path1;
    } else if (Storage.exists(path2.c_str())) {
      manifestPath = path2;
    }

    if (!manifestPath.empty()) {
      String mStr = Storage.readFile(manifestPath.c_str());
      JsonDocument mDoc;
      if (!deserializeJson(mDoc, mStr)) {
        app.isInstalled = true;
        app.installedVersion = mDoc["version"] | "1.0.0";
        return;
      }
    }
    app.isInstalled = false;
    app.installedVersion = "";
  }

  bool handleCatalogInput() {
    int count = static_cast<int>(apps_.size());

    const auto layout = catalogLayout();
    const int rowsPerPage = layout.rowsPerPage;
    int pageStart = (selectedIndex_ / rowsPerPage) * rowsPerPage;

    if (count > 0 && (mappedInput.wasReleased(MappedInputManager::Button::Right) ||
                      mappedInput.wasReleased(MappedInputManager::Button::Down) ||
                      mappedInput.wasReleased(MappedInputManager::Button::PageForward))) {
      selectedIndex_ = (selectedIndex_ + 1) % count;
      requestUpdate();
      return true;
    } else if (count > 0 && (mappedInput.wasReleased(MappedInputManager::Button::Left) ||
                             mappedInput.wasReleased(MappedInputManager::Button::Up) ||
                             mappedInput.wasReleased(MappedInputManager::Button::PageBack))) {
      selectedIndex_ = (selectedIndex_ - 1 + count) % count;
      requestUpdate();
      return true;
    }

    auto swipe = mappedInput.wasSwipe();
    if (count > 0 && (swipe == MappedInputManager::SwipeDir::Up || swipe == MappedInputManager::SwipeDir::Left)) {
      selectedIndex_ = (selectedIndex_ + 1) % count;
      requestUpdate();
      return true;
    } else if (count > 0 && (swipe == MappedInputManager::SwipeDir::Down || swipe == MappedInputManager::SwipeDir::Right)) {
      selectedIndex_ = (selectedIndex_ - 1 + count) % count;
      requestUpdate();
      return true;
    }

    if (count > 0 && mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      state_ = State::APP_DETAIL;
      ensureAppIcon(apps_[selectedIndex_]);
      requestUpdate();
      return true;
    }

    // Touch handling
    int tx = 0, ty = 0;
    if (mappedInput.wasScreenTapped(tx, ty)) {
      if (tx < layout.left || tx >= layout.right || ty < layout.headerY || ty >= layout.bottom) return true;
      if (ty >= layout.controlsY) {
        int action = -1;
        for (int i = 0; i < 4; ++i) {
          const int x = layout.left + (layout.right - layout.left) * i / 4;
          const int end = layout.left + (layout.right - layout.left) * (i + 1) / 4;
          if (tx >= x && tx < end - 2) action = i;
        }
        if (action < 0) return true;
        if (action == 0) {
          finish();
          return true;
        } else if (count == 0) {
          return true;
        } else if (action == 1) {
          state_ = State::APP_DETAIL;
          ensureAppIcon(apps_[selectedIndex_]);
          requestUpdate();
          return true;
        } else if (action == 2) {
          selectedIndex_ = (selectedIndex_ - 1 + count) % count;
          requestUpdate();
          return true;
        } else {
          selectedIndex_ = (selectedIndex_ + 1) % count;
          requestUpdate();
          return true;
        }
      }

      const int startY = layout.startY;
      const int rowH = layout.rowH;
      for (int i = pageStart; i < count && i < pageStart + rowsPerPage; ++i) {
        int ry = startY + (i - pageStart) * rowH;
        if (ty >= ry && ty < ry + rowH - 4) {
          if (selectedIndex_ == i) {
            state_ = State::APP_DETAIL;
            ensureAppIcon(apps_[selectedIndex_]);
          } else {
            selectedIndex_ = i;
          }
          requestUpdate();
          return true;
        }
      }
      return true;
    }
    return false;
  }

  int getDetailActionButtonY() const {
    const auto& m = UITheme::getInstance().getMetrics();
    int cardY = m.topPadding + m.headerHeight + 12;
    int cardH = 96;
    int descY = cardY + cardH + 12;
    int descH = 176;
    int pkgY = descY + descH + 12;
    int pkgH = 118;
    return pkgY + pkgH + 16;
  }

  void handleDetailInput() {
    int count = static_cast<int>(apps_.size());
    if (count == 0 || selectedIndex_ < 0 || selectedIndex_ >= count) {
      state_ = State::CATALOG_READY;
      requestUpdate();
      return;
    }

    auto& app = apps_[selectedIndex_];

    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      state_ = State::CATALOG_READY;
      requestUpdate();
      return;
    }

    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      installApp(app);
      return;
    }

    if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
      if (app.isInstalled) {
        uninstallApp(app);
        return;
      }
    }

    // Touch handling
    int tx = 0, ty = 0;
    if (mappedInput.wasScreenTapped(tx, ty)) {
      int w = renderer.getScreenWidth();
      int h = renderer.getScreenHeight();
      const auto& m = UITheme::getInstance().getMetrics();

      if (ty > h - m.buttonHintsHeight - 20) {
        if (tx < w / 4) {
          state_ = State::CATALOG_READY;
          requestUpdate();
          return;
        } else if (tx < w / 2) {
          installApp(app);
          return;
        } else if (tx < 3 * w / 4 && app.isInstalled) {
          uninstallApp(app);
          return;
        }
      }

      int btnY = getDetailActionButtonY();
      int btnH = 50;
      if (ty >= btnY && ty <= btnY + btnH) {
        if (!app.isInstalled) {
          if (tx >= 40 && tx <= w - 40) {
            installApp(app);
            return;
          }
        } else {
          int bw = 200;
          int bx1 = 24;
          int bx2 = w - 24 - bw;
          if (tx >= bx1 && tx <= bx1 + bw) {
            installApp(app);
            return;
          } else if (tx >= bx2 && tx <= bx2 + bw) {
            uninstallApp(app);
            return;
          }
        }
      }
    }
  }

  void installApp(CatalogApp& app) {
    if (!isWifiConnected()) {
      state_ = State::ERROR;
      lastFailedAppIndex_ = selectedIndex_;
      errorTitle_ = "Network Error";
      errorMessage_ = "Wi-Fi disconnected.\nPlease reconnect to Wi-Fi.";
      requestUpdate();
      return;
    }

    state_ = State::DOWNLOADING;
    cancelDownload_ = false;
    totalFiles_ = static_cast<int>(app.files.size());
    lastFailedAppIndex_ = selectedIndex_;

    sdFontSystem.releaseForNetwork(renderer);

    std::string targetDir = "/.crosssmudge/applications/" + app.id;
    bool dirOk = Storage.ensureDirectoryExists("/.crosssmudge") &&
                 Storage.ensureDirectoryExists("/.crosssmudge/applications") &&
                 Storage.ensureDirectoryExists(targetDir.c_str());
    if (!dirOk) {
      LOG_ERR("STORE", "Failed to create target directory: %s", targetDir.c_str());
      state_ = State::ERROR;
      errorTitle_ = "Storage Error";
      errorMessage_ = "Failed creating app folder.\nCheck SD card space & write-lock.";
      requestUpdate();
      return;
    }

    for (int i = 0; i < totalFiles_; ++i) {
      if (cancelDownload_) {
        LOG_INF("STORE", "Installation cancelled");
        break;
      }
      currentFileNum_ = i + 1;
      currentDownloadingFile_ = app.files[i];
      requestUpdateAndWait();

      std::string destPath = targetDir + "/" + app.files[i];

      // Ensure any subdirectories exist
      size_t lastSlash = destPath.find_last_of('/');
      if (lastSlash != std::string::npos) {
        std::string parentDir = destPath.substr(0, lastSlash);
        if (!Storage.ensureDirectoryExists(parentDir.c_str())) {
          LOG_ERR("STORE", "Failed to create parent directory: %s", parentDir.c_str());
          state_ = State::ERROR;
          errorTitle_ = "Storage Error";
          errorMessage_ = "Failed creating folder:\n" + parentDir;
          requestUpdate();
          return;
        }
      }

      if (Storage.exists(destPath.c_str())) {
        Storage.remove(destPath.c_str());
      }

      std::string fileUrl = baseUrl_ + app.id + "/" + app.files[i];

      HttpDownloader::DownloadOptions options;
      options.shouldCancel = [this]() { return cancelDownload_; };
#if defined(FREEINK_NET_WOLFSSL)
      options.transport = HttpDownloader::Transport::WOLFSSL;
#endif

      HttpDownloader::DownloadError err = HttpDownloader::HTTP_ERROR;

#if defined(SIMULATOR)
      std::string hostSrc = "apps/" + app.id + "/" + app.files[i];
      FILE* inHost = fopen(hostSrc.c_str(), "rb");
      if (!inHost) inHost = fopen(("../" + hostSrc).c_str(), "rb");
      if (!inHost) inHost = fopen(("/var/home/brady/C/crosssmudge/" + hostSrc).c_str(), "rb");
      if (inHost) {
        HalFile outF;
        if (Storage.openFileForWrite("STORE", destPath.c_str(), outF)) {
          uint8_t buf[256];
          size_t bytes;
          while ((bytes = fread(buf, 1, sizeof(buf), inHost)) > 0) {
            outF.write(buf, bytes);
          }
          outF.close();
          err = HttpDownloader::OK;
        } else {
          err = HttpDownloader::FILE_ERROR;
        }
        fclose(inHost);
      }
      if (err != HttpDownloader::OK) {
        err = HttpDownloader::downloadToFile(fileUrl, destPath, nullptr, &cancelDownload_, "", "", options);
      }
#else
      for (int attempt = 0; attempt < 3 && err != HttpDownloader::OK && !cancelDownload_; ++attempt) {
        if (attempt > 0) {
          delay(500);
        }
#if defined(FREEINK_NET_WOLFSSL)
        options.transport = (attempt == 1) ? HttpDownloader::Transport::ESP_HTTP : HttpDownloader::Transport::WOLFSSL;
#endif
        err = HttpDownloader::downloadToFile(fileUrl, destPath, nullptr, &cancelDownload_, "", "", options);
        if (err == HttpDownloader::FILE_ERROR) {
          break;
        }
      }
#endif

      if (err != HttpDownloader::OK) {
        if (cancelDownload_) {
          LOG_INF("STORE", "Download cancelled");
          break;
        }
        LOG_ERR("STORE", "Failed downloading %s: %d", app.files[i].c_str(), err);
        state_ = State::ERROR;
        if (err == HttpDownloader::FILE_ERROR) {
          errorTitle_ = "Storage Error";
          errorMessage_ = "Failed writing to SD Card:\n" + app.files[i] + "\nCheck space & write-lock.";
        } else {
          errorTitle_ = "Network Error";
          if (!isWifiConnected()) {
            errorMessage_ = "Wi-Fi disconnected.\nPlease reconnect to Wi-Fi.";
          } else {
            errorMessage_ = "Failed downloading:\n" + app.files[i] + "\nCheck connection or retry.";
          }
        }
        requestUpdate();
        return;
      }
    }

    checkInstalledStatus(app);
    loadAppIcon(app, false);
    state_ = State::APP_DETAIL;
    requestUpdate();
  }

  void uninstallApp(CatalogApp& app) {
    std::string dir1 = "/.crosssmudge/applications/" + app.id;
    std::string dir2 = "/.smudge/applications/" + app.id;

    if (Storage.exists(dir1.c_str())) {
      Storage.removeDir(dir1.c_str());
    }
    if (Storage.exists(dir2.c_str())) {
      Storage.removeDir(dir2.c_str());
    }

    checkInstalledStatus(app);
    state_ = State::APP_DETAIL;
    requestUpdate();
  }

  void renderLoading(const char* msg) {
    int w = renderer.getScreenWidth();
    int h = renderer.getScreenHeight();
    const auto& m = UITheme::getInstance().getMetrics();

    renderer.clearScreen();
    GUI.drawHeader(renderer, Rect{0, m.topPadding, w, m.headerHeight}, "App Store", "Connecting");
    renderer.drawCenteredText(UI_12_FONT_ID, h / 2 - 20, msg, true, EpdFontFamily::BOLD);

    const auto labels = mappedInput.mapLabels(tr(STR_BACK), "", "", "");
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  }

  void renderNoWifi() {
    int w = renderer.getScreenWidth();
    int h = renderer.getScreenHeight();
    const auto& m = UITheme::getInstance().getMetrics();

    renderer.clearScreen();
    GUI.drawHeader(renderer, Rect{0, m.topPadding, w, m.headerHeight}, "App Store", "Offline");

    renderer.drawCenteredText(UI_12_FONT_ID, h / 2 - 40, "Wi-Fi Not Connected", true, EpdFontFamily::BOLD);
    renderer.drawCenteredText(SMALL_FONT_ID, h / 2 - 10, "Connect to Wi-Fi to browse community apps.", true);

    const auto labels = mappedInput.mapLabels(tr(STR_BACK), "Connect Wi-Fi", "", "");
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  }

  struct CatalogLayout {
    int left, right, headerY, startY, rowH, rowsPerPage, footerY, controlsY, bottom;
  };

  CatalogLayout catalogLayout() const {
    const auto& m = UITheme::getInstance().getMetrics();
    int top, right, bottom, left;
    renderer.getOrientedViewableTRBL(&top, &right, &bottom, &left);
    // Physical hints rotate to the button edge; intersect that reservation with
    // bezel bounds instead of assuming the screen's bottom is always usable.
    const auto safe = UITheme::getInstance().getScreenSafeArea(renderer, !mappedInput.hasTouch());
    left = std::max(left, safe.x) + 16;
    right = std::min(renderer.getScreenWidth() - right, safe.x + safe.width) - 16;
    bottom = std::min(renderer.getScreenHeight() - bottom, safe.y + safe.height) - 6;
    const int headerY = std::max(top, safe.y) + m.topPadding;
    const int startY = headerY + m.headerHeight + 6;
    const int rowH = std::max(56, renderer.getLineHeight(UI_10_FONT_ID) + renderer.getLineHeight(SMALL_FONT_ID) + 20);
    const int controlsY = bottom - std::max(44, renderer.getLineHeight(SMALL_FONT_ID) + 16);
    const int footerY = controlsY - renderer.getLineHeight(SMALL_FONT_ID) - 8;
    const int rows = std::max(1, (footerY - 6 - startY) / rowH);
    return {left, right, headerY, startY, rowH, rows, footerY, controlsY, bottom};
  }

  // Fit bounded UTF-8 buffers without allocating a string on every paint.
  void fitCatalogText(char* text, int font, int width, EpdFontFamily::Style style = EpdFontFamily::REGULAR) const {
    size_t length = strlen(text);
    // snprintf may end in the middle of a multi-byte character even when the
    // resulting byte buffer is narrower than the available screen space.
    for (size_t i = 0; i < length;) {
      const auto c = static_cast<unsigned char>(text[i]);
      const size_t bytes = c < 0x80 ? 1 : (c >= 0xc2 && c < 0xe0 ? 2 : (c < 0xf0 ? 3 : 4));
      bool complete = c < 0x80 || (c >= 0xc2 && c <= 0xf4);
      if (i + bytes > length) complete = false;
      for (size_t j = 1; complete && j < bytes; ++j) {
        complete = (static_cast<unsigned char>(text[i + j]) & 0xc0) == 0x80;
      }
      if (!complete) {
        text[i] = '\0';
        length = i;
        break;
      }
      i += bytes;
    }
    while (length && renderer.getTextWidth(font, text, style) > width) {
      do {
        --length;
      } while (length && (static_cast<unsigned char>(text[length]) & 0xc0) == 0x80);
      text[length] = '\0';
    }
  }

  void renderCatalog() {
    const auto& m = UITheme::getInstance().getMetrics();
    const auto layout = catalogLayout();
    renderer.clearScreen();
    GUI.drawHeader(renderer, Rect{layout.left, layout.headerY, layout.right - layout.left, m.headerHeight},
                   tr(STR_STORE_TITLE));

    const int count = static_cast<int>(apps_.size());
    const int pageStart = selectedIndex_ / layout.rowsPerPage * layout.rowsPerPage;
    for (int i = pageStart; i < count && i < pageStart + layout.rowsPerPage; ++i) {
      const auto& app = apps_[i];
      const int y = layout.startY + (i - pageStart) * layout.rowH;
      const bool isSelected = i == selectedIndex_;
      if (isSelected) {
        renderer.fillRoundedRect(layout.left, y, layout.right - layout.left, layout.rowH - 4, 6, Color::Black);
      } else {
        renderer.drawRoundedRect(layout.left, y, layout.right - layout.left, layout.rowH - 4, 1, 6, true);
      }
      const int iconX = layout.left + 10;
      const int iconY = y + (layout.rowH - 4 - 32) / 2;
      if (app.hasIcon) {
        if (isSelected) {
          renderer.drawIconInverted(app.iconData, iconX, iconY, 32, 32);
        } else {
          renderer.drawIcon(app.iconData, iconX, iconY, 32, 32);
        }
      } else {
        renderer.drawRoundedRect(iconX, iconY, 32, 32, 1, 4, !isSelected);
      }
      const char* status = !app.isInstalled ? tr(STR_STORE_INSTALL)
                           : app.installedVersion != app.version ? tr(STR_UPDATE) : tr(STR_INSTALLED);
      char statusBuf[64];
      snprintf(statusBuf, sizeof(statusBuf), "%s", status);
      fitCatalogText(statusBuf, SMALL_FONT_ID, (layout.right - layout.left) / 3, EpdFontFamily::BOLD);
      const int statusWidth = renderer.getTextWidth(SMALL_FONT_ID, statusBuf, EpdFontFamily::BOLD);
      const int statusX = layout.right - 10 - statusWidth;
      const int textX = iconX + 42;
      const int textWidth = std::max(0, statusX - 10 - textX);
      char text[96];
      snprintf(text, sizeof(text), "%s", app.name.c_str());
      fitCatalogText(text, UI_10_FONT_ID, textWidth, EpdFontFamily::BOLD);
      renderer.drawText(UI_10_FONT_ID, textX, y + 8, text, !isSelected, EpdFontFamily::BOLD);
      snprintf(text, sizeof(text), "%s  •  v%s", app.author.c_str(), app.version.c_str());
      fitCatalogText(text, SMALL_FONT_ID, textWidth);
      renderer.drawText(SMALL_FONT_ID, textX, y + 10 + renderer.getLineHeight(UI_10_FONT_ID), text, !isSelected);
      renderer.drawText(SMALL_FONT_ID, statusX, y + (layout.rowH - 4 - renderer.getLineHeight(SMALL_FONT_ID)) / 2,
                        statusBuf, !isSelected, EpdFontFamily::BOLD);
    }
    char page[64];
    snprintf(page, sizeof(page), tr(STR_STORE_PAGE), selectedIndex_ / layout.rowsPerPage + 1,
             std::max(1, (count + layout.rowsPerPage - 1) / layout.rowsPerPage), count);
    fitCatalogText(page, SMALL_FONT_ID, layout.right - layout.left);
    renderer.drawText(SMALL_FONT_ID,
                      layout.left + (layout.right - layout.left - renderer.getTextWidth(SMALL_FONT_ID, page)) / 2,
                      layout.footerY, page, true);
    // Explicit logical actions stay visible on touch devices, where theme hints
    // are suppressed. Drawing and hit testing use these same quarter-width cells.
    const char* actions[] = {tr(STR_BACK), tr(STR_STORE_DETAILS), tr(STR_DIR_UP), tr(STR_DIR_DOWN)};
    for (int i = 0; i < 4; ++i) {
      const int x = layout.left + (layout.right - layout.left) * i / 4;
      const int end = layout.left + (layout.right - layout.left) * (i + 1) / 4;
      renderer.drawRoundedRect(x, layout.controlsY, end - x - 2, layout.bottom - layout.controlsY, 1, 4, true);
      char label[64];
      snprintf(label, sizeof(label), "%s", actions[i]);
      fitCatalogText(label, SMALL_FONT_ID, end - x - 10);
      renderer.drawText(SMALL_FONT_ID, x + (end - x - renderer.getTextWidth(SMALL_FONT_ID, label)) / 2,
                        layout.controlsY + (layout.bottom - layout.controlsY - renderer.getLineHeight(SMALL_FONT_ID)) / 2,
                        label, true);
    }
    const auto labels = mappedInput.mapLabels(actions[0], actions[1], actions[2], actions[3]);
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  }

  void renderAppDetail() {
    int w = renderer.getScreenWidth();
    int h = renderer.getScreenHeight();
    const auto& m = UITheme::getInstance().getMetrics();

    if (selectedIndex_ < 0 || selectedIndex_ >= static_cast<int>(apps_.size())) {
      state_ = State::CATALOG_READY;
      renderCatalog();
      return;
    }

    const auto& app = apps_[selectedIndex_];

    renderer.clearScreen();
    GUI.drawHeader(renderer, Rect{0, m.topPadding, w, m.headerHeight}, "App Store", app.name.c_str());

    // 1. Hero Card at top
    int cardY = m.topPadding + m.headerHeight + 12;
    int cardH = 96;
    renderer.drawRoundedRect(16, cardY, w - 32, cardH, 1, 8, true);

    // Large 64x64 Hero Icon
    int iconFrameX = 28;
    int iconFrameY = cardY + 10;
    renderer.drawRoundedRect(iconFrameX, iconFrameY, 76, 76, 1, 6, true);
    if (app.hasIcon) {
      uint8_t scaled[512];
      expand32To64(app.iconData, scaled);
      renderer.drawIcon(scaled, iconFrameX + 6, iconFrameY + 6, 64, 64);
    } else {
      renderer.drawCenteredText(SMALL_FONT_ID, iconFrameY + 28, "No Icon", true);
    }

    // App Details beside Hero Icon
    int infoX = 116;
    renderer.drawText(UI_12_FONT_ID, infoX, cardY + 14, app.name.c_str(), true, EpdFontFamily::BOLD);

    char devBuf[64];
    snprintf(devBuf, sizeof(devBuf), "Developer: %s", app.author.c_str());
    renderer.drawText(SMALL_FONT_ID, infoX, cardY + 40, devBuf, true);

    char verBuf[64];
    if (app.isInstalled) {
      if (app.installedVersion != app.version) {
        snprintf(verBuf, sizeof(verBuf), "Installed: v%s  ->  New: v%s", app.installedVersion.c_str(),
                 app.version.c_str());
      } else {
        snprintf(verBuf, sizeof(verBuf), "Version: v%s (Up to date)", app.version.c_str());
      }
    } else {
      snprintf(verBuf, sizeof(verBuf), "Latest Version: v%s", app.version.c_str());
    }
    renderer.drawText(SMALL_FONT_ID, infoX, cardY + 60, verBuf, true, EpdFontFamily::BOLD);

    // 2. About / Description Box
    int descY = cardY + cardH + 12;
    int descH = 176;
    renderer.drawRoundedRect(16, descY, w - 32, descH, 1, 8, true);
    renderer.drawText(UI_10_FONT_ID, 28, descY + 12, "About this Application", true, EpdFontFamily::BOLD);
    renderer.drawLine(28, descY + 34, w - 28, descY + 34, Color::Black);

    auto dLines = renderer.wrappedText(SMALL_FONT_ID, app.description.c_str(), w - 56, 5);
    int dy = descY + 44;
    for (const auto& dl : dLines) {
      renderer.drawText(SMALL_FONT_ID, 28, dy, dl.c_str(), true);
      dy += renderer.getLineHeight(SMALL_FONT_ID) + 4;
    }

    // 3. Package Information Box
    int pkgY = descY + descH + 12;
    int pkgH = 118;
    renderer.drawRoundedRect(16, pkgY, w - 32, pkgH, 1, 8, true);
    renderer.drawText(UI_10_FONT_ID, 28, pkgY + 12, "Package Details", true, EpdFontFamily::BOLD);
    renderer.drawLine(28, pkgY + 34, w - 28, pkgY + 34, Color::Black);

    char pkgIdBuf[64];
    snprintf(pkgIdBuf, sizeof(pkgIdBuf), "Package ID: %s", app.id.c_str());
    renderer.drawText(SMALL_FONT_ID, 28, pkgY + 44, pkgIdBuf, true);

    char pkgFilesBuf[64];
    snprintf(pkgFilesBuf, sizeof(pkgFilesBuf), "Distribution: %zu package files on GitHub", app.files.size());
    renderer.drawText(SMALL_FONT_ID, 28, pkgY + 64, pkgFilesBuf, true);

    std::string pathBuf = "Location: /.crosssmudge/applications/" + app.id;
    renderer.drawText(SMALL_FONT_ID, 28, pkgY + 84, pathBuf.c_str(), true);

    // 4. Action Buttons (Touch + Physical Prompts)
    int btnY = getDetailActionButtonY();
    int btnH = 50;

    if (!app.isInstalled) {
      // Single wide Install button
      int bw = 320;
      int bx = (w - bw) / 2;
      renderer.fillRoundedRect(bx, btnY, bw, btnH, 8, Color::Black);
      int tw = renderer.getTextWidth(UI_12_FONT_ID, "Install Application", EpdFontFamily::BOLD);
      renderer.drawText(UI_12_FONT_ID, bx + (bw - tw) / 2, btnY + 14, "Install Application", false,
                        EpdFontFamily::BOLD);
    } else {
      // Upgrade or Reinstall (Left) + Uninstall (Right)
      int bw = 200;
      int bx1 = 24;
      int bx2 = w - 24 - bw;
      const char* primaryText = (app.installedVersion != app.version) ? "Upgrade App" : "Reinstall";

      renderer.fillRoundedRect(bx1, btnY, bw, btnH, 8, Color::Black);
      int tw1 = renderer.getTextWidth(UI_12_FONT_ID, primaryText, EpdFontFamily::BOLD);
      renderer.drawText(UI_12_FONT_ID, bx1 + (bw - tw1) / 2, btnY + 14, primaryText, false, EpdFontFamily::BOLD);

      renderer.drawRoundedRect(bx2, btnY, bw, btnH, 2, 8, true);
      int tw2 = renderer.getTextWidth(UI_12_FONT_ID, "Uninstall", EpdFontFamily::BOLD);
      renderer.drawText(UI_12_FONT_ID, bx2 + (bw - tw2) / 2, btnY + 14, "Uninstall", true, EpdFontFamily::BOLD);
    }

    // Button hints
    const char* hint1 = tr(STR_BACK);
    const char* hint2 = !app.isInstalled ? "Install" : (app.installedVersion != app.version ? "Upgrade" : "Reinstall");
    const char* hint3 = app.isInstalled ? "Uninstall" : "";
    const auto labels = mappedInput.mapLabels(hint1, hint2, hint3, "");
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  }

  void renderDownloadProgress(const char* appName) {
    int w = renderer.getScreenWidth();
    int h = renderer.getScreenHeight();
    const auto& m = UITheme::getInstance().getMetrics();

    renderer.clearScreen();
    GUI.drawHeader(renderer, Rect{0, m.topPadding, w, m.headerHeight}, "Installing App", appName);

    renderer.drawCenteredText(UI_12_FONT_ID, h / 2 - 40, "Downloading from GitHub...", true, EpdFontFamily::BOLD);

    char fileBuf[80];
    snprintf(fileBuf, sizeof(fileBuf), "File %d of %d: %s", currentFileNum_, totalFiles_,
             currentDownloadingFile_.c_str());
    renderer.drawCenteredText(SMALL_FONT_ID, h / 2, fileBuf, true);

    const auto labels = mappedInput.mapLabels("Cancel", "", "", "");
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  }

  void renderError() {
    int w = renderer.getScreenWidth();
    int h = renderer.getScreenHeight();
    const auto& m = UITheme::getInstance().getMetrics();

    renderer.clearScreen();
    GUI.drawHeader(renderer, Rect{0, m.topPadding, w, m.headerHeight}, "App Store", "Error");

    renderer.drawCenteredText(UI_12_FONT_ID, h / 2 - 40, errorTitle_.empty() ? "Error" : errorTitle_.c_str(), true,
                              EpdFontFamily::BOLD);
    auto lines = renderer.wrappedText(SMALL_FONT_ID, errorMessage_.c_str(), w - 64, 4);
    int dy = h / 2;
    for (const auto& l : lines) {
      renderer.drawCenteredText(SMALL_FONT_ID, dy, l.c_str(), true);
      dy += renderer.getLineHeight(SMALL_FONT_ID) + 2;
    }

    const auto labels = mappedInput.mapLabels(tr(STR_BACK), "Retry", "", "");
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  }
};
