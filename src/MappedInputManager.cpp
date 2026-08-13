#include "MappedInputManager.h"

#include <GfxRenderer.h>

#include <algorithm>
#include <cstdlib>
#include <utility>

#include "CrossPointSettings.h"
#include "GlobalActions.h"
#if CROSSSMUDGE_APP_CAP_TOUCH
#include "components/TouchRegistry.h"
#endif
#include "components/UITheme.h"
#ifdef SIMULATOR
#include "simulator/SimulatorHomeKeyInput.h"
#endif

namespace {
using ButtonIndex = uint8_t;
constexpr ButtonIndex kNoButton = UINT8_MAX;
constexpr float LEFT_EDGE_BACK_GESTURE_FRAC_X = 0.25f;
constexpr float BOTTOM_EDGE_HOME_GESTURE_FRAC_Y = 0.06f;
constexpr float READER_BOTTOM_EDGE_HOME_GESTURE_FRAC_Y = 0.12f;
constexpr float TOP_EDGE_MENU_GESTURE_FRAC_Y = 0.14f;
constexpr unsigned long TOUCH_HELD_OVERRIDE_WINDOW_MS = 250;

constexpr float bottomEdgeHomeGestureFraction(const bool readerMode) {
  return readerMode ? READER_BOTTOM_EDGE_HOME_GESTURE_FRAC_Y : BOTTOM_EDGE_HOME_GESTURE_FRAC_Y;
}

struct SideLayoutMap {
  ButtonIndex pageBackPrimary;
  ButtonIndex pageBackSecondary;
  ButtonIndex pageForwardPrimary;
  ButtonIndex pageForwardSecondary;
};

// Order matches CrossPointSettings::SIDE_BUTTON_LAYOUT.
constexpr SideLayoutMap kSideLayouts[] = {
    {HalGPIO::BTN_UP, kNoButton, HalGPIO::BTN_DOWN, kNoButton},
    {HalGPIO::BTN_DOWN, kNoButton, HalGPIO::BTN_UP, kNoButton},
    {kNoButton, kNoButton, kNoButton, kNoButton},
    {kNoButton, kNoButton, HalGPIO::BTN_UP, HalGPIO::BTN_DOWN},
};

bool shouldSwapReaderSideButtons(const bool readerMode) {
  return readerMode && SETTINGS.sideButtonOrientationAware && SETTINGS.orientation != CrossPointSettings::PORTRAIT;
}

bool shouldSwapReaderFrontNavButtons(const CrossPointSettings::FRONT_BUTTON_ORIENTATION_AWARE orientationMode) {
  if (orientationMode == CrossPointSettings::FRONT_ORIENTATION_AWARE_OFF) {
    return false;
  }
  return SETTINGS.orientation == CrossPointSettings::LANDSCAPE_CW ||
         SETTINGS.orientation == CrossPointSettings::LANDSCAPE_CCW ||
         (orientationMode == CrossPointSettings::FRONT_ORIENTATION_AWARE_NAV_BUTTONS &&
          SETTINGS.orientation == CrossPointSettings::INVERTED);
}

ButtonIndex invertFrontButtonPosition(const ButtonIndex button) {
  switch (button) {
    case HalGPIO::BTN_BACK:
      return HalGPIO::BTN_RIGHT;
    case HalGPIO::BTN_CONFIRM:
      return HalGPIO::BTN_LEFT;
    case HalGPIO::BTN_LEFT:
      return HalGPIO::BTN_CONFIRM;
    case HalGPIO::BTN_RIGHT:
      return HalGPIO::BTN_BACK;
    default:
      return button;
  }
}

ButtonIndex mapFrontButtonForReaderOrientation(const ButtonIndex button, const ButtonIndex leftButton,
                                               const ButtonIndex rightButton, const bool readerMode) {
  if (!readerMode) {
    return button;
  }

  const auto orientationMode =
      static_cast<CrossPointSettings::FRONT_BUTTON_ORIENTATION_AWARE>(SETTINGS.frontButtonOrientationAware);

  if (orientationMode == CrossPointSettings::FRONT_ORIENTATION_AWARE_ALL_BUTTONS &&
      SETTINGS.orientation == CrossPointSettings::INVERTED) {
    return invertFrontButtonPosition(button);
  }

  if (shouldSwapReaderFrontNavButtons(orientationMode)) {
    if (button == leftButton) {
      return rightButton;
    }
    if (button == rightButton) {
      return leftButton;
    }
  }

  return button;
}

SideLayoutMap mapSideLayoutForReaderOrientation(SideLayoutMap side, const bool readerMode) {
  if (shouldSwapReaderSideButtons(readerMode)) {
    const bool hasPageBack = side.pageBackPrimary != kNoButton || side.pageBackSecondary != kNoButton;
    const bool hasPageForward = side.pageForwardPrimary != kNoButton || side.pageForwardSecondary != kNoButton;
    if (hasPageBack && hasPageForward) {
      std::swap(side.pageBackPrimary, side.pageForwardPrimary);
      std::swap(side.pageBackSecondary, side.pageForwardSecondary);
    }
  }
  return side;
}

ButtonIndex mapSideButtonForReaderOrientation(const ButtonIndex button, const bool readerMode) {
  if (!shouldSwapReaderSideButtons(readerMode)) {
    return button;
  }
  if (button == HalGPIO::BTN_UP) {
    return HalGPIO::BTN_DOWN;
  }
  if (button == HalGPIO::BTN_DOWN) {
    return HalGPIO::BTN_UP;
  }
  return button;
}

bool readMappedSideButtons(const HalGPIO& gpio, bool (HalGPIO::*fn)(uint8_t) const, const ButtonIndex primary,
                           const ButtonIndex secondary) {
  return (primary != kNoButton && (gpio.*fn)(primary)) || (secondary != kNoButton && (gpio.*fn)(secondary));
}

#ifdef SIMULATOR
size_t buttonIndex(MappedInputManager::Button button) { return static_cast<size_t>(button); }
#endif

}  // namespace

