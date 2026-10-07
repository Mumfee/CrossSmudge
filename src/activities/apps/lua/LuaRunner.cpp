#include "activities/apps/lua/LuaRunner.h"

#include <ArduinoJson.h>
#include <HalClock.h>
#include <HalStorage.h>

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "CrossPointSettings.h"
#include "GfxRenderer.h"
#include "HalPowerManager.h"
#include "Logging.h"
#include "MappedInputManager.h"
#include "Memory.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace ink {

namespace {
// Thread-local / static pointer to active runner for static Lua C functions
static LuaRunner* s_activeRunner = nullptr;

struct FileReaderContext {
  HalFile file;
  char buffer[512];
};

static const char* sdFileReader(lua_State* /*L*/, void* ud, size_t* sz) {
  auto* ctx = static_cast<FileReaderContext*>(ud);
  if (!ctx->file || !ctx->file.available()) {
    *sz = 0;
    return nullptr;
  }
  size_t bytesRead = ctx->file.read(ctx->buffer, sizeof(ctx->buffer));
  *sz = bytesRead;
  return ctx->buffer;
}
}  // namespace

LuaRunner::LuaRunner(GfxRenderer& renderer, MappedInputManager& input, const std::string& appDir,
                     const std::string& appId)
    : renderer_(renderer), input_(input), appDir_(appDir), appId_(appId) {
  saveDir_ = "/.smudge/saved_data/" + appId_;
}

LuaRunner::~LuaRunner() { shutdown(); }

void* LuaRunner::customLuaAlloc(void* ud, void* ptr, size_t osize, size_t nsize) {
  auto* self = static_cast<LuaRunner*>(ud);
  if (nsize == 0) {
    if (ptr) {
      if (self->currentAllocatedBytes_ >= osize) {
        self->currentAllocatedBytes_ -= osize;
      } else {
        self->currentAllocatedBytes_ = 0;
      }
      free(ptr);
    }
    return nullptr;
  }

  // Check heap limit
  size_t addedBytes = (ptr == nullptr) ? nsize : (nsize > osize ? nsize - osize : 0);
#if !defined(SIMULATOR) && !defined(BOARD_HAS_PSRAM)
  // On memory-constrained devices without PSRAM (e.g. ESP32-C3):
  // Check live system heap dynamically. Maintain a 3 KB safety cushion for FreeRTOS context switches/ISRs.
  // Note: NEVER call lua_gc() from inside customLuaAlloc; Lua's lmem.c tryagain() already executes
  // an internal emergency GC safely (gcemergency=1) if this allocator returns nullptr.
  constexpr size_t kMinSystemSafetyBytes = 3 * 1024;
  uint32_t freeH = ESP.getFreeHeap();
  if (freeH < kMinSystemSafetyBytes + addedBytes ||
      self->currentAllocatedBytes_ + addedBytes > self->maxLuaHeapBytes_) {
    return nullptr;
  }
#else
  if (self->currentAllocatedBytes_ + addedBytes > self->maxLuaHeapBytes_) {
    return nullptr;
  }
#endif

  void* newPtr = realloc(ptr, nsize);
  if (newPtr) {
    if (ptr == nullptr) {
      self->currentAllocatedBytes_ += nsize;
    } else if (nsize > osize) {
      self->currentAllocatedBytes_ += (nsize - osize);
    } else {
      self->currentAllocatedBytes_ -= (osize - nsize);
    }
  }
  return newPtr;
}

bool LuaRunner::init() {
  shutdown();
  requestedExit_ = false;
  hasError_ = false;
  errorMessage_.clear();
  currentAllocatedBytes_ = 0;
  preventAutoSleep_.store(false, std::memory_order_relaxed);

#if defined(SIMULATOR_DEVICE_X3)
  maxLuaHeapBytes_ = 75 * 1024;  // 75 KB exact X3 hardware DRAM ceiling
#elif defined(SIMULATOR)
  const char* simX3Env = getenv("SMUDGE_X3_CONSTRAINTS");
  if (simX3Env && simX3Env[0] == '1') {
    maxLuaHeapBytes_ = 75 * 1024;  // 75 KB exact X3 simulation via env var
  } else {
    maxLuaHeapBytes_ = 2 * 1024 * 1024;  // 2 MB default simulator host
  }
#elif defined(BOARD_HAS_PSRAM)
  maxLuaHeapBytes_ = 2 * 1024 * 1024;  // 2 MB for PSRAM
#else
  maxLuaHeapBytes_ = 220 * 1024;  // 220 KB ceiling on C3 (governed by live free heap)
#endif

  LOG_INF("LUA", "[%s] Init with heap limit: %zu bytes", appId_.c_str(), maxLuaHeapBytes_);

  Storage.ensureDirectoryExists(saveDir_.c_str());

  L = lua_newstate(customLuaAlloc, this);
  if (!L) {
    setError("Failed to create Lua state (out of memory)");
    return false;
  }

  // Configure aggressive incremental GC for embedded microcontrollers:
  // - pause = 105: Start a new GC cycle as soon as memory grows by 5% (instead of default 200%)
  // - stepmul = 250: GC sweeps 2.5x faster than allocations so garbage doesn't accumulate
  lua_gc(L, LUA_GCINC, 105, 250, 0);

  // Open only essential libraries (base, table, string, math, os) to save 12+ KB of heap on embedded devices
  static const luaL_Reg essentialLibs[] = {{LUA_GNAME, luaopen_base},        {LUA_TABLIBNAME, luaopen_table},
                                           {LUA_STRLIBNAME, luaopen_string}, {LUA_MATHLIBNAME, luaopen_math},
                                           {LUA_OSLIBNAME, luaopen_os},      {nullptr, nullptr}};
  for (const luaL_Reg* lib = essentialLibs; lib->func; lib++) {
    luaL_requiref(L, lib->name, lib->func, 1);
    lua_pop(L, 1);
  }
  registerSmudgeApi();

  // Check if application manifest declares prevent_sleep
  std::string manifestPath = appDir_ + "/manifest.json";
  if (Storage.exists(manifestPath.c_str())) {
    String jsonStr = Storage.readFile(manifestPath.c_str());
    JsonDocument doc;
    if (!deserializeJson(doc, jsonStr)) {
      if (doc["prevent_sleep"].is<bool>() && doc["prevent_sleep"].as<bool>()) {
        preventAutoSleep_.store(true, std::memory_order_relaxed);
      }
      if (doc["min_api"].is<int>()) {
        int reqApi = doc["min_api"].as<int>();
        if (reqApi > LUA_API_VERSION) {
          char err[128];
          snprintf(err, sizeof(err), "Requires API v%d (firmware has v%d).\nPlease update device firmware.", reqApi,
                   LUA_API_VERSION);
          setError(err);
          return false;
        }
      }
    }
  }

  return true;
}

void LuaRunner::shutdown() {
  if (L) {
    lua_close(L);
    L = nullptr;
  }
  if (s_activeRunner == this) {
    s_activeRunner = nullptr;
  }
  currentAllocatedBytes_ = 0;
  preventAutoSleep_.store(false, std::memory_order_relaxed);
}

void LuaRunner::setError(const char* msg) {
  hasError_ = true;
  errorMessage_ = (msg ? msg : "Unknown Lua error");
  LOG_ERR("LUA", "[%s] %s", appId_.c_str(), errorMessage_.c_str());
}

bool LuaRunner::loadScript(const std::string& scriptPath) {
  if (!L && !init()) {
    return false;
  }

  FileReaderContext ctx;
  if (!Storage.openFileForRead("LUA", scriptPath.c_str(), ctx.file)) {
    std::string err = "Failed to read script file: " + scriptPath;
    setError(err.c_str());
    return false;
  }

  s_activeRunner = this;

  int status = lua_load(L, sdFileReader, &ctx, scriptPath.c_str(), nullptr);
  ctx.file.close();

  if (status != LUA_OK) {
    const char* err = lua_tostring(L, -1);
    setError(err);
    lua_pop(L, 1);
    return false;
  }

  status = lua_pcall(L, 0, 0, 0);
  if (status != LUA_OK) {
    const char* err = lua_tostring(L, -1);
    setError(err);
    lua_pop(L, 1);
    return false;
  }

  lua_gc(L, LUA_GCCOLLECT, 0);
  return true;
}

void LuaRunner::onInit() {
  if (!L || hasError_) return;
  s_activeRunner = this;
  lua_getglobal(L, "on_init");
  if (lua_isfunction(L, -1)) {
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
      setError(lua_tostring(L, -1));
      lua_pop(L, 1);
    }
  } else {
    lua_pop(L, 1);
  }
}

void LuaRunner::onUpdate() {
  if (!L || hasError_) return;
  s_activeRunner = this;

  // Run an incremental GC step to collect ephemeral garbage smoothly
  lua_gc(L, LUA_GCSTEP, 0);

  lua_getglobal(L, "on_update");
  if (lua_isfunction(L, -1)) {
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
      setError(lua_tostring(L, -1));
      lua_pop(L, 1);
    }
  } else {
    lua_pop(L, 1);
  }
}

