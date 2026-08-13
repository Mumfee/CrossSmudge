#include "LanguageSelectActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include <algorithm>
#include <iterator>

#include "CrossPointSettings.h"
#include "I18nKeys.h"
#include "MappedInputManager.h"
#include "components/TouchHeaderBackButton.h"
#include "components/UITheme.h"
#include "components/UIThemeTokens.h"
#include "components/UiAppHelpers.h"

namespace fui = freeink::ui;
namespace {
constexpr fui::ActionId ACTION_ROW = 1;
}

LanguageSelectActivity::LanguageSelectActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("LanguageSelect", renderer, mappedInput),
      uiTarget(makeUiTarget(renderer)),
      app(uiTarget, uiTarget.deviceContext()) {}

void LanguageSelectActivity::onEnter() {
  Activity::onEnter();
  const auto currentLang = static_cast<uint8_t>(I18N.getLanguage());
  const auto* it = std::find(std::begin(SORTED_LANGUAGE_INDICES), std::end(SORTED_LANGUAGE_INDICES), currentLang);
  selectedIndex = it != std::end(SORTED_LANGUAGE_INDICES) ? std::distance(std::begin(SORTED_LANGUAGE_INDICES), it) : 0;
  topIndex = 0;
  visibleRows = 1;
  initialViewportPending = true;
  uiReady = false;
  app.setTheme(uiThemeTokens(uiTarget));
  app.on(ACTION_ROW, &LanguageSelectActivity::onRowEvent, this);
  app.setScreen(&LanguageSelectActivity::languageScreen, this);
  requestUpdate();
}

void LanguageSelectActivity::onExit() { Activity::onExit(); }

void LanguageSelectActivity::handleSelection() {
  const uint8_t langIndex = SORTED_LANGUAGE_INDICES[selectedIndex];
  {
    RenderLock lock(*this);
    I18N.setLanguage(static_cast<Language>(langIndex));
  }
  SETTINGS.language = langIndex;
  SETTINGS.saveToFile();
  onBack();
}

void LanguageSelectActivity::onRowEvent(const fui::ActionEvent& event, void* user) {
  auto* self = static_cast<LanguageSelectActivity*>(user);
  if (event.value < 0 || event.value >= totalItems) return;
  self->selectedIndex = event.value;
  self->app.clearTapFlash();
  self->handleSelection();
}

void LanguageSelectActivity::loop() {
  if (TouchHeaderBackButton::wasTapped(mappedInput, renderer)) {
    onBack();
    return;
  }
  if (mappedInput.wasPressed(MappedInputManager::Button::Back)) {
    onBack();
    return;
  }
  if (uiReady) {
    const fui::InputSnapshot snap = touchSnapshotFrom(mappedInput);
    if (snap.touchPressed || snap.touchReleased) {
      const auto event = app.route(snap);
      if (app.invalidated()) requestUpdate();
      if (event) return;
    }
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    handleSelection();
    return;
  }
  const auto swipe = mappedInput.wasSwipe();
  if (swipe == MappedInputManager::SwipeDir::Up || swipe == MappedInputManager::SwipeDir::Down) {
    const int next = scrollListBy(topIndex, swipe == MappedInputManager::SwipeDir::Up ? visibleRows : -visibleRows,
                                  visibleRows, totalItems);
    if (next != topIndex) {
      topIndex = next;
      requestUpdate();
    }
    return;
  }
  const auto move = [this](const int index) {
    selectedIndex = index;
    topIndex = followListSelection(selectedIndex, topIndex, visibleRows, totalItems);
    requestUpdate();
  };
  buttonNavigator.onNextRelease([this, &move] { move(ButtonNavigator::nextIndex(selectedIndex, totalItems)); });
  buttonNavigator.onPreviousRelease([this, &move] { move(ButtonNavigator::previousIndex(selectedIndex, totalItems)); });
  buttonNavigator.onNextContinuous(
      [this, &move] { move(ButtonNavigator::nextPageIndex(selectedIndex, totalItems, visibleRows)); });
  buttonNavigator.onPreviousContinuous(
      [this, &move] { move(ButtonNavigator::previousPageIndex(selectedIndex, totalItems, visibleRows)); });
}

void LanguageSelectActivity::languageScreen(UiApp::ScreenType& screen, void* user) {
  static_cast<LanguageSelectActivity*>(user)->buildLanguageScreen(screen);
}

void LanguageSelectActivity::buildLanguageScreen(UiApp::ScreenType& screen) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  screen.setContentMargin(
      fui::Insets{static_cast<int16_t>(metrics.topPadding + TouchHeaderBackButton::height(metrics, mappedInput)), 0,
                  static_cast<int16_t>(metrics.buttonHintsHeight), 0});
  screen.spacer(static_cast<int16_t>(metrics.verticalSpacing));
  std::vector<fui::ListItem> items;
  items.reserve(totalItems);
  const auto currentLang = static_cast<uint8_t>(I18N.getLanguage());
  for (int i = 0; i < totalItems; ++i) {
    fui::ListItem item;
    item.label = I18N.getLanguageName(static_cast<Language>(SORTED_LANGUAGE_INDICES[i]));
    item.value = SORTED_LANGUAGE_INDICES[i] == currentLang ? tr(STR_SELECTED) : nullptr;
    item.actionValue = static_cast<int16_t>(i);
    items.push_back(item);
  }
  fui::ListProps props;
  props.items = items.data();
  props.count = static_cast<uint16_t>(items.size());
  props.selectedIndex = static_cast<int16_t>(selectedIndex);
  props.action = ACTION_ROW;
  props.inputMask = fui::InputTouch;
  props.valueInset = 8;
  props.labelText = screen.theme().bodyText;
  const auto rows = configureUiList(props, screen.theme(), screen.body());
  visibleRows = rows > 0 ? rows : 1;
  topIndex = initialViewportPending ? followListSelection(selectedIndex, 0, visibleRows, totalItems)
                                    : scrollListBy(topIndex, 0, visibleRows, totalItems);
  initialViewportPending = false;
  props.topIndex = static_cast<uint16_t>(topIndex);
  screen.list(props);
}

void LanguageSelectActivity::render(RenderLock&&) {
  renderer.clearScreen();
  const auto& metrics = UITheme::getInstance().getMetrics();
  const Rect header = TouchHeaderBackButton::headerRect(renderer, mappedInput);
  if (mappedInput.hasTouchHardware()) {
    TouchHeaderBackButton::draw(renderer, uiTarget, header, tr(STR_LANGUAGE), false);
  } else {
    GUI.drawHeader(renderer, header, tr(STR_LANGUAGE));
  }
  uiReady = false;
  app.render();
  uiReady = true;
  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}
