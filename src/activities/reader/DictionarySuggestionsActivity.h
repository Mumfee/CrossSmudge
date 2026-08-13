#pragma once
#include <FreeInkApp.h>
#include <FreeInkUIGfxRenderer.h>

#include <string>
#include <vector>

#include "../Activity.h"

class DictionarySuggestionsActivity final : public Activity {
 public:
  explicit DictionarySuggestionsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                         std::vector<std::string> suggestions);

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  using UiApp = freeink::ui::FreeInkApp<20, 4>;
  static constexpr freeink::ui::ActionId ACTION_ROW = 1;

  static void suggestionsScreen(UiApp::ScreenType& screen, void* user);
  static void onRowEvent(const freeink::ui::ActionEvent& event, void* user);
  void buildSuggestionsScreen(UiApp::ScreenType& screen);

  std::vector<std::string> suggestions;
  std::vector<freeink::ui::ListItem> uiItems;
  int selectedIndex = 0;
  freeink::ui::GfxRendererTarget uiTarget;
  UiApp app;
  bool uiReady = false;
  int visibleRows = 1;
  int topIndex = 0;
};
