#include <gtest/gtest.h>

#include <string>
#include <type_traits>

#include "ClippingStore.h"
#include "Epub/Epub/ReaderRenderSpec.h"
#include "activities/reader/WordRef.h"
#include "clippings/ClipTextBuilder.h"

TEST(ClipWordStore, StoresNullTerminatedUtf8TextWithStableOffsets) {
  ClipWordStore store;
  WordRef first;
  WordRef second;

  ASSERT_TRUE(store.appendText(first, "first"));
  ASSERT_TRUE(store.appendText(second, "\xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D"));  // Hebrew UTF-8

  EXPECT_EQ(first.textOffset, 0);
  EXPECT_EQ(first.textLength, 5);
  EXPECT_STREQ(store.text(first), "first");
  EXPECT_STREQ(store.text(second), "\xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D");
  EXPECT_TRUE(std::is_trivially_copyable_v<WordRef>);
}

TEST(ClipWordStore, RejectsTextPastTheUint16PoolBoundary) {
  ClipWordStore store;
  WordRef longWord;
  const std::string text(ClipWordStore::MAX_TEXT_POOL_BYTES - 1, 'x');

  ASSERT_TRUE(store.appendText(longWord, text.c_str()));
  EXPECT_EQ(store.textPool.size(), ClipWordStore::MAX_TEXT_POOL_BYTES);

  WordRef overflow;
  EXPECT_FALSE(store.appendText(overflow, "x"));
}

TEST(ClipTextBuilder, JoinsInsertedHyphenAcrossParagraphBoundary) {
  ClipWordStore store;
  WordRef prefix;
  prefix.pageIdx = 0;
  prefix.pageWordIndex = 0;
  prefix.endsWithInsertedHyphen = true;
  ASSERT_TRUE(store.appendText(prefix, "hyphen-"));
  store.words.push_back(prefix);

  WordRef suffix;
  suffix.pageIdx = 1;
  suffix.pageWordIndex = 0;
  suffix.paragraphStart = true;  // Layout-derived marker at the start of the next page.
  ASSERT_TRUE(store.appendText(suffix, "ated"));
  store.words.push_back(suffix);

  const uint16_t order[] = {0, 1};
  const ClippingResult result = ClipTextBuilder::build(store, order, 0, 1, 2, 0, 2);

  EXPECT_EQ(result.text, "hyphenated");
}

TEST(ClippingLayout, RejectsStoredRangeWhenFontChangesWithoutChangingPageCount) {
  ReaderRenderSpec original;
  original.fontId = 12;
  original.viewportWidth = 760;
  original.viewportHeight = 430;

  ReaderRenderSpec changed = original;
  changed.fontId = 16;

  Clipping clipping;
  clipping.pageCount = 20;
  clipping.layoutSignature = readerRenderSpecSignature(original);

  EXPECT_TRUE(clippingStoredRangeMatchesLayout(clipping, 20, readerRenderSpecSignature(original)));
  EXPECT_FALSE(clippingStoredRangeMatchesLayout(clipping, 20, readerRenderSpecSignature(changed)));
}

TEST(ClippingLayout, PreservesLegacyFastPathUntilRelayout) {
  Clipping clipping;
  clipping.pageCount = 20;
  clipping.layoutSignature = 0;

  ReaderRenderSpec current;
  current.fontId = 12;
  EXPECT_TRUE(clippingStoredRangeMatchesLayout(clipping, 20, readerRenderSpecSignature(current)));
  EXPECT_FALSE(clippingStoredRangeMatchesLayout(clipping, 21, readerRenderSpecSignature(current)));
}
