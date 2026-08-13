#include <gtest/gtest.h>

#include <string>

#include "Utf8.h"

namespace {

// Helpers to build NFD / expected byte sequences explicitly so the test does not
// depend on the encoding of this source file.
const std::string kCombGrave = "\xCC\x80";     // U+0300 COMBINING GRAVE ACCENT
const std::string kCombAcute = "\xCC\x81";     // U+0301 COMBINING ACUTE ACCENT
const std::string kCombCirc = "\xCC\x82";      // U+0302 COMBINING CIRCUMFLEX ACCENT
const std::string kCombDotBelow = "\xCC\xA3";  // U+0323 COMBINING DOT BELOW

}  // namespace

// ASCII and already-precomposed (NFC) text must pass through untouched (fast path).
TEST(Utf8ComposeNfc, PassesThroughAsciiAndNfc) {
  EXPECT_EQ(utf8ComposeNfc(""), "");
  EXPECT_EQ(utf8ComposeNfc("hello world"), "hello world");
  EXPECT_EQ(utf8ComposeNfc("caf\xC3\xA9"), "caf\xC3\xA9");  // é already U+00E9
}

// Single combining mark composes onto its base letter.
TEST(Utf8ComposeNfc, ComposesSingleMark) {
  EXPECT_EQ(utf8ComposeNfc("e" + kCombAcute), "\xC3\xA9");  // e + ́  -> é  (U+00E9)
  EXPECT_EQ(utf8ComposeNfc("a" + kCombGrave), "\xC3\xA0");  // a + ̀  -> à  (U+00E0)
}

// Vietnamese letters carry two stacked marks; composition must accumulate them
// onto the intermediate precomposed character (this is the crux of the feature).
TEST(Utf8ComposeNfc, ComposesStackedVietnameseMarks) {
  // a + circumflex + acute -> ấ (U+1EA5)
  EXPECT_EQ(utf8ComposeNfc("a" + kCombCirc + kCombAcute), "\xE1\xBA\xA5");
  // a + dot-below + circumflex (canonical order) -> ậ (U+1EAD)
  EXPECT_EQ(utf8ComposeNfc("a" + kCombDotBelow + kCombCirc), "\xE1\xBA\xAD");
}

// A combining mark with no composition for its base is left unchanged, and the
// base is preserved.
TEST(Utf8ComposeNfc, LeavesUncomposableMarksIntact) {
  const std::string in = "q" + kCombAcute;  // no precomposed "q with acute"
  EXPECT_EQ(utf8ComposeNfc(in), in);
}

// A leading combining mark (no preceding base) is emitted unchanged.
TEST(Utf8ComposeNfc, HandlesLeadingMark) { EXPECT_EQ(utf8ComposeNfc(kCombAcute), kCombAcute); }

// Marks embedded in a longer word compose while surrounding text is preserved.
TEST(Utf8ComposeNfc, ComposesWithinWord) {
  // "Ti" + e+circ+acute + "ng" -> "Tiếng"
  EXPECT_EQ(utf8ComposeNfc("Ti" + std::string("e") + kCombCirc + kCombAcute + "ng"), "Ti\xE1\xBA\xBFng");
}

TEST(Utf8CjkWordCharacter, AcceptsLettersAndRejectsStandalonePunctuation) {
  EXPECT_TRUE(utf8IsCjkWordCharacter(0xAC00));   // 가 (Hangul syllable)
  EXPECT_TRUE(utf8IsCjkWordCharacter(0x4E00));   // 一 (CJK ideograph)
  EXPECT_TRUE(utf8IsCjkWordCharacter(0x3042));   // あ (Hiragana)
  EXPECT_TRUE(utf8IsCjkWordCharacter(0xFF21));   // Ａ (fullwidth Latin uppercase)
  EXPECT_FALSE(utf8IsCjkWordCharacter(0x3002));  // 。 (ideographic full stop)
  EXPECT_FALSE(utf8IsCjkWordCharacter(0xFF0C));  // ， (fullwidth comma)
}

TEST(Utf8LookupWord, RecognizesNonLatinScripts) {
  EXPECT_TRUE(utf8ContainsLookupCharacter("\xE4\xB8\xAD\xE6\x96\x87"));                          // Chinese
  EXPECT_TRUE(utf8ContainsLookupCharacter("\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82"));  // Russian
  EXPECT_TRUE(utf8ContainsLookupCharacter("\xD8\xA7\xD9\x84\xD8\xB9\xD8\xB1\xD8\xA8\xD9\x8A\xD8\xA9"));
  EXPECT_FALSE(utf8ContainsLookupCharacter("\xE2\x80\x9C\xE2\x80\x9D"));  // smart quotes
  EXPECT_FALSE(utf8ContainsLookupCharacter("\xF0\x9F\x98\x80"));          // emoji
}

TEST(Utf8LookupWord, TrimsUnicodePunctuationWithoutDamagingWords) {
  EXPECT_EQ(utf8CleanLookupWord("\xE2\x80\x9C"
                                "caf\xC3\xA9"
                                "\xE2\x80\x9D"),
            "caf\xC3\xA9");
  EXPECT_EQ(utf8CleanLookupWord("("
                                "\xC3\xA9l\xC3\xA8ve"
                                ")"),
            "\xC3\xA9l\xC3\xA8ve");
  EXPECT_EQ(utf8CleanLookupWord("\xC2\xA1hola!"), "hola");
  EXPECT_EQ(utf8CleanLookupWord("\xE4\xB8\xAD\xE6\x96\x87\xE3\x80\x82"), "\xE4\xB8\xAD\xE6\x96\x87");
  EXPECT_EQ(utf8CleanLookupWord("\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82!"),
            "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82");
  EXPECT_EQ(utf8CleanLookupWord("l'ete"), "l'ete");
  EXPECT_EQ(utf8CleanLookupWord("mother-in-law"), "mother-in-law");
  EXPECT_EQ(utf8CleanLookupWord("..."), "");
}

TEST(Utf8LookupWord, ComposesRetainedCombiningMarks) {
  EXPECT_EQ(utf8CleanLookupWord("(cafe" + kCombAcute + ")"), "caf\xC3\xA9");
}
