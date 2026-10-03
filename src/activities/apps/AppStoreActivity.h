#pragma once

#include <ArduinoJson.h>
#include <HalStorage.h>

#include <algorithm>
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
    errorMessage_.clear();
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
        handleCatalogInput();
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
            state_ = State::FETCHING_CATALOG;
            requestUpdateAndWait();
            fetchCatalog();
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
                state_ = State::FETCHING_CATALOG;
                requestUpdateAndWait();
                fetchCatalog();
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
    uint8_t iconData[128] = {0};
  };

  State state_ = State::CHECK_WIFI;
  std::string catalogUrl_;
  std::string baseUrl_;
  std::string errorMessage_;
  std::vector<CatalogApp> apps_;
  int selectedIndex_ = 0;
  bool cancelDownload_ = false;

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

  void loadAppIcon(CatalogApp& app, bool allowDownload = false) {
    app.hasIcon = false;
    std::string iconPath1 = "/.crosssmudge/applications/" + app.id + "/icon.raw";
    std::string iconPath2 = "/.smudge/applications/" + app.id + "/icon.raw";
    std::string cachePath = "/.crosssmudge/cache/icons/" + app.id + ".raw";

    HalFile f;
    if (Storage.openFileForRead("STORE", iconPath1.c_str(), f) ||
        Storage.openFileForRead("STORE", iconPath2.c_str(), f) ||
        Storage.openFileForRead("STORE", cachePath.c_str(), f)) {
      size_t readBytes = f.read(app.iconData, sizeof(app.iconData));
      f.close();
      if (readBytes == sizeof(app.iconData)) {
        app.hasIcon = true;
        return;
      }
    }

#if defined(SIMULATOR)
    std::string hostPath = "apps/" + app.id + "/icon.raw";
    FILE* hf = fopen(hostPath.c_str(), "rb");
    if (!hf) hf = fopen(("../" + hostPath).c_str(), "rb");
    if (!hf) hf = fopen(("/var/home/brady/C/crosssmudge/" + hostPath).c_str(), "rb");
    if (hf) {
      size_t r = fread(app.iconData, 1, sizeof(app.iconData), hf);
      fclose(hf);
      if (r == sizeof(app.iconData)) {
        app.hasIcon = true;
        return;
      }
    }
#endif

    if (allowDownload && isWifiConnected()) {
      Storage.ensureDirectoryExists("/.crosssmudge");
      Storage.ensureDirectoryExists("/.crosssmudge/cache");
      Storage.ensureDirectoryExists("/.crosssmudge/cache/icons");
      std::string iconUrl = baseUrl_ + app.id + "/icon.raw";
      HttpDownloader::DownloadOptions options;
#if defined(FREEINK_NET_WOLFSSL)
      options.transport = HttpDownloader::Transport::WOLFSSL;
#endif
      if (HttpDownloader::downloadToFile(iconUrl, cachePath, nullptr, nullptr, "", "", options) == HttpDownloader::OK) {
        if (Storage.openFileForRead("STORE", cachePath.c_str(), f)) {
          size_t readBytes = f.read(app.iconData, sizeof(app.iconData));
          f.close();
          if (readBytes == sizeof(app.iconData)) {
            app.hasIcon = true;
          }
        }
      }
    }
  }

  void ensureAppIcon(CatalogApp& app) {
    if (app.hasIcon) return;
    loadAppIcon(app, true);
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
      errorMessage_ = "Failed to download catalog.json.\nCheck network connection.";
      requestUpdate();
      return;
    }

    FsFile catalogFile;
    if (!Storage.openFileForRead("STORE", tmpCatalog.c_str(), catalogFile)) {
      Storage.remove(tmpCatalog.c_str());
      state_ = State::ERROR;
      errorMessage_ = "Failed to read catalog.tmp.";
      requestUpdate();
      return;
    }

    JsonDocument doc;
    DeserializationError jsonErr = deserializeJson(doc, catalogFile);
    catalogFile.close();
    Storage.remove(tmpCatalog.c_str());

    if (jsonErr) {
      state_ = State::ERROR;
      errorMessage_ = std::string("JSON Parse Error: ") + jsonErr.c_str();
      requestUpdate();
      return;
    }

    apps_.clear();
    JsonArray appList = doc["apps"].as<JsonArray>();
    for (JsonObject obj : appList) {
      CatalogApp app;
      app.id = obj["id"] | "";
      app.name = obj["name"] | app.id.c_str();
      app.version = obj["version"] | "1.0.0";
      app.author = obj["author"] | "Unknown";
      app.description = obj["description"] | "";

      JsonArray files = obj["files"].as<JsonArray>();
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

  void handleCatalogInput() {
    int count = static_cast<int>(apps_.size());
    if (count == 0) return;

    const int rowsPerPage = 8;
    int pageStart = (selectedIndex_ / rowsPerPage) * rowsPerPage;

    if (mappedInput.wasReleased(MappedInputManager::Button::Right) ||
        mappedInput.wasReleased(MappedInputManager::Button::Down) ||
        mappedInput.wasReleased(MappedInputManager::Button::PageForward)) {
      selectedIndex_ = (selectedIndex_ + 1) % count;
      requestUpdate();
      return;
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Left) ||
               mappedInput.wasReleased(MappedInputManager::Button::Up) ||
               mappedInput.wasReleased(MappedInputManager::Button::PageBack)) {
      selectedIndex_ = (selectedIndex_ - 1 + count) % count;
      requestUpdate();
      return;
    }

    auto swipe = mappedInput.wasSwipe();
    if (swipe == MappedInputManager::SwipeDir::Up || swipe == MappedInputManager::SwipeDir::Left) {
      selectedIndex_ = (selectedIndex_ + 1) % count;
      requestUpdate();
      return;
    } else if (swipe == MappedInputManager::SwipeDir::Down || swipe == MappedInputManager::SwipeDir::Right) {
      selectedIndex_ = (selectedIndex_ - 1 + count) % count;
      requestUpdate();
      return;
    }

    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      state_ = State::APP_DETAIL;
      ensureAppIcon(apps_[selectedIndex_]);
      requestUpdate();
      return;
    }

    // Touch handling
    int tx = 0, ty = 0;
    if (mappedInput.wasScreenTapped(tx, ty)) {
      int w = renderer.getScreenWidth();
      int h = renderer.getScreenHeight();
      const auto& m = UITheme::getInstance().getMetrics();

      if (ty > h - m.buttonHintsHeight - 20) {
        if (tx < w / 4) {
          finish();
          return;
        } else if (tx < w / 2) {
          state_ = State::APP_DETAIL;
          ensureAppIcon(apps_[selectedIndex_]);
          requestUpdate();
          return;
        } else if (tx < 3 * w / 4) {
          selectedIndex_ = (selectedIndex_ - 1 + count) % count;
          requestUpdate();
          return;
        } else {
          selectedIndex_ = (selectedIndex_ + 1) % count;
          requestUpdate();
          return;
        }
      }

      int startY = m.topPadding + m.headerHeight + 6;
      int rowH = 56;
      for (int i = pageStart; i < count && i < pageStart + rowsPerPage; ++i) {
        int ry = startY + (i - pageStart) * rowH;
        if (tx >= 16 && tx <= w - 16 && ty >= ry && ty <= ry + rowH - 4) {
          if (selectedIndex_ == i) {
            state_ = State::APP_DETAIL;
            ensureAppIcon(apps_[selectedIndex_]);
          } else {
            selectedIndex_ = i;
          }
          requestUpdate();
          return;
        }
      }
    }
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
    state_ = State::DOWNLOADING;
    cancelDownload_ = false;
    totalFiles_ = static_cast<int>(app.files.size());

    sdFontSystem.releaseForNetwork(renderer);

    std::string targetDir = "/.crosssmudge/applications/" + app.id;
    Storage.ensureDirectoryExists("/.crosssmudge");
    Storage.ensureDirectoryExists("/.crosssmudge/applications");
    Storage.ensureDirectoryExists(targetDir.c_str());

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
        Storage.ensureDirectoryExists(parentDir.c_str());
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

