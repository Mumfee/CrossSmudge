#pragma once
// Host-only adapters. Geometry/text recording is not a raster or on-glass renderer.
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
using String = std::string;
namespace EpdFontFamily {
  enum Style {
    REGULAR, BOLD
  };
}
namespace Color {
  enum Value {
    Black
  };
}
struct Rect {
  int x, y, width, height;
  int right() const {
    return x+width;
  }
  int bottom() const {
    return y+height;
  }
};
struct Draw {
  std::string kind, text;
  int x, y, w, h;
};
class GfxRenderer {
  public:  enum class Orientation {
    Portrait, LandscapeClockwise, PortraitInverted, LandscapeCounterClockwise
  };
  Orientation orientation = Orientation::Portrait;
  Orientation getOrientation()const {
    return orientation;
  }
  int width = 480, height = 800, top = 0, right = 0, bottom = 0, left = 0, smallHeight = 16, titleHeight = 20;
  std::vector<Draw> draws;
  int getScreenWidth() const {
    return width;
  }
  int getScreenHeight() const {
    return height;
  }
  void getOrientedViewableTRBL(int* t, int* r, int* b, int* l) const {
    *t = top;
    *r = right;
    *b = bottom;
    *l = left;
  }
  int getLineHeight(int font) const {
    return font == 1 ? titleHeight : smallHeight;
  }
  int getTextWidth(int, const char* s, EpdFontFamily::Style = EpdFontFamily::REGULAR) const {
    return std::strlen(s)*7;
  }
  void clearScreen() {
    draws.clear();
  }
  void displayBuffer() {}
  void fillRoundedRect(int x, int y, int w, int h, int, Color::Value) {
    draws.push_back( {
      "row", "", x, y, w, h
    });
  }
  void drawRoundedRect(int x, int y, int w, int h, int, int, bool) {
    draws.push_back( {
      w>100 ? "row":"fallback", "", x, y, w, h
    });
  }
  void drawText(int f, int x, int y, const char* s, bool, EpdFontFamily::Style style = EpdFontFamily::REGULAR) {
    draws.push_back( {
      "text", s, x, y, getTextWidth(f, s, style), getLineHeight(f)
    });
  }
  void drawCenteredText(int f, int y, const char* s, bool c, EpdFontFamily::Style style = EpdFontFamily::REGULAR) {
    drawText(f, (width-getTextWidth(f, s, style))/2, y, s, c, style);
  }
  void drawIcon(const uint8_t*, int x, int y, int w, int h) {
    draws.push_back( {
      "icon", "", x, y, w, h
    });
  }
  void drawIconInverted(const uint8_t* b, int x, int y, int w, int h) {
    drawIcon(b, x, y, w, h);
  }
  void drawLine(int, int, int, int, Color::Value) {}
  std::vector<std::string> wrappedText(int, const char* s, int, int) {
    return {
      s
    };
  }
  std::string truncatedText(int f, const char* s, int w, EpdFontFamily::Style style = EpdFontFamily::REGULAR) const {
    std::string v = s;
    while (!v.empty() && getTextWidth(f, v.c_str(), style)>w)v.pop_back();
    return v;
  }
};
class MappedInputManager {
  public:  enum class Button {
    Back, Confirm, Right, Down, PageForward, Left, Up, PageBack
  };
  enum class SwipeDir {
    None, Up, Down, Left, Right
  };
  bool touch = false;
  std::set<Button> pressed;
  bool hasTouch()const {
    return touch;
  }
  bool isPressed(Button b)const {
    return pressed.count(b) != 0;
  }
  std::set<Button> released;
  SwipeDir swipe = SwipeDir::None;
  bool tapped = false;
  int x = 0, y = 0;
  bool wasReleased(Button b)const {
    return released.count(b) != 0;
  }
  SwipeDir wasSwipe()const {
    return swipe;
  }
  bool wasScreenTapped(int& tx, int& ty)const {
    tx = x;
    ty = y;
    return tapped;
  }
  void tap(int tx, int ty) {
    x = tx;
    y = ty;
    tapped = true;
  }
  struct Labels {
    const char* btn1;
    const char* btn2;
    const char* btn3;
    const char* btn4;
  };
  Labels mapLabels(const char* a, const char* b, const char* c, const char* d) {
    return {
      a, b, c, d
    };
  }
};
class Activity;
struct RenderLock {
  static inline int depth = 0;
  RenderLock() {
    ++depth;
  }
  explicit RenderLock(Activity&) {
    ++depth;
  }
  ~RenderLock() {
    --depth;
  }
  void unlock() {
    --depth;
  }
};
struct ActivityResult {};
class Activity {
  public:  GfxRenderer& renderer;
  MappedInputManager& mappedInput;
  bool finished = false;
  int childStarts = 0;
  std::function<void(const ActivityResult&)> childResult;
  int updates = 0;
  Activity(const char*, GfxRenderer& r, MappedInputManager& i):renderer(r), mappedInput(i) {}
  virtual ~Activity() = default;
  virtual void onEnter() {}
  virtual void onExit() {}
  virtual void loop() {}
  virtual void render(RenderLock && ) {}
  void requestUpdate() {
    ++updates;
  }
  void requestUpdateAndWait() {
    ++updates;
  }
  void finish() {
    finished = true;
  }
  template<class T, class F>void startActivityForResult(std::unique_ptr<T>, F callback) {
    ++childStarts;
    childResult = callback;
  }
};
class WifiSelectionActivity:public Activity {
  public:WifiSelectionActivity(GfxRenderer&r, MappedInputManager&i):Activity("wifi", r, i) {}
};
struct Metrics {
  int topPadding = 10, headerHeight = 48, buttonHintsHeight = 40, contentSidePadding = 16;
};
class UITheme {
  public:Metrics metrics;
  static inline int statusInset = 0;
  static UITheme&getInstance() {
    static UITheme v;
    return v;
  }
  const Metrics&getMetrics()const {
    return metrics;
  }
  static int getTopStatusBarInset(const GfxRenderer&) {
    return statusInset;
  }
  Rect getScreenSafeArea(const GfxRenderer&r, bool front = false, bool = false) {
    Rect v {
      0, 0, r.width, r.height
    };
    if (front) {
      switch (r.orientation) {
        case GfxRenderer::Orientation::Portrait:v.height-= metrics.buttonHintsHeight;
        break;
        case GfxRenderer::Orientation::PortraitInverted:v.y += metrics.buttonHintsHeight;
        v.height-= metrics.buttonHintsHeight;
        break;
        case GfxRenderer::Orientation::LandscapeClockwise:v.x += metrics.buttonHintsHeight;
        v.width-= metrics.buttonHintsHeight;
        break;
        case GfxRenderer::Orientation::LandscapeCounterClockwise:v.width-= metrics.buttonHintsHeight;
        break;
      }
    }
    return v;
  }
};
struct Gui {
  void drawHeader(GfxRenderer&r, Rect p, const char*, const char* = nullptr) {
    r.draws.push_back( {
      "header", "", p.x, p.y, p.width, p.height
    });
  }
  void drawButtonHints(GfxRenderer&, const char*, const char*, const char*, const char*) {}
};
inline Gui GUI;
struct Fonts {
  void releaseForNetwork(GfxRenderer&) {}
  void ensureLoaded(GfxRenderer&) {}
  void releaseRegistry() {}
};
inline Fonts sdFontSystem;
constexpr int UI_10_FONT_ID = 1, SMALL_FONT_ID = 2, UI_12_FONT_ID = 3;
constexpr int STR_BACK = 1, STR_DIR_UP = 2, STR_DIR_DOWN = 3, STR_STORE_TITLE = 4, STR_STORE_INSTALL = 5, STR_UPDATE = 6, STR_INSTALLED = 7, STR_STORE_PAGE = 8, STR_STORE_DETAILS = 9, STR_CONNECT_WIFI = 10;
inline const char*tr(int s) {
  switch (s) {
    case STR_BACK:return "Back";
    case STR_DIR_UP:return "Up";
    case STR_DIR_DOWN:return "Down";
    case STR_STORE_TITLE:return "App Store";
    case STR_STORE_INSTALL:return "Install";
    case STR_UPDATE:return "Update";
    case STR_INSTALLED:return "Installed";
    case STR_STORE_PAGE:return "Page %d of %d · %d Apps";
    case STR_STORE_DETAILS:return "Details";
  }
  return "";
}
#define LOG_ERR(...) ((void)0)
#define LOG_DBG(...) ((void)0)
#define LOG_INF(...) ((void)0)
inline uint32_t fakeMillis = 1000;
inline uint32_t millis() {
  return fakeMillis;
}
inline void delay(int) {}
constexpr int WL_CONNECTED = 1, WIFI_MODE_NULL = 0, WIFI_OFF = 0;
struct IPAddress {
  int v;
  IPAddress(int a, int b, int c, int d):v(a|b|c|d) {}
  bool operator != (const IPAddress&o)const {
    return v != o.v;
  }
};
struct Wifi {
  bool connected = true;
  int status()const {
    return connected?WL_CONNECTED:0;
  }
  IPAddress localIP()const {
    return IPAddress(1, 2, 3, 4);
  }
  int getMode() {
    return 1;
  }
  void disconnect(bool) {}
  void mode(int) {}
};
inline Wifi WiFi;
struct FakeFileData {
  std::vector<uint8_t> bytes;
  size_t readLimit = SIZE_MAX, writeLimit = SIZE_MAX;
  bool readable = true;
};
class HalFile {
  public:  FakeFileData* data = nullptr;
  size_t pos = 0;
  bool opened = false;
  static inline int openCount = 0;
  static inline bool syncOK = true, closeOK = true, corruptReads = false;
  explicit operator bool()const {
    return opened;
  }
  bool isOpen()const {
    return opened;
  }
  size_t size()const {
    return data?data->bytes.size():0;
  }
  int read() {
    uint8_t b;
    return read(&b, 1) == 1?b:-1;
  }
  int read(void* dst, size_t n) {
    if (!opened)return -1;
    n = std::min( {
      n, data->bytes.size()-pos, data->readLimit
    });
    std::memcpy(dst, data->bytes.data()+pos, n);
    if (n && corruptReads) static_cast<uint8_t*>(dst)[0] ^= 1;
    pos += n;
    return n;
  }
  size_t readBytes(char* p, size_t n) {
    return read(p, n);
  }
  size_t write(const uint8_t* src, size_t n) {
    n = std::min(n, data->writeLimit);
    data->bytes.insert(data->bytes.end(), src, src+n);
    return n;
  }
  void flush() {}
  bool sync() {
    return syncOK;
  }
  bool close() {
    if (opened)--openCount;
    opened = false;
    return closeOK;
  }
};
using FsFile = HalFile;
struct FakeStorage {
  std::map<std::string, FakeFileData> files;
  std::vector<std::string> reads, writes;
  std::vector<int> ioLockDepths;
  bool directories = true, writeOpen = true;
  size_t nextWriteLimit = SIZE_MAX;
  bool openFileForRead(const char*, const char*p, HalFile&f) {
    reads.emplace_back(p);
    ioLockDepths.push_back(RenderLock::depth);
    auto i = files.find(p);
    if (i == files.end() || !i->second.readable)return false;
    f.data = &i->second;
    f.pos = 0;
    f.opened = true;
    ++HalFile::openCount;
    return true;
  }
  bool openFileForWrite(const char*, const char*p, HalFile&f) {
    writes.emplace_back(p);
    ioLockDepths.push_back(RenderLock::depth);
    if (!writeOpen)return false;
    auto&v = files[p];
    v.bytes.clear();
    v.writeLimit = nextWriteLimit;
    f.data = &v;
    f.pos = 0;
    f.opened = true;
    ++HalFile::openCount;
    return true;
  }
  bool ensureDirectoryExists(const char*) {
    return directories;
  }
  bool remove(const char*p) {
    return files.erase(p) != 0;
  }
  bool exists(const char*p) {
    return files.count(p) != 0;
  }
  bool removeDir(const char*) {
    return true;
  }
  bool rename(const char*a, const char*b) {
    auto i = files.find(a);
    if (i == files.end())return false;
    files[b] = i->second;
    files.erase(i);
    return true;
  }
  String readFile(const char*p) {
    auto&b = files[p].bytes;
    return {
      b.begin(), b.end()
    };
  }
};
inline FakeStorage Storage;
