#pragma once

#include "activities/Activity.h"
#include "components/UITheme.h"
#include "FsHelpers.h"
#include <Serialization.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>

class TwoZeroFourEightActivity : public Activity {
public:
  TwoZeroFourEightActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("2048", renderer, mappedInput) {}

  ~TwoZeroFourEightActivity() override {
    // Save on activity destruction / sleep transition
    if (!gameOver && !showResetConfirm) {
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
      if (!gameOver) {
        saveGameState();
      } else {
        deleteSaveState();
      }
      finish();
      return;
    }

    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      if (gameOver) {
        newGame();
      } else {
        showResetConfirm = true;
      }
      requestUpdate();
      return;
    }

    bool moved = false;

    if (mappedInput.wasReleased(MappedInputManager::Button::Up)) {
      moved = moveLeft();
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Down)) {
      moved = moveRight();
    } 
    else if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
      moved = moveUp();
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Right)) {
      moved = moveDown();
    }

    if (moved) {
      spawnTile();
      checkGameOver();
      
      // Clear saved file if the move results in Game Over
      if (gameOver) {
        deleteSaveState();
      }
      
      requestUpdate();
    }
  }

  void render(RenderLock&& lock) override {
    renderer.clearScreen();

    const int pageWidth = renderer.getScreenWidth();
    const int pageHeight = renderer.getScreenHeight();

    const auto& metrics = UITheme::getInstance().getMetrics();
    const auto font = SETTINGS.getReaderFontId();

    // 1. Header with Score Display
    char scoreBuf[32];
    snprintf(scoreBuf, sizeof(scoreBuf), "Score: %d", score);
    GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, "2048",
                   gameOver ? "GAME OVER!" : scoreBuf);

    // 2. Maximize Grid Geometry
    const int boardSize = 4;
    const int padding = 6;
    const int footerHeight = 48;
    const int headerBottomY = metrics.topPadding + metrics.headerHeight;

    const int availableWidth = pageWidth - 16;
    const int tileSize = (availableWidth - (padding * (boardSize - 1))) / boardSize;

    const int gridPixelSize = (tileSize * boardSize) + (padding * (boardSize - 1));
    const int startX = (pageWidth - gridPixelSize) / 2;

    const int contentAreaHeight = pageHeight - headerBottomY - footerHeight;
    const int startY = headerBottomY + (contentAreaHeight - gridPixelSize) / 2;

    const int fontYOffset = 18;

    // 3. Render Grid & Tiles
    for (int r = 0; r < boardSize; ++r) {
      for (int c = 0; c < boardSize; ++c) {
        int x = startX + c * (tileSize + padding);
        int y = startY + r * (tileSize + padding);
        int val = board[r][c];

        if (val == 0) {
          renderer.drawRect(x, y, tileSize, tileSize);
        } else {
          char valStr[16];
          snprintf(valStr, sizeof(valStr), "%d", val);
          int textW = renderer.getTextWidth(font, valStr);
          int textX = x + (tileSize - textW) / 2;
          int textY = y + (tileSize / 2) - fontYOffset;

          if (val >= 2048) {
            // Inverted solid black box for high-value targets (2048+)
            renderer.fillRect(x, y, tileSize, tileSize);
            renderer.drawText(font, textX, textY, valStr, false);
          } else if (val >= 512) {
            renderer.fillRectDither(x, y, tileSize, tileSize, Color::DarkGray);
            renderer.drawRect(x, y, tileSize, tileSize, true);
            renderer.drawText(font, textX, textY, valStr, true);
          } else if (val >= 64) {
            renderer.fillRectDither(x, y, tileSize, tileSize, Color::LightGray);
            renderer.drawRect(x, y, tileSize, tileSize, true);
            renderer.drawText(font, textX, textY, valStr, true);
          } else if (val >= 8) {
            renderer.drawRect(x, y, tileSize, tileSize);
            renderer.drawRect(x + 2, y + 2, tileSize - 4, tileSize - 4);
            renderer.drawText(font, textX, textY, valStr, true);
          } else {
            renderer.drawRect(x, y, tileSize, tileSize);
            renderer.drawText(font, textX, textY, valStr, true);
          }
        }
      }
    }

    // 4. Modal Confirmation Dialog
    if (showResetConfirm) {
      const int modalW = pageWidth - 60;
      const int modalH = 60;
      const int modalX = (pageWidth - modalW) / 2;
      const int modalY = (pageHeight - modalH) / 2 - 20;

      renderer.fillRect(modalX, modalY, modalW, modalH);
      renderer.drawRect(modalX + 2, modalY + 2, modalW - 4, modalH - 4, false);

      renderer.drawCenteredText(font, modalY + (modalH / 2) - fontYOffset, "RESET GAME?", false);
    }

    // 5. Footer Hints
    if (showResetConfirm) {
      const auto labels = mappedInput.mapLabels("Cancel", "Confirm", "", "");
      GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
    } else {
      const auto labels = mappedInput.mapLabels(tr(STR_BACK), "Reset", "Up", "Down");
      GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
    }

    renderer.displayBuffer();
  }

