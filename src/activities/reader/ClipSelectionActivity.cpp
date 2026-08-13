#include "ClipSelectionActivity.h"

#include <FontCacheManager.h>
#include <GfxRenderer.h>
#include <I18n.h>
#include <Logging.h>

#include <algorithm>
#include <cstring>

#include "MappedInputManager.h"
#include "ReaderUtils.h"
#include "activities/ActivityResult.h"
#include "clippings/ClipTextBuilder.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {

constexpr int CLIP_SELECTION_FALLBACK_FONT_ID = UI_12_FONT_ID;
constexpr unsigned long TOUCH_CLIP_HOLD_MS = 500;

bool hasEmSpace(const char* text) { return text[0] == '\xe2' && text[1] == '\x80' && text[2] == '\x83'; }

}  // namespace

ClipSelectionActivity::ClipSelectionActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                             ClipWordStore wordStore, const int fontId, Section& section,
                                             const int startPageInSection, const int marginTop, const int marginLeft)
    : Activity("ClipSelection", renderer, mappedInput),
      wordStore(std::move(wordStore)),
      renderFontId(fontId),
      section(section),
      startPageInSection(startPageInSection),
      marginTop(marginTop),
      marginLeft(marginLeft) {}

void ClipSelectionActivity::onEnter() {
  Activity::onEnter();

  if (wordStore.words.empty()) {
    LOG_ERR("CLIP", "No words available for selection");
    ActivityResult result;
    result.isCancelled = true;
    setResult(std::move(result));
    finish();
    return;
  }
  buildReadingOrder();
  if (readingOrderSize == 0) {
    LOG_ERR("CLIP", "No readable word order available");
    ActivityResult result;
    result.isCancelled = true;
    setResult(std::move(result));
    finish();
    return;
  }
  cursorIdx = 0;

  savedSectionPage = section.currentPage;

  if (!switchToPage(0)) {
    ActivityResult result;
    result.isCancelled = true;
    setResult(std::move(result));
    finish();
    return;
  }
  requestUpdate();
}

void ClipSelectionActivity::onExit() {
  section.currentPage = savedSectionPage;
  resetSavedBufferChunks();
  hasSavedBuffer = false;
  if (usingFallbackFont) {
    if (auto* fcm = renderer.getFontCacheManager()) {
      fcm->clearCache();
    }
  }
  Activity::onExit();
}

void ClipSelectionActivity::allocateSavedBuffer() {
  savedBufferSize = renderer.getBufferSize();
  const size_t chunkCount = (savedBufferSize + BUFFER_CHUNK_SIZE - 1) / BUFFER_CHUNK_SIZE;
  savedBufferChunkCount = 0;

  if (chunkCount > savedBufferChunks.size()) {
    LOG_ERR("CLIP", "Framebuffer snapshot needs %u chunks; using rerender fallback", static_cast<unsigned>(chunkCount));
    savedBufferSize = 0;
    return;
  }

  for (size_t i = 0; i < chunkCount; i++) {
    const size_t offset = i * BUFFER_CHUNK_SIZE;
    const size_t chunkSize = std::min(BUFFER_CHUNK_SIZE, savedBufferSize - offset);
    auto chunk = makeUniqueNoThrow<uint8_t[]>(chunkSize);
    if (!chunk) {
      LOG_ERR("CLIP", "OOM: clipping page snapshot chunk %u (%u bytes); using rerender fallback",
              static_cast<unsigned>(i), static_cast<unsigned>(chunkSize));
      resetSavedBufferChunks();
      savedBufferSize = 0;
      return;
    }
    savedBufferChunks[i] = std::move(chunk);
  }
  savedBufferChunkCount = chunkCount;
}

void ClipSelectionActivity::resetSavedBufferChunks() {
  for (auto& chunk : savedBufferChunks) {
    chunk.reset();
  }
  savedBufferChunkCount = 0;
}

void ClipSelectionActivity::storeCurrentBuffer() {
  if (savedBufferChunkCount == 0) {
    hasSavedBuffer = false;
    return;
  }

  const uint8_t* frameBuffer = renderer.getFrameBuffer();
  for (size_t i = 0; i < savedBufferChunkCount; i++) {
    const size_t offset = i * BUFFER_CHUNK_SIZE;
    const size_t chunkSize = std::min(BUFFER_CHUNK_SIZE, savedBufferSize - offset);
    memcpy(savedBufferChunks[i].get(), frameBuffer + offset, chunkSize);
  }
  hasSavedBuffer = true;
}