bool MappedInputManager::mapButton(const Button button, bool (HalGPIO::*fn)(uint8_t) const) const {
  const auto sideLayout = static_cast<CrossPointSettings::SIDE_BUTTON_LAYOUT>(SETTINGS.sideButtonLayout);
  const auto side = mapSideLayoutForReaderOrientation(kSideLayouts[sideLayout], readerMode);

  const ButtonIndex frontButton = mappedFrontButtonFor(button);

  switch (button) {
    case Button::Back:
    case Button::Confirm:
    case Button::Left:
    case Button::Right:
      return frontButton != kNoButton && (gpio.*fn)(frontButton);
    case Button::Up:
      // Reader menus should follow the same top/bottom side-button orientation as reader page turns.
      return (gpio.*fn)(mapSideButtonForReaderOrientation(HalGPIO::BTN_UP, readerMode));
    case Button::Down:
      // Reader menus should follow the same top/bottom side-button orientation as reader page turns.
      return (gpio.*fn)(mapSideButtonForReaderOrientation(HalGPIO::BTN_DOWN, readerMode));
    case Button::Power:
      // Power button bypasses remapping.
      return (gpio.*fn)(HalGPIO::BTN_POWER);
    case Button::PageBack:
      // Reader page navigation uses side buttons and can be swapped via settings.
      return readMappedSideButtons(gpio, fn, side.pageBackPrimary, side.pageBackSecondary);
    case Button::PageForward:
      // Reader page navigation uses side buttons and can be swapped via settings.
      return readMappedSideButtons(gpio, fn, side.pageForwardPrimary, side.pageForwardSecondary);
  }

  return false;
}

uint8_t MappedInputManager::mappedFrontButtonFor(const Button button) const {
  const bool useReaderMapping = readerMode && SETTINGS.readerFrontButtonsEnabled;
  const ButtonIndex btnBack = useReaderMapping ? SETTINGS.readerFrontButtonBack : SETTINGS.frontButtonBack;
  const ButtonIndex btnConfirm = useReaderMapping ? SETTINGS.readerFrontButtonConfirm : SETTINGS.frontButtonConfirm;
  const ButtonIndex btnLeft = useReaderMapping ? SETTINGS.readerFrontButtonLeft : SETTINGS.frontButtonLeft;
  const ButtonIndex btnRight = useReaderMapping ? SETTINGS.readerFrontButtonRight : SETTINGS.frontButtonRight;
  const ButtonIndex mappedBack = mapFrontButtonForReaderOrientation(btnBack, btnLeft, btnRight, readerMode);
  const ButtonIndex mappedConfirm = mapFrontButtonForReaderOrientation(btnConfirm, btnLeft, btnRight, readerMode);
  const ButtonIndex mappedLeft = mapFrontButtonForReaderOrientation(btnLeft, btnLeft, btnRight, readerMode);
  const ButtonIndex mappedRight = mapFrontButtonForReaderOrientation(btnRight, btnLeft, btnRight, readerMode);

  switch (button) {
    case Button::Back:
      return mappedBack;
    case Button::Confirm:
      return mappedConfirm;
    case Button::Left:
      return mappedLeft;
    case Button::Right:
      return mappedRight;
    default:
      return kNoButton;
  }
}

bool MappedInputManager::shouldUsePowerAsConfirmFallback() const { return !readerMode || powerAsConfirmInReaderMode; }

bool MappedInputManager::shouldMirrorPowerAsConfirmHold() const {
  return shouldUsePowerAsConfirmFallback() &&
         !isPowerButtonActionAvailableOutsideReader(static_cast<CrossPointSettings::SHORT_PWRBTN>(SETTINGS.longPwrBtn));
}

