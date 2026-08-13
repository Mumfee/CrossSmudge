#pragma once
#include <FreeInkApp.h>
#include <FreeInkUIGfxRenderer.h>
#include <FreeInkUIIcon.h>

#include "MappedInputManager.h"
#include "components/UIScale.h"
#include "components/UITheme.h"
#include "components/icons/listIcons.h"

// Shared glue for activities hosting a FreeInkApp: the font-bound render
// target and the touch snapshot FreeInkApp routing consumes.

// Held duration past which a stationary touch release routes as a long-press
// (InputSnapshot::longPress). Mirrors the physical-button hold-to-act
// convention but shorter, since a finger hold has no button travel to absorb.
// Rows must opt in via InputLongPress to receive it.
inline constexpr unsigned long UI_TOUCH_LONG_PRESS_MS = 500;

// Bind the uiScale fonts before FreeInkApp's constructor derives its theme
// metrics from the body font's line height.
inline freeink::ui::GfxRendererTarget makeUiTarget(const GfxRenderer& renderer) {
  freeink::ui::GfxRendererTarget target(renderer);
  const auto spec = uiScaleSpec();
  target.setFont(freeink::ui::GfxRendererTarget::FONT_SMALL, spec.smallFontId);
  target.setFont(freeink::ui::GfxRendererTarget::FONT_BODY, spec.bodyFontId);
  target.setFont(freeink::ui::GfxRendererTarget::FONT_TITLE, spec.titleFontId);
  return target;
}

// Tap release with coords, plus the raw release the tap classifier never
// reports (swipe end, drag-off) delivered off-target: nothing dispatches,
// but routing drops its pressed-element state instead of ghosting it onto
// the next render.
// Firmware UIIcon -> FreeInkUI bitmap for list rows (SDK-format icons only;
// the legacy drawIcon assets use a different bit layout). Two crisp sizes:
// 24 for single-line rows, 32 for label+subtitle rows.
inline freeink::ui::BitmapRef listIconFor(const UIIcon icon, const int size = 24) {
  if (size >= 32) {
    switch (icon) {
      case UIIcon::Folder:
        return freeink::ui::bitmapFromIcon(icon_folder_32);
      case UIIcon::Text:
        return freeink::ui::bitmapFromIcon(icon_file_text_32);
      case UIIcon::Image:
        return freeink::ui::bitmapFromIcon(icon_image_32);
      case UIIcon::Book:
        return freeink::ui::bitmapFromIcon(icon_book_32);
      case UIIcon::File:
        return freeink::ui::bitmapFromIcon(icon_file_32);
      case UIIcon::Wifi:
        return freeink::ui::bitmapFromIcon(icon_wifi_32);
      case UIIcon::Library:
        return freeink::ui::bitmapFromIcon(icon_lyra_library_32);
      case UIIcon::Hotspot:
        return freeink::ui::bitmapFromIcon(icon_radio_tower_32);
      default:
        return {};
    }
  }
  switch (icon) {
    case UIIcon::Folder:
      return freeink::ui::bitmapFromIcon(icon_folder_24);
    case UIIcon::Text:
      return freeink::ui::bitmapFromIcon(icon_file_text_24);
    case UIIcon::Image:
      return freeink::ui::bitmapFromIcon(icon_image_24);
    case UIIcon::Book:
      return freeink::ui::bitmapFromIcon(icon_book_24);
    case UIIcon::File:
      return freeink::ui::bitmapFromIcon(icon_file_24);
    case UIIcon::Wifi:
      return freeink::ui::bitmapFromIcon(icon_wifi_24);
    case UIIcon::Library:
      return freeink::ui::bitmapFromIcon(icon_lyra_library_24);
    case UIIcon::Hotspot:
      return freeink::ui::bitmapFromIcon(icon_radio_tower_24);
    default:
      return {};
  }
}

// Legacy GfxRenderer icon arrays are pre-rotated. Render SDK Lucide assets through
// its adapter instead so the same bitmap remains correct in every orientation.
inline void drawLucideIcon(const GfxRenderer& renderer, const freeink::Icon& icon, const int x, const int y,
                           const bool foregroundBlack = true) {
  freeink::ui::GfxRendererTarget target(renderer);
  target.bitmap(freeink::ui::Rect{static_cast<int16_t>(x), static_cast<int16_t>(y), static_cast<int16_t>(icon.w),
                                  static_cast<int16_t>(icon.h)},
                freeink::ui::bitmapFromIcon(icon), freeink::ui::BitmapMode::Center,
                freeink::ui::Paint::solid(foregroundBlack ? freeink::ui::Color::Black : freeink::ui::Color::White));
}

