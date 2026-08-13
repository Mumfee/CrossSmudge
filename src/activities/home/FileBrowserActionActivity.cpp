#include "FileBrowserActionActivity.h"

#include <I18n.h>

#include <algorithm>

FileBrowserActionActivity::FileBrowserActionActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                                     std::string title, std::vector<MenuItem> items,
                                                     const bool ignoreInitialConfirmRelease)
    : Activity("FileBrowserAction", renderer, mappedInput),
      title(std::move(title)),
      items(std::move(items)),
      ignoreConfirmRelease(ignoreInitialConfirmRelease) {}

void FileBrowserActionActivity::onEnter() {
  Activity::onEnter();
  // A touch long-press opens this activity while the finger is still down.
  // Wait for that contact to end so its release cannot activate a menu row.
  int touchX = 0;
  int touchY = 0;
  ignoreTouchRelease = mappedInput.isScreenTouchHeld(touchX, touchY);
  optionLabels.resize(items.size());
  std::transform(items.begin(), items.end(), optionLabels.begin(),
                 [](const MenuItem& item) { return std::string(I18N.get(item.labelId)); });
  optionPopup.show(title.c_str(), optionLabels, 0, [this](const int index) {
    if (index < 0 || index >= static_cast<int>(items.size())) return;
    selectionMade = true;
    setResult(FileBrowserActionResult{static_cast<int>(items[index].action)});
    finish();
  });
  requestUpdate();
}

void FileBrowserActionActivity::finishCancelled() {
  ActivityResult result;
  result.isCancelled = true;
  setResult(std::move(result));
  finish();
}

void FileBrowserActionActivity::loop() {
  if (ignoreTouchRelease) {
    if (mappedInput.wasScreenTouchReleased()) {
      ignoreTouchRelease = false;
    }
    return;
  }

  if (ignoreConfirmRelease) {
    const bool confirmReleased = mappedInput.wasReleased(MappedInputManager::Button::Confirm);
    if (confirmReleased || !mappedInput.isPressed(MappedInputManager::Button::Confirm)) {
      ignoreConfirmRelease = false;
      return;
    }
  }

  optionPopup.handleInput(mappedInput, [this] { requestUpdate(); });
  if (!optionPopup.isActive() && !selectionMade) finishCancelled();
}

void FileBrowserActionActivity::render(RenderLock&&) { optionPopup.processRender(renderer, mappedInput); }
