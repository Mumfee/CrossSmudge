#include "ControlsOptionsActivity.h"

#include <GfxRenderer.h>
#include <HalGPIO.h>
#include <I18n.h>

#include <algorithm>
#include <iterator>

#include "CrossPointSettings.h"
#include "MappedInputManager.h"
#include "SettingsList.h"
#include "activities/settings/ButtonRemapActivity.h"
#include "components/TouchHeaderBackButton.h"
#include "components/UITheme.h"

namespace fui = freeink::ui;

namespace {
uint8_t enumDisplayIndexForRawValue(const SettingInfo& setting, uint8_t rawValue) {
  if (setting.enumRawValues.empty()) {
    return rawValue;
  }

  auto it = std::find(setting.enumRawValues.begin(), setting.enumRawValues.end(), rawValue);
  if (it == setting.enumRawValues.end()) {
    return 0;
  }
  return static_cast<uint8_t>(std::distance(setting.enumRawValues.begin(), it));
}

uint8_t enumRawValueForDisplayIndex(const SettingInfo& setting, uint8_t displayIndex) {
  if (setting.enumRawValues.empty()) {
    return displayIndex;
  }
  if (displayIndex >= setting.enumRawValues.size()) {
    return setting.enumRawValues.front();
  }
  return setting.enumRawValues[displayIndex];
}
}  // namespace

void ControlsOptionsActivity::onEnter() {
  Activity::onEnter();

  activeSubmenu = SettingAction::None;
  rebuildSettingsList();
  uiReady = false;
  visibleRows = 1;
  topIndex = 0;
  app.setTheme(uiThemeTokens(uiTarget));
  app.on(ACTION_ROW, &ControlsOptionsActivity::onRowEvent, this);
  app.setScreen(&ControlsOptionsActivity::optionsScreen, this);
  requestUpdate();
}

void ControlsOptionsActivity::onExit() { Activity::onExit(); }

void ControlsOptionsActivity::rebuildSettingsList() {
  settings.clear();
  powerSettings.clear();
  frontButtonSettings.clear();
  sideButtonSettings.clear();

  const auto allSettings = getSettingsList();
  settings = buildControlsSettingsParentList(allSettings);
  powerSettings = buildControlsPowerSettingsList(allSettings);
#if CROSSSMUDGE_APP_CAP_TOUCH
  if (!gpio.hasTouch()) {
    frontButtonSettings = buildControlsFrontButtonSettingsList(allSettings);
  }
#else
  frontButtonSettings = buildControlsFrontButtonSettingsList(allSettings);
#endif
  sideButtonSettings = buildControlsSideButtonSettingsList(allSettings);

  setCurrentSettings();
  selectedIndex = 0;
}

void ControlsOptionsActivity::setCurrentSettings() {
  switch (activeSubmenu) {
    case SettingAction::ControlsPowerButton:
      currentSettings = &powerSettings;
      break;
    case SettingAction::ControlsFrontButtons:
      currentSettings = &frontButtonSettings;
      break;
    case SettingAction::ControlsSideButtons:
      currentSettings = &sideButtonSettings;
      break;
    default:
      currentSettings = &settings;
      break;
  }
  settingsCount = static_cast<int>(currentSettings->size());
}

StrId ControlsOptionsActivity::activeSubmenuTitleId() const {
  switch (activeSubmenu) {
    case SettingAction::ControlsPowerButton:
      return StrId::STR_POWER_BUTTON;
    case SettingAction::ControlsFrontButtons:
      return StrId::STR_FRONT_BUTTONS;
    case SettingAction::ControlsSideButtons:
      return StrId::STR_SIDE_BUTTONS;
    default:
      return StrId::STR_NONE_OPT;
  }
}

void ControlsOptionsActivity::openSubmenu(SettingAction action) {
  activeSubmenu = action;
  setCurrentSettings();
  selectedIndex = 0;
  topIndex = 0;
}

