#pragma once

#include <HalGPIO.h>

#include <array>

class GfxRenderer;

class MappedInputManager {
 public:
  enum class Button { Back, Confirm, Left, Right, Up, Down, Power, PageBack, PageForward };
  static constexpr size_t BUTTON_COUNT = static_cast<size_t>(Button::PageForward) + 1;
  enum class SwipeDir { None, Left, Right, Up, Down };

  struct Labels {
    const char* btn1;
    const char* btn2;
    const char* btn3;
    const char* btn4;
  };

  explicit MappedInputManager(HalGPIO& gpio, const GfxRenderer& renderer) : gpio(gpio), renderer(renderer) {}

  // Enable/disable reader-specific front button mapping.
  // Call with true in reader activity onEnter(), false in onExit().
  void setReaderMode(bool enabled) { readerMode = enabled; }
  void setPowerAsConfirmInReaderMode(bool enabled) { powerAsConfirmInReaderMode = enabled; }
#if CROSSSMUDGE_APP_CAP_TOUCH
  void setReaderTouchscreenOverride(bool enabled) { readerTouchscreenOverride = enabled; }
#else
  constexpr void setReaderTouchscreenOverride(bool) {}
#endif

  void update() const { gpio.update(); }
  void suppressNextBackRelease() { suppressBackRelease = true; }
  void suppressNextConfirmRelease() { suppressConfirmRelease = true; }
  void suppressNextPowerRelease() { suppressPowerRelease = true; }
  void suppressNextPowerConfirmRelease() { suppressPowerConfirmRelease = true; }
  bool wasPressed(Button button) const;
  bool wasReleased(Button button) const;
  bool isPressed(Button button) const;
  const GfxRenderer& getRenderer() const { return renderer; }
  enum class RowTouch : uint8_t { None, Down, Tap };
#if CROSSSMUDGE_APP_CAP_TOUCH
  bool hasTouch() const;
  bool hasTouchHardware() const;
  // True on boards with a capacitive home key (X4 Pro), where the bottom-edge
  // up-swipe is the reader-menu gesture rather than the exit-to-home gesture.
  bool hasHomeKey() const { return hasHomeKeyHardware(); }
  bool wasScreenTapped(int& x, int& y) const;
  // Also reports how long the finger was held before release.
  bool wasScreenTapped(int& x, int& y, unsigned long& heldMs) const;
  bool isScreenTouchLongPress(int& x, int& y, unsigned long thresholdMs) const;
  bool isInVerticalEdgeGestureZone(int y) const;
  bool wasScreenTouchDown(int& x, int& y) const;
  bool isScreenTouchTapCandidate(int& x, int& y, unsigned long& heldMs) const;
  bool isScreenTouchHeld(int& x, int& y) const;
  bool wasItemTapped(int& id) const;
  bool wasItemTouchedDown(int& id) const;
  bool wasTabTapped(int& id) const;
  bool wasTabTouchedDown(int& id) const;
  bool wasCoverTapped(int& id) const;
  bool wasCoverTouchedDown(int& id) const;
  // Raw release edge, also true when the contact ended in a swipe or drag-off
  // (which wasScreenTapped never reports). InputSnapshot builders forward it
  // off-target so FreeInkUI routing clears its pressed-element state.
  bool wasScreenTouchReleased() const;
  bool wasTapInRect(int x, int y, int width, int height) const;
  bool wasListItemTapped(int& index, int itemCount, int selectedIndex, int listTop, int listHeight,
                         bool hasSubtitle) const;
  bool wasListItemTouchedDown(int& index, int itemCount, int selectedIndex, int listTop, int listHeight,
                              bool hasSubtitle) const;
  bool isListItemTouchLongPressed(int& index, int itemCount, int selectedIndex, int listTop, int listHeight,
                                  bool hasSubtitle, unsigned long thresholdMs) const;
  void suppressNextTouchTap() { suppressTouchTap = true; }
  RowTouch rowTouch(int& row, int top, int rowStep, int rowCount, int xStart = 0, int xEnd = INT32_MAX,
                    int rowHeight = 0) const;
  RowTouch colTouch(int& col, int left, int colStep, int colCount, int yStart, int yEnd, int colWidth = 0) const;
  SwipeDir wasSwipe() const;
  bool wasSwipeWithPoints(SwipeDir& direction, int& startX, int& startY, int& endX, int& endY) const;
  bool wasLeftEdgeGesture() const;
  // Exit-to-home intent. Boards with a capacitive home key (X4 Pro) use the
  // key's press edge; everywhere else it's the bottom-edge up-swipe.
  bool wasHomeGesture() const;
  // Contextual menu intent (the reader menu). Home-key boards move this to the
  // bottom-edge up-swipe (freed by the home key); others keep the top-edge
  // down-swipe.
  bool wasMenuGesture() const;
  // Frontlight quick panel: top-edge down-swipe, only on home-key boards where
  // that edge is no longer the menu gesture.
  bool wasLightPanelGesture() const;
  // Full-screen vertical reader gestures. These preserve each board's existing
  // action mapping, but intentionally do not apply to reader submenus or lists.
  bool wasReaderMenuGesture() const;
  bool wasReaderHomeGesture() const;
  bool wasReaderLightPanelGesture() const;
  // Reader-menu shortcut: a long press of the capacitive home key.
  bool wasReaderMenuHold() const;
#else
  constexpr bool hasTouch() const { return false; }
  constexpr bool hasTouchHardware() const { return false; }
  constexpr bool hasHomeKey() const { return false; }
  constexpr bool wasScreenTapped(int&, int&) const { return false; }
  constexpr bool wasScreenTapped(int&, int&, unsigned long&) const { return false; }
  constexpr bool isScreenTouchLongPress(int&, int&, unsigned long) const { return false; }
  constexpr bool isInVerticalEdgeGestureZone(int) const { return false; }
  constexpr bool wasScreenTouchDown(int&, int&) const { return false; }
  constexpr bool isScreenTouchTapCandidate(int&, int&, unsigned long&) const { return false; }
  constexpr bool isScreenTouchHeld(int&, int&) const { return false; }
  constexpr bool wasItemTapped(int&) const { return false; }
  constexpr bool wasItemTouchedDown(int&) const { return false; }
  constexpr bool wasTabTapped(int&) const { return false; }
  constexpr bool wasTabTouchedDown(int&) const { return false; }
  constexpr bool wasCoverTapped(int&) const { return false; }
  constexpr bool wasCoverTouchedDown(int&) const { return false; }
  constexpr bool wasScreenTouchReleased() const { return false; }
  constexpr bool wasTapInRect(int, int, int, int) const { return false; }
  constexpr bool wasListItemTapped(int&, int, int, int, int, bool) const { return false; }
  constexpr bool wasListItemTouchedDown(int&, int, int, int, int, bool) const { return false; }
  constexpr bool isListItemTouchLongPressed(int&, int, int, int, int, bool, unsigned long) const { return false; }
  constexpr void suppressNextTouchTap() {}
  constexpr RowTouch rowTouch(int&, int, int, int, int = 0, int = INT32_MAX, int = 0) const { return RowTouch::None; }
  constexpr RowTouch colTouch(int&, int, int, int, int, int, int = 0) const { return RowTouch::None; }
  constexpr SwipeDir wasSwipe() const { return SwipeDir::None; }
  constexpr bool wasSwipeWithPoints(SwipeDir& direction, int&, int&, int&, int&) const {
    direction = SwipeDir::None;
    return false;
  }
  constexpr bool wasLeftEdgeGesture() const { return false; }
  constexpr bool wasHomeGesture() const { return false; }
  constexpr bool wasMenuGesture() const { return false; }
  constexpr bool wasLightPanelGesture() const { return false; }
  constexpr bool wasReaderMenuGesture() const { return false; }
  constexpr bool wasReaderHomeGesture() const { return false; }
  constexpr bool wasReaderLightPanelGesture() const { return false; }
  constexpr bool wasReaderMenuHold() const { return false; }
#endif
  bool wasAnyPressed() const;
  bool wasAnyReleased() const;
  unsigned long getHeldTime() const;
  Labels mapLabels(const char* back, const char* confirm, const char* previous, const char* next) const;
  // Returns the raw front button index that was pressed this frame (or -1 if none).
  int getPressedFrontButton() const;
  // Returns the raw front button index that was released this frame (or -1 if none).
  int getReleasedFrontButton() const;
  bool isFrontButtonPressed(uint8_t buttonIndex) const;

#ifdef SIMULATOR
  void simulatorInjectPress(Button button);
  void simulatorInjectRelease(Button button);
  void simulatorClearInputFrame();
#if CROSSSMUDGE_APP_CAP_TOUCH
  void simulatorInjectTouchDown(int x, int y);
  void simulatorInjectTouchMove(int x, int y);
  void simulatorInjectTouchRelease(int x, int y);
#endif
#endif