void LuaRunner::onDraw() {
  if (!L || hasError_) return;
  s_activeRunner = this;
  lua_getglobal(L, "on_draw");
  if (lua_isfunction(L, -1)) {
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
      setError(lua_tostring(L, -1));
      lua_pop(L, 1);
    }
  } else {
    lua_pop(L, 1);
  }
  lua_gc(L, LUA_GCSTEP, 50);
}

void LuaRunner::onButton(const char* buttonName, bool isPressed) {
  if (!L || hasError_) return;
  s_activeRunner = this;
  lua_getglobal(L, "on_button");
  if (lua_isfunction(L, -1)) {
    lua_pushstring(L, buttonName);
    lua_pushboolean(L, isPressed);
    if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
      setError(lua_tostring(L, -1));
      lua_pop(L, 1);
    }
  } else {
    lua_pop(L, 1);
  }
  lua_gc(L, LUA_GCSTEP, 50);
}

void LuaRunner::onTouch(int x, int y, const char* eventType) {
  if (!L || hasError_) return;
  s_activeRunner = this;
  lua_getglobal(L, "on_touch");
  if (lua_isfunction(L, -1)) {
    lua_pushinteger(L, x);
    lua_pushinteger(L, y);
    lua_pushstring(L, eventType ? eventType : "tap");
    if (lua_pcall(L, 3, 0, 0) != LUA_OK) {
      setError(lua_tostring(L, -1));
      lua_pop(L, 1);
    }
  } else {
    lua_pop(L, 1);
  }
  lua_gc(L, LUA_GCSTEP, 50);
}

void LuaRunner::onTap(int x, int y) {
  if (!L || hasError_) return;
  s_activeRunner = this;
  lua_getglobal(L, "on_tap");
  if (lua_isfunction(L, -1)) {
    lua_pushinteger(L, x);
    lua_pushinteger(L, y);
    if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
      setError(lua_tostring(L, -1));
      lua_pop(L, 1);
    }
  } else {
    lua_pop(L, 1);
  }
  lua_gc(L, LUA_GCSTEP, 50);
}

void LuaRunner::onSwipe(const char* dir) {
  if (!L || hasError_) return;
  s_activeRunner = this;
  lua_getglobal(L, "on_swipe");
  if (lua_isfunction(L, -1)) {
    lua_pushstring(L, dir ? dir : "none");
    if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
      setError(lua_tostring(L, -1));
      lua_pop(L, 1);
    }
  } else {
    lua_pop(L, 1);
  }
}

void LuaRunner::onExit() {
  if (!L || hasError_) return;
  s_activeRunner = this;
  lua_getglobal(L, "on_exit");
  if (lua_isfunction(L, -1)) {
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
      LOG_DBG("LUA", "[%s] on_exit failed: %s", appId_.c_str(), lua_tostring(L, -1));
      lua_pop(L, 1);
    }
  } else {
    lua_pop(L, 1);
  }
}

void LuaRunner::registerSmudgeApi() {
  lua_newtable(L);  // table 'smudge' (and 'ink')

  lua_pushcfunction(L, l_clear);
  lua_setfield(L, -2, "clear");

  lua_pushcfunction(L, l_refresh);
  lua_setfield(L, -2, "refresh");

  lua_pushcfunction(L, l_text);
  lua_setfield(L, -2, "text");

  lua_pushcfunction(L, l_centeredText);
  lua_setfield(L, -2, "centered_text");

  lua_pushcfunction(L, l_textWidth);
  lua_setfield(L, -2, "text_width");

  lua_pushcfunction(L, l_wrappedText);
  lua_setfield(L, -2, "wrapped_text");

  lua_pushcfunction(L, l_lineHeight);
  lua_setfield(L, -2, "line_height");

  lua_pushcfunction(L, l_pixel);
  lua_setfield(L, -2, "pixel");

  lua_pushcfunction(L, l_line);
  lua_setfield(L, -2, "line");

  lua_pushcfunction(L, l_thickLine);
  lua_setfield(L, -2, "thick_line");

  lua_pushcfunction(L, l_rect);
  lua_setfield(L, -2, "rect");

  lua_pushcfunction(L, l_rectDither);
  lua_setfield(L, -2, "rect_dither");

  lua_pushcfunction(L, l_roundedRect);
  lua_setfield(L, -2, "rounded_rect");

  lua_pushcfunction(L, l_circle);
  lua_setfield(L, -2, "circle");

  lua_pushcfunction(L, l_heart);
  lua_setfield(L, -2, "heart");

  lua_pushcfunction(L, l_drawSprite);
  lua_setfield(L, -2, "draw_sprite");
  lua_pushcfunction(L, l_drawSprite);
  lua_setfield(L, -2, "sprite");
  lua_pushcfunction(L, l_drawSprite);
  lua_setfield(L, -2, "bitmap");

  lua_pushcfunction(L, l_getBounds);
  lua_setfield(L, -2, "get_bounds");

  lua_pushcfunction(L, l_getMetrics);
  lua_setfield(L, -2, "get_metrics");

  lua_pushcfunction(L, l_buttonHints);
  lua_setfield(L, -2, "button_hints");

  lua_pushcfunction(L, l_header);
  lua_setfield(L, -2, "header");

  lua_pushcfunction(L, l_save);
  lua_setfield(L, -2, "save");

  lua_pushcfunction(L, l_load);
  lua_setfield(L, -2, "load");

  lua_pushcfunction(L, l_readFile);
  lua_setfield(L, -2, "read_file");

  lua_pushcfunction(L, l_findSection);
  lua_setfield(L, -2, "find_section");
  lua_pushcfunction(L, l_findSection);
  lua_setfield(L, -2, "find_file_section");
  lua_pushcfunction(L, l_findSection);
  lua_setfield(L, -2, "read_lines");

  lua_pushcfunction(L, l_fileExists);
  lua_setfield(L, -2, "file_exists");

  lua_pushcfunction(L, l_exit);
  lua_setfield(L, -2, "exit");

  lua_pushcfunction(L, l_log);
  lua_setfield(L, -2, "log");

  lua_pushcfunction(L, l_time);
  lua_setfield(L, -2, "time");

  lua_pushcfunction(L, l_getDate);
  lua_setfield(L, -2, "get_date");
  lua_pushcfunction(L, l_getDate);
  lua_setfield(L, -2, "date");

  lua_pushcfunction(L, l_random);
  lua_setfield(L, -2, "random");

  lua_pushcfunction(L, l_millis);
  lua_setfield(L, -2, "millis");
  lua_pushcfunction(L, l_millis);
  lua_setfield(L, -2, "get_time_ms");

  lua_pushcfunction(L, l_requestUpdate);
  lua_setfield(L, -2, "request_update");
  lua_pushcfunction(L, l_requestUpdate);
  lua_setfield(L, -2, "redraw");

  lua_pushcfunction(L, l_isButtonDown);
  lua_setfield(L, -2, "is_button_down");

  lua_pushcfunction(L, l_fullRefresh);
  lua_setfield(L, -2, "full_refresh");

  lua_pushcfunction(L, l_invertRect);
  lua_setfield(L, -2, "invert_rect");

  lua_pushcfunction(L, l_invertScreen);
  lua_setfield(L, -2, "invert_screen");
  lua_pushcfunction(L, l_invertScreen);
  lua_setfield(L, -2, "invert");

  lua_pushcfunction(L, l_triangle);
  lua_setfield(L, -2, "triangle");

  lua_pushcfunction(L, l_setOrientation);
  lua_setfield(L, -2, "set_orientation");

  lua_pushcfunction(L, l_getOrientation);
  lua_setfield(L, -2, "get_orientation");

  lua_pushcfunction(L, l_preventSleep);
  lua_setfield(L, -2, "prevent_sleep");
  lua_pushcfunction(L, l_preventSleep);
  lua_setfield(L, -2, "keep_awake");

  lua_pushcfunction(L, l_isPreventSleep);
  lua_setfield(L, -2, "is_prevent_sleep");
  lua_pushcfunction(L, l_isPreventSleep);
  lua_setfield(L, -2, "is_keep_awake");

  lua_pushcfunction(L, l_inRect);
  lua_setfield(L, -2, "in_rect");
  lua_pushcfunction(L, l_inRect);
  lua_setfield(L, -2, "point_in_rect");

  lua_pushcfunction(L, l_hasTouch);
  lua_setfield(L, -2, "has_touch");
  lua_pushcfunction(L, l_hasTouch);
  lua_setfield(L, -2, "has_touchscreen");

  lua_pushcfunction(L, l_getBattery);
  lua_setfield(L, -2, "get_battery");
  lua_pushcfunction(L, l_getBattery);
  lua_setfield(L, -2, "battery");

  lua_pushcfunction(L, l_getDevice);
  lua_setfield(L, -2, "get_device");

  lua_pushcfunction(L, l_writeFile);
  lua_setfield(L, -2, "write_file");

  lua_pushcfunction(L, l_listFiles);
  lua_setfield(L, -2, "list_files");

  lua_pushcfunction(L, l_deleteFile);
  lua_setfield(L, -2, "delete_file");

  lua_pushcfunction(L, l_popup);
  lua_setfield(L, -2, "popup");
  lua_pushcfunction(L, l_popup);
  lua_setfield(L, -2, "draw_popup");

  lua_pushcfunction(L, l_getMemory);
  lua_setfield(L, -2, "get_memory");
  lua_pushcfunction(L, l_getMemory);
  lua_setfield(L, -2, "memory");

  lua_pushcfunction(L, l_dofile);
  lua_setfield(L, -2, "dofile");

  lua_pushcfunction(L, l_getVersion);
  lua_setfield(L, -2, "version");
  lua_pushcfunction(L, l_getVersion);
  lua_setfield(L, -2, "get_version");

  lua_pushinteger(L, LUA_API_VERSION);
  lua_setfield(L, -2, "api_version");
  lua_pushcfunction(L, l_getApiVersion);
  lua_setfield(L, -2, "get_api_version");

  lua_pushcfunction(L, l_hasFeature);
  lua_setfield(L, -2, "has_feature");

  lua_pushvalue(L, -1);
  lua_setglobal(L, "smudge");
  lua_setglobal(L, "ink");  // backwards compatibility alias

  lua_register(L, "dofile", l_dofile);
}