#if CROSSSMUDGE_APP_CAP_TOUCH
bool MappedInputManager::touchInputEnabled() const {
  return gpio.hasTouch() && (!readerMode || !SETTINGS.disableReaderTouchscreen || readerTouchscreenOverride);
}

bool MappedInputManager::hasTouch() const { return touchInputEnabled(); }

bool MappedInputManager::hasTouchHardware() const { return gpio.hasTouch(); }

void MappedInputManager::rememberTouchHeldTime() const {
  touchHeldOverrideValid = true;
  touchHeldOverrideMs = gpio.lastTouchHeldMs();
  touchHeldOverrideAt = millis();
}

bool MappedInputManager::wasScreenTapped(int& x, int& y) const {
  if (!touchInputEnabled()) return false;
  if (suppressTouchTap) {
    // The release edge may be observed after the new activity's first render. Clear
    // on either edge so a missed release cannot suppress a later deliberate tap.
    if (gpio.wasTouchActivity()) suppressTouchTap = false;
    return false;
  }
#ifdef SIMULATOR
  if (simulatorTouch.releasedThisFrame) {
    x = simulatorTouch.startX;
    y = simulatorTouch.startY;
    rememberTouchHeldTime();
    return true;
  }
#endif
  float nx = 0.0f;
  float ny = 0.0f;
  if (!gpio.wasTouchTap(nx, ny)) return false;
  renderer.tapToLogical(nx, ny, x, y);
  rememberTouchHeldTime();
  return true;
}

bool MappedInputManager::wasScreenTapped(int& x, int& y, unsigned long& heldMs) const {
  if (!wasScreenTapped(x, y)) return false;
  heldMs = touchHeldOverrideMs;
  return true;
}

bool MappedInputManager::isScreenTouchLongPress(int& x, int& y, const unsigned long thresholdMs) const {
  unsigned long heldMs = 0;
  return isScreenTouchTapCandidate(x, y, heldMs) && heldMs >= thresholdMs;
}

bool MappedInputManager::isInVerticalEdgeGestureZone(const int y) const {
  const int screenHeight = renderer.getScreenHeight();
  if (screenHeight <= 0) return false;
  const int topEdgeBottom = static_cast<int>(screenHeight * TOP_EDGE_MENU_GESTURE_FRAC_Y);
  const int bottomEdgeTop = screenHeight - static_cast<int>(screenHeight * bottomEdgeHomeGestureFraction(readerMode));
  return y <= topEdgeBottom || y >= bottomEdgeTop;
}

bool MappedInputManager::wasScreenTouchDown(int& x, int& y) const {
  if (!touchInputEnabled()) return false;
#ifdef SIMULATOR
  if (simulatorTouch.pressedThisFrame) {
    x = simulatorTouch.startX;
    y = simulatorTouch.startY;
    return true;
  }
#endif
  float nx = 0.0f;
  float ny = 0.0f;
  if (!gpio.wasTouchDown(nx, ny)) return false;
  renderer.tapToLogical(nx, ny, x, y);
  return true;
}

bool MappedInputManager::isScreenTouchTapCandidate(int& x, int& y, unsigned long& heldMs) const {
  if (!touchInputEnabled()) return false;
#ifdef SIMULATOR
  if (simulatorTouch.pressed) {
    x = simulatorTouch.startX;
    y = simulatorTouch.startY;
    heldMs = millis() - simulatorTouch.startedAt;
    return true;
  }
#endif
  float nx = 0.0f;
  float ny = 0.0f;
  if (!gpio.isTouchTapCandidate(nx, ny, heldMs)) return false;
  renderer.tapToLogical(nx, ny, x, y);
  return true;
}

bool MappedInputManager::isScreenTouchHeld(int& x, int& y) const {
  if (!touchInputEnabled()) return false;
#ifdef SIMULATOR
  if (simulatorTouch.pressed) {
    x = simulatorTouch.currentX;
    y = simulatorTouch.currentY;
    return true;
  }
#endif
  // Live contact position while the finger is down (no tap-slop gate) — drag tracking.
  float nx = 0.0f;
  float ny = 0.0f;
  if (!gpio.isTouchHeldAt(nx, ny)) return false;
  renderer.tapToLogical(nx, ny, x, y);
  return true;
}

bool MappedInputManager::wasRegistryTargetTapped(const uint8_t kind, int& id) const {
  int tx = 0;
  int ty = 0;
  return wasScreenTapped(tx, ty) &&
         TouchRegistry::getInstance().hitTest(tx, ty, static_cast<TouchRegistry::Kind>(kind), id);
}

bool MappedInputManager::wasRegistryTargetTouchedDown(const uint8_t kind, int& id) const {
  int tx = 0;
  int ty = 0;
  return wasScreenTouchDown(tx, ty) &&
         TouchRegistry::getInstance().hitTest(tx, ty, static_cast<TouchRegistry::Kind>(kind), id);
}