private:
  int board[4][4] = {};
  int score = 0;
  bool gameOver = false;
  bool showResetConfirm = false;

  static constexpr uint32_t GAME_MAGIC = 0x32303438; // '2048'
  static constexpr char stateFile[] = "/.smudge/2048/state.bin";

  void ensureDirectoriesExist() {
    if (!Storage.exists("/.smudge")) {
      Storage.mkdir("/.smudge");
    }
    if (!Storage.exists("/.smudge/2048")) {
      Storage.mkdir("/.smudge/2048");
    }
  }

  void saveGameState() {
    ensureDirectoriesExist();

    HalFile file;
    if (!Storage.openFileForWrite("2048", stateFile, file)) {
      return;
    }

    serialization::writePod(file, GAME_MAGIC);
    serialization::writePod(file, static_cast<uint32_t>(score));
    serialization::writePod(file, static_cast<uint32_t>(gameOver ? 1 : 0));

    for (int r = 0; r < 4; ++r) {
      for (int c = 0; c < 4; ++c) {
        serialization::writePod(file, board[r][c]);
      }
    }

    file.close();
  }

  bool loadGameState() {
    if (!Storage.exists(stateFile)) {
      return false;
    }

    HalFile file;
    if (!Storage.openFileForRead("2048", stateFile, file)) {
      return false;
    }

    uint32_t magic = 0;
    if (!serialization::tryReadPod(file, magic) || magic != GAME_MAGIC) {
      file.close();
      Storage.remove(stateFile);
      return false;
    }

    uint32_t loadedScore = 0;
    uint32_t loadedGameOver = 0;

    if (!serialization::tryReadPod(file, loadedScore) ||
        !serialization::tryReadPod(file, loadedGameOver)) {
      file.close();
      Storage.remove(stateFile);
      return false;
    }

    int tempBoard[4][4] = {};
    for (int r = 0; r < 4; ++r) {
      for (int c = 0; c < 4; ++c) {
        if (!serialization::tryReadPod(file, tempBoard[r][c])) {
          file.close();
          Storage.remove(stateFile);
          return false;
        }
      }
    }

    file.close();

    score = static_cast<int>(loadedScore);
    gameOver = (loadedGameOver != 0);
    for (int r = 0; r < 4; ++r) {
      for (int c = 0; c < 4; ++c) {
        board[r][c] = tempBoard[r][c];
      }
    }

    return true;
  }

  void deleteSaveState() {
    if (Storage.exists(stateFile)) {
      Storage.remove(stateFile);
    }
  }

  void newGame() {
    score = 0;
    gameOver = false;
    showResetConfirm = false;
    deleteSaveState();
    for (int r = 0; r < 4; ++r) {
      for (int c = 0; c < 4; ++c) {
        board[r][c] = 0;
      }
    }
    spawnTile();
    spawnTile();
  }

  void spawnTile() {
    std::vector<std::pair<int, int>> emptyCells;
    for (int r = 0; r < 4; ++r) {
      for (int c = 0; c < 4; ++c) {
        if (board[r][c] == 0) {
          emptyCells.push_back({r, c});
        }
      }
    }

    if (!emptyCells.empty()) {
      int idx = rand() % emptyCells.size();
      auto cell = emptyCells[idx];
      board[cell.first][cell.second] = (rand() % 10 == 0) ? 4 : 2;
    }
  }

  bool slideAndMergeRow(int row[4]) {
    bool changed = false;
    int temp[4] = {0};
    int target = 0;

    for (int i = 0; i < 4; ++i) {
      if (row[i] != 0) {
        temp[target++] = row[i];
      }
    }

    for (int i = 0; i < 3; ++i) {
      if (temp[i] != 0 && temp[i] == temp[i + 1]) {
        temp[i] *= 2;
        score += temp[i];
        temp[i + 1] = 0;
        changed = true;
      }
    }

    int finalRow[4] = {0};
    target = 0;
    for (int i = 0; i < 4; ++i) {
      if (temp[i] != 0) {
        finalRow[target++] = temp[i];
      }
    }

    for (int i = 0; i < 4; ++i) {
      if (row[i] != finalRow[i]) {
        changed = true;
      }
      row[i] = finalRow[i];
    }

    return changed;
  }

  bool moveLeft() {
    bool moved = false;
    for (int r = 0; r < 4; ++r) {
      if (slideAndMergeRow(board[r])) {
        moved = true;
      }
    }
    return moved;
  }

  bool moveRight() {
    bool moved = false;
    for (int r = 0; r < 4; ++r) {
      int row[4] = {board[r][3], board[r][2], board[r][1], board[r][0]};
      if (slideAndMergeRow(row)) {
        moved = true;
        board[r][3] = row[0];
        board[r][2] = row[1];
        board[r][1] = row[2];
        board[r][0] = row[3];
      }
    }
    return moved;
  }

  bool moveUp() {
    bool moved = false;
    for (int c = 0; c < 4; ++c) {
      int col[4] = {board[0][c], board[1][c], board[2][c], board[3][c]};
      if (slideAndMergeRow(col)) {
        moved = true;
        board[0][c] = col[0];
        board[1][c] = col[1];
        board[2][c] = col[2];
        board[3][c] = col[3];
      }
    }
    return moved;
  }

  bool moveDown() {
    bool moved = false;
    for (int c = 0; c < 4; ++c) {
      int col[4] = {board[3][c], board[2][c], board[1][c], board[0][c]};
      if (slideAndMergeRow(col)) {
        moved = true;
        board[3][c] = col[0];
        board[2][c] = col[1];
        board[1][c] = col[2];
        board[0][c] = col[3];
      }
    }
    return moved;
  }

  void checkGameOver() {
    for (int r = 0; r < 4; ++r) {
      for (int c = 0; c < 4; ++c) {
        if (board[r][c] == 0) {
          gameOver = false;
          return;
        }
      }
    }

    for (int r = 0; r < 4; ++r) {
      for (int c = 0; c < 4; ++c) {
        if (c < 3 && board[r][c] == board[r][c + 1]) {
          gameOver = false;
          return;
        }
        if (r < 3 && board[r][c] == board[r + 1][c]) {
          gameOver = false;
          return;
        }
      }
    }

    gameOver = true;
  }
};