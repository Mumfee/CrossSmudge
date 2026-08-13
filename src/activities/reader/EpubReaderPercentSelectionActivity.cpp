#include "EpubReaderPercentSelectionActivity.h"

#include <GfxRenderer.h>
#include <HalGPIO.h>
#include <I18n.h>

#include <algorithm>
#include <cstdio>

#include "DeviceCapabilities.h"
#include "MappedInputManager.h"
#include "components/TouchHeaderBackButton.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
// Fine/coarse slider step sizes for percent adjustments.
constexpr int kSmallStep = 1;
constexpr int kLargeStep = 10;
constexpr int kTouchStepButtonSize = 56;
constexpr int kTouchStepButtonGap = 32;
constexpr int kTouchActionButtonWidth = 120;
constexpr int kTouchActionButtonHeight = 48;

Rect touchStepButtonRect(const Rect& screen, const int y, const int index) {
  const int totalWidth = kTouchStepButtonSize * 4 + kTouchStepButtonGap * 3;
  const int x = screen.x + (screen.width - totalWidth) / 2 + index * (kTouchStepButtonSize + kTouchStepButtonGap);
  return Rect{x, y, kTouchStepButtonSize, kTouchStepButtonSize};
}

Rect touchActionButtonRect(const Rect& screen, const bool confirm) {
  constexpr int sideMargin = 46;
  return Rect{confirm ? screen.x + screen.width - sideMargin - kTouchActionButtonWidth : screen.x + sideMargin,
              screen.y + screen.height - kTouchActionButtonHeight - 28, kTouchActionButtonWidth,
              kTouchActionButtonHeight};
}

bool contains(const Rect& rect, const int x, const int y) {
  return x >= rect.x && x < rect.x + rect.width && y >= rect.y && y < rect.y + rect.height;
}
}  // namespace

void EpubReaderPercentSelectionActivity::onEnter() {
  Activity::onEnter();
  // Set up rendering task and mark first frame dirty.
  requestUpdate();
}

void EpubReaderPercentSelectionActivity::onExit() { Activity::onExit(); }

void EpubReaderPercentSelectionActivity::adjustPercent(const int delta) {
  // Wrap using a 100-value ring (0% and 100% are the same wrap point), but keep 100 as the
  // natural landing value when reached without crossing the boundary (e.g. 90 + 10 = 100).
  const int raw = percent + delta;
  if (raw > 0 && raw % 100 == 0) {
    percent = 100;
  } else {
    percent = ((raw % 100) + 100) % 100;
  }
  requestUpdate();
}

void EpubReaderPercentSelectionActivity::loop() {
  auto& theme = UITheme::getInstance();
  auto metrics = theme.getMetrics();
  Rect screen = theme.getScreenSafeArea(renderer, true, false);
  const int contentTop =
      screen.y + metrics.topPadding + TouchHeaderBackButton::height(metrics, mappedInput) + metrics.verticalSpacing * 4;
  constexpr int barWidth = 360;
  constexpr int barHeight = 16;
  const int barX = screen.x + (screen.width - barWidth) / 2;
  const int barY = contentTop + metrics.verticalSpacing * 2;
  int tx = 0;
  int ty = 0;

  // Live drag on the slider: once a touch lands on the bar, the percent follows the
  // finger until release. Runs before the Back handler because the release of a drag
  // can also register as a swipe (e.g. the left-edge rightward back gesture) — the
  // drag must consume it so it can't cancel the dialog or step the percent.
  if (mappedInput.isScreenTouchHeld(tx, ty)) {
    if (draggingBar ||
        (tx >= barX - 20 && tx < barX + barWidth + 20 && ty >= barY - 24 && ty < barY + barHeight + 24)) {
      draggingBar = true;
      const int dragged = std::clamp((tx - barX) * 100 / barWidth, 0, 100);
      if (dragged != percent) {
        percent = dragged;
        requestUpdate();
      }
      return;
    }
  } else if (draggingBar) {
    // Release frame of a drag: swallow the tap/swipe events it produced.
    draggingBar = false;
    return;
  }

  const Rect header{screen.x, screen.y + metrics.topPadding, screen.width,
                    TouchHeaderBackButton::height(metrics, mappedInput)};
  if (TouchHeaderBackButton::wasTapped(mappedInput, header)) {
    ActivityResult result;
    result.isCancelled = true;
    setResult(std::move(result));
    finish();
    return;
  }

  if (mappedInput.hasTouch() && mappedInput.wasScreenTouchDown(tx, ty)) {
    if (contains(touchActionButtonRect(screen, false), tx, ty)) {
      ActivityResult result;
      result.isCancelled = true;
      setResult(std::move(result));
      finish();
      return;
    }
    if (contains(touchActionButtonRect(screen, true), tx, ty)) {
      setResult(PercentResult{percent});
      finish();
      return;
    }
  }

  // Back cancels, confirm selects, arrows adjust the percent.
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    ActivityResult result;
    result.isCancelled = true;
    setResult(std::move(result));
    finish();
    return;
  }

  if (mappedInput.wasScreenTapped(tx, ty)) {
    if (tx >= barX - 20 && tx < barX + barWidth + 20 && ty >= barY - 24 && ty < barY + barHeight + 24) {
      percent = std::clamp((tx - barX) * 100 / barWidth, 0, 100);
      requestUpdate();
      return;
    }
    if (mappedInput.hasTouch()) {
      constexpr int deltas[] = {-1, -1, 1, 1};
      for (int index = 0; index < 4; ++index) {
        if (!contains(touchStepButtonRect(screen, barY + 80, index), tx, ty)) continue;
        const int step = (index == 0 || index == 3) ? kLargeStep : kSmallStep;
        adjustPercent(deltas[index] * step);
        return;
      }
      return;
    }
  }

  const auto swipe = mappedInput.wasSwipe();
  if (swipe == MappedInputManager::SwipeDir::Right) {
    adjustPercent(kLargeStep);
    return;
  }
  if (swipe == MappedInputManager::SwipeDir::Left) {
    adjustPercent(-kLargeStep);
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    setResult(PercentResult{percent});
    finish();
    return;
  }

  buttonNavigator.onPressAndContinuous({MappedInputManager::Button::Left}, [this] { adjustPercent(-kSmallStep); });
  buttonNavigator.onPressAndContinuous({MappedInputManager::Button::Right}, [this] { adjustPercent(kSmallStep); });

  // On edge-button boards (X3, X4 Pro) the side buttons sit on the left/right edges of the screen rather
  // than as a vertical up/down rocker (X4), so BTN_UP is physically the left button and BTN_DOWN the right
  // one. Flip the large-step direction there so the left button decreases and the right button increases.
  const int upDelta = deviceHasEdgeSideButtons(gpio) ? -kLargeStep : kLargeStep;
  const int downDelta = deviceHasEdgeSideButtons(gpio) ? kLargeStep : -kLargeStep;
  buttonNavigator.onPressAndContinuous({MappedInputManager::Button::Up}, [this, upDelta] { adjustPercent(upDelta); });
  buttonNavigator.onPressAndContinuous({MappedInputManager::Button::Down},
                                       [this, downDelta] { adjustPercent(downDelta); });
}