void ClipSelectionActivity::restoreSavedBuffer() const {
  if (!hasSavedBuffer) return;

  uint8_t* frameBuffer = renderer.getFrameBuffer();
  for (size_t i = 0; i < savedBufferChunkCount; i++) {
    const size_t offset = i * BUFFER_CHUNK_SIZE;
    const size_t chunkSize = std::min(BUFFER_CHUNK_SIZE, savedBufferSize - offset);
    memcpy(frameBuffer + offset, savedBufferChunks[i].get(), chunkSize);
  }
}

void ClipSelectionActivity::buildReadingOrder() {
  readingOrderSize = 0;

  int lineStart = 0;
  const auto& words = wordStore.words;
  const int total = static_cast<int>(words.size());
  while (lineStart < total) {
    int lineEnd = lineStart + 1;
    while (lineEnd < total && words[lineEnd].pageIdx == words[lineStart].pageIdx &&
           words[lineEnd].y == words[lineStart].y) {
      lineEnd++;
    }

    if (words[lineStart].lineIsRtl) {
      for (int i = lineEnd - 1; i >= lineStart; --i) {
        if (readingOrderSize >= readingOrder.size()) {
          LOG_ERR("CLIP", "Reading order cap hit (%u words); clipping range truncated",
                  static_cast<unsigned>(readingOrder.size()));
          return;
        }
        readingOrder[readingOrderSize++] = static_cast<uint16_t>(i);
      }
    } else {
      for (int i = lineStart; i < lineEnd; ++i) {
        if (readingOrderSize >= readingOrder.size()) {
          LOG_ERR("CLIP", "Reading order cap hit (%u words); clipping range truncated",
                  static_cast<unsigned>(readingOrder.size()));
          return;
        }
        readingOrder[readingOrderSize++] = static_cast<uint16_t>(i);
      }
    }
    lineStart = lineEnd;
  }
}

void ClipSelectionActivity::loop() {
  const int total = static_cast<int>(readingOrderSize);
  using Button = MappedInputManager::Button;

  auto moveCursor = [this](const int nextOrderIdx) {
    if (nextOrderIdx == cursorIdx || nextOrderIdx < 0 || nextOrderIdx >= static_cast<int>(readingOrderSize)) return;
    const int previousPage = wordStore.words[readingOrder[cursorIdx]].pageIdx;
    cursorIdx = nextOrderIdx;
    if (wordStore.words[readingOrder[cursorIdx]].pageIdx != previousPage) {
      needsPageSwitch = true;
    }
    requestUpdate();
  };

  int touchX = 0;
  int touchY = 0;
  if (touchDragSelecting) {
    if (mappedInput.isScreenTouchHeld(touchX, touchY)) {
      if (selectWordAtPoint(touchX, touchY)) requestUpdate();
      return;
    }
    if (mappedInput.wasScreenTouchReleased()) {
      touchDragSelecting = false;
      confirmSelection();
      return;
    }
  } else if (mappedInput.isScreenTouchLongPress(touchX, touchY, TOUCH_CLIP_HOLD_MS) &&
             selectWordAtPoint(touchX, touchY)) {
    if (startMarkIdx == -1) startMarkIdx = cursorIdx;
    touchDragSelecting = true;
    requestUpdate();
    return;
  }

  if (mappedInput.wasScreenTapped(touchX, touchY) && selectWordAtPoint(touchX, touchY)) {
    confirmSelection();
    return;
  }

  buttonNavigator.onRelease({Button::Left}, [this, &moveCursor] {
    if (cursorIdx > 0) moveCursor(cursorIdx - 1);
  });
  buttonNavigator.onContinuous({Button::Left}, [this, &moveCursor] {
    if (cursorIdx > 0) moveCursor(cursorIdx - 1);
  });
  buttonNavigator.onRelease({Button::Right}, [this, total, &moveCursor] {
    if (cursorIdx + 1 < total) moveCursor(cursorIdx + 1);
  });
  buttonNavigator.onContinuous({Button::Right}, [this, total, &moveCursor] {
    if (cursorIdx + 1 < total) moveCursor(cursorIdx + 1);
  });
  buttonNavigator.onRelease({Button::Down}, [this, &moveCursor] { moveCursor(lineEndForward(cursorIdx)); });
  buttonNavigator.onContinuous({Button::Down}, [this, &moveCursor] { moveCursor(lineEndForward(cursorIdx)); });
  buttonNavigator.onRelease({Button::Up}, [this, &moveCursor] { moveCursor(lineEndBackward(cursorIdx)); });
  buttonNavigator.onContinuous({Button::Up}, [this, &moveCursor] { moveCursor(lineEndBackward(cursorIdx)); });

  if (mappedInput.wasReleased(Button::Confirm)) {
    confirmSelection();
    return;
  }

  if (mappedInput.wasReleased(Button::Back)) {
    if (startMarkIdx != -1) {
      startMarkIdx = -1;
      requestUpdate();
      return;
    }

    ActivityResult result;
    result.isCancelled = true;
    setResult(std::move(result));
    finish();
  }
}

