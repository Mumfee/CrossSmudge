#include "IntervalSelectionActivity.h"

#include <GfxRenderer.h>
#include <HalGPIO.h>
#include <I18n.h>

#include <algorithm>
#include <cstdio>
#include <utility>

#include "DeviceCapabilities.h"
#include "components/TouchHeaderBackButton.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
constexpr int TOUCH_STEP_BUTTON_SIZE = 56;
constexpr int TOUCH_STEP_BUTTON_GAP = 32;
constexpr int TOUCH_ACTION_BUTTON_WIDTH = 120;
constexpr int TOUCH_ACTION_BUTTON_HEIGHT = 48;

Rect touchStepButtonRect(const Rect& screen, const int index) {
  const int totalWidth = TOUCH_STEP_BUTTON_SIZE * 4 + TOUCH_STEP_BUTTON_GAP * 3;
  const int x = screen.x + (screen.width - totalWidth) / 2 + index * (TOUCH_STEP_BUTTON_SIZE + TOUCH_STEP_BUTTON_GAP);
  return Rect{x, 220, TOUCH_STEP_BUTTON_SIZE, TOUCH_STEP_BUTTON_SIZE};
}

Rect touchActionButtonRect(const Rect& screen, const bool confirm) {
  constexpr int sideMargin = 46;
  return Rect{confirm ? screen.x + screen.width - sideMargin - TOUCH_ACTION_BUTTON_WIDTH : screen.x + sideMargin,
              screen.y + screen.height - TOUCH_ACTION_BUTTON_HEIGHT - 28, TOUCH_ACTION_BUTTON_WIDTH,
              TOUCH_ACTION_BUTTON_HEIGHT};
}

bool contains(const Rect& rect, const int x, const int y) {
  return x >= rect.x && x < rect.x + rect.width && y >= rect.y && y < rect.y + rect.height;
}

void formatCompactSeconds(const int seconds, char* buf, const size_t len) {
  if (seconds < 60) {
    snprintf(buf, len, "%ds", seconds);
  } else if (seconds % 60 == 0) {
    snprintf(buf, len, "%dm", seconds / 60);
  } else {
    snprintf(buf, len, "%dm %ds", seconds / 60, seconds % 60);
  }
}
}  // namespace

int IntervalSelectionActivity::clampedValue(const int candidate) const {
  return std::clamp(candidate, minValue, maxValue);
}

void IntervalSelectionActivity::onEnter() {
  Activity::onEnter();
  if (overrideDisabledReaderTouchscreen) {
    mappedInput.setReaderTouchscreenOverride(true);
  }
  value = clampedValue(value);
  requestUpdate();
}

void IntervalSelectionActivity::onExit() {
  if (overrideDisabledReaderTouchscreen) {
    mappedInput.setReaderTouchscreenOverride(false);
  }
  Activity::onExit();
}

void IntervalSelectionActivity::adjustValue(const int delta) {
  value = clampedValue(value + delta);
  requestUpdate();
}

void IntervalSelectionActivity::drawStepHintLine(const int y, const StrId labelId, const int step) {
  char stepText[24];
  if (valueFormatId != StrId::STR_NONE_OPT) {
    snprintf(stepText, sizeof(stepText), I18N.get(valueFormatId), static_cast<unsigned int>(step));
  } else {
    snprintf(stepText, sizeof(stepText), "%d", step);
  }
  char line[64];
  snprintf(line, sizeof(line), "%s %s", I18N.get(labelId), stepText);
  renderer.drawCenteredText(SMALL_FONT_ID, y, line, true);
}

