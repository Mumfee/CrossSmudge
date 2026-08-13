#include "DictLayout.h"

#include <Utf8.h>

#include <numeric>
#include <utility>

#include "IpaUtils.h"

namespace DictLayout {

static bool hasAsciiSpace(const char* text) {
  if (!text) return false;
  while (*text) {
    if (*text == ' ') return true;
    ++text;
  }
  return false;
}

Wrapper::Wrapper(const WrapMetrics& metrics, const Measurer& measure, const LineSink& sink)
    : maxWidth_(metrics.maxWidth),
      indentStep_(metrics.indentStep),
      bulletWidth_(metrics.bulletWidth),
      measure_(measure),
      sink_(sink) {
  ipaRuns_.reserve(4);
  lineTextPool_.reserve(128);
  lineSegments_.reserve(4);
  startLine(0, false);
}

// Width of a string, accounting for mixed IPA/non-IPA runs (each run measured
// with the appropriate font via the injected measurer).
int Wrapper::getMixedWidth(const char* text, EpdFontFamily::Style style) {
  ipaRuns_.clear();
  splitIpaRuns(text, ipaRuns_);
  return std::accumulate(ipaRuns_.begin(), ipaRuns_.end(), 0, [&](int sum, const IpaTextSpan& run) {
    return sum + measure_(run.text.c_str(), style, run.isIpa);
  });
}

void Wrapper::flushLine() {
  if (!lineSegments_.empty()) {
    sink_({lineTextPool_.c_str(), lineSegments_.data(), static_cast<uint16_t>(lineSegments_.size()), lineIndent_,
           lineIsListItem_});
    lineTextPool_.clear();
    lineSegments_.clear();
  }
  pendingInterSpanSpace_ = false;
}

void Wrapper::startLine(uint8_t indent, bool listItem) {
  lineIndent_ = indent;
  lineIsListItem_ = listItem;
  currentX_ = indent * indentStep_ + (listItem ? bulletWidth_ : 0);
  pendingInterSpanSpace_ = false;
}

void Wrapper::appendToLine(const std::string& text, EpdFontFamily::Style style, bool isIpa, int width) {
  if (!lineSegments_.empty() && lineSegments_.back().style == style && lineSegments_.back().isIpa == isIpa) {
    lineSegments_.back().length = static_cast<uint16_t>(lineSegments_.back().length + text.size());
  } else {
    lineSegments_.push_back(
        {static_cast<uint16_t>(lineTextPool_.size()), static_cast<uint16_t>(text.size()), style, isIpa});
  }
  lineTextPool_ += text;
  currentX_ += width;
}

void Wrapper::appendMixed(const char* text, EpdFontFamily::Style style) {
  ipaRuns_.clear();
  splitIpaRuns(text, ipaRuns_);
  for (const auto& run : ipaRuns_) {
    appendToLine(run.text, style, run.isIpa, measure_(run.text.c_str(), style, run.isIpa));
  }
}

// Break a single token at codepoint boundaries when it is wider than the available line width.
// IPA combining-mark handling (pendingIsIpa) mirrors splitIpaRuns and must be preserved.
void Wrapper::breakToken(const std::string& tok, EpdFontFamily::Style style, uint8_t indentLevel) {
  const auto* bp = reinterpret_cast<const uint8_t*>(tok.c_str());
  std::string pending;
  int pendingWidth = 0;
  bool pendingIsIpa = false;
  uint32_t cp;
  while ((cp = utf8NextCodepoint(&bp))) {
    const bool combining = utf8IsCombiningMark(cp);
    const bool cpIsIpa = combining ? pendingIsIpa : isIpaCodepoint(cp);
    if (pending.empty()) pendingIsIpa = cpIsIpa;
    std::string cpStr;
    utf8AppendCodepoint(cp, cpStr);
    const int cpWidth = measure_(cpStr.c_str(), style, cpIsIpa);
    if (!pending.empty() && currentX_ + pendingWidth + cpWidth > maxWidth_) {
      appendMixed(pending.c_str(), style);
      flushLine();
      startLine(indentLevel, false);
      pending.clear();
      pendingWidth = 0;
      pendingIsIpa = cpIsIpa;
    }
    pending += cpStr;
    pendingWidth += cpWidth;
  }
  if (!pending.empty()) appendMixed(pending.c_str(), style);
}

void Wrapper::onSpan(const StyledSpan& span) {
  if (!span.text || span.text[0] == '\0') return;

  EpdFontFamily::Style style;
  if (span.bold && span.italic) {
    style = EpdFontFamily::BOLD_ITALIC;
  } else if (span.bold) {
    style = EpdFontFamily::BOLD;
  } else if (span.italic) {
    style = EpdFontFamily::ITALIC;
  } else {
    style = EpdFontFamily::REGULAR;
  }
  if (span.underline) style = static_cast<EpdFontFamily::Style>(style | EpdFontFamily::UNDERLINE);

  if (span.newlineBefore) {
    flushLine();
    startLine(span.indentLevel, span.isListItem);
  }

  const int spanWidth = getMixedWidth(span.text, style);
  if (!pendingInterSpanSpace_ && !hasAsciiSpace(span.text) && currentX_ + spanWidth <= maxWidth_) {
    // Fast path: a single no-space token fits on the current line.
    appendMixed(span.text, style);
    pendingInterSpanSpace_ = false;
  } else {
    // Word-wrap within the span.
    const char* p = span.text;
    while (*p) {
      bool hadSpace = false;
      while (*p == ' ') {
        hadSpace = true;
        ++p;
      }
      if (!*p) {
        pendingInterSpanSpace_ = hadSpace && !lineSegments_.empty();
        break;
      }

      const char* tokStart = p;
      while (*p && *p != ' ') ++p;
      const std::string tok(tokStart, p - tokStart);

      bool lineIsEmpty = lineSegments_.empty();
      bool useSpace = !lineIsEmpty && (hadSpace || pendingInterSpanSpace_);
      pendingInterSpanSpace_ = false;
      const int tokWidth = getMixedWidth(tok.c_str(), style);
      const int spaceWidth = useSpace ? measure_(" ", style, false) : 0;

      if (currentX_ + spaceWidth + tokWidth > maxWidth_ && !lineIsEmpty) {
        flushLine();
        startLine(span.indentLevel, false);
        useSpace = false;
      }

      if (currentX_ + (useSpace ? spaceWidth : 0) + tokWidth > maxWidth_) {
        breakToken(tok, style, span.indentLevel);
      } else {
        if (useSpace) appendToLine(" ", style, false, spaceWidth);
        appendMixed(tok.c_str(), style);
      }
    }
  }
}

void Wrapper::lineBreak(const uint8_t indent, const bool listItem) {
  flushLine();
  startLine(indent, listItem);
}

void Wrapper::finish() { flushLine(); }

// --------------------------------------------------------------------------
// Free-function drivers over Wrapper
// --------------------------------------------------------------------------

void wrapSpans(const std::vector<StyledSpan>& spans, const WrapMetrics& metrics, const Measurer& measure,
               const LineSink& sink) {
  Wrapper wrapper(metrics, measure, sink);
  for (const auto& span : spans) wrapper.onSpan(span);
  wrapper.finish();
}

void wrapSpans(const std::vector<StyledSpan>& spans, const WrapMetrics& metrics, const Measurer& measure,
               std::vector<LayoutLine>& out) {
  out.clear();
  out.reserve(32);
  const LineSink sink{&out, [](void* ctx, const LayoutLineView& line) {
                        auto& lines = *static_cast<std::vector<LayoutLine>*>(ctx);
                        LayoutLine owned;
                        owned.indentLevel = line.indentLevel;
                        owned.isListItem = line.isListItem;
                        owned.segments.reserve(line.segmentCount);
                        for (uint16_t i = 0; i < line.segmentCount; ++i) {
                          const auto& segment = line.segments[i];
                          owned.segments.push_back({std::string(line.textPool + segment.offset, segment.length),
                                                    segment.style, segment.isIpa});
                        }
                        lines.push_back(std::move(owned));
                      }};
  wrapSpans(spans, metrics, measure, sink);
}

}  // namespace DictLayout