void EpubReaderPercentSelectionActivity::render(RenderLock&&) {
  renderer.clearScreen();

  auto& theme = UITheme::getInstance();
  auto metrics = theme.getMetrics();
  Rect screen = theme.getScreenSafeArea(renderer, true, false);

  const Rect header{screen.x, screen.y + metrics.topPadding, screen.width,
                    TouchHeaderBackButton::height(metrics, mappedInput)};
  if (mappedInput.hasTouchHardware()) {
    TouchHeaderBackButton::draw(renderer, header, tr(STR_GO_TO_PERCENT), true);
  } else {
    GUI.drawHeader(renderer, header, tr(STR_GO_TO_PERCENT), nullptr, true);
  }

  const int contentTop =
      screen.y + metrics.topPadding + TouchHeaderBackButton::height(metrics, mappedInput) + metrics.verticalSpacing * 4;

  const std::string percentText = std::to_string(percent) + "%";
  UITheme::drawCenteredText(renderer, screen, UI_12_FONT_ID, contentTop, percentText.c_str(), true,
                            EpdFontFamily::BOLD);

  // Draw slider track.
  constexpr int barWidth = 360;
  constexpr int barHeight = 16;
  const int barX = screen.x + (screen.width - barWidth) / 2;
  const int barY = contentTop + metrics.verticalSpacing * 2;

  renderer.drawRect(barX, barY, barWidth, barHeight);

  // Fill slider based on percent.
  const int fillWidth = (barWidth - 4) * percent / 100;
  if (fillWidth > 0) {
    renderer.fillRect(barX + 2, barY + 2, fillWidth, barHeight - 4);
  }

  // Draw a simple knob centered at the current percent.
  const int knobX = barX + 2 + fillWidth - 2;
  renderer.fillRect(knobX, barY - 4, 4, barHeight + 8, true);

  if (mappedInput.hasTouch()) {
    auto drawButton = [&](const Rect& rect) {
      renderer.fillRectDither(rect.x, rect.y, rect.width, rect.height, Color::White);
      renderer.drawRect(rect.x, rect.y, rect.width, rect.height, true);
    };
    auto drawChevron = [&](const Rect& rect, const bool pointsRight, const bool doubleChevron) {
      const int centreY = rect.y + rect.height / 2;
      const int firstX = rect.x + (doubleChevron ? 13 : 20);
      const int chevronCount = doubleChevron ? 2 : 1;
      for (int i = 0; i < chevronCount; ++i) {
        const int x = firstX + i * 14;
        if (pointsRight) {
          renderer.drawLine(x, centreY - 12, x + 12, centreY, 2, true);
          renderer.drawLine(x + 12, centreY, x, centreY + 12, 2, true);
        } else {
          renderer.drawLine(x + 12, centreY - 12, x, centreY, 2, true);
          renderer.drawLine(x, centreY, x + 12, centreY + 12, 2, true);
        }
      }
    };
    for (int index = 0; index < 4; ++index) {
      const Rect rect = touchStepButtonRect(screen, barY + 80, index);
      drawButton(rect);
      drawChevron(rect, index >= 2, index == 0 || index == 3);
    }

    const Rect cancelRect = touchActionButtonRect(screen, false);
    const Rect confirmRect = touchActionButtonRect(screen, true);
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
    // Two-line step hint built from separate label + value strings (front buttons = fine step, side
    // buttons = coarse step), so the layout doesn't depend on a separator hidden in translated text.
    char line[64];
    snprintf(line, sizeof(line), "%s %d%%", I18N.get(StrId::STR_STEP_HINT_FRONT), kSmallStep);
    UITheme::drawCenteredText(renderer, screen, SMALL_FONT_ID, barY + 30, line, true);
    snprintf(line, sizeof(line), "%s %d%%", I18N.get(StrId::STR_STEP_HINT_SIDE), kLargeStep);
    UITheme::drawCenteredText(renderer, screen, SMALL_FONT_ID, barY + 52, line, true);

    // Button hints follow the current front button layout.
    const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), "-", "+");
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4, true);
  }

  renderer.displayBuffer();
}
