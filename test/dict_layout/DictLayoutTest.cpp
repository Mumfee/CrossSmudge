#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "util/DictLayout.h"

namespace {

int measure(void*, const char* text, EpdFontFamily::Style, bool) { return static_cast<int>(std::string(text).size()); }

struct CapturedLine {
  std::string text;
  std::vector<EpdFontFamily::Style> styles;
};

void capture(void* ctx, const DictLayout::LayoutLineView& line) {
  auto& lines = *static_cast<std::vector<CapturedLine>*>(ctx);
  CapturedLine captured;
  for (uint16_t i = 0; i < line.segmentCount; ++i) {
    const auto& segment = line.segments[i];
    captured.text.append(line.textPool + segment.offset, segment.length);
    captured.styles.push_back(segment.style);
  }
  lines.push_back(std::move(captured));
}

}  // namespace

TEST(DictLayout, ReusableLineViewPreservesWrapAndStyleRuns) {
  const std::vector<StyledSpan> spans = {
      {.text = "alpha beta", .bold = true},
      {.text = " gamma"},
  };
  std::vector<CapturedLine> lines;
  const DictLayout::Measurer measurer{nullptr, measure};
  const DictLayout::LineSink sink{&lines, capture};

  DictLayout::wrapSpans(spans, {.maxWidth = 10}, measurer, sink);

  ASSERT_EQ(lines.size(), 2u);
  EXPECT_EQ(lines[0].text, "alpha beta");
  EXPECT_EQ(lines[1].text, "gamma");
  ASSERT_EQ(lines[0].styles.size(), 1u);
  EXPECT_EQ(lines[0].styles[0], EpdFontFamily::BOLD);
  ASSERT_EQ(lines[1].styles.size(), 1u);
  EXPECT_EQ(lines[1].styles[0], EpdFontFamily::REGULAR);
}

TEST(DictLayout, SameStyleSegmentsMergeInTheBorrowedView) {
  const std::vector<StyledSpan> spans = {{.text = "one"}, {.text = "two"}};
  std::vector<CapturedLine> lines;
  const DictLayout::Measurer measurer{nullptr, measure};
  const DictLayout::LineSink sink{&lines, capture};

  DictLayout::wrapSpans(spans, {.maxWidth = 20}, measurer, sink);

  ASSERT_EQ(lines.size(), 1u);
  EXPECT_EQ(lines[0].text, "onetwo");
  EXPECT_EQ(lines[0].styles.size(), 1u);
}

TEST(DictLayout, OversizedTokenBreaksAtCharacterBoundaries) {
  const std::vector<StyledSpan> spans = {{.text = "abcdefghij"}};
  std::vector<CapturedLine> lines;
  const DictLayout::Measurer measurer{nullptr, measure};
  const DictLayout::LineSink sink{&lines, capture};

  DictLayout::wrapSpans(spans, {.maxWidth = 4}, measurer, sink);

  ASSERT_EQ(lines.size(), 3u);
  EXPECT_EQ(lines[0].text, "abcd");
  EXPECT_EQ(lines[1].text, "efgh");
  EXPECT_EQ(lines[2].text, "ij");
}

TEST(DictLayout, ExplicitLineBreakFlushesStreamedText) {
  std::vector<CapturedLine> lines;
  const DictLayout::Measurer measurer{nullptr, measure};
  const DictLayout::LineSink sink{&lines, capture};
  DictLayout::Wrapper wrapper({.maxWidth = 20}, measurer, sink);

  wrapper.onSpan({.text = "alpha"});
  wrapper.lineBreak();
  wrapper.onSpan({.text = "beta"});
  wrapper.finish();

  ASSERT_EQ(lines.size(), 2u);
  EXPECT_EQ(lines[0].text, "alpha");
  EXPECT_EQ(lines[1].text, "beta");
}