void ControlsOptionsActivity::closeSubmenu() {
  activeSubmenu = SettingAction::None;
  setCurrentSettings();
  selectedIndex = 0;
  topIndex = 0;
}

void ControlsOptionsActivity::moveSelection(bool forward) {
  if (settingsCount <= 0) return;

  for (int i = 0; i < settingsCount; i++) {
    selectedIndex = forward ? ButtonNavigator::nextIndex(selectedIndex, settingsCount)
                            : ButtonNavigator::previousIndex(selectedIndex, settingsCount);
    if ((*currentSettings)[selectedIndex].type != SettingType::SECTION_HEADER) {
      topIndex = followListSelection(selectedIndex, topIndex, visibleRows, settingsCount);
      break;
    }
  }
}

bool ControlsOptionsActivity::currentSettingUsesOptionMenu(const SettingInfo& setting) const {
  return setting.type == SettingType::ENUM && setting.valuePtr != nullptr && settingEnumOptionCount(setting) > 2;
}

void ControlsOptionsActivity::openEnumOptionPicker(const SettingInfo& setting) {
  const size_t optionCount = settingEnumOptionCount(setting);
  if (optionCount == 0) return;

  std::vector<std::string> options;
  options.reserve(optionCount);
  for (uint8_t i = 0; i < optionCount; i++) {
    options.push_back(settingEnumOptionLabel(setting, i));
  }

  uint8_t currentIndex = 0;
  if (setting.valuePtr != nullptr) {
    currentIndex = enumDisplayIndexForRawValue(setting, SETTINGS.*(setting.valuePtr));
  }
  if (currentIndex >= optionCount) currentIndex = 0;

  const SettingInfo selectedSetting = setting;
  optionPopup.show(setting.nameId, options, currentIndex, [selectedSetting](int selectedIndex) {
    if (selectedSetting.valuePtr != nullptr) {
      SETTINGS.*(selectedSetting.valuePtr) =
          enumRawValueForDisplayIndex(selectedSetting, static_cast<uint8_t>(selectedIndex));
      SETTINGS.saveToFile();
    }
  });
  requestUpdate();
}

void ControlsOptionsActivity::toggleCurrentSetting() {
  if (selectedIndex < 0 || selectedIndex >= settingsCount) return;
  const auto& setting = (*currentSettings)[selectedIndex];

  if (setting.type == SettingType::TOGGLE && setting.valuePtr != nullptr) {
    const bool cur = SETTINGS.*(setting.valuePtr);
    SETTINGS.*(setting.valuePtr) = !cur;
    SETTINGS.saveToFile();
  } else if (setting.type == SettingType::ENUM && setting.valuePtr != nullptr) {
    if (currentSettingUsesOptionMenu(setting)) {
      openEnumOptionPicker(setting);
      return;
    }
    const uint8_t cur = SETTINGS.*(setting.valuePtr);
    const uint8_t currentIndex = enumDisplayIndexForRawValue(setting, cur);
    const size_t optionCount = settingEnumOptionCount(setting);
    if (optionCount == 0) return;
    const uint8_t nextIndex = (currentIndex + 1) % static_cast<uint8_t>(optionCount);
    SETTINGS.*(setting.valuePtr) = enumRawValueForDisplayIndex(setting, nextIndex);
    SETTINGS.saveToFile();
  } else if (setting.type == SettingType::VALUE && setting.valuePtr != nullptr) {
    const int8_t cur = SETTINGS.*(setting.valuePtr);
    if (cur + setting.valueRange.step > setting.valueRange.max) {
      SETTINGS.*(setting.valuePtr) = setting.valueRange.min;
    } else {
      SETTINGS.*(setting.valuePtr) = cur + setting.valueRange.step;
    }
    SETTINGS.saveToFile();
  } else if (setting.type == SettingType::ACTION) {
    if (setting.action == SettingAction::RemapFrontButtons) {
      startActivityForResult(std::make_unique<ButtonRemapActivity>(renderer, mappedInput, false, true),
                             [](const ActivityResult&) { SETTINGS.saveToFile(); });
      return;
    }
    if (setting.action == SettingAction::RemapFrontButtonsReader) {
      startActivityForResult(std::make_unique<ButtonRemapActivity>(renderer, mappedInput, true, true),
                             [](const ActivityResult&) { SETTINGS.saveToFile(); });
      return;
    }
  } else if (setting.type == SettingType::SUBMENU) {
    openSubmenu(setting.action);
    return;
  }
}