bool ClipSelectionActivity::selectWordAtPoint(const int x, const int y) {
  for (size_t orderIdx = 0; orderIdx < readingOrderSize; orderIdx++) {
    const WordRef& word = wordStore.words[readingOrder[orderIdx]];
    if (word.pageIdx != currentDisplayPage || x < word.x || x >= word.x + word.w || y < word.y ||
        y >= word.y + word.h) {
      continue;
    }
    if (cursorIdx != static_cast<int>(orderIdx)) {
      cursorIdx = static_cast<int>(orderIdx);
      requestUpdate();
    }
    return true;
  }
  return false;
}

void ClipSelectionActivity::confirmSelection() {
  if (startMarkIdx == -1) {
    startMarkIdx = cursorIdx;
    requestUpdate();
    return;
  }

  const int total = static_cast<int>(readingOrderSize);
  const int from = std::min(startMarkIdx, cursorIdx);
  const int to = std::max(startMarkIdx, cursorIdx);
  auto result =
      ClipTextBuilder::build(wordStore, readingOrder.data(), from, to, total, startPageInSection, section.pageCount);
  if (const auto paragraphIndex = section.getParagraphIndexForPage(result.sectionPage)) {
    result.paragraphIndex = *paragraphIndex;
  }
  setResult(std::move(result));
  finish();
}

void ClipSelectionActivity::render(RenderLock&&) {
  if (needsPageSwitch) {
    switchToPage(wordStore.words[readingOrder[cursorIdx]].pageIdx);
    needsPageSwitch = false;
  } else if (hasSavedBuffer) {
    restoreSavedBuffer();
  } else if (!switchToPage(currentDisplayPage)) {
    return;
  }

  drawHighlights();

  const auto confirmLabel = startMarkIdx == -1 ? tr(STR_SELECT) : tr(STR_DONE);
  const auto labels = mappedInput.mapLabels(tr(STR_BACK), confirmLabel, tr(STR_DIR_LEFT), tr(STR_DIR_RIGHT));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}

bool ClipSelectionActivity::switchToPage(const int pageIdx) {
  const int oldPage = section.currentPage;
  section.currentPage = startPageInSection + pageIdx;
  auto page = section.loadPageFromSectionFile();
  if (!page) {
    section.currentPage = oldPage;
    LOG_ERR("CLIP", "Failed to load selection page %d", pageIdx);
    return false;
  }

  // The snapshot is almost a full framebuffer. Release it before SD-font
  // prewarming so its chunks do not crowd out the contiguous glyph bitmap.
  resetSavedBufferChunks();
  hasSavedBuffer = false;

  if (auto* fcm = renderer.getFontCacheManager()) {
    bool renderWithFallback = false;
    {
      auto scope = fcm->createPrewarmScope();
      page->renderText(renderer, renderFontId, marginLeft, marginTop, ReaderUtils::readerForegroundBlack());
      if (!scope.endScanAndPrewarm() && renderer.isSdCardFont(renderFontId)) {
        useFallbackFont("page prewarm");
        renderWithFallback = true;
      } else {
        renderer.clearScreen(ReaderUtils::readerBackgroundColor());
        page->render(renderer, renderFontId, marginLeft, marginTop, ReaderUtils::readerForegroundBlack());
      }
    }
    if (renderWithFallback) {
      auto fallbackScope = fcm->createPrewarmScope();
      page->renderText(renderer, renderFontId, marginLeft, marginTop, ReaderUtils::readerForegroundBlack());
      fallbackScope.endScanAndPrewarm();
      renderer.clearScreen(ReaderUtils::readerBackgroundColor());
      page->render(renderer, renderFontId, marginLeft, marginTop, ReaderUtils::readerForegroundBlack());
    }
  } else {
    renderer.clearScreen(ReaderUtils::readerBackgroundColor());
    page->render(renderer, renderFontId, marginLeft, marginTop, ReaderUtils::readerForegroundBlack());
  }

  // The rendered page is now in the framebuffer, so its deserialized objects
  // no longer need to overlap with the snapshot allocation.
  page.reset();
  allocateSavedBuffer();
  storeCurrentBuffer();
  currentDisplayPage = pageIdx;
  return true;
}