void IntervalSelectionActivity::loop() {
  if (ignoreConfirmRelease) {
    const bool confirmReleased = mappedInput.wasReleased(MappedInputManager::Button::Confirm);
    if (confirmReleased || !mappedInput.isPressed(MappedInputManager::Button::Confirm)) {
      ignoreConfirmRelease = false;
      return;
    }
  }

  if (showTouchHeaderBackButton && TouchHeaderBackButton::wasTapped(mappedInput, renderer)) {
    ActivityResult result;
    result.isCancelled = true;
    setResult(std::move(result));
    finish();
    return;
  }

  int tx = 0;
  int ty = 0;
  const int screenWidth = renderer.getScreenWidth();
  const Rect touchScreen = UITheme::getInstance().getScreenSafeArea(renderer, !mappedInput.hasTouchHardware(), false);
  const int barWidth = std::min(360, std::max(0, screenWidth - 40));
  constexpr int barHeight = 16;
  const int barX = std::max(0, (screenWidth - barWidth) / 2);
  const int barY = 140;

  // Live drag on the slider: once a touch lands on the bar, the value follows the
  // finger until release. Runs before the Back/Confirm handlers because the release
  // of a drag can also register as a swipe (e.g. the left-edge rightward back
  // gesture) — the drag must consume it so it can't cancel or confirm the dialog.
  if (mappedInput.isScreenTouchHeld(tx, ty)) {
    if (draggingBar || (ty >= barY - 20 && ty < barY + barHeight + 20 && tx >= barX && tx < barX + barWidth)) {
      draggingBar = true;
      const int range = std::max(1, maxValue - minValue);
      const int dragged =
          clampedValue(minValue + std::clamp(tx - barX, 0, barWidth - 1) * range / std::max(1, barWidth - 1));
      if (dragged != value) {
        value = dragged;
        requestUpdate();
      }
      return;
    }
  } else if (draggingBar) {
    // Release frame of a drag: swallow the tap/swipe events it produced.
    draggingBar = false;
    return;
  }

  // Cancel and Confirm act on touch-down. Unlike the adjustment controls, these
  // are terminal actions, so waiting for a release can make a perfectly still
  // tap feel ignored while the controller settles its release event.
  if (mappedInput.hasTouch() && mappedInput.wasScreenTouchDown(tx, ty)) {
    if (contains(touchActionButtonRect(touchScreen, false), tx, ty)) {
      ActivityResult result;
      result.isCancelled = true;
      setResult(std::move(result));
      finish();
      return;
    }
    if (contains(touchActionButtonRect(touchScreen, true), tx, ty)) {
      setResult(IntervalResult{static_cast<uint32_t>(value)});
      finish();
      return;
    }
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    ActivityResult result;
    result.isCancelled = true;
    setResult(std::move(result));
    finish();
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    setResult(IntervalResult{static_cast<uint32_t>(value)});
    finish();
    return;
  }

  if (mappedInput.wasScreenTapped(tx, ty)) {
    if (ty >= barY - 20 && ty < barY + barHeight + 20 && tx >= barX && tx < barX + barWidth) {
      const int range = std::max(1, maxValue - minValue);
      value = clampedValue(minValue + (tx - barX) * range / std::max(1, barWidth - 1));
      requestUpdate();
      return;
    }

    if (mappedInput.hasTouch()) {
      for (int index = 0; index < 4; ++index) {
        if (!contains(touchStepButtonRect(touchScreen, index), tx, ty)) continue;
        constexpr int deltas[] = {-1, -1, 1, 1};
        const int step = (index == 0 || index == 3) ? largeStep : smallStep;
        adjustValue(deltas[index] * step);
        return;
      }

      return;
    }

    if (ty >= renderer.getScreenHeight() - 80) {
      if (tx < screenWidth / 3) {
        ActivityResult result;
        result.isCancelled = true;
        setResult(std::move(result));
        finish();
      } else if (tx > screenWidth * 2 / 3) {
        setResult(IntervalResult{static_cast<uint32_t>(value)});
        finish();
      }
      return;
    }
  }

  buttonNavigator.onPressAndContinuous({MappedInputManager::Button::Left}, [this] { adjustValue(-smallStep); });
  buttonNavigator.onPressAndContinuous({MappedInputManager::Button::Right}, [this] { adjustValue(smallStep); });

  // On edge-button boards (X3, X4 Pro) the side buttons sit on the left/right edges of the screen rather
  // than as a vertical up/down rocker (X4), so BTN_UP is physically the left button and BTN_DOWN the right
  // one. Flip the large-step direction there so the left button decreases and the right button increases.
  const int upDelta = deviceHasEdgeSideButtons(gpio) ? -largeStep : largeStep;
  const int downDelta = deviceHasEdgeSideButtons(gpio) ? largeStep : -largeStep;
  buttonNavigator.onPressAndContinuous({MappedInputManager::Button::Up}, [this, upDelta] { adjustValue(upDelta); });
  buttonNavigator.onPressAndContinuous({MappedInputManager::Button::Down},
                                       [this, downDelta] { adjustValue(downDelta); });
}

void IntervalSelectionActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto& metrics = UITheme::getInstance().getMetrics();
  const int screenWidth = renderer.getScreenWidth();
  const Rect safe = UITheme::getInstance().getScreenSafeArea(renderer, !mappedInput.hasTouchHardware(), false);
  const Rect touchScreen = safe;

  // The Auto Page Turn picker has no touch back button. Keep its title in the
  // normal-height header rather than centering it in the taller touch-navigation
  // header, which puts the title against the divider.
  Rect header = showTouchHeaderBackButton ? TouchHeaderBackButton::headerRect(renderer, mappedInput)
                                          : TouchHeaderBackButton::standardHeaderRect(renderer);
  header.x = safe.x;
  header.width = safe.width;
  if (showTouchHeaderBackButton && mappedInput.hasTouchHardware()) {
    TouchHeaderBackButton::draw(renderer, header, I18N.get(titleId), readerActivity);
  } else {
    GUI.drawHeader(renderer, header, I18N.get(titleId), nullptr, readerActivity);
  }

  char formattedValue[32];
  if (maxBoundaryLabelId != StrId::STR_NONE_OPT && value == maxValue) {
    snprintf(formattedValue, sizeof(formattedValue), "%s", I18N.get(maxBoundaryLabelId));
  } else if (showPercentValue) {
    snprintf(formattedValue, sizeof(formattedValue), "%d%%", value);
  } else if (valueFormatId == StrId::STR_SECONDS_VALUE_FORMAT) {
    formatCompactSeconds(value, formattedValue, sizeof(formattedValue));
  } else if (valueFormatId != StrId::STR_NONE_OPT) {
    snprintf(formattedValue, sizeof(formattedValue), I18N.get(valueFormatId), static_cast<unsigned int>(value));
  } else {
    snprintf(formattedValue, sizeof(formattedValue), "%d", value);
  }
  renderer.drawCenteredText(UI_12_FONT_ID, 90, formattedValue, true, EpdFontFamily::BOLD);

  const int barWidth = std::min(360, std::max(0, screenWidth - 40));
  constexpr int barHeight = 16;
  const int barX = std::max(0, (screenWidth - barWidth) / 2);
  const int barY = 140;

  renderer.drawRect(barX, barY, barWidth, barHeight);

  const int range = std::max(1, maxValue - minValue);
  const int fillWidth = (barWidth - 4) * (value - minValue) / range;
  if (fillWidth > 0) {
    renderer.fillRect(barX + 2, barY + 2, fillWidth, barHeight - 4);
  }

  const int knobX = std::max(barX + 2, barX + 2 + fillWidth - 2);
  renderer.fillRect(knobX, barY - 4, 4, barHeight + 8, true);

  if (mappedInput.hasTouch()) {
    auto drawButton = [&](const Rect& rect) {
      renderer.fillRectDither(rect.x, rect.y, rect.width, rect.height, Color::White);
      renderer.drawRect(rect.x, rect.y, rect.width, rect.height, true);
    };
    auto drawChevron = [&](const Rect& rect, const bool pointsRight, const bool doubleChevron) {
      const int centreY = rect.y + rect.height / 2;
      const int halfHeight = 12;
      const int firstX = rect.x + (doubleChevron ? 13 : 20);
      const int spacing = 14;
      const int chevronCount = doubleChevron ? 2 : 1;
      for (int i = 0; i < chevronCount; ++i) {
        const int x = firstX + i * spacing;
        if (pointsRight) {
          renderer.drawLine(x, centreY - halfHeight, x + 12, centreY, 2, true);
          renderer.drawLine(x + 12, centreY, x, centreY + halfHeight, 2, true);
        } else {
          renderer.drawLine(x + 12, centreY - halfHeight, x, centreY, 2, true);
          renderer.drawLine(x, centreY, x + 12, centreY + halfHeight, 2, true);
        }
      }
    };

    for (int index = 0; index < 4; ++index) {
      const Rect rect = touchStepButtonRect(touchScreen, index);
      drawButton(rect);
      drawChevron(rect, index >= 2, index == 0 || index == 3);
    }

    const Rect cancelRect = touchActionButtonRect(touchScreen, false);
    const Rect confirmRect = touchActionButtonRect(touchScreen, true);
    drawButton(cancelRect);
    drawButton(confirmRect);
    auto drawButtonLabel = [&](const Rect& rect, const char* label) {
      const int x = rect.x + (rect.width - renderer.getTextWidth(UI_10_FONT_ID, label)) / 2;
      const int y = rect.y + (rect.height - renderer.getLineHeight(UI_10_FONT_ID)) / 2;
      renderer.drawText(UI_10_FONT_ID, x, y, label);
    };
    drawButtonLabel(cancelRect, tr(STR_CANCEL));
    drawButtonLabel(confirmRect, tr(STR_CONFIRM));
  } else {
    // Two-line step hint: front buttons do the small step, side buttons the large step. Built from
    // separate label + value strings (rather than splitting one localized sentence) so the layout
    // doesn't depend on translators preserving a hidden separator.
    drawStepHintLine(barY + 30, StrId::STR_STEP_HINT_FRONT, smallStep);
    drawStepHintLine(barY + 52, StrId::STR_STEP_HINT_SIDE, largeStep);

    const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), "-", "+");
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4, readerActivity);
  }

  renderer.displayBuffer();
}