void ControlsOptionsActivity::loop() {
  if (optionPopup.handleInput(mappedInput, [this] { requestUpdate(); })) return;
  if (TouchHeaderBackButton::wasTapped(mappedInput, renderer)) {
    if (activeSubmenu != SettingAction::None) {
      closeSubmenu();
      requestUpdate();
      return;
    }
    SETTINGS.saveToFile();
    finish();
    return;
  }
  if (mappedInput.wasHomeGesture()) {
    finish();
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

  if (mappedInput.hasTouch()) {
    const auto swipe = mappedInput.wasSwipe();
    if (swipe == MappedInputManager::SwipeDir::Up || swipe == MappedInputManager::SwipeDir::Down) {
      const int delta = swipe == MappedInputManager::SwipeDir::Up ? visibleRows : -visibleRows;
      const int next = scrollListBy(topIndex, delta, visibleRows, settingsCount);
      if (next != topIndex) {
        topIndex = next;
        requestUpdate();
      }
      return;
    }
  }

  buttonNavigator.onNextRelease([this] {
    moveSelection(true);
    requestUpdate();
  });

  buttonNavigator.onPreviousRelease([this] {
    moveSelection(false);
    requestUpdate();
  });

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    toggleCurrentSetting();
    requestUpdate();
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    if (activeSubmenu != SettingAction::None) {
      closeSubmenu();
      requestUpdate();
      return;
    }
    SETTINGS.saveToFile();
    finish();
    return;
  }
}

void ControlsOptionsActivity::optionsScreen(UiApp::ScreenType& screen, void* user) {
  static_cast<ControlsOptionsActivity*>(user)->buildOptionsScreen(screen);
}

void ControlsOptionsActivity::onRowEvent(const fui::ActionEvent& event, void* user) {
  auto* self = static_cast<ControlsOptionsActivity*>(user);
  if (self->optionPopup.isActive() || event.value < 0 || event.value >= self->settingsCount) return;
  if ((*self->currentSettings)[event.value].type == SettingType::SECTION_HEADER) return;
  self->selectedIndex = event.value;
  self->app.clearTapFlash();
  self->toggleCurrentSetting();
}