#if defined(SIMULATOR)
      HttpDownloader::DownloadError err = HttpDownloader::HTTP_ERROR;
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
        }
        fclose(inHost);
      }
      if (err != HttpDownloader::OK) {
        err = HttpDownloader::downloadToFile(fileUrl, destPath, nullptr, &cancelDownload_, "", "", options);
      }
#else
      auto err = HttpDownloader::downloadToFile(fileUrl, destPath, nullptr, &cancelDownload_, "", "", options);
#if defined(FREEINK_NET_WOLFSSL)
      if (err != HttpDownloader::OK && !cancelDownload_) {
        options.transport = HttpDownloader::Transport::ESP_HTTP;
        err = HttpDownloader::downloadToFile(fileUrl, destPath, nullptr, &cancelDownload_, "", "", options);
      }
#endif
#endif

      if (err != HttpDownloader::OK) {
        if (cancelDownload_) {
          LOG_INF("STORE", "Download cancelled");
          break;
        }
        LOG_ERR("STORE", "Failed downloading %s: %d", app.files[i].c_str(), err);
        state_ = State::ERROR;
        errorMessage_ = "Failed downloading: " + app.files[i];
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

  void renderCatalog() {
    int w = renderer.getScreenWidth();
    int h = renderer.getScreenHeight();
    const auto& m = UITheme::getInstance().getMetrics();

    renderer.clearScreen();
    char subBuf[64];
    snprintf(subBuf, sizeof(subBuf), "%zu Apps Available", apps_.size());
    GUI.drawHeader(renderer, Rect{0, m.topPadding, w, m.headerHeight}, "App Store", subBuf);

    int startY = m.topPadding + m.headerHeight + 6;
    int rowH = 56;
    int count = static_cast<int>(apps_.size());
    const int rowsPerPage = 8;
    int pageStart = (selectedIndex_ / rowsPerPage) * rowsPerPage;

    for (int i = pageStart; i < count && i < pageStart + rowsPerPage; ++i) {
      const auto& app = apps_[i];
      int y = startY + (i - pageStart) * rowH;
      bool isSelected = (i == selectedIndex_);

      if (isSelected) {
        renderer.fillRoundedRect(16, y, w - 32, rowH - 4, 6, Color::Black);
      } else {
        renderer.drawRoundedRect(16, y, w - 32, rowH - 4, 1, 6, true);
      }

      // 32x32 App Icon
      int iconX = 26;
      int iconY = y + 8;
      if (app.hasIcon) {
        if (isSelected) {
          renderer.drawIconInverted(app.iconData, iconX, iconY, 32, 32);
        } else {
          renderer.drawIcon(app.iconData, iconX, iconY, 32, 32);
        }
      } else {
        renderer.drawRoundedRect(iconX, iconY, 32, 32, 1, 4, !isSelected);
      }

      // App Name
      char titleBuf[64];
      snprintf(titleBuf, sizeof(titleBuf), "%s", app.name.c_str());
      renderer.drawText(UI_10_FONT_ID, 68, y + 8, titleBuf, !isSelected, EpdFontFamily::BOLD);

      // Author & Version
      char authBuf[64];
      snprintf(authBuf, sizeof(authBuf), "by %s  •  v%s", app.author.c_str(), app.version.c_str());
      renderer.drawText(SMALL_FONT_ID, 68, y + 28, authBuf, !isSelected);

      // Status badge on the right
      const char* statusStr = "[ Install ]";
      if (app.isInstalled) {
        if (app.installedVersion != app.version) {
          statusStr = "[ Update ]";
        } else {
          statusStr = "[ Installed ]";
        }
      }
      int sw = renderer.getTextWidth(SMALL_FONT_ID, statusStr, EpdFontFamily::BOLD);
      renderer.drawText(SMALL_FONT_ID, w - 32 - sw - 10, y + 18, statusStr, !isSelected, EpdFontFamily::BOLD);
    }

    // Page indicator and prompt below list
    int totalPages = std::max(1, (count + rowsPerPage - 1) / rowsPerPage);
    int curPage = (selectedIndex_ / rowsPerPage) + 1;
    char pageBuf[64];
    snprintf(pageBuf, sizeof(pageBuf), "Page %d of %d  •  %d Apps", curPage, totalPages, count);
    int footY = startY + (rowsPerPage * rowH) + 12;
    renderer.drawCenteredText(SMALL_FONT_ID, footY, pageBuf, true, EpdFontFamily::BOLD);
    renderer.drawCenteredText(SMALL_FONT_ID, footY + 18, "Press Confirm or Tap to view details", true);

    const auto labels = mappedInput.mapLabels(tr(STR_BACK), "Details", tr(STR_DIR_UP), tr(STR_DIR_DOWN));
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

    renderer.drawCenteredText(UI_12_FONT_ID, h / 2 - 40, "Network Error", true, EpdFontFamily::BOLD);
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
