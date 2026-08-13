#include "NetworkModeSelectionActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include "MappedInputManager.h"
#include "components/TouchHeaderBackButton.h"
#include "components/UITheme.h"
#include "components/UIThemeTokens.h"
#include "components/UiAppHelpers.h"
#include "components/icons/chart.h"
#include "fontIds.h"

namespace fui = freeink::ui;

namespace {
constexpr int MENU_ITEM_COUNT = 5;
constexpr int LIST_ITEM_COUNT = MENU_ITEM_COUNT + 1;
constexpr int NEARBY_SECTION_INDEX = 3;
constexpr fui::ActionId ACTION_ROW = 1;
constexpr StrId menuItems[MENU_ITEM_COUNT] = {StrId::STR_JOIN_NETWORK, StrId::STR_CALIBRE_WIRELESS,
                                              StrId::STR_CREATE_HOTSPOT, StrId::STR_RECEIVE_NEARBY_BOOK,
                                              StrId::STR_NEARBY_STATS_SYNC};
constexpr StrId menuDescs[MENU_ITEM_COUNT] = {StrId::STR_JOIN_DESC, StrId::STR_CALIBRE_DESC, StrId::STR_HOTSPOT_DESC,
                                              StrId::STR_RECEIVE_NEARBY_BOOK_DESC, StrId::STR_NEARBY_STATS_SYNC_DESC};
constexpr UIIcon menuIcons[MENU_ITEM_COUNT] = {UIIcon::Wifi, UIIcon::Library, UIIcon::Hotspot, UIIcon::Transfer,
                                               UIIcon::Transfer};

int listIndexForMenuIndex(const int menuIndex) { return menuIndex < NEARBY_SECTION_INDEX ? menuIndex : menuIndex + 1; }
}  // namespace

NetworkModeSelectionActivity::NetworkModeSelectionActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("NetworkModeSelection", renderer, mappedInput),
      uiTarget(makeUiTarget(renderer)),
      app(uiTarget, uiTarget.deviceContext()) {}

void NetworkModeSelectionActivity::selectCurrent() {
  NetworkMode mode = NetworkMode::JOIN_NETWORK;
  if (selectedIndex == 1) {
    mode = NetworkMode::CONNECT_CALIBRE;
  } else if (selectedIndex == 2) {
    mode = NetworkMode::CREATE_HOTSPOT;
  } else if (selectedIndex == 3) {
    mode = NetworkMode::NEARBY_BOOK_RECEIVE;
  } else if (selectedIndex == 4) {
    mode = NetworkMode::NEARBY_STATS_SYNC;
  }
  onModeSelected(mode);
}

void NetworkModeSelectionActivity::onRowEvent(const fui::ActionEvent& event, void* user) {
  auto* self = static_cast<NetworkModeSelectionActivity*>(user);
  if (event.value < 0 || event.value >= MENU_ITEM_COUNT) return;
  self->selectedIndex = event.value;
  // Selection leaves this screen; a lingering flash would gray an unrelated
  // element on the next render.
  self->app.clearTapFlash();
  self->selectCurrent();
}

void NetworkModeSelectionActivity::onEnter() {
  Activity::onEnter();

  selectedIndex = 0;
  uiReady = false;
  visibleRows = 1;
  topIndex = 0;
  app.setTheme(uiThemeTokens(uiTarget));
  app.on(ACTION_ROW, &NetworkModeSelectionActivity::onRowEvent, this);
  app.setScreen(&NetworkModeSelectionActivity::listScreen, this);
  requestUpdate();
}

void NetworkModeSelectionActivity::onExit() { Activity::onExit(); }

void NetworkModeSelectionActivity::loop() {
  if (TouchHeaderBackButton::wasTapped(mappedInput, renderer)) {
    onCancel();
    return;
  }
  // Handle back button - cancel
  if (mappedInput.wasPressed(MappedInputManager::Button::Back)) {
    onCancel();
    return;
  }

  // Handle confirm button - select current option
  if (mappedInput.wasPressed(MappedInputManager::Button::Confirm)) {
    mappedInput.suppressNextConfirmRelease();
    selectCurrent();
    return;
  }

  // Touch goes through the FreeInkApp: render() registered the row hit rects;
  // route the snapshot and let onRowEvent dispatch.
  if (uiReady) {
    const fui::InputSnapshot snap = touchSnapshotFrom(mappedInput);
    if (snap.touchPressed || snap.touchReleased) {
      const auto event = app.route(snap);
      if (app.invalidated()) requestUpdate();
      if (event) return;  // dispatched to onRowEvent
    }
  }

  // Handle navigation
  buttonNavigator.onNext([this] {
    selectedIndex = ButtonNavigator::nextIndex(selectedIndex, MENU_ITEM_COUNT);
    topIndex = followListSelection(listIndexForMenuIndex(selectedIndex), topIndex, visibleRows, LIST_ITEM_COUNT);
    requestUpdate();
  });

  buttonNavigator.onPrevious([this] {
    selectedIndex = ButtonNavigator::previousIndex(selectedIndex, MENU_ITEM_COUNT);
    topIndex = followListSelection(listIndexForMenuIndex(selectedIndex), topIndex, visibleRows, LIST_ITEM_COUNT);
    requestUpdate();
  });
}