// ----------------------------------------------------
// C-to-Lua Bridge Implementations
// ----------------------------------------------------

static inline int getInt(lua_State* L, int idx) { return static_cast<int>(std::round(luaL_checknumber(L, idx))); }

static inline int getOptInt(lua_State* L, int idx, int def) {
  return static_cast<int>(std::round(luaL_optnumber(L, idx, def)));
}

static int resolveFont(lua_State* L, int idx) {
  if (lua_type(L, idx) == LUA_TSTRING) {
    const char* str = lua_tostring(L, idx);
    if (strcmp(str, "small") == 0 || strcmp(str, "small_font") == 0 || strcmp(str, "8") == 0) return SMALL_FONT_ID;
    if (strcmp(str, "ui10") == 0 || strcmp(str, "ui_10") == 0 || strcmp(str, "10") == 0) return UI_10_FONT_ID;
    if (strcmp(str, "ui12") == 0 || strcmp(str, "ui_12") == 0 || strcmp(str, "12") == 0) return UI_12_FONT_ID;
    if (strcmp(str, "lexend10") == 0 || strcmp(str, "lexend_10") == 0) return LEXENDDECA_10_FONT_ID;
    if (strcmp(str, "lexend12") == 0 || strcmp(str, "lexend_12") == 0) return LEXENDDECA_12_FONT_ID;
    if (strcmp(str, "lexend14") == 0 || strcmp(str, "lexend_14") == 0 || strcmp(str, "14") == 0)
      return LEXENDDECA_14_FONT_ID;
    if (strcmp(str, "lexend16") == 0 || strcmp(str, "lexend_16") == 0 || strcmp(str, "16") == 0)
      return LEXENDDECA_16_FONT_ID;
    if (strcmp(str, "bitter10") == 0 || strcmp(str, "bitter_10") == 0) return BITTER_10_FONT_ID;
    if (strcmp(str, "bitter12") == 0 || strcmp(str, "bitter_12") == 0) return BITTER_12_FONT_ID;
    if (strcmp(str, "bitter14") == 0 || strcmp(str, "bitter_14") == 0) return BITTER_14_FONT_ID;
    if (strcmp(str, "bitter16") == 0 || strcmp(str, "bitter_16") == 0) return BITTER_16_FONT_ID;
    if (strcmp(str, "reader") == 0) return SETTINGS.getReaderFontId();
  }
  int size = getOptInt(L, idx, 10);
  if (size == 0 || size >= 18) return SETTINGS.getReaderFontId();
  if (size <= 8) return SMALL_FONT_ID;
  if (size <= 10) return UI_10_FONT_ID;
  if (size <= 12) return UI_12_FONT_ID;
  if (size <= 14) return LEXENDDECA_14_FONT_ID;
  return LEXENDDECA_16_FONT_ID;
}

static EpdFontFamily::Style resolveStyle(lua_State* L, int idx) {
  if (lua_isboolean(L, idx)) {
    return lua_toboolean(L, idx) ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;
  }
  if (lua_type(L, idx) == LUA_TSTRING) {
    const char* str = lua_tostring(L, idx);
    if (strcmp(str, "bold") == 0) return EpdFontFamily::BOLD;
    if (strcmp(str, "italic") == 0) return EpdFontFamily::ITALIC;
    if (strcmp(str, "bold_italic") == 0 || strcmp(str, "bolditalic") == 0) return EpdFontFamily::BOLD_ITALIC;
    if (strcmp(str, "regular") == 0) return EpdFontFamily::REGULAR;
  }
  return EpdFontFamily::REGULAR;
}

int LuaRunner::l_dofile(lua_State* L) {
  if (!s_activeRunner) return 0;
  const char* filename = luaL_checkstring(L, 1);
  std::string fullPath;
  if (filename[0] == '/') {
    fullPath = filename;
  } else {
    fullPath = s_activeRunner->appDir_ + "/" + filename;
  }

  FileReaderContext ctx;
  if (!Storage.openFileForRead("LUA", fullPath.c_str(), ctx.file)) {
    return luaL_error(L, "cannot open %s", filename);
  }

  // Collect dead objects before compiling/loading new file to free heap
  lua_gc(L, LUA_GCCOLLECT, 0);

  int status = lua_load(L, sdFileReader, &ctx, fullPath.c_str(), nullptr);
  ctx.file.close();

  if (status != LUA_OK) {
    return lua_error(L);
  }

  int base = lua_gettop(L) - 1;
  lua_call(L, 0, LUA_MULTRET);
  lua_gc(L, LUA_GCCOLLECT, 0);
  return lua_gettop(L) - base;
}

int LuaRunner::l_clear(lua_State*) {
  if (s_activeRunner) {
    s_activeRunner->renderer_.clearScreen();
  }
  return 0;
}

int LuaRunner::l_refresh(lua_State* L) {
  if (s_activeRunner) {
    bool full = false;
    if (lua_gettop(L) >= 1) {
      if (lua_isboolean(L, 1)) {
        full = lua_toboolean(L, 1);
      } else if (lua_type(L, 1) == LUA_TSTRING) {
        full = (strcmp(lua_tostring(L, 1), "full") == 0);
      }
    }
    s_activeRunner->renderer_.displayBuffer(full ? HalDisplay::RefreshMode::FULL_REFRESH
                                                 : HalDisplay::RefreshMode::FAST_REFRESH);
  }
  return 0;
}

int LuaRunner::l_fullRefresh(lua_State*) {
  if (s_activeRunner) {
    s_activeRunner->fullRefreshRequested_ = true;
    s_activeRunner->redrawRequested_ = true;
  }
  return 0;
}

int LuaRunner::l_text(lua_State* L) {
  if (!s_activeRunner) return 0;
  int x = getInt(L, 1);
  int y = getInt(L, 2);
  const char* text = luaL_checkstring(L, 3);
  int fontId = resolveFont(L, 4);
  auto style = resolveStyle(L, 5);
  const char* align = luaL_optstring(L, 6, "left");
  bool color = true;
  if (lua_gettop(L) >= 7) {
    color = lua_toboolean(L, 7);
  }

  if (strcmp(align, "center") == 0) {
    int w = s_activeRunner->renderer_.getTextWidth(fontId, text, style);
    x -= w / 2;
  } else if (strcmp(align, "right") == 0) {
    int w = s_activeRunner->renderer_.getTextWidth(fontId, text, style);
    x -= w;
  }

  s_activeRunner->renderer_.drawText(fontId, x, y, text, color, style);
  return 0;
}

int LuaRunner::l_centeredText(lua_State* L) {
  if (!s_activeRunner) return 0;
  int y = getInt(L, 1);
  const char* text = luaL_checkstring(L, 2);
  int fontId = resolveFont(L, 3);
  auto style = resolveStyle(L, 4);
  bool color = true;
  if (lua_gettop(L) >= 5) {
    color = lua_toboolean(L, 5);
  }

  s_activeRunner->renderer_.drawCenteredText(fontId, y, text, color, style);
  return 0;
}