bool MappedInputManager::wasItemTapped(int& id) const { return wasRegistryTargetTapped(TouchRegistry::Kind::Item, id); }

bool MappedInputManager::wasItemTouchedDown(int& id) const {
  return wasRegistryTargetTouchedDown(TouchRegistry::Kind::Item, id);
}

bool MappedInputManager::wasTabTapped(int& id) const { return wasRegistryTargetTapped(TouchRegistry::Kind::Tab, id); }

bool MappedInputManager::wasTabTouchedDown(int& id) const {
  return wasRegistryTargetTouchedDown(TouchRegistry::Kind::Tab, id);
}

bool MappedInputManager::wasCoverTapped(int& id) const {
  return wasRegistryTargetTapped(TouchRegistry::Kind::Cover, id);
}

bool MappedInputManager::wasCoverTouchedDown(int& id) const {
  return wasRegistryTargetTouchedDown(TouchRegistry::Kind::Cover, id);
}

bool MappedInputManager::wasFrontButtonHintTapped(const uint8_t buttonIndex) const {
  int id = -1;
  return wasRegistryTargetTapped(TouchRegistry::Kind::Button, id) && id == buttonIndex;
}

bool MappedInputManager::wasFrontButtonHintTouchedDown(const uint8_t buttonIndex) const {
  int id = -1;
  return wasRegistryTargetTouchedDown(TouchRegistry::Kind::Button, id) && id == buttonIndex;
}

bool MappedInputManager::wasScreenTouchReleased() const { return gpio.wasTouchReleased(); }

bool MappedInputManager::wasTapInRect(const int x, const int y, const int width, const int height) const {
  int tx = 0;
  int ty = 0;
  return wasScreenTapped(tx, ty) && tx >= x && tx < x + width && ty >= y && ty < y + height;
}

bool MappedInputManager::listItemFromPoint(const int x, const int y, int& index, const int itemCount,
                                           const int selectedIndex, const int listTop, const int listHeight,
                                           const bool hasSubtitle) const {
  (void)x;
  if (itemCount <= 0) return false;
  if (y < listTop || y >= listTop + listHeight) return false;

  const auto& theme = UITheme::getInstance().getTheme();
  const int rowStep = theme.getListRowStep(hasSubtitle);
  if (rowStep <= 0) return false;

  const int pageItems = theme.getListPageItems(listHeight, hasSubtitle);
  if (pageItems <= 0) return false;
  const int pageStart = std::max(0, selectedIndex / pageItems) * pageItems;
  const int row = (y - listTop) / rowStep;
  const int tapped = pageStart + row;
  if (row < 0 || row >= pageItems || tapped >= itemCount) return false;
  index = tapped;
  return true;
}

bool MappedInputManager::wasListItemTapped(int& index, const int itemCount, const int selectedIndex, const int listTop,
                                           const int listHeight, const bool hasSubtitle) const {
  int tx = 0;
  int ty = 0;
  if (!wasScreenTapped(tx, ty)) return false;
  if (TouchRegistry::getInstance().hitTest(tx, ty, TouchRegistry::Item, index) && index >= 0 && index < itemCount) {
    return true;
  }
  return listItemFromPoint(tx, ty, index, itemCount, selectedIndex, listTop, listHeight, hasSubtitle);
}

bool MappedInputManager::wasListItemTouchedDown(int& index, const int itemCount, const int selectedIndex,
                                                const int listTop, const int listHeight, const bool hasSubtitle) const {
  int tx = 0;
  int ty = 0;
  if (!wasScreenTouchDown(tx, ty)) return false;
  if (TouchRegistry::getInstance().hitTest(tx, ty, TouchRegistry::Item, index) && index >= 0 && index < itemCount) {
    return true;
  }
  return listItemFromPoint(tx, ty, index, itemCount, selectedIndex, listTop, listHeight, hasSubtitle);
}

bool MappedInputManager::isListItemTouchLongPressed(int& index, const int itemCount, const int selectedIndex,
                                                    const int listTop, const int listHeight, const bool hasSubtitle,
                                                    const unsigned long thresholdMs) const {
  int tx = 0;
  int ty = 0;
  if (!isScreenTouchLongPress(tx, ty, thresholdMs)) return false;
  if (TouchRegistry::getInstance().hitTest(tx, ty, TouchRegistry::Item, index) && index >= 0 && index < itemCount) {
    return true;
  }
  return listItemFromPoint(tx, ty, index, itemCount, selectedIndex, listTop, listHeight, hasSubtitle);
}