void ControlsOptionsActivity::buildOptionsScreen(UiApp::ScreenType& screen) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const Rect safe = UITheme::getInstance().getScreenSafeArea(renderer, !mappedInput.hasTouchHardware(), false);
  screen.setContentMargin(fui::Insets{
      static_cast<int16_t>(safe.y + metrics.topPadding + TouchHeaderBackButton::height(metrics, mappedInput)),
      static_cast<int16_t>(renderer.getScreenWidth() - safe.x - safe.width),
      static_cast<int16_t>(renderer.getScreenHeight() - safe.y - safe.height), static_cast<int16_t>(safe.x)});
  screen.spacer(static_cast<int16_t>(metrics.verticalSpacing));

  const StrId submenuTitleId = activeSubmenuTitleId();
  if (submenuTitleId != StrId::STR_NONE_OPT) {
    fui::TextStyle titleStyle = screen.theme().smallText;
    titleStyle.bold = true;
    titleStyle.maxLines = 1;
    const int16_t titleHeight = screen.target().lineHeight(titleStyle.font);
    fui::Rect titleRect = screen.takeTop(titleHeight, static_cast<int16_t>(metrics.verticalSpacing));
    const int16_t sidePadding = static_cast<int16_t>(metrics.contentSidePadding);
    titleRect.x = static_cast<int16_t>(titleRect.x + sidePadding);
    titleRect.width = static_cast<int16_t>(titleRect.width > sidePadding * 2 ? titleRect.width - sidePadding * 2 : 0);
    screen.target().text(titleRect, I18N.get(submenuTitleId), titleStyle);
  }

  const auto& currentSettingsList = *currentSettings;
  std::vector<std::string> values(currentSettingsList.size());
  std::vector<fui::ListItem> items;
  items.reserve(currentSettingsList.size());
  for (size_t i = 0; i < currentSettingsList.size(); ++i) {
    const auto& setting = currentSettingsList[i];
    if (settingShowsNavigationCaret(setting)) {
      values[i] = ">";
    } else if (setting.type == SettingType::TOGGLE && setting.valuePtr != nullptr) {
      values[i] = SETTINGS.*(setting.valuePtr) ? tr(STR_STATE_ON) : tr(STR_STATE_OFF);
    } else if (setting.type == SettingType::ENUM && setting.valuePtr != nullptr) {
      const uint8_t displayValue = enumDisplayIndexForRawValue(setting, SETTINGS.*(setting.valuePtr));
      values[i] = settingEnumOptionLabel(setting, displayValue < settingEnumOptionCount(setting) ? displayValue : 0);
    } else if (setting.type == SettingType::VALUE && setting.valuePtr != nullptr) {
      values[i] = std::to_string(SETTINGS.*(setting.valuePtr));
    }

    const bool isSectionHeader = setting.type == SettingType::SECTION_HEADER;
    fui::ListItem item;
    item.label =
        isSectionHeader ? uiListSectionHeaderLabel(values[i], I18N.get(setting.nameId)) : I18N.get(setting.nameId);
    if (!isSectionHeader && !values[i].empty()) item.value = values[i].c_str();
    item.isHeader = isSectionHeader;
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
  props.labelText.maxLines = 2;
  configureUiListSectionHeaders(props, screen.theme());
  const auto rows = configureUiList(props, screen.theme(), screen.body());
  visibleRows = rows > 0 ? rows : 1;
  topIndex = scrollListBy(topIndex, 0, visibleRows, settingsCount);
  props.topIndex = static_cast<uint16_t>(topIndex);
  screen.list(props);
}

void ControlsOptionsActivity::render(RenderLock&&) {
  if (optionPopup.processRender(renderer, mappedInput)) return;

  renderer.clearScreen();

  const auto& metrics = UITheme::getInstance().getMetrics();
  Rect header = TouchHeaderBackButton::headerRect(renderer, mappedInput);
  const Rect safe = UITheme::getInstance().getScreenSafeArea(renderer, !mappedInput.hasTouchHardware(), false);
  header.x = safe.x;
  header.width = safe.width;
  if (mappedInput.hasTouchHardware()) {
    TouchHeaderBackButton::draw(renderer, uiTarget, header, tr(STR_CAT_CONTROLS), true);
  } else {
    GUI.drawHeader(renderer, header, tr(STR_CAT_CONTROLS), nullptr, true);
  }

  uiReady = false;
  app.render();
  uiReady = true;

  const bool currentIsAction = selectedIndex >= 0 && selectedIndex < settingsCount &&
                               ((*currentSettings)[selectedIndex].type == SettingType::ACTION ||
                                (*currentSettings)[selectedIndex].type == SettingType::SUBMENU ||
                                currentSettingUsesOptionMenu((*currentSettings)[selectedIndex]));
  const auto labels = mappedInput.mapLabels(tr(STR_BACK), currentIsAction ? tr(STR_SELECT) : tr(STR_TOGGLE),
                                            tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4, true);

  renderer.displayBuffer();
}