int LuaRunner::l_textWidth(lua_State* L) {
  if (!s_activeRunner) {
    lua_pushinteger(L, 0);
    return 1;
  }
  const char* text = luaL_checkstring(L, 1);
  int fontId = resolveFont(L, 2);
  auto style = resolveStyle(L, 3);
  int w = s_activeRunner->renderer_.getTextWidth(fontId, text, style);
  lua_pushinteger(L, w);
  return 1;
}

int LuaRunner::l_wrappedText(lua_State* L) {
  if (!s_activeRunner) {
    lua_newtable(L);
    return 1;
  }
  const char* text = luaL_checkstring(L, 1);
  int maxWidth = getInt(L, 2);
  int fontId = resolveFont(L, 3);
  auto style = resolveStyle(L, 4);
  int maxLines = (lua_gettop(L) >= 5) ? getInt(L, 5) : 100;

  // Fast-path: single line without newlines that fits maxWidth
  if (std::strchr(text, '\n') == nullptr && s_activeRunner->renderer_.getTextWidth(fontId, text, style) <= maxWidth) {
    lua_createtable(L, 1, 2);
    lua_pushstring(L, text);
    lua_rawseti(L, -2, 1);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "lines");
    lua_pushinteger(L, static_cast<lua_Integer>(s_activeRunner->renderer_.getLineHeight(fontId)));
    lua_setfield(L, -2, "total_height");
    return 1;
  }

  auto lines = s_activeRunner->renderer_.wrappedText(fontId, text, maxWidth, maxLines, style);
  lua_createtable(L, lines.size(), 2);
  for (size_t i = 0; i < lines.size(); ++i) {
    lua_pushstring(L, lines[i].c_str());
    lua_rawseti(L, -2, i + 1);
  }
  // Attach .lines to itself so both ipairs(res) and ipairs(res.lines) work
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "lines");
  lua_pushinteger(L, static_cast<lua_Integer>(lines.size() * s_activeRunner->renderer_.getLineHeight(fontId)));
  lua_setfield(L, -2, "total_height");
  return 1;
}

int LuaRunner::l_lineHeight(lua_State* L) {
  if (!s_activeRunner) {
    lua_pushinteger(L, 20);
    return 1;
  }
  int fontId = resolveFont(L, 1);
  lua_pushinteger(L, s_activeRunner->renderer_.getLineHeight(fontId));
  return 1;
}

int LuaRunner::l_line(lua_State* L) {
  if (!s_activeRunner) return 0;
  int x1 = getInt(L, 1);
  int y1 = getInt(L, 2);
  int x2 = getInt(L, 3);
  int y2 = getInt(L, 4);
  int thickness = 1;
  bool color = true;

  if (lua_gettop(L) >= 5) {
    if (lua_isboolean(L, 5)) {
      color = lua_toboolean(L, 5);
      if (lua_gettop(L) >= 6) thickness = getInt(L, 6);
    } else {
      thickness = getInt(L, 5);
      if (lua_gettop(L) >= 6) {
        color = lua_isboolean(L, 6) ? lua_toboolean(L, 6) : (lua_tointeger(L, 6) != 0);
      }
    }
  }

  s_activeRunner->renderer_.drawLine(x1, y1, x2, y2, thickness, color);
  return 0;
}

int LuaRunner::l_thickLine(lua_State* L) {
  if (!s_activeRunner) return 0;
  int x1 = getInt(L, 1);
  int y1 = getInt(L, 2);
  int x2 = getInt(L, 3);
  int y2 = getInt(L, 4);
  int thickness = getOptInt(L, 5, 2);
  s_activeRunner->renderer_.drawLine(x1, y1, x2, y2, thickness, true);
  return 0;
}

int LuaRunner::l_pixel(lua_State* L) {
  if (!s_activeRunner) return 0;
  int x = getInt(L, 1);
  int y = getInt(L, 2);
  bool state = true;
  if (lua_gettop(L) >= 3) {
    if (lua_isboolean(L, 3)) {
      state = lua_toboolean(L, 3);
    } else {
      state = (lua_tointeger(L, 3) != 0);
    }
  }
  s_activeRunner->renderer_.drawPixel(x, y, state);
  return 0;
}

int LuaRunner::l_rect(lua_State* L) {
  if (!s_activeRunner) return 0;
  int x = getInt(L, 1);
  int y = getInt(L, 2);
  int w = getInt(L, 3);
  int h = getInt(L, 4);
  bool fill = (lua_gettop(L) >= 5) ? lua_toboolean(L, 5) : false;
  int thickness = 1;
  bool color = true;

  if (lua_gettop(L) >= 6) {
    if (fill) {
      color = lua_isboolean(L, 6) ? lua_toboolean(L, 6) : (lua_tointeger(L, 6) != 0);
    } else if (lua_isboolean(L, 6)) {
      color = lua_toboolean(L, 6);
      if (lua_gettop(L) >= 7) thickness = getInt(L, 7);
    } else {
      thickness = getInt(L, 6);
      if (lua_gettop(L) >= 7) {
        color = lua_isboolean(L, 7) ? lua_toboolean(L, 7) : (lua_tointeger(L, 7) != 0);
      }
    }
  }

  if (fill) {
    s_activeRunner->renderer_.fillRect(x, y, w, h, color);
  } else {
    s_activeRunner->renderer_.drawRect(x, y, w, h, thickness, color);
  }
  return 0;
}

int LuaRunner::l_rectDither(lua_State* L) {
  if (!s_activeRunner) return 0;
  int x = getInt(L, 1);
  int y = getInt(L, 2);
  int w = getInt(L, 3);
  int h = getInt(L, 4);
  bool isDark = lua_toboolean(L, 5);
  s_activeRunner->renderer_.fillRectDither(x, y, w, h, isDark ? Color::DarkGray : Color::LightGray);
  return 0;
}

int LuaRunner::l_roundedRect(lua_State* L) {
  if (!s_activeRunner) return 0;
  int x = getInt(L, 1);
  int y = getInt(L, 2);
  int w = getInt(L, 3);
  int h = getInt(L, 4);
  int r = getInt(L, 5);
  bool fill = (lua_gettop(L) >= 6) ? lua_toboolean(L, 6) : false;
  int stroke = 1;
  bool color = true;
  if (lua_gettop(L) >= 7) {
    if (lua_isboolean(L, 7)) {
      color = lua_toboolean(L, 7);
      if (lua_gettop(L) >= 8) stroke = getInt(L, 8);
    } else {
      stroke = getInt(L, 7);
      if (lua_gettop(L) >= 8) {
        color = lua_isboolean(L, 8) ? lua_toboolean(L, 8) : (lua_tointeger(L, 8) != 0);
      }
    }
  }

  if (fill) {
    s_activeRunner->renderer_.fillRoundedRect(x, y, w, h, r, color ? Color::Black : Color::White);
  } else {
    s_activeRunner->renderer_.drawRoundedRect(x, y, w, h, stroke, r, color);
  }
  return 0;
}

int LuaRunner::l_circle(lua_State* L) {
  if (!s_activeRunner) return 0;
  int x = getInt(L, 1);
  int y = getInt(L, 2);
  int r = getInt(L, 3);
  bool fill = (lua_gettop(L) >= 4) ? lua_toboolean(L, 4) : false;
  int thickness = 1;
  bool color = true;

  if (lua_gettop(L) >= 5) {
    if (lua_isboolean(L, 5)) {
      color = lua_toboolean(L, 5);
      if (lua_gettop(L) >= 6) thickness = getInt(L, 6);
    } else {
      thickness = getInt(L, 5);
      if (lua_gettop(L) >= 6) {
        color = lua_isboolean(L, 6) ? lua_toboolean(L, 6) : (lua_tointeger(L, 6) != 0);
      }
    }
  }

  if (fill) {
    s_activeRunner->renderer_.fillRoundedRect(x - r, y - r, 2 * r, 2 * r, r, color ? Color::Black : Color::White);
  } else {
    s_activeRunner->renderer_.drawRoundedRect(x - r, y - r, 2 * r, 2 * r, thickness, r, color);
  }
  return 0;
}