MappedInputManager::RowTouch MappedInputManager::rowTouch(int& row, const int top, const int rowStep,
                                                          const int rowCount, const int xStart, const int xEnd,
                                                          const int rowHeight) const {
  if (rowStep <= 0 || rowCount <= 0) return RowTouch::None;
  const auto hit = [&](const int x, const int y) {
    if (x < xStart || x >= xEnd || y < top) return false;
    const int r = (y - top) / rowStep;
    if (r >= rowCount) return false;
    if (rowHeight > 0 && (y - top) % rowStep >= rowHeight) return false;
    row = r;
    return true;
  };
  int x = 0;
  int y = 0;
  if (wasScreenTouchDown(x, y) && hit(x, y)) return RowTouch::Down;
  if (wasScreenTapped(x, y) && hit(x, y)) return RowTouch::Tap;
  return RowTouch::None;
}

MappedInputManager::RowTouch MappedInputManager::colTouch(int& col, const int left, const int colStep,
                                                          const int colCount, const int yStart, const int yEnd,
                                                          const int colWidth) const {
  if (colStep <= 0 || colCount <= 0) return RowTouch::None;
  const auto hit = [&](const int x, const int y) {
    if (y < yStart || y >= yEnd || x < left) return false;
    const int c = (x - left) / colStep;
    if (c >= colCount) return false;
    if (colWidth > 0 && (x - left) % colStep >= colWidth) return false;
    col = c;
    return true;
  };
  int x = 0;
  int y = 0;
  if (wasScreenTouchDown(x, y) && hit(x, y)) return RowTouch::Down;
  if (wasScreenTapped(x, y) && hit(x, y)) return RowTouch::Tap;
  return RowTouch::None;
}

bool MappedInputManager::decodeSwipe(int& sx, int& sy, int& ex, int& ey) const {
  if (!touchInputEnabled()) return false;
#ifdef SIMULATOR
  if (simulatorTouch.releasedThisFrame) {
    sx = simulatorTouch.startX;
    sy = simulatorTouch.startY;
    ex = simulatorTouch.currentX;
    ey = simulatorTouch.currentY;
    return sx != ex || sy != ey;
  }
#endif
  float nxs = 0.0f;
  float nys = 0.0f;
  float nxe = 0.0f;
  float nye = 0.0f;
  if (!gpio.wasSwipe(nxs, nys, nxe, nye)) return false;
  renderer.tapToLogical(nxs, nys, sx, sy);
  renderer.tapToLogical(nxe, nye, ex, ey);
  return true;
}

bool MappedInputManager::wasSwipeWithPoints(SwipeDir& direction, int& startX, int& startY, int& endX, int& endY) const {
  if (!decodeSwipe(startX, startY, endX, endY)) {
    direction = SwipeDir::None;
    return false;
  }

  const int dx = endX - startX;
  const int dy = endY - startY;
  if (std::abs(dx) >= std::abs(dy)) {
    direction = dx < 0 ? SwipeDir::Left : SwipeDir::Right;
  } else {
    direction = dy < 0 ? SwipeDir::Up : SwipeDir::Down;
  }
  return true;
}

MappedInputManager::SwipeDir MappedInputManager::wasSwipe() const {
  SwipeDir direction = SwipeDir::None;
  int sx = 0;
  int sy = 0;
  int ex = 0;
  int ey = 0;
  wasSwipeWithPoints(direction, sx, sy, ex, ey);
  return direction;
}

bool MappedInputManager::wasBackGesture() const {
  if (!touchInputEnabled()) return false;
  // Back = left-to-right swipe starting near the left edge. Edge-anchored so that
  // mid-screen horizontal swipes stay available to activities that consume
  // SwipeDir::Left/Right (e.g. percent selection, image viewer).
  int sx = 0;
  int sy = 0;
  int ex = 0;
  int ey = 0;
  if (!decodeSwipe(sx, sy, ex, ey)) return false;
  const bool hit = sx <= renderer.getScreenWidth() * LEFT_EDGE_BACK_GESTURE_FRAC_X && ex > sx &&
                   std::abs(ex - sx) > std::abs(ey - sy);
  if (hit) rememberTouchHeldTime();
  return hit;
}

bool MappedInputManager::wasLeftEdgeGesture() const { return wasBackGesture(); }

bool MappedInputManager::hasHomeKeyHardware() const {
#ifdef SIMULATOR
#ifdef SIMULATOR_DEVICE_X4PRO
  return true;
#else
  return false;
#endif
#else
  return gpio.hasHomeKey();
#endif
}

bool MappedInputManager::wasTopEdgeDownSwipe() const {
  // Downward swipe starting at the top edge (mirror of the bottom-edge swipe).
  int sx = 0;
  int sy = 0;
  int ex = 0;
  int ey = 0;
  if (!decodeSwipe(sx, sy, ex, ey)) return false;
  const int topEdgeBottom = static_cast<int>(renderer.getScreenHeight() * TOP_EDGE_MENU_GESTURE_FRAC_Y);
  const bool hit = sy <= topEdgeBottom && ey > sy && std::abs(ey - sy) > std::abs(ex - sx);
  if (hit) rememberTouchHeldTime();
  return hit;
}

