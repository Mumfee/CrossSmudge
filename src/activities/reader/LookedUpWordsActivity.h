#pragma once
#include <FreeInkApp.h>
#include <FreeInkUIGfxRenderer.h>

#include <cstring>
#include <string>
#include <vector>

#include "../Activity.h"
#include "util/ButtonNavigator.h"
#include "util/DictionaryLookupController.h"
#include "util/LookupHistory.h"

class LookedUpWordsActivity final : public Activity {
 public:
  explicit LookedUpWordsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string bookCachePath,
                                 const char* dictionaryFontFamilyName = nullptr, uint8_t dictionaryFontPointSize = 0);

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  using UiApp = freeink::ui::FreeInkApp<20, 4>;
  static constexpr freeink::ui::ActionId ACTION_ROW = 1;

  static void historyScreen(UiApp::ScreenType& screen, void* user);
  static void onRowEvent(const freeink::ui::ActionEvent& event, void* user);
  void buildHistoryScreen(UiApp::ScreenType& screen);
  void reloadEntries();

  std::string cachePath;
  char dictionaryFontFamilyName[64] = "";
  uint8_t dictionaryFontPointSize = 0;
  std::vector<LookupHistory::Entry> entries;
  std::vector<std::string> labels;
  std::vector<freeink::ui::ListItem> uiItems;
  int selectedIndex = 0;

  DictionaryLookupController controller;
  ButtonNavigator buttonNavigator;
  freeink::ui::GfxRendererTarget uiTarget;
  UiApp app;
  bool uiReady = false;
  int visibleRows = 1;
  int topIndex = 0;

  bool skipLoopDelay() override { return controller.skipLoopDelay(); }

  void showDeleteConfirmation(bool ignoreInitialConfirmRelease);
  static const char* glyphFor(LookupHistory::Status s);
};