 private:
  HalGPIO& gpio;
  const GfxRenderer& renderer;
  bool readerMode = false;
  bool powerAsConfirmInReaderMode = false;
#if CROSSSMUDGE_APP_CAP_TOUCH
  bool readerTouchscreenOverride = false;
#endif
  mutable bool suppressBackRelease = false;
  mutable bool suppressConfirmRelease = false;
  mutable bool suppressPowerRelease = false;
  mutable bool suppressPowerConfirmRelease = false;
#if CROSSSMUDGE_APP_CAP_TOUCH
  mutable bool suppressTouchTap = false;
#endif
#ifdef SIMULATOR
  std::array<bool, BUTTON_COUNT> simulatorPressed{};
  std::array<bool, BUTTON_COUNT> simulatorReleased{};
  std::array<bool, BUTTON_COUNT> simulatorHeld{};
  std::array<unsigned long, BUTTON_COUNT> simulatorPressStart{};
#if CROSSSMUDGE_APP_CAP_TOUCH
  struct SimulatorTouch {
    bool pressed = false;
    bool pressedThisFrame = false;
    bool releasedThisFrame = false;
    int startX = 0;
    int startY = 0;
    int currentX = 0;
    int currentY = 0;
    unsigned long startedAt = 0;
  };
  mutable SimulatorTouch simulatorTouch;
#endif
#endif