void ClipSelectionActivity::applyWordStyle(const WordRef& word, const ClipWordStyle& style) const {
  const auto textStyle = static_cast<EpdFontFamily::Style>(word.style & ~EpdFontFamily::UNDERLINE);
  const int skipX =
      hasEmSpace(wordStore.text(word)) ? renderer.getTextAdvanceX(renderFontId, "\xe2\x80\x83", textStyle) : 0;
  const int drawX = word.x + skipX;
  const int drawW = word.w - skipX;
  if (drawW <= 0) return;
  const bool foregroundBlack = ReaderUtils::readerForegroundBlack();

  const bool fill = (style.flags & ClipWordStyle::FILL) != 0;
  if (fill) {
    // Build the highlight from the existing framebuffer instead of redrawing
    // the word, which avoids font-cache work on every selection step.
    for (int y = word.y; y < word.y + word.h; y += 2) {
      for (int x = drawX; x < drawX + drawW; x += 2) {
        renderer.drawPixel(x, y, foregroundBlack);
      }
    }
    if (!foregroundBlack) {
      // Dark mode starts with white text on black. Inverting after adding the
      // dither produces black text on a light-gray highlight.
      renderer.invertRect(drawX, word.y, drawW, word.h);
    }
  }

  if ((style.flags & ClipWordStyle::BORDER) != 0) {
    renderer.drawRect(drawX, word.y, drawW, word.h, foregroundBlack);
  }
}

void ClipSelectionActivity::useFallbackFont(const char* reason) {
  if (usingFallbackFont) return;
  LOG_ERR("CLIP", "SD font %d failed during %s; using fallback font %d for clipping selection", renderFontId, reason,
          CLIP_SELECTION_FALLBACK_FONT_ID);
  renderFontId = CLIP_SELECTION_FALLBACK_FONT_ID;
  usingFallbackFont = true;
}

void ClipSelectionActivity::drawHighlights() {
  // An underline sits below the ascender and collides with descenders when
  // the reader's line height has no spare leading. The fill and cursor border
  // already distinguish the selected range and its active endpoint.
  static constexpr ClipWordStyle selectionStyle{ClipWordStyle::FILL};
  static constexpr ClipWordStyle initialCursorStyle{ClipWordStyle::FILL | ClipWordStyle::BORDER};
  static constexpr ClipWordStyle selectedCursorStyle{ClipWordStyle::BORDER};

  if (startMarkIdx != -1) {
    const int from = std::min(startMarkIdx, cursorIdx);
    const int to = std::max(startMarkIdx, cursorIdx);
    for (int i = from; i <= to; i++) {
      const WordRef& word = wordStore.words[readingOrder[i]];
      if (word.pageIdx == currentDisplayPage) {
        applyWordStyle(word, selectionStyle);
      }
    }
  }

  const WordRef& cursorWord = wordStore.words[readingOrder[cursorIdx]];
  if (cursorWord.pageIdx == currentDisplayPage) {
    // Before the first endpoint, the cursor supplies its own fill. Afterwards
    // selectionStyle has already filled it, so only add the active border.
    applyWordStyle(cursorWord, startMarkIdx == -1 ? initialCursorStyle : selectedCursorStyle);
  }
}

int ClipSelectionActivity::lineEndForward(const int orderIdx) const {
  const int total = static_cast<int>(readingOrder.size());
  const auto& words = wordStore.words;
  const WordRef& current = words[readingOrder[orderIdx]];
  for (int i = orderIdx + 1; i < total; ++i) {
    const WordRef& word = words[readingOrder[i]];
    if (word.pageIdx != current.pageIdx || word.y != current.y) return i;
  }
  return orderIdx;
}

int ClipSelectionActivity::lineEndBackward(const int orderIdx) const {
  const auto& words = wordStore.words;
  const WordRef& current = words[readingOrder[orderIdx]];
  int i = orderIdx - 1;
  for (; i >= 0; --i) {
    const WordRef& word = words[readingOrder[i]];
    if (word.pageIdx != current.pageIdx || word.y != current.y) break;
  }
  if (i < 0) return orderIdx;

  const WordRef& previous = words[readingOrder[i]];
  int first = i;
  for (; i >= 0; --i) {
    const WordRef& word = words[readingOrder[i]];
    if (word.pageIdx != previous.pageIdx || word.y != previous.y) break;
    first = i;
  }
  return first;
}
