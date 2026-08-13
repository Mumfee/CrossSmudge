#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "PreviewBlockLocator.h"

namespace {

// Mirrors ChapterHtmlSlimParser's notion of a block-level tag (HEADER_TAGS + BLOCK_TAGS).
bool isBlockTag(const char* name) {
  static const char* const kBlockTags[] = {"h1", "h2", "h3", "h4", "h5", "h6", "p", "li", "div", "br", "blockquote"};
  for (const char* tag : kBlockTags) {
    if (strcmp(name, tag) == 0) {
      return true;
    }
  }
  return false;
}

// Feeds the document in small chunks so the chunk boundaries are exercised too.
uint32_t locate(const std::string& xml, const char* anchorId, const size_t chunkSize = 8) {
  PreviewBlockLocator locator(anchorId, isBlockTag);
  EXPECT_TRUE(locator.ok());

  for (size_t offset = 0; offset < xml.size() && !locator.done();) {
    const size_t len = std::min(chunkSize, xml.size() - offset);
    const bool isFinal = offset + len >= xml.size();
    if (!locator.feed(xml.data() + offset, static_cast<int>(len), isFinal)) {
      break;
    }
    offset += len;
  }
  return locator.startOrdinal();
}

// The note file from an example epub with issues, Lord of the Rings, where each
// cross-reference targets an empty inline <span> sitting in the middle of the note paragraph.
constexpr const char* kLotrNote = R"(<?xml version='1.0' encoding='utf-8'?>
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en">
  <head>
    <title>cE61</title>
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8"/>
    <link rel="stylesheet" type="text/css" href="../stylesheet.css"/>
    <link rel="stylesheet" type="text/css" href="../page_styles.css"/>
  </head>
  <body class="class1">
<p class="class_s3"><a href="cBDX.xhtml#aECB" class="class_seca">1</a> As in <span class="class_sec1">galadhremmin ennorath</span> (p. <span id="aEU6"></span><a href="c2DJ.xhtml#aECC">238</a>) tree-woven lands of Middle-earth. <span class="class_sec1">Remmirath</span> (p. <span id="aEUZ"></span><a href="cTT.xhtml#aECD">81</a>) contains <span class="class_sec1">rem</span> mesh, Q. <span class="class_sec1">rembe, + mir</span> jewel.</p>
</body>
</html>)";

// Element ordinals in kLotrNote: html=1, head=2, title=3, meta=4, link=5, link=6, body=7, p=8.
constexpr uint32_t kLotrParagraphOrdinal = 8;

TEST(PreviewBlockLocatorTest, InlineAnchorResolvesToEnclosingParagraph) {
  // Both references sit mid-paragraph and must resolve to the same <p>, not to their own <span>.
  EXPECT_EQ(locate(kLotrNote, "aEUZ"), kLotrParagraphOrdinal);
  EXPECT_EQ(locate(kLotrNote, "aEU6"), kLotrParagraphOrdinal);
}

TEST(PreviewBlockLocatorTest, BlockAnchorResolvesToItself) {
  // A note whose id is already on the block must not widen outwards to the enclosing <div>.
  const std::string xml = R"(<html><body><div><p>before</p><p id="note">target</p></div></body></html>)";
  // html=1, body=2, div=3, p=4, p=5
  EXPECT_EQ(locate(xml, "note"), 5u);
}

TEST(PreviewBlockLocatorTest, PicksInnermostEnclosingBlock) {
  const std::string xml = R"(<html><body><div><blockquote><span id="a"/></blockquote></div></body></html>)";
  // html=1, body=2, div=3, blockquote=4
  EXPECT_EQ(locate(xml, "a"), 4u);
}

TEST(PreviewBlockLocatorTest, ClosedBlocksAreNotUsedAsAncestors) {
  // The anchor follows a sibling paragraph that has already closed; that paragraph must have been
  // popped, so the anchor resolves to the still-open enclosing block.
  const std::string xml = R"(<html><body><div><p>done</p><span id="a"/></div></body></html>)";
  // html=1, body=2, div=3, p=4, span=5
  EXPECT_EQ(locate(xml, "a"), 3u);
}

TEST(PreviewBlockLocatorTest, SelfClosingBlockDoesNotUnbalanceTheStack) {
  // <br> is treated as a block tag but never has children, so it must not swallow later pops.
  const std::string xml = R"(<html><body><div><p>a<br/>b</p><span id="a"/></div></body></html>)";
  // html=1, body=2, div=3, p=4, br=5, span=6
  EXPECT_EQ(locate(xml, "a"), 3u);
}

TEST(PreviewBlockLocatorTest, AnchorWithNoBlockAncestorResolvesToItself) {
  const std::string xml = R"(<html><body><span id="a"/></body></html>)";
  // html=1, body=2, span=3 - body is not a block tag, so there is nothing to widen to.
  EXPECT_EQ(locate(xml, "a"), 3u);
}

TEST(PreviewBlockLocatorTest, MissingAnchorReportsNotFound) { EXPECT_EQ(locate(kLotrNote, "nosuchid"), 0u); }

TEST(PreviewBlockLocatorTest, MalformedMarkupReportsNotFound) {
  // Caller falls back to anchor-id matching when the locate pass cannot finish.
  const std::string xml = R"(<html><body><p><span id="a")";
  EXPECT_EQ(locate(xml, "a"), 0u);
}

TEST(PreviewBlockLocatorTest, EmptyAnchorIsRejected) {
  PreviewBlockLocator locator("", isBlockTag);
  EXPECT_FALSE(locator.ok());
  EXPECT_EQ(locator.startOrdinal(), 0u);
}

TEST(PreviewBlockLocatorTest, ChunkSizeDoesNotAffectResult) {
  for (const size_t chunkSize : {1u, 3u, 64u, 4096u}) {
    EXPECT_EQ(locate(kLotrNote, "aEUZ", chunkSize), kLotrParagraphOrdinal) << "chunk size " << chunkSize;
  }
}

}  // namespace