int LuaRunner::l_heart(lua_State* L) {
  if (!s_activeRunner) return 0;
  int cx = getInt(L, 1);
  int cy = getInt(L, 2);
  int size = getOptInt(L, 3, 165);
  int thickness = getOptInt(L, 4, 5);
  bool fillDither = lua_isboolean(L, 5) ? lua_toboolean(L, 5) : true;

  const int numSamples = 360;
  struct Point {
    int x, y;
  };
  std::vector<Point> points;
  points.reserve(numSamples + 1);

  int minY = 10000, maxY = -10000;

  for (int i = 0; i < numSamples; ++i) {
    const float t = (2.0f * M_PI * i) / numSamples;
    const float heartX = 16.0f * std::pow(std::sin(t), 3);
    const float heartY =
        -(13.0f * std::cos(t) - 5.0f * std::cos(2.0f * t) - 2.0f * std::cos(3.0f * t) - std::cos(4.0f * t));

    const int px = cx + static_cast<int>((heartX / 16.0f) * size);
    const int py = cy + static_cast<int>((heartY / 16.0f) * size);

    points.push_back({px, py});

    if (py < minY) minY = py;
    if (py > maxY) maxY = py;
  }

  if (fillDither) {
    for (int y = minY; y <= maxY; ++y) {
      std::vector<int> nodeX;
      size_t j = points.size() - 1;
      for (size_t i = 0; i < points.size(); ++i) {
        if ((points[i].y < y && points[j].y >= y) || (points[j].y < y && points[i].y >= y)) {
          int x = points[i].x + (y - points[i].y) * (points[j].x - points[i].x) / (points[j].y - points[i].y);
          nodeX.push_back(x);
        }
        j = i;
      }

      std::sort(nodeX.begin(), nodeX.end());

      for (size_t k = 0; k + 1 < nodeX.size(); k += 2) {
        for (int x = nodeX[k]; x <= nodeX[k + 1]; ++x) {
          if (x % 2 == 0 && y % 2 == 0) {  // 25% density (lighter gray)
            s_activeRunner->renderer_.drawPixel(x, y, true);
          }
        }
      }
    }
  }

  const int halfThick = thickness / 2;
  for (size_t i = 0; i < points.size(); ++i) {
    size_t nextIdx = (i + 1) % points.size();
    for (int dx = -halfThick; dx <= halfThick; ++dx) {
      for (int dy = -halfThick; dy <= halfThick; ++dy) {
        s_activeRunner->renderer_.drawLine(points[i].x + dx, points[i].y + dy, points[nextIdx].x + dx,
                                           points[nextIdx].y + dy, true);
      }
    }
  }
  return 0;
}

int LuaRunner::l_drawSprite(lua_State* L) {
  if (!s_activeRunner) return 0;
  int x = getInt(L, 1);
  int y = getInt(L, 2);
  int w = getInt(L, 3);
  int h = getInt(L, 4);
  if (w <= 0 || h <= 0) return 0;

  bool inverted = (lua_gettop(L) >= 6) ? lua_toboolean(L, 6) : false;
  bool dither = (lua_gettop(L) >= 7) ? lua_toboolean(L, 7) : false;

  size_t stride = (static_cast<size_t>(w) + 7) / 8;
  size_t expectedBytes = stride * static_cast<size_t>(h);

  const uint8_t* spriteData = nullptr;
  std::unique_ptr<uint8_t[]> loadedBuffer;

  if (lua_isstring(L, 5)) {
    size_t strLen = 0;
    const char* strData = lua_tolstring(L, 5, &strLen);

    if (strLen >= expectedBytes) {
      spriteData = reinterpret_cast<const uint8_t*>(strData);
    } else if (strLen > 0) {
      std::string fullPath = s_activeRunner->appDir_ + "/" + strData;
      if (Storage.exists(fullPath.c_str())) {
        HalFile file = Storage.open(fullPath.c_str());
        if (file) {
          size_t fileSize = file.size();
          if (fileSize >= expectedBytes) {
            loadedBuffer = makeUniqueNoThrow<uint8_t[]>(expectedBytes);
            if (loadedBuffer) {
              size_t bytesRead = file.read(loadedBuffer.get(), expectedBytes);
              if (bytesRead == expectedBytes) {
                spriteData = loadedBuffer.get();
              }
            }
          }
          file.close();
        }
      }
    }
  } else if (lua_istable(L, 5)) {
    loadedBuffer = makeUniqueNoThrow<uint8_t[]>(expectedBytes);
    if (loadedBuffer) {
      for (size_t i = 0; i < expectedBytes; ++i) {
        lua_rawgeti(L, 5, static_cast<int>(i + 1));
        loadedBuffer[i] = static_cast<uint8_t>(lua_tointeger(L, -1));
        lua_pop(L, 1);
      }
      spriteData = loadedBuffer.get();
    }
  }

  if (!spriteData) {
    LOG_DBG("LUA", "[%s] draw_sprite: failed to load %dx%d sprite data", s_activeRunner->appId_.c_str(), w, h);
    return 0;
  }

  auto& renderer = s_activeRunner->renderer_;

  for (int cy = 0; cy < h; ++cy) {
    for (int cx = 0; cx < w; ++cx) {
      size_t byteIdx = (cy * stride) + (cx / 8);
      int bitIdx = 7 - (cx % 8);
      bool isBlack = ((spriteData[byteIdx] >> bitIdx) & 1) == 0;
      if (isBlack) {
        if (dither) {
          renderer.fillRectDither(x + cx, y + cy, 1, 1, Color::LightGray);
        } else if (inverted) {
          renderer.drawPixel(x + cx, y + cy, false);
        } else {
          renderer.drawPixel(x + cx, y + cy, true);
        }
      }
    }
  }

  return 0;
}

int LuaRunner::l_getMetrics(lua_State* L) {
  const auto& m = UITheme::getInstance().getMetrics();
  lua_newtable(L);
  lua_pushinteger(L, m.topPadding);
  lua_setfield(L, -2, "top_padding");
  lua_pushinteger(L, m.headerHeight);
  lua_setfield(L, -2, "header_height");
  lua_pushinteger(L, m.buttonHintsHeight);
  lua_setfield(L, -2, "button_hints_height");
  lua_pushinteger(L, m.contentSidePadding);
  lua_setfield(L, -2, "content_side_padding");
  return 1;
}

int LuaRunner::l_getBounds(lua_State* L) {
  if (!s_activeRunner) {
    lua_pushinteger(L, 480);
    lua_pushinteger(L, 800);
    return 2;
  }
  lua_pushinteger(L, s_activeRunner->renderer_.getScreenWidth());
  lua_pushinteger(L, s_activeRunner->renderer_.getScreenHeight());
  return 2;
}

