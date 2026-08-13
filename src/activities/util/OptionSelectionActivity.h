#pragma once

#include <FreeInkApp.h>
#include <FreeInkUIGfxRenderer.h>
#include <I18n.h>

#include <atomic>
#include <string>
#include <vector>

#include "activities/Activity.h"
#include "util/ButtonNavigator.h"

class OptionSelectionActivity final : public Activity {
 public:
  OptionSelectionActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string activityName,
                          StrId titleId, std::vector<std::string> options, uint8_t selectedIndex,
                          bool readerMode = false, bool showTouchHeaderBackButton = false);

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;
  bool allowPowerAsConfirmInReaderMode() const override { return readerMode_; }

 private:
  void cancel();
  void select();

  ButtonNavigator buttonNavigator_;
  StrId titleId_;
  std::vector<std::string> options_;
  int currentIndex_ = 0;
  int selectedIndex_ = 0;
  bool readerMode_ = false;
  bool showTouchHeaderBackButton_ = false;
  using UiApp = freeink::ui::FreeInkApp<20, 4>;
  freeink::ui::GfxRendererTarget uiTarget_;
  UiApp app_;
  std::atomic<bool> uiReady_{false};
  int visibleRows_ = 1;
  int topIndex_ = 0;
  bool initialViewportPending_ = true;

  static void optionsScreen(UiApp::ScreenType& screen, void* user);
  static void onRowEvent(const freeink::ui::ActionEvent& event, void* user);
  void buildOptionsScreen(UiApp::ScreenType& screen);
};