bool MappedInputManager::wasBottomEdgeUpSwipe() const {
  int sx = 0;
  int sy = 0;
  int ex = 0;
  int ey = 0;
  if (decodeSwipe(sx, sy, ex, ey)) {
    const int screenHeight = renderer.getScreenHeight();
    const int bottomEdgeTop = screenHeight - static_cast<int>(screenHeight * bottomEdgeHomeGestureFraction(readerMode));
    if (sy >= bottomEdgeTop && ey < sy && std::abs(ey - sy) > std::abs(ex - sx)) {
      rememberTouchHeldTime();
      return true;
    }
  }
  return false;
}

// Home-key boards (X4 Pro) rearrange the edge gestures: the capacitive key
// takes over "exit to home", the bottom edge (freed by the key) becomes the
// menu gesture, and the top edge opens the frontlight quick panel.
bool MappedInputManager::wasMenuGesture() const {
  return hasHomeKeyHardware() ? wasBottomEdgeUpSwipe() : wasTopEdgeDownSwipe();
}

bool MappedInputManager::wasReaderMenuGesture() const {
  const SwipeDir direction = wasSwipe();
  return hasHomeKeyHardware() ? direction == SwipeDir::Up : direction == SwipeDir::Down;
}

bool MappedInputManager::wasReaderHomeGesture() const { return !hasHomeKeyHardware() && wasSwipe() == SwipeDir::Up; }

bool MappedInputManager::wasReaderLightPanelGesture() const {
  return hasHomeKeyHardware() && wasSwipe() == SwipeDir::Down;
}

bool MappedInputManager::wasHomeGesture() const {
  if (!hasHomeKeyHardware()) return wasBottomEdgeUpSwipe();
#ifdef SIMULATOR
  return simulatorHomeKeyInput.wasTapped();
#else
  return gpio.wasHomeKeyTapped();
#endif
}

bool MappedInputManager::wasReaderMenuHold() const {
  if (!hasHomeKeyHardware()) return false;
#ifdef SIMULATOR
  return simulatorHomeKeyInput.wasLongPressed();
#else
  return gpio.wasHomeKeyLongPressed();
#endif
}

bool MappedInputManager::wasLightPanelGesture() const { return hasHomeKeyHardware() && wasTopEdgeDownSwipe(); }
#endif

bool MappedInputManager::wasPressed(const Button button) const {
#ifdef SIMULATOR
  if (simulatorPressed[buttonIndex(button)]) {
    return true;
  }
#endif

  if (button == Button::Confirm) {
    if (mapButton(button, &HalGPIO::wasPressed)) {
      return true;
    }
    if (wasFrontButtonHintTouchedDown(mappedFrontButtonFor(button))) {
      return true;
    }

    return shouldUsePowerAsConfirmFallback() &&
           !isPowerButtonActionAvailableOutsideReader(
               static_cast<CrossPointSettings::SHORT_PWRBTN>(SETTINGS.shortPwrBtn)) &&
           gpio.wasPressed(HalGPIO::BTN_POWER);
  }

  if (button == Button::Back && wasBackGesture()) {
    return true;
  }

  const uint8_t frontButton = mappedFrontButtonFor(button);
  return mapButton(button, &HalGPIO::wasPressed) ||
         (frontButton != kNoButton && wasFrontButtonHintTouchedDown(frontButton));
}

bool MappedInputManager::wasReleased(const Button button) const {
#ifdef SIMULATOR
  if (simulatorReleased[buttonIndex(button)]) {
    return true;
  }
#endif

  if (button == Button::Back) {
    if (wasBackGesture()) {
      return true;
    }

#if CROSSSMUDGE_APP_CAP_TOUCH
    if (!mapButton(button, &HalGPIO::wasReleased) && !wasFrontButtonHintTapped(mappedFrontButtonFor(button))) {
      return false;
    }
#else
    if (!mapButton(button, &HalGPIO::wasReleased)) {
      return false;
    }
#endif

    if (suppressBackRelease) {
      suppressBackRelease = false;
      return false;
    }

    return true;
  }

  if (button == Button::Confirm) {
    if (mapButton(button, &HalGPIO::wasReleased) || wasFrontButtonHintTapped(mappedFrontButtonFor(button))) {
      if (suppressConfirmRelease) {
        suppressConfirmRelease = false;
        return false;
      }
      return true;
    }

    if (!shouldUsePowerAsConfirmFallback() || !gpio.wasReleased(HalGPIO::BTN_POWER)) {
      return false;
    }

    if (suppressConfirmRelease) {
      suppressConfirmRelease = false;
      suppressPowerConfirmRelease = false;
      return false;
    }

    if (suppressPowerConfirmRelease) {
      suppressPowerConfirmRelease = false;
      return false;
    }

    const bool longPress = gpio.getHeldTime() >= SETTINGS.getPowerButtonLongPressDuration();
    const auto action = longPress ? static_cast<CrossPointSettings::SHORT_PWRBTN>(SETTINGS.longPwrBtn)
                                  : static_cast<CrossPointSettings::SHORT_PWRBTN>(SETTINGS.shortPwrBtn);
    return !isPowerButtonActionAvailableOutsideReader(action);
  }

  if (button == Button::Power) {
    if (!mapButton(button, &HalGPIO::wasReleased)) {
      return false;
    }

    if (suppressPowerRelease) {
      suppressPowerRelease = false;
      return false;
    }

    return true;
  }

  const uint8_t frontButton = mappedFrontButtonFor(button);
  return mapButton(button, &HalGPIO::wasReleased) ||
         (frontButton != kNoButton && wasFrontButtonHintTapped(frontButton));
}