int LuaRunner::l_buttonHints(lua_State* L) {
  if (!s_activeRunner) return 0;
  if (s_activeRunner->renderer_.getOrientation() == GfxRenderer::Orientation::LandscapeClockwise ||
      s_activeRunner->renderer_.getOrientation() == GfxRenderer::Orientation::LandscapeCounterClockwise) {
    return 0;
  }
  const char* b1 = luaL_optstring(L, 1, nullptr);
  const char* b2 = luaL_optstring(L, 2, nullptr);
  const char* b3 = luaL_optstring(L, 3, nullptr);
  const char* b4 = luaL_optstring(L, 4, nullptr);

  const auto labels = s_activeRunner->input_.mapLabels(b1 ? b1 : "", b2 ? b2 : "", b3 ? b3 : "", b4 ? b4 : "");
  GUI.drawButtonHints(s_activeRunner->renderer_, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  return 0;
}

int LuaRunner::l_header(lua_State* L) {
  if (!s_activeRunner) return 0;
  const char* title = luaL_checkstring(L, 1);
  const char* subtitle = luaL_optstring(L, 2, nullptr);

  const auto& m = UITheme::getInstance().getMetrics();
  int w = s_activeRunner->renderer_.getScreenWidth();
  int x = 0;
  if (s_activeRunner->renderer_.getOrientation() == GfxRenderer::Orientation::LandscapeClockwise) {
    x = 54;
    w -= 54;
  }
  GUI.drawHeader(s_activeRunner->renderer_, Rect{x, m.topPadding, w, m.headerHeight}, title, subtitle);
  return 0;
}

int LuaRunner::l_save(lua_State* L) {
  if (!s_activeRunner) return 0;
  const char* key = luaL_checkstring(L, 1);
  const char* val = luaL_checkstring(L, 2);

  std::string filePath = s_activeRunner->saveDir_ + "/" + key + ".dat";
  Storage.writeFile(filePath.c_str(), String(val));
  return 0;
}

int LuaRunner::l_load(lua_State* L) {
  if (!s_activeRunner) {
    lua_pushnil(L);
    return 1;
  }
  const char* key = luaL_checkstring(L, 1);
  std::string filePath = s_activeRunner->saveDir_ + "/" + key + ".dat";

  if (Storage.exists(filePath.c_str())) {
    String val = Storage.readFile(filePath.c_str());
    lua_pushstring(L, val.c_str());
  } else if (lua_gettop(L) >= 2) {
    lua_pushvalue(L, 2);  // Return default argument
  } else {
    lua_pushnil(L);
  }
  return 1;
}

int LuaRunner::l_readFile(lua_State* L) {
  if (!s_activeRunner) {
    lua_pushnil(L);
    return 1;
  }
  const char* path = luaL_checkstring(L, 1);
  size_t maxBytes = (lua_gettop(L) >= 2 && !lua_isnil(L, 2)) ? static_cast<size_t>(lua_tointeger(L, 2)) : 65536;

  std::string fullPath = path;
  if (path[0] != '/') {
    fullPath = s_activeRunner->appDir_ + "/" + path;
  }
  HalFile file;
  if (!Storage.openFileForRead("Lua", fullPath.c_str(), file)) {
    if (path[0] != '/' && Storage.openFileForRead("Lua", path, file)) {
      // opened
    } else {
      lua_pushnil(L);
      return 1;
    }
  }

  size_t sz = file.size();
  if (sz > maxBytes) sz = maxBytes;
#if !defined(SIMULATOR) && !defined(BOARD_HAS_PSRAM)
  // Ensure we don't exhaust ESP32-C3 internal DRAM reading huge files
  uint32_t freeH = ESP.getFreeHeap();
  if (freeH < 16384 || sz > freeH - 12288) {
    sz = (freeH > 16384) ? (freeH - 16384) : 0;
  }
#endif
  if (sz == 0) {
    file.close();
    lua_pushliteral(L, "");
    return 1;
  }

  // Allocate lua string buffer directly without intermediate Arduino String allocation
  luaL_Buffer b;
  char* buf = luaL_buffinitsize(L, &b, sz);
  size_t bytesRead = file.read(reinterpret_cast<uint8_t*>(buf), sz);
  file.close();
  luaL_pushresultsize(&b, bytesRead);
  return 1;
}

int LuaRunner::l_getMemory(lua_State* L) {
  lua_newtable(L);
  if (s_activeRunner) {
    lua_pushinteger(L, static_cast<lua_Integer>(s_activeRunner->currentAllocatedBytes_ / 1024));
    lua_setfield(L, -2, "lua_kb");
    lua_pushinteger(L, static_cast<lua_Integer>(s_activeRunner->maxLuaHeapBytes_ / 1024));
    lua_setfield(L, -2, "lua_max_kb");
  }
#if !defined(SIMULATOR)
  lua_pushinteger(L, static_cast<lua_Integer>(ESP.getFreeHeap()));
  lua_setfield(L, -2, "free_heap");
  lua_pushinteger(L, static_cast<lua_Integer>(ESP.getMaxAllocHeap()));
  lua_setfield(L, -2, "max_alloc");
#else
  lua_pushinteger(L, 1024 * 1024);
  lua_setfield(L, -2, "free_heap");
  lua_pushinteger(L, 512 * 1024);
  lua_setfield(L, -2, "max_alloc");
#endif
  return 1;
}

int LuaRunner::l_findSection(lua_State* L) {
  if (!s_activeRunner) {
    lua_pushnil(L);
    return 1;
  }
  const char* path = luaL_checkstring(L, 1);
  const char* tag = (lua_gettop(L) >= 2 && !lua_isnil(L, 2)) ? lua_tostring(L, 2) : "";
  const char* endPrefix = "[";
  size_t maxBytes = 16384;
  int callbackIdx = 0;

  if (lua_gettop(L) >= 3) {
    if (lua_isfunction(L, 3)) {
      callbackIdx = 3;
    } else if (!lua_isnil(L, 3)) {
      endPrefix = lua_tostring(L, 3);
    }
  }
  if (lua_gettop(L) >= 4) {
    if (lua_isfunction(L, 4)) {
      callbackIdx = 4;
    } else if (!lua_isnil(L, 4)) {
      maxBytes = static_cast<size_t>(lua_tointeger(L, 4));
    }
  }
  if (lua_gettop(L) >= 5) {
    if (lua_isfunction(L, 5)) {
      callbackIdx = 5;
    }
  }

#if !defined(SIMULATOR) && !defined(BOARD_HAS_PSRAM)
  // Ensure we don't request more than available heap on memory-constrained devices (ESP32-C3)
  if (callbackIdx == 0) {
    uint32_t freeH = ESP.getFreeHeap();
    if (freeH < 16384) {
      size_t safeCap = (freeH > 8192) ? (freeH - 6144) : 2048;
      if (maxBytes > safeCap) maxBytes = safeCap;
    }
  }
#endif

  std::string fullPath = path;
  if (path[0] != '/') {
    fullPath = s_activeRunner->appDir_ + "/" + path;
  }

  HalFile file;
  if (!Storage.openFileForRead("Lua", fullPath.c_str(), file)) {
    if (path[0] != '/' && Storage.openFileForRead("Lua", path, file)) {
      // opened relative or alternate path
    } else {
      if (callbackIdx != 0) {
        lua_pushboolean(L, false);
      } else {
        lua_pushnil(L);
      }
      return 1;
    }
  }

  // Use stack buffers: 512-byte SD card read chunk, 512-byte line buffer.
  // ZERO dynamic C++ heap allocation (no std::string, no bare new/abort()).
  char chunk[512];
  size_t chunkPos = 0, chunkSize = 0;
  bool inTarget = false;
  size_t tagLen = tag ? strlen(tag) : 0;
  size_t endLen = endPrefix ? strlen(endPrefix) : 0;

  luaL_Buffer resultBuf;
  bool bufInitialized = false;
  bool firstResultLine = true;
  size_t totalResultBytes = 0;

  char lineBuf[512];
  size_t lineLen = 0;

  auto getNextChar = [&]() -> int {
    if (chunkPos >= chunkSize) {
      int r = file.read(reinterpret_cast<uint8_t*>(chunk), sizeof(chunk));
      if (r <= 0) return -1;
      chunkPos = 0;
      chunkSize = static_cast<size_t>(r);
    }
    return static_cast<unsigned char>(chunk[chunkPos++]);
  };

  while (true) {
    lineLen = 0;
    int ch = -1;
    bool hasLine = false;

    while ((ch = getNextChar()) != -1) {
      hasLine = true;
      if (ch == '\n') break;
      if (ch != '\r') {
        if (lineLen + 1 < sizeof(lineBuf)) {
          lineBuf[lineLen++] = static_cast<char>(ch);
        }
      }
    }
    if (!hasLine) break;  // EOF reached

    // Trim trailing whitespace
    while (lineLen > 0 &&
           (lineBuf[lineLen - 1] == ' ' || lineBuf[lineLen - 1] == '\t' || lineBuf[lineLen - 1] == '\r')) {
      lineLen--;
    }
    lineBuf[lineLen] = '\0';

    if (!inTarget) {
      if (tagLen == 0 || (lineLen >= tagLen && strncmp(lineBuf, tag, tagLen) == 0)) {
        inTarget = true;
        if (callbackIdx == 0 && !bufInitialized) {
          luaL_buffinit(L, &resultBuf);
          bufInitialized = true;
        }
        if (tagLen > 0) {
          continue;  // Tag line itself is skipped
        }
        // If tagLen == 0, fall through and process line 1 immediately
      }
    }

    if (inTarget) {
      if (endLen > 0 && lineLen >= endLen && strncmp(lineBuf, endPrefix, endLen) == 0) {
        break;  // Reached end of target section
      }

      if (callbackIdx != 0) {
        lua_pushvalue(L, callbackIdx);
        lua_pushlstring(L, lineBuf, lineLen);
        if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
          LOG_ERR("LUA", "[%s] find_section callback error: %s", s_activeRunner->appId_.c_str(), lua_tostring(L, -1));
          lua_pop(L, 1);
          file.close();
          lua_pushboolean(L, false);
          return 1;
        }
        // If callback explicitly returned boolean false, break early
        if (lua_isboolean(L, -1) && !lua_toboolean(L, -1)) {
          lua_pop(L, 1);
          break;
        }
        lua_pop(L, 1);
      } else {
        if (!firstResultLine) {
          luaL_addchar(&resultBuf, '\n');
          totalResultBytes++;
        }
        firstResultLine = false;
        luaL_addlstring(&resultBuf, lineBuf, lineLen);
        totalResultBytes += lineLen;
        if (totalResultBytes >= maxBytes) {
          break;
        }
      }
    }
  }

  file.close();

  if (callbackIdx != 0) {
    lua_pushboolean(L, inTarget);
    return 1;
  }

  if (!inTarget || !bufInitialized) {
    lua_pushnil(L);
    return 1;
  }

  luaL_pushresult(&resultBuf);
  return 1;
}

int LuaRunner::l_fileExists(lua_State* L) {
  if (!s_activeRunner) {
    lua_pushboolean(L, false);
    return 1;
  }
  const char* path = luaL_checkstring(L, 1);
  std::string fullPath = path;
  if (path[0] != '/') {
    fullPath = s_activeRunner->appDir_ + "/" + path;
  }
  lua_pushboolean(L, Storage.exists(fullPath.c_str()));
  return 1;
}

int LuaRunner::l_exit(lua_State*) {
  if (s_activeRunner) {
    s_activeRunner->requestedExit_ = true;
  }
  return 0;
}

int LuaRunner::l_log(lua_State* L) {
  const char* msg = luaL_checkstring(L, 1);
  LOG_INF("LUA", "%s", msg);
  return 0;
}

static bool isLeapYear(int y) { return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0); }

static int daysInMonth(int y, int m) {
  static const int kDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (m < 1 || m > 12) return 30;
  if (m == 2 && isLeapYear(y)) return 29;
  return kDays[m - 1];
}

