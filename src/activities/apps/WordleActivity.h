#pragma once

#include "assets/WordList.h"
#include "activities/Activity.h"
#include "components/UITheme.h"
#include "FsHelpers.h"
#include <Serialization.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

class WordleActivity : public Activity {
public:
  WordleActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Wordle", renderer, mappedInput) {}

  ~WordleActivity() override {
    if (inGame && !showResetConfirm) {
      saveGameState();
    }
  }

  void onEnter() override {
    Activity::onEnter();
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    if (!loadGameState()) {
      newGame();
    }
    requestUpdate();
  }

  void loop() override {
    if (showResetConfirm) {
      if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
        showResetConfirm = false;
        requestUpdate();
        return;
      }
      if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
        showResetConfirm = false;
        newGame();
        requestUpdate();
        return;
      }
      return;
    }

    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      if (inGame) {
        saveGameState();
      } else {
        deleteSaveState();
      }
      finish();
      return;
    }

    if (inGame) {
      if (mappedInput.wasReleased(MappedInputManager::Button::Up)) {
        if (kbIndex == 29) {
          // Stay on reset button
        } else if (kbIndex == 0) kbIndex = 9;
        else if (kbIndex == 10) kbIndex = 19;
        else if (kbIndex == 20) kbIndex = 28;
        else if (kbIndex == 28) kbIndex = 27;
        else kbIndex--;
        requestUpdate();

      } else if (mappedInput.wasReleased(MappedInputManager::Button::Down)) {
        if (kbIndex == 29) {
          // Stay on reset button
        } else if (kbIndex == 9) kbIndex = 0;
        else if (kbIndex == 19) kbIndex = 10;
        else if (kbIndex == 28) kbIndex = 20;
        else if (kbIndex == 27) kbIndex = 28;
        else kbIndex++;
        requestUpdate();

      } else if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
        if (kbIndex == 29) { // reset -> Y
          kbIndex = 24; 
        } else if (kbIndex == 4 || kbIndex == 5) { // E or F -> reset
          kbIndex = 29; 
        } else if (kbIndex < 10) { // Rest of Row 0 -> Row 2 (Column strict wrap)
          if (kbIndex >= 8) kbIndex = 28; // I or J -> clear
          else kbIndex = 20 + kbIndex;
        } else if (kbIndex < 20) { // Row 1 -> Row 0
          kbIndex -= 10;
        } else { // Row 2 -> Row 1
          if (kbIndex == 28) kbIndex = 19; // clear -> T
          else kbIndex -= 10;
        }
        requestUpdate();

      } else if (mappedInput.wasReleased(MappedInputManager::Button::Right)) {
        if (kbIndex == 29) { // reset -> E
          kbIndex = 4;
        } else if (kbIndex == 24 || kbIndex == 25) { // Y or Z -> reset
          kbIndex = 29;
        } else if (kbIndex < 10) { // Row 0 -> Row 1
          kbIndex += 10;
        } else if (kbIndex < 20) { // Row 1 -> Row 2
          if (kbIndex >= 18) kbIndex = 28; // S or T -> clear
          else kbIndex += 10;
        } else { // Rest of Row 2 -> Row 0 (Column strict wrap)
          int col = kbIndex - 20;
          if (kbIndex == 28) kbIndex = 9; // clear -> J
          else kbIndex = col;
        }
        requestUpdate();

      } else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
        handleKeyboardSelect();
        requestUpdate();
      }
    } else {
      if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
        newGame();
        requestUpdate();
      }
    }
  }

  void render(RenderLock&& lock) override {
    renderer.clearScreen();

    const int pageWidth = renderer.getScreenWidth();
    const int pageHeight = renderer.getScreenHeight();

    const auto& metrics = UITheme::getInstance().getMetrics();
    const auto font = SETTINGS.getReaderFontId();

    GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, "Wordle",
                   gameWon ? "WINNER!" : (!inGame ? "GAME OVER" : ""));

    const int availableWidth = pageWidth - 24;
    const int tilePadding = 6;
    
    int tileSize = (availableWidth - (tilePadding * 4)) / 5;
    if (tileSize > 54) tileSize = 54;

    const int gridWidth = (tileSize * 5) + (tilePadding * 4);
    const int startX = (pageWidth - gridWidth) / 2;
    const int gridTopY = metrics.topPadding + metrics.headerHeight + 8;

    const int fontYOffset = 18; 

    // Render Grid
    for (int row = 0; row < 6; ++row) {
      const int rowY = gridTopY + row * (tileSize + tilePadding);

      for (int col = 0; col < 5; ++col) {
        const int tileX = startX + col * (tileSize + tilePadding);

        if (row < (int)guesses.size()) {
          const auto& g = guesses[row];
          int checkResult = g.check[col];
          char charStr[2] = {g.guess[col], '\0'};
          int textW = renderer.getTextWidth(font, charStr);
          int textX = tileX + (tileSize - textW) / 2;
          int textY = rowY + (tileSize / 2) - fontYOffset;

          if (checkResult == 2) { // Correct (Solid Box)
            renderer.fillRect(tileX, rowY, tileSize, tileSize);
            renderer.drawText(font, textX, textY, charStr, false);
          } else if (checkResult == 1) { // Present (Grey Box)
            renderer.fillRectDither(tileX, rowY, tileSize, tileSize, Color::LightGray);
            renderer.drawRect(tileX, rowY, tileSize, tileSize, true);
            renderer.drawText(font, textX, textY, charStr, true);
          } else { // Absent
            renderer.drawRect(tileX, rowY, tileSize, tileSize);
            renderer.drawText(font, textX, textY, charStr, true);
          }
        } else if (row == (int)guesses.size() && inGame) {
          renderer.drawRect(tileX, rowY, tileSize, tileSize);

          if (col == (int)currentGuess.length() && currentGuess.length() < 5) {
            renderer.fillRect(tileX + 20, rowY + 20, tileSize - 40, tileSize - 40);
          }

          if (col < (int)currentGuess.length()) {
            char charStr[2] = {currentGuess[col], '\0'};
            int textW = renderer.getTextWidth(font, charStr);
            int textX = tileX + (tileSize - textW) / 2;
            int textY = rowY + (tileSize / 2) - fontYOffset;
            renderer.drawText(font, textX, textY, charStr, true);
          }
        } else {
          renderer.drawRect(tileX, rowY, tileSize, tileSize);
        }
      }
    }

    // Render Keyboard (Rows 0-2)
    const int kbTopY = gridTopY + (6 * (tileSize + tilePadding)) + 8;
    const int keyPad = 3;
    const int keyW = (availableWidth - (9 * keyPad)) / 10;
    const int keyH = keyW; 

    const int kbRowWidth = (10 * keyW) + (9 * keyPad);
    const int kbStartX = (pageWidth - kbRowWidth) / 2;

    for (int i = 0; i < 29; ++i) {
      int kRow = i / 10;
      int kCol = i % 10;

      int kX = kbStartX + kCol * (keyW + keyPad);
      int kY = kbTopY + kRow * (keyH + keyPad);

      char kLabel[8] = {0};
      if (i < 26) {
        kLabel[0] = kKeys[i];
      } else if (i == 26) {
        snprintf(kLabel, sizeof(kLabel), "<-");
      } else if (i == 27) {
        snprintf(kLabel, sizeof(kLabel), "ok");
      } else {
        snprintf(kLabel, sizeof(kLabel), "clear");
      }

      int keyWD = (i == 28) ? (keyW * 2 + keyPad) : keyW;

      int labelW = renderer.getTextWidth(font, kLabel);
      int kTextX = kX + (keyWD - labelW) / 2;
      int kTextY = kY + (keyH / 2) - fontYOffset;

      bool isSelected = (i == kbIndex && inGame && !showResetConfirm);
      bool isAbsent = false;
      if (i < 26) {
        char keyChar = kKeys[i];
        if (std::find(absentKeys.begin(), absentKeys.end(), keyChar) != absentKeys.end()) {
          isAbsent = true;
        }
      }

      if (isSelected) {
        renderer.fillRect(kX, kY, keyWD, keyH);
        renderer.drawText(font, kTextX, kTextY, kLabel, false);
      } else if (isAbsent) {
        renderer.fillRectDither(kX, kY, keyWD, keyH, Color::LightGray);
        renderer.drawRect(kX, kY, keyWD, keyH, true);
        renderer.drawText(font, kTextX, kTextY, kLabel, true);
      } else {
        renderer.drawRect(kX, kY, keyWD, keyH);
        renderer.drawText(font, kTextX, kTextY, kLabel, true);
      }
    }

    // Render Row 3: reset Button (matching width of clear button)
    const int resetW = keyW * 2 + keyPad;
    const int resetX = (pageWidth - resetW) / 2;
    const int resetY = kbTopY + 3 * (keyH + keyPad);
    bool isResetSelected = (kbIndex == 29 && inGame && !showResetConfirm);

    int resetTextW = renderer.getTextWidth(font, "reset");
    int resetTextX = resetX + (resetW - resetTextW) / 2;
    int resetTextY = resetY + (keyH / 2) - fontYOffset;

    if (isResetSelected) {
      renderer.fillRect(resetX, resetY, resetW, keyH);
      renderer.drawText(font, resetTextX, resetTextY, "reset", false);
    } else {
      renderer.drawRect(resetX, resetY, resetW, keyH);
      renderer.drawText(font, resetTextX, resetTextY, "reset", true);
    }

    // Modal Confirmation Dialog
    if (showResetConfirm) {
      const int modalW = pageWidth - 60;
      const int modalH = 60;
      const int modalX = (pageWidth - modalW) / 2;
      const int modalY = (pageHeight - modalH) / 2 - 20;

      renderer.fillRect(modalX, modalY, modalW, modalH);
      renderer.drawRect(modalX + 2, modalY + 2, modalW - 4, modalH - 4, false);

      renderer.drawCenteredText(font, modalY + (modalH / 2) - fontYOffset, "RESET GAME?", false);
    }

    if (!inGame) {
      const int statusY = resetY + keyH + 6;
      if (gameWon) {
        renderer.drawCenteredText(font, statusY, "YOU WON!");
      } else {
        char lossBuf[32];
        snprintf(lossBuf, sizeof(lossBuf), "WORD WAS: %s", currentWord.c_str());
        renderer.drawCenteredText(font, statusY, lossBuf);
      }
    }

    if (showResetConfirm) {
      const auto labels = mappedInput.mapLabels("Cancel", "Confirm", "", "");
      GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
    } else if (inGame) {
      const auto labels = mappedInput.mapLabels(tr(STR_BACK), "Select", "Up", "Down");
      GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
    } else {
      const auto labels = mappedInput.mapLabels(tr(STR_BACK), "Play", "", "");
      GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
    }

    renderer.displayBuffer();
  }