bool MappedInputManager::isPressed(const Button button) const {
#ifdef SIMULATOR
  if (simulatorHeld[buttonIndex(button)]) {
    return true;
  }
#endif

  if (button == Button::Confirm) {
    if (mapButton(button, &HalGPIO::isPressed)) {
      return true;
    }

    if (!shouldMirrorPowerAsConfirmHold() || !gpio.isPressed(HalGPIO::BTN_POWER)) {
      return false;
    }

    return !isPowerButtonActionAvailableOutsideReader(
               static_cast<CrossPointSettings::SHORT_PWRBTN>(SETTINGS.shortPwrBtn)) ||
           gpio.getHeldTime() >= SETTINGS.getPowerButtonLongPressDuration();
  }

  return mapButton(button, &HalGPIO::isPressed);
}

bool MappedInputManager::wasAnyPressed() const {
#ifdef SIMULATOR
  if (std::any_of(simulatorPressed.begin(), simulatorPressed.end(), [](bool pressed) { return pressed; })) {
    return true;
  }
#endif
#if CROSSSMUDGE_APP_CAP_TOUCH
  int id = -1;
  if (wasRegistryTargetTouchedDown(TouchRegistry::Kind::Button, id)) {
    return true;
  }
#endif
  return gpio.wasAnyPressed();
}

bool MappedInputManager::wasAnyReleased() const {
#ifdef SIMULATOR
  if (std::any_of(simulatorReleased.begin(), simulatorReleased.end(), [](bool released) { return released; })) {
    return true;
  }
#endif
#if CROSSSMUDGE_APP_CAP_TOUCH
  int id = -1;
  if (wasRegistryTargetTapped(TouchRegistry::Kind::Button, id)) {
    return true;
  }
#endif
  return gpio.wasAnyReleased();
}

unsigned long MappedInputManager::getHeldTime() const {
  unsigned long heldTime = gpio.getHeldTime();
#if CROSSSMUDGE_APP_CAP_TOUCH
  if (!gpio.wasAnyPressed() && !gpio.wasAnyReleased() && touchHeldOverrideValid &&
      millis() - touchHeldOverrideAt <= TOUCH_HELD_OVERRIDE_WINDOW_MS) {
    heldTime = touchHeldOverrideMs;
  } else {
    touchHeldOverrideValid = false;
  }
#endif
#ifdef SIMULATOR
  const unsigned long now = millis();
  for (size_t i = 0; i < BUTTON_COUNT; i++) {
    if (simulatorHeld[i] && simulatorPressStart[i] > 0) {
      heldTime = std::max(heldTime, now - simulatorPressStart[i]);
    }
  }
#endif
  return heldTime;
}

MappedInputManager::Labels MappedInputManager::mapLabels(const char* back, const char* confirm, const char* previous,
                                                         const char* next) const {
  const bool useReaderMapping = readerMode && SETTINGS.readerFrontButtonsEnabled;
  const ButtonIndex btnBack = useReaderMapping ? SETTINGS.readerFrontButtonBack : SETTINGS.frontButtonBack;
  const ButtonIndex btnConfirm = useReaderMapping ? SETTINGS.readerFrontButtonConfirm : SETTINGS.frontButtonConfirm;
  const ButtonIndex btnLeft = useReaderMapping ? SETTINGS.readerFrontButtonLeft : SETTINGS.frontButtonLeft;
  const ButtonIndex btnRight = useReaderMapping ? SETTINGS.readerFrontButtonRight : SETTINGS.frontButtonRight;
  const ButtonIndex mappedBack = mapFrontButtonForReaderOrientation(btnBack, btnLeft, btnRight, readerMode);
  const ButtonIndex mappedConfirm = mapFrontButtonForReaderOrientation(btnConfirm, btnLeft, btnRight, readerMode);
  const ButtonIndex mappedLeft = mapFrontButtonForReaderOrientation(btnLeft, btnLeft, btnRight, readerMode);
  const ButtonIndex mappedRight = mapFrontButtonForReaderOrientation(btnRight, btnLeft, btnRight, readerMode);

  // Build the label order based on the configured hardware mapping.
  auto labelForHardware = [&](ButtonIndex hw) -> const char* {
    if (hw == mappedBack) return back;
    if (hw == mappedConfirm) return confirm;
    if (hw == mappedLeft) return previous;
    if (hw == mappedRight) return next;
    return "";
  };

  return {labelForHardware(HalGPIO::BTN_BACK), labelForHardware(HalGPIO::BTN_CONFIRM),
          labelForHardware(HalGPIO::BTN_LEFT), labelForHardware(HalGPIO::BTN_RIGHT)};
}

