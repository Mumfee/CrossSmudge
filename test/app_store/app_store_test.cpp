#include "stubs/TestPlatform.h"
#include <ArduinoJson.h>
#include <cstdio>
#include <cstdlib>
#include "stubs/network/HttpDownloader.h"
// Expose state only to seed deterministic catalog fixtures and inspect outcomes.
// All loading, rendering, input and scheduling behavior runs production methods.
#define private public
#ifndef APP_STORE_ACTIVITY_HEADER
#define APP_STORE_ACTIVITY_HEADER "activities/apps/AppStoreActivity.h"
#endif
#include APP_STORE_ACTIVITY_HEADER
#undef private
static int failures = 0;
#define CHECK(c) do {if (!(c)){std::printf("FAIL %s:%d: %s\n", __func__, __LINE__, #c); ++failures; }}while (0)
struct Fixture {
  GfxRenderer renderer;
  MappedInputManager input;
  AppStoreActivity app {
    renderer, input
  };
  Fixture(int count = 16) {
    Storage = FakeStorage {};
    WiFi.connected = true;
    HalFile::syncOK = HalFile::closeOK = true;
    HalFile::corruptReads = false;
    fakeMillis = 1000;
    UITheme::getInstance().metrics = Metrics {};
    UITheme::statusInset = 0;
    HttpDownloader::requests.clear();
    HttpDownloader::transports.clear();
    HttpDownloader::lockDepths.clear();
    HttpDownloader::duringRequest = nullptr;
    HttpDownloader::result = HttpDownloader::OK;
    HttpDownloader::body.assign(128, 0xa5);
    HttpDownloader::chunkSize = 31;
    app.baseUrl_ = "https://example.invalid/apps/";
    app.state_ = AppStoreActivity::State::CATALOG_READY;
    for (int i = 0; i<count; ++i) {
      AppStoreActivity::CatalogApp a;
      a.id = "fixture-"+std::to_string(i);
      a.name = "Fixture "+std::to_string(i);
      a.version = "1.0";
      a.author = "Author";
      app.apps_.push_back(a);
    }
  }
  ~Fixture() {
    CHECK(HalFile::openCount == 0);
  }
  void tick() {
    fakeMillis += 300;
    app.loop();
    input.released.clear();
    input.tapped = false;
    input.swipe = MappedInputManager::SwipeDir::None;
  }
  std::vector<Draw> rows() {
    app.render(RenderLock {});
    std::vector<Draw> v;
    for (auto&d:renderer.draws)if (d.kind == "row" && d.w>renderer.width/2 && d.h>= 48)v.push_back(d);
    return v;
  }
  static std::string cache() {
    return "/.crosssmudge/cache/icons/fixture-0.raw";
  }
  static std::string installed() {
    return "/.crosssmudge/applications/fixture-0/icon.raw";
  }
  static std::string legacy() {
    return "/.smudge/applications/fixture-0/icon.raw";
  }
};
void lazyIcons() {
  Fixture f;
  f.app.render(RenderLock {});
  CHECK(HttpDownloader::requests.empty());
  f.tick();
  CHECK(HttpDownloader::requests.size() == 1);
  CHECK(f.app.apps_[0].hasIcon);
  CHECK(!f.app.apps_[0].isInstalled);
  CHECK(HttpDownloader::transports == std::vector<HttpDownloader::Transport> {
    HttpDownloader::Transport::ESP_HTTP
  });
  CHECK(HttpDownloader::lockDepths == std::vector<int> {
    0
  });
  for (int depth:Storage.ioLockDepths)CHECK(depth>0);
  f.app.render(RenderLock {});
  CHECK(HttpDownloader::requests.size() == 1);
  f.app.loop();
  CHECK(HttpDownloader::requests.size() == 1);
}
void portrait() {
  Fixture f;
  auto rows = f.rows();
  CHECK(rows.size()>8);
  CHECK(rows.back().y+rows.back().h>650);
}
void landscape() {
  Fixture f;
  f.renderer.width = 800;
  f.renderer.height = 480;
  f.renderer.orientation = GfxRenderer::Orientation::LandscapeClockwise;
  auto rows = f.rows();
  CHECK(!rows.empty());
  for (auto&r:rows)CHECK(r.y+r.h<440);
}
void localIconValidation() {
  for (size_t n: {
    size_t(0), size_t(127), size_t(129), size_t(512)
  }) {
    Fixture f(1);
    Storage.files[Fixture::cache()].bytes.assign(n, 0xcc);
    f.app.loadAppIcon(f.app.apps_[0], false);
    CHECK(!f.app.apps_[0].hasIcon);
    CHECK(HttpDownloader::requests.empty());
  }
  {
    Fixture f(1);
    Storage.files[Fixture::installed()].bytes.assign(127, 0);
    Storage.files[Fixture::legacy()].bytes.assign(128, 0xb6);
    f.app.loadAppIcon(f.app.apps_[0], false);
    CHECK(f.app.apps_[0].hasIcon);
    CHECK(f.app.apps_[0].iconData[0] == 0xb6);
  }
  {
    Fixture f(1);
    Storage.files[Fixture::installed()].bytes.assign(128, 0);
    Storage.files[Fixture::installed()].readLimit = 127;
    Storage.files[Fixture::cache()].bytes.assign(128, 0xb7);
    f.app.loadAppIcon(f.app.apps_[0], false);
    CHECK(f.app.apps_[0].hasIcon);
    CHECK(f.app.apps_[0].iconData[0] == 0xb7);
  }
  {
    Fixture f(1);
    WiFi.connected = false;
    Storage.files[Fixture::cache()].bytes.assign(128, 0xb8);
    f.app.loadAppIcon(f.app.apps_[0], true);
    CHECK(f.app.apps_[0].hasIcon);
    CHECK(f.app.apps_[0].iconData[127] == 0xb8);
    CHECK(HttpDownloader::requests.empty());
  }
}
void downloadValidation() {
  for (size_t n: {
    size_t(0), size_t(127), size_t(129), size_t(512)
  }) {
    Fixture f(1);
    HttpDownloader::body.assign(n, 0xcc);
    HttpDownloader::chunkSize = 128;
    f.app.loadAppIcon(f.app.apps_[0], true);
    CHECK(!f.app.apps_[0].hasIcon);
    CHECK(!Storage.exists(Fixture::cache().c_str()));
    CHECK(Storage.writes.empty());
  }
  {
    Fixture f(1);
    HttpDownloader::result = HttpDownloader::HTTP_ERROR;
    f.app.loadAppIcon(f.app.apps_[0], true);
    CHECK(!f.app.apps_[0].hasIcon);
    CHECK(Storage.writes.empty());
  }
  for (int failure = 0; failure<5; ++failure) {
    Fixture f(1);
    if (failure == 0)Storage.directories = false;
    if (failure == 1)Storage.writeOpen = false;
    if (failure == 2)Storage.nextWriteLimit = 127;
    if (failure == 3)HalFile::syncOK = false;
    if (failure == 4)HalFile::closeOK = false;
    f.app.loadAppIcon(f.app.apps_[0], true);
    CHECK(!f.app.apps_[0].hasIcon);
    CHECK(!Storage.exists(Fixture::cache().c_str()));
  }
  {
    Fixture f(1);
    Storage.files[Fixture::cache()].readable = false;
    f.app.loadAppIcon(f.app.apps_[0], true);
    CHECK(!f.app.apps_[0].hasIcon);
    CHECK(!Storage.exists(Fixture::cache().c_str()));
  }
  {
    Fixture f(1);
    Storage.files[Fixture::cache()].readLimit = 127;
    f.app.loadAppIcon(f.app.apps_[0], true);
    CHECK(!f.app.apps_[0].hasIcon);
    CHECK(!Storage.exists(Fixture::cache().c_str()));
  }
}
void corruptedReadback() {
  Fixture f(1);
  HalFile::corruptReads = true;
  f.app.loadAppIcon(f.app.apps_[0], true);
  CHECK(!f.app.apps_[0].hasIcon);
  CHECK(!Storage.exists(Fixture::cache().c_str()));
}
void safeIds() {
  for (const auto&id:std::vector<std::string> {
    "", "..", "../escape", "a/b", "a\\b", "%2e%2e", "a?x", "a#x", "a.b", std::string(65, 'a'), std::string("a\0b", 3)
  }) {
    Fixture f(1);
    f.app.apps_[0].id = id;
    f.app.loadAppIcon(f.app.apps_[0], true);
    CHECK(!f.app.apps_[0].hasIcon);
    CHECK(HttpDownloader::requests.empty());
    CHECK(Storage.reads.empty());
    CHECK(Storage.writes.empty());
  }
}
void attemptsAndBack() {
  {
    Fixture f(1);
    HttpDownloader::result = HttpDownloader::HTTP_ERROR;
    for (int i = 0; i<5; ++i)f.tick();
    CHECK(HttpDownloader::requests.size() == 1);
    f.app.ensureAppIcon(f.app.apps_[0]);
    CHECK(HttpDownloader::requests.size() == 1);
  }
  {
    Fixture f;
    auto count = f.rows().size();
    for (int i = 0; i<30; ++i)f.tick();
    CHECK(HttpDownloader::requests.size() == count);
    for (size_t i = count; i<f.app.apps_.size(); ++i)CHECK(!f.app.apps_[i].hasIcon);
    f.app.selectedIndex_ = count;
    f.tick();
    CHECK(HttpDownloader::requests.size() == count+1);
    CHECK(f.app.apps_[count].hasIcon);
  }
  {
    Fixture f;
    f.input.released.insert(MappedInputManager::Button::Back);
    f.tick();
    CHECK(f.app.finished);
    CHECK(HttpDownloader::requests.empty());
  }
  {
    Fixture f(1);
    WiFi.connected = false;
    f.tick();
    CHECK(HttpDownloader::requests.empty());
    WiFi.connected = true;
    f.tick();
    CHECK(HttpDownloader::requests.size() == 1);
  }
}
void inputAndPages() {
  for (bool landscape: {
    false, true
  }) {
    Fixture f;
    f.renderer.width = landscape?800:480;
    f.renderer.height = landscape?480:800;
    f.renderer.orientation = landscape?GfxRenderer::Orientation::LandscapeClockwise:GfxRenderer::Orientation::Portrait;
    auto rows = f.rows();
    CHECK(rows.size()>1);
    auto target = rows.back();
    f.input.tap(target.x+2, target.y+2);
    f.tick();
    CHECK(f.app.selectedIndex_ == static_cast<int>(rows.size())-1);
    CHECK(f.app.state_ == AppStoreActivity::State::CATALOG_READY);
    CHECK(HttpDownloader::requests.empty());
    f.input.tap(target.x+2, target.y+2);
    f.tick();
    CHECK(f.app.state_ == AppStoreActivity::State::APP_DETAIL);
    f.input.released.insert(MappedInputManager::Button::Back);
    f.tick();
    CHECK(f.app.state_ == AppStoreActivity::State::CATALOG_READY);
    f.app.selectedIndex_ = 0;
    f.input.released.insert(MappedInputManager::Button::Up);
    f.tick();
    CHECK(f.app.selectedIndex_ == 15);
    auto lastRows = f.rows();
    CHECK(lastRows.size() == 16%rows.size() || (lastRows.size() == rows.size() && 16%rows.size() == 0));
    f.input.released.insert(MappedInputManager::Button::PageForward);
    f.tick();
    CHECK(f.app.selectedIndex_ == 0);
    f.input.swipe = MappedInputManager::SwipeDir::Up;
    f.tick();
    CHECK(f.app.selectedIndex_ == 1);
  }
  {
    Fixture f(0);
    f.app.render(RenderLock {});
    f.tick();
    CHECK(HttpDownloader::requests.empty());
  }
}
void safeGeometry() {
  for (auto orientation: {
    GfxRenderer::Orientation::Portrait, GfxRenderer::Orientation::PortraitInverted, GfxRenderer::Orientation::LandscapeClockwise, GfxRenderer::Orientation::LandscapeCounterClockwise
  }) {
    Fixture f;
    f.renderer.orientation = orientation;
    if (orientation == GfxRenderer::Orientation::LandscapeClockwise || orientation == GfxRenderer::Orientation::LandscapeCounterClockwise) {
      f.renderer.width = 800;
      f.renderer.height = 480;
    }
    f.renderer.top = 13;
    f.renderer.right = 17;
    f.renderer.bottom = 21;
    f.renderer.left = 23;
    UITheme::statusInset = 9;
    f.renderer.smallHeight = 22;
    f.renderer.titleHeight = 26;
    for (auto&a:f.app.apps_) {
      a.name = std::string(150, 'N');
      a.author = std::string(120, 'A');
      a.version = std::string(70, 'V');
      a.isInstalled = true;
      a.installedVersion = a.version;
    }
    auto rows = f.rows();
    auto safe = UITheme::getInstance().getScreenSafeArea(f.renderer, true);
    int left = std::max(safe.x, f.renderer.left), right = std::min(safe.right(), f.renderer.width-f.renderer.right), bottom = std::min(safe.bottom(), f.renderer.height-f.renderer.bottom);
    for (auto&r:rows) {
      CHECK(r.x>= left);
      CHECK(r.x+r.w<= right);
      CHECK(r.y>= f.renderer.top);
      CHECK(r.y+r.h<bottom);
    }
    for (auto&d:f.renderer.draws)if (d.kind == "text") {
      CHECK(d.x>= left);
      CHECK(d.x+d.w<= right);
      CHECK(d.y+d.h<= bottom);
    }
    auto before = f.app.selectedIndex_;
    f.input.tap(-1, f.renderer.height-1);
    f.tick();
    CHECK(!f.app.finished);
    CHECK(f.app.selectedIndex_ == before);
  }
}
void footerBoundaries() {
#if defined(APP_STORE_BASELINE)
  std::puts("SKIP: new footer-only boundary cases on baseline without catalogLayout()");
#else
  for (int width: {
    479, 480, 481, 800
  })for (int action = 0; action<4; ++action) {
    Fixture f;
    f.input.touch = true;
    f.renderer.width = width;
    const auto l = f.app.catalogLayout();
    const int x = l.left+(l.right-l.left)*action/4;
    f.input.tap(x, l.controlsY+3);
    f.tick();
    if (action == 0)CHECK(f.app.finished);
    if (action == 1)CHECK(f.app.state_ == AppStoreActivity::State::APP_DETAIL);
    if (action == 2)CHECK(f.app.selectedIndex_ == 15);
    if (action == 3)CHECK(f.app.selectedIndex_ == 1);
  }
  {
    Fixture f(0);
    f.input.touch = true;
    auto l = f.app.catalogLayout();
    f.input.tap(l.left, l.controlsY);
    f.tick();
    CHECK(f.app.finished);
  }
  {
    Fixture f;
    auto l = f.app.catalogLayout();
    for (auto xy:std::vector<std::pair<int, int>> {
      {
        l.left-1, l.controlsY
      }, {
        l.right, l.controlsY
      }, {
        l.left, l.bottom
      }, {
        l.left, l.startY+l.rowH-1
      }
    }) {
      f.input.tap(xy.first, xy.second);
      f.tick();
      CHECK(!f.app.finished);
      CHECK(f.app.state_ == AppStoreActivity::State::CATALOG_READY);
      CHECK(f.app.selectedIndex_ == 0);
    }
  }
  {
    Fixture f;
    f.input.pressed.insert(MappedInputManager::Button::Back);
    f.tick();
    CHECK(HttpDownloader::requests.empty());
  }
#endif
}
void catalogParsing() {
  Fixture f(0);
  const std::string json = R"({"apps":[{"id":"../bad"},{"id":"fixture-0","name":"Remote app","version":"2"},{"id":"fixture-1","version":"1"}]})";
  HttpDownloader::body.assign(json.begin(), json.end());
  const std::string manifest = R"({"version":"1"})";
  Storage.files["/.crosssmudge/applications/fixture-1/manifest.json"].bytes.assign(manifest.begin(), manifest.end());
  Storage.files[Fixture::cache()].bytes.assign(128, 0xbc);
  f.app.catalogUrl_ = "https://example.invalid/catalog.json";
  f.app.fetchCatalog();
  CHECK(f.app.apps_.size() == 2);
  CHECK(f.app.apps_[0].id == "fixture-0");
  CHECK(f.app.apps_[0].hasIcon);
  CHECK(!f.app.apps_[0].isInstalled);
  CHECK(f.app.apps_[1].isInstalled);
  CHECK(HttpDownloader::requests.size() == 1);
  CHECK(HttpDownloader::requests[0] == f.app.catalogUrl_);
}
bool validUtf8(const std::string&s) {
  for (size_t i = 0; i<s.size(); ) {
    auto c = static_cast<unsigned char>(s[i]);
    size_t n = c<128?1:(c&0xe0) == 0xc0?2:(c&0xf0) == 0xe0?3:(c&0xf8) == 0xf0?4:0;
    if (!n || i+n>s.size())return false;
    for (size_t j = 1; j<n; ++j)if ((static_cast<unsigned char>(s[i+j])&0xc0) != 0x80)return false;
    i += n;
  }
  return true;
}
void utf8Names() {
  Fixture f(1);
  f.app.apps_[0].name = std::string(94, 'a')+"é";
  f.renderer.width = 4000;
  f.rows();
  for (auto&d:f.renderer.draws)if (d.kind == "text")CHECK(validUtf8(d.text));
}
#if defined(APP_STORE_WIFI_COMPOSITION)
void wifiComposition() {
  Fixture f(0);
  f.input.touch = true;
  WiFi.connected = false;
  f.app.state_ = AppStoreActivity::State::CHECK_WIFI;
  f.app.render(RenderLock{});
  const auto action = f.app.wifiActionLayout().container;
  f.input.tap(action.x + 8, action.y + 8);
  f.tick();
  CHECK(f.app.childStarts == 1);
  CHECK(HttpDownloader::requests.empty());
  CHECK(!f.app.finished);
  WiFi.connected = true;
  f.app.childResult(ActivityResult{});
  CHECK(f.app.state_ == AppStoreActivity::State::FETCHING_CATALOG);
  CHECK(HttpDownloader::requests.empty());
  const std::string catalog = R"({"apps":[{"id":"fixture-0"}]})";
  HttpDownloader::body.assign(catalog.begin(), catalog.end());
  f.tick();
  CHECK(f.app.state_ == AppStoreActivity::State::CATALOG_READY);
  CHECK(f.app.apps_.size() == 1);
  CHECK(HttpDownloader::requests.size() == 1);
  HttpDownloader::body.assign(128, 0xa5);
  f.tick();
  CHECK(f.app.apps_[0].hasIcon);
  CHECK(HttpDownloader::requests.size() == 2);
}
#endif
int main() {
  corruptedReadback();
#if defined(APP_STORE_WIFI_COMPOSITION)
  wifiComposition();
  std::printf("Wi-Fi composition failures: %d\n", failures);
#endif
  const std::pair<const char*, void(*)()> cases[] = {
    {
      "lazy icons and render isolation", lazyIcons
    }, {
      "portrait full screen", portrait
    }, {
      "landscape containment", landscape
    }, {
      "local cache validation", localIconValidation
    }, {
      "download and write failures", downloadValidation
    }, {
      "safe app IDs", safeIds
    }, {
      "attempt scheduling and Back", attemptsAndBack
    }, {
      "input and pages", inputAndPages
    }, {
      "bezel/font/text geometry", safeGeometry
    }, {
      "footer exact boundaries", footerBoundaries
    }, {
      "real catalog parsing", catalogParsing
    }, {
      "UTF-8 truncation", utf8Names
    }
  };
  for (const auto& c:cases) {
    int before = failures;
    c.second();
    std::printf("%s: %s\n", failures == before?"PASS":"FAIL", c.first);
  }
  std::printf("%d failures\n", failures);
  return failures?1:0;
}