private:
  struct GuessEntry {
    std::string guess;
    std::vector<int> check;
  };

  static constexpr int kKeyboardSize = 30; // 26 letters + 3 controls + 1 reset
  const char kKeys[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  const size_t wordCount = words_bin_len / 5;

  std::string currentWord = "";
  std::string currentGuess = "";
  std::vector<GuessEntry> guesses;
  std::vector<char> absentKeys;

  int kbIndex = 0;
  bool gameWon = false;
  bool inGame = true;
  bool showResetConfirm = false;

  static constexpr uint32_t GAME_MAGIC = 0x5752444C; // 'WRDL'
  static constexpr char stateFile[] = "/.smudge/wordle/state.bin";

  void ensureDirectoriesExist() {
    if (!Storage.exists("/.smudge")) {
      Storage.mkdir("/.smudge");
    }
    if (!Storage.exists("/.smudge/wordle")) {
      Storage.mkdir("/.smudge/wordle");
    }
  }

  void saveGameState() {
    ensureDirectoriesExist();

    HalFile file;
    if (!Storage.openFileForWrite("Wordle", stateFile, file)) {
      return;
    }

    serialization::writePod(file, GAME_MAGIC);
    serialization::writePod(file, static_cast<uint32_t>(inGame ? 1 : 0));
    serialization::writePod(file, static_cast<uint32_t>(gameWon ? 1 : 0));
    serialization::writePod(file, static_cast<uint32_t>(kbIndex));

    serialization::writeString(file, currentWord);
    serialization::writeString(file, currentGuess);

    uint32_t guessCount = static_cast<uint32_t>(guesses.size());
    serialization::writePod(file, guessCount);
    for (const auto& g : guesses) {
      serialization::writeString(file, g.guess);
      for (int c : g.check) {
        serialization::writePod(file, static_cast<int32_t>(c));
      }
    }

    uint32_t absentCount = static_cast<uint32_t>(absentKeys.size());
    serialization::writePod(file, absentCount);
    for (char c : absentKeys) {
      serialization::writePod(file, static_cast<uint8_t>(c));
    }

    file.close();
  }

  bool loadGameState() {
    if (!Storage.exists(stateFile)) {
      return false;
    }

    HalFile file;
    if (!Storage.openFileForRead("Wordle", stateFile, file)) {
      return false;
    }

    uint32_t magic = 0;
    if (!serialization::tryReadPod(file, magic) || magic != GAME_MAGIC) {
      file.close();
      Storage.remove(stateFile);
      return false;
    }

    uint32_t rawInGame = 0, rawGameWon = 0, rawKbIndex = 0;
    if (!serialization::tryReadPod(file, rawInGame) ||
        !serialization::tryReadPod(file, rawGameWon) ||
        !serialization::tryReadPod(file, rawKbIndex) ||
        !serialization::tryReadString(file, currentWord) ||
        !serialization::tryReadString(file, currentGuess)) {
      file.close();
      Storage.remove(stateFile);
      return false;
    }

    inGame = (rawInGame != 0);
    gameWon = (rawGameWon != 0);
    kbIndex = static_cast<int>(rawKbIndex);

    if (!inGame) {
      file.close();
      Storage.remove(stateFile);
      return false;
    }

    uint32_t guessCount = 0;
    if (!serialization::tryReadPod(file, guessCount)) {
      file.close();
      Storage.remove(stateFile);
      return false;
    }

    guesses.clear();
    for (uint32_t i = 0; i < guessCount; ++i) {
      GuessEntry entry;
      if (!serialization::tryReadString(file, entry.guess)) {
        file.close();
        Storage.remove(stateFile);
        return false;
      }
      entry.check.resize(5);
      for (int c = 0; c < 5; ++c) {
        int32_t val = 0;
        if (!serialization::tryReadPod(file, val)) {
          file.close();
          Storage.remove(stateFile);
          return false;
        }
        entry.check[c] = static_cast<int>(val);
      }
      guesses.push_back(entry);
    }

    uint32_t absentCount = 0;
    if (!serialization::tryReadPod(file, absentCount)) {
      file.close();
      Storage.remove(stateFile);
      return false;
    }

    absentKeys.clear();
    for (uint32_t i = 0; i < absentCount; ++i) {
      uint8_t keyChar = 0;
      if (!serialization::tryReadPod(file, keyChar)) {
        file.close();
        Storage.remove(stateFile);
        return false;
      }
      absentKeys.push_back(static_cast<char>(keyChar));
    }

    file.close();
    return true;
  }

  void deleteSaveState() {
    if (Storage.exists(stateFile)) {
      Storage.remove(stateFile);
    }
  }

  void newGame() {
    currentGuess = "";
    guesses.clear();
    absentKeys.clear();
    gameWon = false;
    inGame = true;
    showResetConfirm = false;
    kbIndex = 0;
    deleteSaveState();

    if (wordCount > 0) {
      size_t idx = rand() % wordCount;
      currentWord.assign(reinterpret_cast<const char*>(words_bin + (idx * 5)), 5);
    } else {
      currentWord = "CRANE";
    }
  }

  bool isValidWord(const std::string& word) {
    if (wordCount == 0 || word.length() != 5) return false;

    size_t low = 0;
    size_t high = wordCount;

    while (low < high) {
      size_t mid = low + (high - low) / 2;
      int cmp = memcmp(words_bin + (mid * 5), word.c_str(), 5);
      if (cmp == 0) return true;
      if (cmp < 0) low = mid + 1;
      else high = mid;
    }
    return false;
  }

  std::vector<int> checkWord(const std::string& word, const std::string& guess) {
    std::vector<int> returnCheck = {0, 0, 0, 0, 0};
    std::string runningCheck = "";
    std::string runningCheck2 = "";

    for (size_t i = 0; i < word.length() && i < 5; ++i) {
      if (std::tolower(word[i]) == std::tolower(guess[i])) {
        returnCheck[i] = 2;
      } else {
        runningCheck += std::tolower(word[i]);
      }
    }

    for (size_t i = 0; i < guess.length() && i < 5; ++i) {
      if (returnCheck[i] != 0) continue;

      for (int ii = (int)runningCheck.length() - 1; ii >= 0; --ii) {
        if (returnCheck[i] == 0 && std::tolower(guess[i]) == runningCheck[ii]) {
          returnCheck[i] = 1;
        } else {
          runningCheck2 += runningCheck[ii];
        }
      }
      runningCheck = runningCheck2;
      runningCheck2 = "";
    }

    return returnCheck;
  }

  void submitGuess() {
    if (currentGuess.length() != 5) return;

    if (!isValidWord(currentGuess)) {
      // Invalid word: keep the typed word in the grid and snap selection to 'clear' key (28)
      kbIndex = 28;
      return;
    }

    auto check = checkWord(currentWord, currentGuess);
    guesses.push_back({currentGuess, check});

    for (size_t i = 0; i < 5; ++i) {
      if (check[i] == 0) {
        char letter = currentGuess[i];
        bool appearsElsewhere = false;
        
        for (const auto& g : guesses) {
          for (size_t j = 0; j < 5; ++j) {
            if (g.guess[j] == letter && g.check[j] > 0) {
              appearsElsewhere = true;
              break;
            }
          }
          if (appearsElsewhere) break;
        }

        if (!appearsElsewhere && std::find(absentKeys.begin(), absentKeys.end(), letter) == absentKeys.end()) {
          absentKeys.push_back(letter);
        }
      }
    }

    bool win = true;
    for (int c : check) {
      if (c != 2) win = false;
    }

    if (win) {
      gameWon = true;
      inGame = false;
      deleteSaveState();
    } else if (guesses.size() >= 6) {
      inGame = false;
      deleteSaveState();
    }

    currentGuess = "";
  }

  void handleKeyboardSelect() {
    if (kbIndex < 26) {
      if (currentGuess.length() < 5) {
        currentGuess += kKeys[kbIndex];
        if (currentGuess.length() == 5) {
          kbIndex = 27; // Snap to 'ok' key
        }
      }
    } else if (kbIndex == 26) { // Backspace
      if (!currentGuess.empty()) {
        currentGuess.pop_back();
      }
    } else if (kbIndex == 27) { // ok key
      if (currentGuess.length() == 5) {
        submitGuess();
      }
    } else if (kbIndex == 28) { // clear key
      currentGuess.clear();
    } else if (kbIndex == 29) { // reset key
      showResetConfirm = true;
    }
  }
};