int MappedInputManager::getPressedFrontButton() const {
  // Scan the raw front buttons in hardware order.
  // This bypasses remapping so the remap activity can capture physical presses.
  if (gpio.wasPressed(HalGPIO::BTN_BACK)) {
    return HalGPIO::BTN_BACK;
  }
  if (gpio.wasPressed(HalGPIO::BTN_CONFIRM)) {
    return HalGPIO::BTN_CONFIRM;
  }
  if (gpio.wasPressed(HalGPIO::BTN_LEFT)) {
    return HalGPIO::BTN_LEFT;
  }
  if (gpio.wasPressed(HalGPIO::BTN_RIGHT)) {
    return HalGPIO::BTN_RIGHT;
  }
#if CROSSSMUDGE_APP_CAP_TOUCH
  int id = -1;
  if (wasRegistryTargetTouchedDown(TouchRegistry::Kind::Button, id) && id >= HalGPIO::BTN_BACK &&
      id <= HalGPIO::BTN_RIGHT) {
    return id;
  }
#endif
  return -1;
}

int MappedInputManager::getReleasedFrontButton() const {
  // Scan the raw front buttons in hardware order.
  // This bypasses remapping for screens whose labels are fixed to physical slots.
  if (gpio.wasReleased(HalGPIO::BTN_BACK)) {
    return HalGPIO::BTN_BACK;
  }
  if (gpio.wasReleased(HalGPIO::BTN_CONFIRM)) {
    return HalGPIO::BTN_CONFIRM;
  }
  if (gpio.wasReleased(HalGPIO::BTN_LEFT)) {
    return HalGPIO::BTN_LEFT;
  }
  if (gpio.wasReleased(HalGPIO::BTN_RIGHT)) {
    return HalGPIO::BTN_RIGHT;
  }
#if CROSSSMUDGE_APP_CAP_TOUCH
  int id = -1;
  if (wasRegistryTargetTapped(TouchRegistry::Kind::Button, id) && id >= HalGPIO::BTN_BACK && id <= HalGPIO::BTN_RIGHT) {
    return id;
  }
#endif
  return -1;
}

bool MappedInputManager::isFrontButtonPressed(const uint8_t buttonIndex) const { return gpio.isPressed(buttonIndex); }

#ifdef SIMULATOR
void MappedInputManager::simulatorInjectPress(Button button) {
  const size_t idx = buttonIndex(button);
  simulatorPressed[idx] = true;
  simulatorReleased[idx] = false;
  simulatorHeld[idx] = true;
  simulatorPressStart[idx] = millis();
}

void MappedInputManager::simulatorInjectRelease(Button button) {
  const size_t idx = buttonIndex(button);
  simulatorPressed[idx] = false;
  simulatorReleased[idx] = true;
  simulatorHeld[idx] = false;
}

void MappedInputManager::simulatorClearInputFrame() {
  simulatorPressed.fill(false);
  simulatorReleased.fill(false);
#if CROSSSMUDGE_APP_CAP_TOUCH
  simulatorTouch.pressedThisFrame = false;
  simulatorTouch.releasedThisFrame = false;
#endif
}

#if CROSSSMUDGE_APP_CAP_TOUCH
void MappedInputManager::simulatorInjectTouchDown(const int x, const int y) {
  simulatorTouch.pressed = true;
  simulatorTouch.pressedThisFrame = true;
  simulatorTouch.releasedThisFrame = false;
  simulatorTouch.startX = x;
  simulatorTouch.startY = y;
  simulatorTouch.currentX = x;
  simulatorTouch.currentY = y;
  simulatorTouch.startedAt = millis();
}

void MappedInputManager::simulatorInjectTouchMove(const int x, const int y) {
  if (!simulatorTouch.pressed) return;
  simulatorTouch.currentX = x;
  simulatorTouch.currentY = y;
}

void MappedInputManager::simulatorInjectTouchRelease(const int x, const int y) {
  if (!simulatorTouch.pressed) return;
  simulatorTouch.currentX = x;
  simulatorTouch.currentY = y;
  simulatorTouch.pressed = false;
  simulatorTouch.releasedThisFrame = true;
}
#endif
#endif