  bool mapButton(Button button, bool (HalGPIO::*fn)(uint8_t) const) const;
  uint8_t mappedFrontButtonFor(Button button) const;
  bool shouldUsePowerAsConfirmFallback() const;
  bool shouldMirrorPowerAsConfirmHold() const;
#if CROSSSMUDGE_APP_CAP_TOUCH
  bool touchInputEnabled() const;
  bool hasHomeKeyHardware() const;
  bool wasBackGesture() const;
  bool wasTopEdgeDownSwipe() const;
  bool wasBottomEdgeUpSwipe() const;
  // Fetch the pending swipe (if any) and map both endpoints to logical screen coords.
  bool decodeSwipe(int& sx, int& sy, int& ex, int& ey) const;
  bool listItemFromPoint(int x, int y, int& index, int itemCount, int selectedIndex, int listTop, int listHeight,
                         bool hasSubtitle) const;
  bool wasRegistryTargetTapped(uint8_t kind, int& id) const;
  bool wasRegistryTargetTouchedDown(uint8_t kind, int& id) const;
  bool wasFrontButtonHintTapped(uint8_t buttonIndex) const;
  bool wasFrontButtonHintTouchedDown(uint8_t buttonIndex) const;
  void rememberTouchHeldTime() const;
  mutable bool touchHeldOverrideValid = false;
  mutable unsigned long touchHeldOverrideMs = 0;
  mutable unsigned long touchHeldOverrideAt = 0;
#else
  constexpr bool wasBackGesture() const { return false; }
  constexpr bool wasFrontButtonHintTapped(uint8_t) const { return false; }
  constexpr bool wasFrontButtonHintTouchedDown(uint8_t) const { return false; }
#endif
};