// Scroll semantics shared by every FreeInkUI list screen: swipes move the
// viewport (topIndex) without touching the selection; button navigation moves
// the selection and pulls the viewport along just enough to keep it visible.

inline int scrollListBy(const int topIndex, const int delta, const int visibleRows, const int count) {
  int maxTop = count - visibleRows;
  if (maxTop < 0) maxTop = 0;
  int next = topIndex + delta;
  if (next > maxTop) next = maxTop;
  if (next < 0) next = 0;
  return next;
}

inline int followListSelection(const int selectedIndex, const int topIndex, const int visibleRows, const int count) {
  return static_cast<int>(freeink::ui::listTopIndexFor(
      static_cast<int16_t>(selectedIndex), static_cast<uint16_t>(topIndex),
      static_cast<uint16_t>(visibleRows > 0 ? visibleRows : 1), static_cast<uint16_t>(count)));
}

template <size_t MaxInteractions>
inline void drawUiTabBar(freeink::ui::Screen<MaxInteractions>& screen, freeink::ui::TabBarProps props,
                         const freeink::ui::Rect rect, const ThemeTabBarAppearance appearance) {
  namespace fui = freeink::ui;
  if (props.tabs == nullptr || props.count == 0) return;
  if (fui::textStyleUnset(props.text)) props.text = screen.theme().smallText;
  props.minTouchSize = screen.theme().minTouchSize;

  switch (appearance) {
    case ThemeTabBarAppearance::Pill:
      fui::tabBar(screen.frame(), rect, props);
      return;
    case ThemeTabBarAppearance::BorderedText:
      break;
  }

  props.tabInset = fui::Insets{};
  props.contentInset = fui::Insets{};
  props.tabStyles = fui::plainStyles();

  const int16_t gap = props.gap > 0 ? props.gap : 0;
  const int16_t slotWidth = static_cast<int16_t>((rect.width - gap * (props.count - 1)) / props.count);
  const int16_t contentHeight = static_cast<int16_t>(rect.height - (props.divider ? 1 : 0));
  for (uint8_t i = 0; i < props.count; ++i) {
    const int16_t slotX = static_cast<int16_t>(rect.x + i * (slotWidth + gap));
    const int16_t width = static_cast<int16_t>(i == props.count - 1 ? rect.right() - slotX : slotWidth);
    fui::TabBarProps tab = props;
    tab.tabs = &props.tabs[i];
    tab.count = 1;
    tab.gap = 0;
    tab.divider = false;
    tab.text.bold = props.tabs[i].selected;
    fui::tabBar(screen.frame(), fui::Rect{slotX, rect.y, width, contentHeight}, tab);
  }

  screen.target().fill(fui::Rect{rect.x, rect.y, rect.width, 1}, props.dividerPaint);
  if (props.divider) {
    screen.target().fill(fui::Rect{rect.x, static_cast<int16_t>(rect.bottom() - 1), rect.width, 1}, props.dividerPaint);
  }
}

inline freeink::ui::InputSnapshot touchSnapshotFrom(const MappedInputManager& mappedInput) {
  freeink::ui::InputSnapshot snap{};
  int tx = 0;
  int ty = 0;
  // Live contact position: only InputDrag-masked elements (sliders) react, so
  // carrying it in every snapshot is free for ordinary screens.
  if (mappedInput.isScreenTouchHeld(tx, ty)) {
    snap.touchHeld = true;
    snap.touchX = static_cast<int16_t>(tx);
    snap.touchY = static_cast<int16_t>(ty);
  }
  if (mappedInput.wasScreenTouchDown(tx, ty)) {
    snap.touchPressed = true;
    snap.touchX = static_cast<int16_t>(tx);
    snap.touchY = static_cast<int16_t>(ty);
  }
  unsigned long heldMs = 0;
  if (mappedInput.wasScreenTapped(tx, ty, heldMs)) {
    snap.touchReleased = true;
    // A stationary press held past the threshold routes as a long-press so
    // rows masked InputLongPress can offer a hold action (delete / forget).
    snap.longPress = heldMs >= UI_TOUCH_LONG_PRESS_MS;
    snap.touchX = static_cast<int16_t>(tx);
    snap.touchY = static_cast<int16_t>(ty);
  } else if (mappedInput.wasScreenTouchReleased()) {
    snap.touchReleased = true;
    snap.touchX = -1;
    snap.touchY = -1;
  }
  // A swipe is also a cancelled row press. Some touch backends report the
  // gesture before exposing their raw release edge, so clear it explicitly
  // before list code repaints the new viewport.
  if (!snap.touchReleased && mappedInput.wasSwipe() != MappedInputManager::SwipeDir::None) {
    snap.touchReleased = true;
    snap.touchX = -1;
    snap.touchY = -1;
  }
  return snap;
}
