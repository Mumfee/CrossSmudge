#include "BookmarksHomeActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include "../reader/EpubReaderBookmarkListActivity.h"
#include "BookmarkStore.h"
#include "CrossPointState.h"
#include "FileBrowserActionActivity.h"
#include "MappedInputManager.h"
#include "components/TouchHeaderBackButton.h"
#include "components/UITheme.h"
#include "components/UIThemeTokens.h"
#include "components/UiAppHelpers.h"

namespace fui = freeink::ui;

namespace {
constexpr unsigned long BOOKMARK_DELETE_HOLD_MS = 1000;
constexpr fui::ActionId ACTION_ROW = 1;
}  // namespace

BookmarksHomeActivity::BookmarksHomeActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("BookmarksHome", renderer, mappedInput),
      uiTarget(makeUiTarget(renderer)),
      app(uiTarget, uiTarget.deviceContext()) {}

void BookmarksHomeActivity::reloadBookmarks() {
  books.clear();
  BookmarkStore::getAllBookmarkedBooks(books);
  if (books.empty()) {
    selectedIndex = 0;
  } else if (selectedIndex >= static_cast<int>(books.size())) {
    selectedIndex = static_cast<int>(books.size()) - 1;
  }
}

void BookmarksHomeActivity::onEnter() {
  Activity::onEnter();

  reloadBookmarks();
  selectedIndex = 0;
  topIndex = 0;
  visibleRows = 1;
  uiReady = false;
  app.setTheme(uiThemeTokens(uiTarget));
  app.on(ACTION_ROW, &BookmarksHomeActivity::onRowEvent, this);
  app.setScreen(&BookmarksHomeActivity::listScreen, this);
  requestUpdate();
}

void BookmarksHomeActivity::onExit() {
  Activity::onExit();
  books.clear();
}

void BookmarksHomeActivity::loop() {
  if (TouchHeaderBackButton::wasTapped(mappedInput, renderer)) {
    onGoHome();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    onGoHome();
    return;
  }

  if (!books.empty() && !longPressOpenHandled && mappedInput.isPressed(MappedInputManager::Button::Confirm) &&
      mappedInput.getHeldTime() >= BOOKMARK_DELETE_HOLD_MS) {
    longPressOpenHandled = true;
    showBookmarkBookActionMenu(selectedIndex, true);
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    if (longPressOpenHandled) {
      longPressOpenHandled = false;
      return;
    }
    if (!books.empty() && selectedIndex >= 0 && selectedIndex < static_cast<int>(books.size())) {
      openBookmarkList(selectedIndex);
    }
    return;
  }

  const int listSize = static_cast<int>(books.size());
  if (listSize == 0) return;

  int tx = 0;
  int ty = 0;
  if (mappedInput.isScreenTouchLongPress(tx, ty, BOOKMARK_DELETE_HOLD_MS) && listRowStep > 0 && ty >= listTop &&
      ty < listBottom) {
    const int offset = ty - listTop;
    const int row = offset / listRowStep;
    const int touchedIndex = topIndex + row;
    if (row >= visibleRows || offset % listRowStep >= listRowHeight || touchedIndex >= listSize) return;
    selectedIndex = touchedIndex;
    mappedInput.suppressNextTouchTap();
    longPressOpenHandled = true;
    showBookmarkBookActionMenu(selectedIndex, true);
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

  const auto swipe = mappedInput.wasSwipe();
  if (swipe == MappedInputManager::SwipeDir::Up || swipe == MappedInputManager::SwipeDir::Down) {
    const int next = scrollListBy(topIndex, swipe == MappedInputManager::SwipeDir::Up ? visibleRows : -visibleRows,
                                  visibleRows, listSize);
    if (next != topIndex) {
      topIndex = next;
      requestUpdate();
    }
    return;
  }

  const auto moveSelection = [this, listSize](const int next) {
    selectedIndex = next;
    topIndex = followListSelection(selectedIndex, topIndex, visibleRows, listSize);
    requestUpdate();
  };
  buttonNavigator.onNextRelease(
      [this, listSize, &moveSelection] { moveSelection(ButtonNavigator::nextIndex(selectedIndex, listSize)); });
  buttonNavigator.onPreviousRelease(
      [this, listSize, &moveSelection] { moveSelection(ButtonNavigator::previousIndex(selectedIndex, listSize)); });
  buttonNavigator.onNextContinuous([this, listSize, &moveSelection] {
    moveSelection(ButtonNavigator::nextPageIndex(selectedIndex, listSize, visibleRows));
  });
  buttonNavigator.onPreviousContinuous([this, listSize, &moveSelection] {
    moveSelection(ButtonNavigator::previousPageIndex(selectedIndex, listSize, visibleRows));
  });
}

void BookmarksHomeActivity::onRowEvent(const fui::ActionEvent& event, void* user) {
  auto* self = static_cast<BookmarksHomeActivity*>(user);
  if (event.value < 0 || event.value >= static_cast<int16_t>(self->books.size())) return;
  self->selectedIndex = event.value;
  self->app.clearTapFlash();
  self->openBookmarkList(self->selectedIndex);
}

void BookmarksHomeActivity::listScreen(UiApp::ScreenType& screen, void* user) {
  static_cast<BookmarksHomeActivity*>(user)->buildListScreen(screen);
}

void BookmarksHomeActivity::buildListScreen(UiApp::ScreenType& screen) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  screen.setContentMargin(
      fui::Insets{static_cast<int16_t>(metrics.topPadding + TouchHeaderBackButton::height(metrics, mappedInput)), 0,
                  static_cast<int16_t>(metrics.buttonHintsHeight), 0});
  screen.spacer(static_cast<int16_t>(metrics.verticalSpacing));
  if (books.empty()) {
    screen.centeredText(tr(STR_NO_BOOKMARKS), screen.theme().bodyText);
    return;
  }
  std::vector<fui::ListItem> items;
  items.reserve(books.size());
  for (size_t i = 0; i < books.size(); ++i) {
    fui::ListItem item;
    item.label = books[i].bookTitle.c_str();
    if (!books[i].bookAuthor.empty()) item.subtitle = books[i].bookAuthor.c_str();
    item.actionValue = static_cast<int16_t>(i);
    items.push_back(item);
  }
  fui::ListProps props;
  props.items = items.data();
  props.count = static_cast<uint16_t>(items.size());
  props.selectedIndex = static_cast<int16_t>(selectedIndex);
  props.action = ACTION_ROW;
  props.inputMask = fui::InputTouch;
  const fui::Rect bounds = screen.body();
  listTop = bounds.y;
  listBottom = bounds.bottom();
  const auto rows = configureUiList(props, screen.theme(), bounds, UiListRowType::WithSubtitle);
  listRowHeight = props.rowHeight;
  listRowStep = props.rowHeight + props.rowGap;
  visibleRows = rows > 0 ? rows : 1;
  topIndex = scrollListBy(topIndex, 0, visibleRows, static_cast<int>(books.size()));
  props.topIndex = static_cast<uint16_t>(topIndex);
  screen.list(props);
}

void BookmarksHomeActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const Rect header = TouchHeaderBackButton::headerRect(renderer, mappedInput);
  if (mappedInput.hasTouchHardware()) {
    TouchHeaderBackButton::draw(renderer, uiTarget, header, tr(STR_BOOKMARKS), false);
  } else {
    GUI.drawHeader(renderer, header, tr(STR_BOOKMARKS));
  }

  uiReady = false;
  app.render();
  uiReady = true;

  const auto labels = mappedInput.mapLabels(tr(STR_HOME), tr(STR_OPEN), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}

void BookmarksHomeActivity::showBookmarkBookActionMenu(int bookIndex, bool ignoreInitialConfirmRelease) {
  if (bookIndex < 0 || bookIndex >= static_cast<int>(books.size())) return;

  const BookmarkedBookEntry entry = books[bookIndex];
  std::vector<FileBrowserActionActivity::MenuItem> items;
  items.reserve(1);
  items.push_back({FileBrowserAction::Delete, StrId::STR_DELETE});

  startActivityForResult(std::make_unique<FileBrowserActionActivity>(renderer, mappedInput, entry.bookTitle,
                                                                     std::move(items), ignoreInitialConfirmRelease),
                         [this, entry](const ActivityResult& result) {
                           longPressOpenHandled = false;
                           const auto* actionResult = std::get_if<FileBrowserActionResult>(&result.data);
                           if (!result.isCancelled && actionResult &&
                               static_cast<FileBrowserAction>(actionResult->action) == FileBrowserAction::Delete) {
                             BOOKMARKS.loadForBook(entry.bookPath, entry.bookTitle, entry.bookAuthor, entry.bookType);
                             BOOKMARKS.clearAll();
                           }
                           reloadBookmarks();
                           requestUpdate();
                         });
}

void BookmarksHomeActivity::openBookmarkList(int bookIndex) {
  const BookmarkedBookEntry entry = books[bookIndex];
  BOOKMARKS.loadForBook(entry.bookPath, entry.bookTitle, entry.bookAuthor, entry.bookType);

  startActivityForResult(
      std::make_unique<EpubReaderBookmarkListActivity>(renderer, mappedInput, BOOKMARKS.getBookmarks()),
      [this, entry](const ActivityResult& result) {
        if (!result.isCancelled) {
          const auto* bm = std::get_if<BookmarkResult>(&result.data);
          if (bm) {
            APP_STATE.pendingBookmarkSpine = bm->spineIndex;
            APP_STATE.pendingBookmarkProgress = bm->progress;
            APP_STATE.pendingBookmarkParagraphIndex = bm->paragraphIndex;
            APP_STATE.pendingClippingIndex = UINT16_MAX;
            APP_STATE.saveToFile();
            onSelectBook(entry.bookPath);
          } else {
            LOG_ERR("BKA", "openBookmarkList: result.data holds unexpected variant type, not BookmarkResult");
            requestUpdate();
          }
        } else {
          reloadBookmarks();
          requestUpdate();
        }
      });
}
