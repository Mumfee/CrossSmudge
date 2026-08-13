#include "LookupHistory.h"

#include <HalStorage.h>
#include <Logging.h>
#include <common/FsApiConstants.h>

#include <algorithm>

std::string LookupHistory::filePath(const std::string& cachePath) { return cachePath + "/dictionary_history.txt"; }

static LookupHistory::Status parseStatusCode(char code) {
  switch (code) {
    case 'D':
      return LookupHistory::Status::Direct;
    case 'T':
      return LookupHistory::Status::Stem;
    case 'Y':
      return LookupHistory::Status::AltForm;
    case 'S':
      return LookupHistory::Status::Suggestion;
    default:
      return LookupHistory::Status::NotFound;
  }
}

static LookupHistory::Entry parseLine(const char* lineBuf, int lineLen) {
  LookupHistory::Entry entry;
  int separator = -1;
  for (int i = lineLen - 1; i >= 0; i--) {
    if (lineBuf[i] == '|') {
      separator = i;
      break;
    }
  }
  if (separator >= 0 && separator + 1 < lineLen) {
    entry.word = std::string(lineBuf, separator);
    entry.status = parseStatusCode(lineBuf[separator + 1]);
  } else {
    entry.word = std::string(lineBuf, lineLen);
  }
  return entry;
}

std::vector<LookupHistory::Entry> LookupHistory::readRecent(const std::string& path) {
  std::vector<Entry> entries;
  entries.reserve(MAX_VISIBLE_ENTRIES);

  HalFile file;
  if (!Storage.openFileForRead("LH", path.c_str(), file)) return entries;

  char lineBuf[256];
  int lineLen = 0;
  auto retainLine = [&]() {
    if (lineLen == 0) return;
    Entry entry = parseLine(lineBuf, lineLen);
    if (!entry.word.empty()) {
      if (entries.size() == MAX_VISIBLE_ENTRIES) entries.erase(entries.begin());
      entries.push_back(std::move(entry));
    }
    lineLen = 0;
  };

  while (file.available()) {
    const int byte = file.read();
    if (byte < 0) break;
    if (byte == '\n' || byte == '\r') {
      retainLine();
    } else if (lineLen < static_cast<int>(sizeof(lineBuf)) - 1) {
      lineBuf[lineLen++] = static_cast<char>(byte);
    }
  }
  retainLine();
  file.close();
  return entries;
}

bool LookupHistory::addWord(const std::string& cachePath, const std::string& word, Status status) {
  if (word.empty()) return false;

  const std::string path = filePath(cachePath);
  HalFile file = Storage.open(path.c_str(), O_WRONLY | O_CREAT | O_AT_END);
  if (!file) {
    LOG_ERR("LH", "Failed to open for append: %s", path.c_str());
    return false;
  }

  const char suffix[] = {'|', static_cast<char>(status), '\n'};
  const bool ok =
      file.write(word.c_str(), word.size()) == word.size() && file.write(suffix, sizeof(suffix)) == sizeof(suffix);
  file.flush();
  file.close();
  if (!ok) LOG_ERR("LH", "Failed to append history: %s", path.c_str());
  return ok;
}

void LookupHistory::addWordIf(const std::string& cachePath, const std::string& word, Status status, bool enabled) {
  if (!enabled || word.empty() || cachePath.empty()) return;
  addWord(cachePath, word, status);
}

std::vector<LookupHistory::Entry> LookupHistory::load(const std::string& cachePath) {
  auto entries = readRecent(filePath(cachePath));
  std::reverse(entries.begin(), entries.end());
  return entries;
}

bool LookupHistory::removeRecentAt(const std::string& cachePath, int indexFromNewest) {
  if (indexFromNewest < 0) return false;
  const std::string path = filePath(cachePath);
  HalFile source;
  if (!Storage.openFileForRead("LH", path.c_str(), source)) return false;

  int lineCount = 0;
  bool inLine = false;
  while (source.available()) {
    const int byte = source.read();
    if (byte < 0) break;
    if (byte == '\n') {
      lineCount++;
      inLine = false;
    } else if (byte != '\r') {
      inLine = true;
    }
  }
  if (inLine) lineCount++;
  const int skipLine = lineCount - 1 - indexFromNewest;
  if (skipLine < 0) {
    source.close();
    return false;
  }

  source.seek(0);
  const std::string tempPath = path + ".tmp";
  HalFile destination;
  if (!Storage.openFileForWrite("LH", tempPath.c_str(), destination)) {
    source.close();
    return false;
  }

  int currentLine = 0;
  bool ok = true;
  while (source.available()) {
    const int byte = source.read();
    if (byte < 0) break;
    if (currentLine != skipLine && destination.write(static_cast<uint8_t>(byte)) != 1) {
      ok = false;
      break;
    }
    if (byte == '\n') currentLine++;
  }
  destination.flush();
  source.close();
  destination.close();

  if (!ok || !Storage.remove(path.c_str()) || !Storage.rename(tempPath.c_str(), path.c_str())) {
    LOG_ERR("LH", "Failed to remove history entry: %s", path.c_str());
    Storage.remove(tempPath.c_str());
    return false;
  }
  return true;
}