void NetworkModeSelectionActivity::listScreen(UiApp::ScreenType& screen, void* user) {
  static_cast<NetworkModeSelectionActivity*>(user)->buildListScreen(screen);
}

void NetworkModeSelectionActivity::buildListScreen(UiApp::ScreenType& screen) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  // Content below the GUI.drawHeader band, above the button hints.
  screen.setContentMargin(
      fui::Insets{static_cast<int16_t>(metrics.topPadding + TouchHeaderBackButton::height(metrics, mappedInput)), 0,
                  static_cast<int16_t>(metrics.buttonHintsHeight), 0});
  screen.spacer(static_cast<int16_t>(metrics.verticalSpacing));

  std::vector<fui::ListItem> items;
  items.reserve(LIST_ITEM_COUNT);
  for (int i = 0; i < MENU_ITEM_COUNT; i++) {
    if (i == NEARBY_SECTION_INDEX) {
      fui::ListItem header;
      header.label = I18N.get(StrId::STR_NEARBY_DEVICE);
      header.isHeader = true;
      items.push_back(header);
    }

    fui::ListItem item;
    item.label = I18N.get(menuItems[i]);
    item.subtitle = I18N.get(menuDescs[i]);
    if (i == 3) {
      item.icon = fui::bitmapFromIcon(icon_chevrons_left_right_ellipsis_32);
    } else if (i == 4) {
      item.icon = fui::BitmapRef{ChartListIcon, 32, 32, fui::BitmapFormat::Mask1};
    } else {
      item.icon = listIconFor(menuIcons[i], 32);  // subtitle rows carry the larger icon
    }
    item.actionValue = static_cast<int16_t>(i);
    items.push_back(item);
  }

  fui::ListProps props;
  props.items = items.data();
  props.count = static_cast<uint16_t>(items.size());
  props.selectedIndex = static_cast<int16_t>(listIndexForMenuIndex(selectedIndex));
  props.action = ACTION_ROW;
  props.inputMask = fui::InputTouch;  // physical buttons stay in loop()
  props.labelText = screen.theme().bodyText;
  props.labelText.bold = true;
  props.subtitleText = screen.theme().smallText;
  props.subtitleText.bold = false;
  props.headerText = screen.theme().bodyText;
  props.headerText.bold = true;
  props.rowGap = 10;
  // The normal 10px row gap plus this extra 10px separates Nearby Device
  // from the hotspot group by about 20px.
  props.sectionGap = 10;
  const auto rows = configureUiList(props, screen.theme(), screen.body(), UiListRowType::WithSubtitle);
  visibleRows = rows > 0 ? rows : 1;
  topIndex = scrollListBy(topIndex, 0, visibleRows, LIST_ITEM_COUNT);  // clamp to range
  props.topIndex = static_cast<uint16_t>(topIndex);
  screen.list(props);
}

void NetworkModeSelectionActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto& metrics = UITheme::getInstance().getMetrics();
  const auto pageWidth = renderer.getScreenWidth();

  // Header via GUI.drawHeader (already FreeInkUI-themed) for the battery
  // indicator; the rest of the screen renders through the app.
  const Rect header = TouchHeaderBackButton::headerRect(renderer, mappedInput);
  if (mappedInput.hasTouchHardware()) {
    TouchHeaderBackButton::draw(renderer, uiTarget, header, tr(STR_FILE_TRANSFER), false);
  } else {
    GUI.drawHeader(renderer, header, tr(STR_FILE_TRANSFER));
  }

  uiReady = false;
  app.render();
  uiReady = true;

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer(screenTransitionRefresh.modeFor(0));
}

void NetworkModeSelectionActivity::onModeSelected(NetworkMode mode) {
  setResult(NetworkModeResult{mode});
  finish();
}

void NetworkModeSelectionActivity::onCancel() {
  ActivityResult result;
  result.isCancelled = true;
  setResult(std::move(result));
  finish();
}
