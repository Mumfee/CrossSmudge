#pragma once

#include <string>
#include <vector>

// Per-book lookup history. Stored as <cachePath>/dictionary_history.txt.
// Format: one entry per line, "word|STATUS\n" where STATUS is a single char.
// Oldest entry at top; newest at bottom. Append-only, with no deduplication or
// automatic truncation. UI readers retain only the newest visible entries.
class LookupHistory {
 public:
  static constexpr int MAX_VISIBLE_ENTRIES = 50;

  enum class Status { Direct = 'D', Stem = 'T', AltForm = 'Y', Suggestion = 'S', NotFound = 'X' };

  struct Entry {
    std::string word;
    Status status = Status::NotFound;
  };

  // Append word+status without reading or rewriting existing history.
  static bool addWord(const std::string& cachePath, const std::string& word, Status status);

  // Conditional addWord: short-circuits if disabled, word empty, or cachePath empty.
  // Single guarded entry point used by all dictionary lookup recording sites.
  static void addWordIf(const std::string& cachePath, const std::string& word, Status status, bool enabled);

  // Load up to MAX_VISIBLE_ENTRIES in most-recent-first order.
  static std::vector<Entry> load(const std::string& cachePath);

  // Remove one of the visible entries by distance from newest (0 = newest).
  static bool removeRecentAt(const std::string& cachePath, int indexFromNewest);

 private:
  static std::string filePath(const std::string& cachePath);
  static std::vector<Entry> readRecent(const std::string& path);
};