static int dayOfWeek(int y, int m, int d) {
  static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (m < 3) y -= 1;
  return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

static void adjustDateByDays(uint16_t& year, uint8_t& month, uint8_t& day, int delta) {
  while (delta > 0) {
    const int monthDays = daysInMonth(year, month);
    if (day < monthDays) {
      day++;
    } else {
      day = 1;
      if (month < 12) {
        month++;
      } else {
        month = 1;
        year++;
      }
    }
    delta--;
  }

  while (delta < 0) {
    if (day > 1) {
      day--;
    } else {
      if (month > 1) {
        month--;
      } else {
        month = 12;
        year--;
      }
      day = daysInMonth(year, month);
    }
    delta++;
  }
}

int LuaRunner::l_getDate(lua_State* L) {
  uint16_t yr = 0;
  uint8_t mo = 0, dy = 0, hr = 0, mn = 0;
  bool valid = false;

  const uint8_t offsetQ = std::min<uint8_t>(SETTINGS.clockUtcOffsetQ, 104);
  const int offsetMinutes = (static_cast<int>(offsetQ) - 48) * 15;

  if (halClock.isAvailable()) {
    if (halClock.getDateTime(yr, mo, dy, hr, mn)) {
      if (yr >= 2024 && yr <= 2040 && mo >= 1 && mo <= 12 && dy >= 1 && dy <= 31) {
        int localMinutes = static_cast<int>(hr) * 60 + static_cast<int>(mn) + offsetMinutes;
        while (localMinutes < 0) {
          adjustDateByDays(yr, mo, dy, -1);
          localMinutes += 24 * 60;
        }
        while (localMinutes >= 24 * 60) {
          adjustDateByDays(yr, mo, dy, 1);
          localMinutes -= 24 * 60;
        }
        hr = localMinutes / 60;
        mn = localMinutes % 60;
        valid = true;
      }
    }
  }

  if (!valid) {
    time_t now = time(nullptr);
    if (now > 1700000000) {
      time_t localSec = now + static_cast<time_t>(offsetMinutes * 60);
      struct tm tm_info;
      if (gmtime_r(&localSec, &tm_info)) {
        int y = tm_info.tm_year + 1900;
        int m = tm_info.tm_mon + 1;
        int d = tm_info.tm_mday;
        if (y >= 2024 && y <= 2040 && m >= 1 && m <= 12 && d >= 1 && d <= 31) {
          yr = y;
          mo = m;
          dy = d;
          hr = tm_info.tm_hour;
          mn = tm_info.tm_min;
          valid = true;
        }
      }
    }
  }

  if (!valid) {
    yr = 2026;
    mo = 10;
    dy = 2;
    hr = 12;
    mn = 0;
  }

  int wday = dayOfWeek(yr, mo, dy) + 1;  // 1 = Sunday, 7 = Saturday

  lua_newtable(L);
  lua_pushinteger(L, yr);
  lua_setfield(L, -2, "year");
  lua_pushinteger(L, mo);
  lua_setfield(L, -2, "month");
  lua_pushinteger(L, dy);
  lua_setfield(L, -2, "day");
  lua_pushinteger(L, hr);
  lua_setfield(L, -2, "hour");
  lua_pushinteger(L, mn);
  lua_setfield(L, -2, "min");
  lua_pushinteger(L, 0);
  lua_setfield(L, -2, "sec");
  lua_pushinteger(L, wday);
  lua_setfield(L, -2, "wday");
  lua_pushinteger(L, offsetMinutes);
  lua_setfield(L, -2, "offset_minutes");
  return 1;
}

int LuaRunner::l_time(lua_State* L) {
  uint16_t yr = 0;
  uint8_t mo = 0, dy = 0, hr = 0, mn = 0;
  const uint8_t offsetQ = std::min<uint8_t>(SETTINGS.clockUtcOffsetQ, 104);
  const int offsetMinutes = (static_cast<int>(offsetQ) - 48) * 15;

  if (halClock.isAvailable() && halClock.getDateTime(yr, mo, dy, hr, mn)) {
    if (yr >= 2024 && yr <= 2040 && mo >= 1 && mo <= 12 && dy >= 1 && dy <= 31) {
      struct tm t = {};
      t.tm_year = yr - 1900;
      t.tm_mon = mo - 1;
      t.tm_mday = dy;
      t.tm_hour = hr;
      t.tm_min = mn;
      time_t epoch = mktime(&t);
      if (epoch != (time_t)-1) {
        epoch += static_cast<time_t>(offsetMinutes * 60);
        lua_pushinteger(L, static_cast<lua_Integer>(epoch));
        return 1;
      }
    }
  }

  time_t now = time(nullptr);
  if (now > 1700000000) {
    now += static_cast<time_t>(offsetMinutes * 60);
    lua_pushinteger(L, static_cast<lua_Integer>(now));
    return 1;
  }

  lua_pushinteger(L, static_cast<lua_Integer>(1790942400));
  return 1;
}

int LuaRunner::l_random(lua_State* L) {
  int top = lua_gettop(L);
  if (top == 0) {
    double r = static_cast<double>(rand()) / (static_cast<double>(RAND_MAX) + 1.0);
    lua_pushnumber(L, r);
  } else if (top == 1) {
    int max = luaL_checkinteger(L, 1);
    if (max < 1) max = 1;
    lua_pushinteger(L, (rand() % max) + 1);
  } else {
    int min = luaL_checkinteger(L, 1);
    int max = luaL_checkinteger(L, 2);
    if (max < min) std::swap(min, max);
    int range = (max - min) + 1;
    lua_pushinteger(L, min + (rand() % range));
  }
  return 1;
}

int LuaRunner::l_millis(lua_State* L) {
#if defined(SIMULATOR)
  auto now = std::chrono::steady_clock::now().time_since_epoch();
  uint32_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
  lua_pushinteger(L, ms);
#else
  lua_pushinteger(L, millis());
#endif
  return 1;
}

int LuaRunner::l_requestUpdate(lua_State* L) {
  if (s_activeRunner) {
    s_activeRunner->redrawRequested_ = true;
  }
  return 0;
}

int LuaRunner::l_isButtonDown(lua_State* L) {
  if (!s_activeRunner) {
    lua_pushboolean(L, false);
    return 1;
  }
  const char* btn = luaL_checkstring(L, 1);
  bool pressed = false;
  if (strcmp(btn, "back") == 0) {
    pressed = s_activeRunner->input_.isPressed(MappedInputManager::Button::Back);
  } else if (strcmp(btn, "confirm") == 0) {
    pressed = s_activeRunner->input_.isPressed(MappedInputManager::Button::Confirm);
  } else if (strcmp(btn, "up") == 0) {
    pressed = s_activeRunner->input_.isPressed(MappedInputManager::Button::Up);
  } else if (strcmp(btn, "down") == 0) {
    pressed = s_activeRunner->input_.isPressed(MappedInputManager::Button::Down);
  } else if (strcmp(btn, "left") == 0) {
    pressed = s_activeRunner->input_.isPressed(MappedInputManager::Button::Left);
  } else if (strcmp(btn, "right") == 0) {
    pressed = s_activeRunner->input_.isPressed(MappedInputManager::Button::Right);
  }
  lua_pushboolean(L, pressed);
  return 1;
}

int LuaRunner::l_triangle(lua_State* L) {
  if (!s_activeRunner) return 0;
  int x0 = getInt(L, 1);
  int y0 = getInt(L, 2);
  int x1 = getInt(L, 3);
  int y1 = getInt(L, 4);
  int x2 = getInt(L, 5);
  int y2 = getInt(L, 6);
  bool fill = (lua_gettop(L) >= 7) ? lua_toboolean(L, 7) : false;
  int thickness = 1;
  bool color = true;

  if (lua_gettop(L) >= 8) {
    if (lua_isboolean(L, 8)) {
      color = lua_toboolean(L, 8);
      if (lua_gettop(L) >= 9) thickness = getInt(L, 9);
    } else {
      thickness = getInt(L, 8);
      if (lua_gettop(L) >= 9) color = lua_isboolean(L, 9) ? lua_toboolean(L, 9) : (lua_tointeger(L, 9) != 0);
    }
  }

  auto& renderer = s_activeRunner->renderer_;

  if (!fill) {
    renderer.drawLine(x0, y0, x1, y1, thickness, color);
    renderer.drawLine(x1, y1, x2, y2, thickness, color);
    renderer.drawLine(x2, y2, x0, y0, thickness, color);
    return 0;
  }

  // Scanline filled triangle
  if (y0 > y1) {
    std::swap(x0, x1);
    std::swap(y0, y1);
  }
  if (y1 > y2) {
    std::swap(x1, x2);
    std::swap(y1, y2);
  }
  if (y0 > y1) {
    std::swap(x0, x1);
    std::swap(y0, y1);
  }

  int total_height = y2 - y0;
  if (total_height == 0) return 0;

  for (int y = y0; y <= y2; ++y) {
    bool second_half = y > y1 || y1 == y0;
    int segment_height = second_half ? (y2 - y1) : (y1 - y0);
    if (segment_height == 0) continue;

    float alpha = static_cast<float>(y - y0) / total_height;
    float beta = static_cast<float>(y - (second_half ? y1 : y0)) / segment_height;

    int ax = x0 + static_cast<int>((x2 - x0) * alpha);
    int bx = second_half ? (x1 + static_cast<int>((x2 - x1) * beta)) : (x0 + static_cast<int>((x1 - x0) * beta));

    if (ax > bx) std::swap(ax, bx);
    renderer.drawLine(ax, y, bx, y, 1, color);
  }

  return 0;
}

int LuaRunner::l_invertRect(lua_State* L) {
  if (!s_activeRunner) return 0;
  int x = getInt(L, 1);
  int y = getInt(L, 2);
  int w = getInt(L, 3);
  int h = getInt(L, 4);
  if (w <= 0 || h <= 0) return 0;
  s_activeRunner->renderer_.invertRect(x, y, w, h);
  return 0;
}

int LuaRunner::l_invertScreen(lua_State*) {
  if (s_activeRunner) {
    s_activeRunner->renderer_.invertScreen();
  }
  return 0;
}

int LuaRunner::l_inRect(lua_State* L) {
  int px = getInt(L, 1);
  int py = getInt(L, 2);
  int rx = getInt(L, 3);
  int ry = getInt(L, 4);
  int rw = getInt(L, 5);
  int rh = getInt(L, 6);
  bool inside = (px >= rx && px < rx + rw && py >= ry && py < ry + rh);
  lua_pushboolean(L, inside);
  return 1;
}

int LuaRunner::l_hasTouch(lua_State* L) {
  if (!s_activeRunner) {
    lua_pushboolean(L, false);
    return 1;
  }
  lua_pushboolean(L, s_activeRunner->input_.hasTouchHardware());
  return 1;
}

int LuaRunner::l_getBattery(lua_State* L) {
  lua_pushinteger(L, powerManager.getBatteryPercentage());
  return 1;
}

int LuaRunner::l_getDevice(lua_State* L) {
#if defined(SIMULATOR)
  lua_pushstring(L, "simulator");
#elif defined(CROSSINK_APP_DEVICE_STICKY)
  lua_pushstring(L, "sticky");
#elif defined(CROSSINK_APP_DEVICE_X4PRO)
  lua_pushstring(L, "x4pro");
#elif defined(CROSSINK_APP_DEVICE_X4CLASSIC)
  lua_pushstring(L, "x4");
#else
  lua_pushstring(L, "x3");
#endif
  return 1;
}

int LuaRunner::l_writeFile(lua_State* L) {
  if (!s_activeRunner) {
    lua_pushboolean(L, false);
    return 1;
  }
  const char* path = luaL_checkstring(L, 1);
  size_t len = 0;
  const char* content = luaL_checklstring(L, 2, &len);
  bool append = (lua_gettop(L) >= 3) ? lua_toboolean(L, 3) : false;

  std::string fullPath = path;
  if (path[0] != '/') {
    fullPath = s_activeRunner->appDir_ + "/" + path;
  }

  HalFile f = Storage.open(fullPath.c_str(), append ? (O_WRONLY | O_CREAT | O_APPEND) : (O_WRONLY | O_CREAT | O_TRUNC));
  if (!f) {
    lua_pushboolean(L, false);
    return 1;
  }

  size_t written = f.write(reinterpret_cast<const uint8_t*>(content), len);
  f.close();
  lua_pushboolean(L, written == len);
  return 1;
}

int LuaRunner::l_deleteFile(lua_State* L) {
  if (!s_activeRunner) {
    lua_pushboolean(L, false);
    return 1;
  }
  const char* path = luaL_checkstring(L, 1);
  std::string fullPath = path;
  if (path[0] != '/') {
    fullPath = s_activeRunner->appDir_ + "/" + path;
  }
  bool ok = Storage.remove(fullPath.c_str());
  lua_pushboolean(L, ok);
  return 1;
}

int LuaRunner::l_listFiles(lua_State* L) {
  if (!s_activeRunner) {
    lua_newtable(L);
    return 1;
  }
  std::string dirPath = s_activeRunner->appDir_;
  if (lua_gettop(L) >= 1 && lua_isstring(L, 1)) {
    const char* sub = lua_tostring(L, 1);
    if (sub[0] == '/') {
      dirPath = sub;
    } else if (strlen(sub) > 0) {
      dirPath = s_activeRunner->appDir_ + "/" + sub;
    }
  }

  lua_newtable(L);
  HalFile dir = Storage.open(dirPath.c_str());
  if (dir && dir.isDirectory()) {
    int idx = 1;
    char name[128];
    for (HalFile entry = dir.openNextFile(); entry; entry = dir.openNextFile()) {
      name[0] = '\0';
      entry.getName(name, sizeof(name));
      entry.close();
      if (name[0] != '\0' && name[0] != '.') {
        size_t len = strlen(name);
        while (len > 0 && (name[len - 1] == '/' || name[len - 1] == '\\')) {
          name[--len] = '\0';
        }
        lua_pushstring(L, name);
        lua_rawseti(L, -2, idx++);
      }
    }
    dir.close();
  }
  return 1;
}

int LuaRunner::l_popup(lua_State* L) {
  if (!s_activeRunner) return 0;
  const char* msg = luaL_checkstring(L, 1);
  GUI.drawPopup(s_activeRunner->renderer_, msg);
  return 0;
}

int LuaRunner::l_setOrientation(lua_State* L) {
  if (!s_activeRunner) return 0;
  const char* str = luaL_checkstring(L, 1);
  if (strcmp(str, "landscape") == 0 || strcmp(str, "landscape_cw") == 0) {
    s_activeRunner->renderer_.setOrientation(GfxRenderer::Orientation::LandscapeClockwise);
  } else if (strcmp(str, "landscape_ccw") == 0) {
    s_activeRunner->renderer_.setOrientation(GfxRenderer::Orientation::LandscapeCounterClockwise);
  } else if (strcmp(str, "portrait_inverted") == 0) {
    s_activeRunner->renderer_.setOrientation(GfxRenderer::Orientation::PortraitInverted);
  } else {
    s_activeRunner->renderer_.setOrientation(GfxRenderer::Orientation::Portrait);
  }
  return 0;
}

int LuaRunner::l_getOrientation(lua_State* L) {
  if (!s_activeRunner) {
    lua_pushliteral(L, "portrait");
    return 1;
  }
  switch (s_activeRunner->renderer_.getOrientation()) {
    case GfxRenderer::Orientation::LandscapeClockwise:
      lua_pushliteral(L, "landscape");
      break;
    case GfxRenderer::Orientation::LandscapeCounterClockwise:
      lua_pushliteral(L, "landscape_ccw");
      break;
    case GfxRenderer::Orientation::PortraitInverted:
      lua_pushliteral(L, "portrait_inverted");
      break;
    default:
      lua_pushliteral(L, "portrait");
      break;
  }
  return 1;
}

int LuaRunner::l_preventSleep(lua_State* L) {
  if (!s_activeRunner) return 0;
  bool prevent = true;
  if (lua_gettop(L) >= 1) {
    prevent = lua_toboolean(L, 1);
  }
  s_activeRunner->setPreventAutoSleep(prevent);
  return 0;
}

int LuaRunner::l_isPreventSleep(lua_State* L) {
  if (!s_activeRunner) {
    lua_pushboolean(L, false);
    return 1;
  }
  lua_pushboolean(L, s_activeRunner->preventAutoSleep());
  return 1;
}

int LuaRunner::l_getVersion(lua_State* L) {
#ifdef CROSSSMUDGE_VERSION
  lua_pushstring(L, CROSSSMUDGE_VERSION);
#else
  lua_pushliteral(L, "unknown");
#endif
  return 1;
}

int LuaRunner::l_getApiVersion(lua_State* L) {
  lua_pushinteger(L, LUA_API_VERSION);
  return 1;
}

int LuaRunner::l_hasFeature(lua_State* L) {
  const char* feat = luaL_checkstring(L, 1);
  bool supported = false;
  if (strcmp(feat, "prevent_sleep") == 0 || strcmp(feat, "keep_awake") == 0 || strcmp(feat, "orientation") == 0 ||
      strcmp(feat, "set_orientation") == 0 || strcmp(feat, "full_refresh") == 0 || strcmp(feat, "invert") == 0 ||
      strcmp(feat, "touch") == 0 || strcmp(feat, "version") == 0) {
    supported = true;
  }
  lua_pushboolean(L, supported);
  return 1;
}

}  // namespace ink
