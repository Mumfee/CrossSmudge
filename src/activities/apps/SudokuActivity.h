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

class SudokuActivity : public Activity {
public:
  enum class Difficulty : uint8_t { Easy = 0, Medium = 1, Hard = 2 };

  SudokuActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Sudoku", renderer, mappedInput) {}

  ~SudokuActivity() override {
    // Only save state on activity destruction/sleep transition if a game is actively in progress
    if (!gameOver && !showDifficultyPicker) {
      saveGameState();
    }
  }

  void onEnter() override {
    Activity::onEnter();
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    if (!loadGameState()) {
      showDifficultyPicker = true;
    }
    requestUpdate();
  }

  void loop() override {
    // Handle Difficulty Selection Modal Controls
    if (showDifficultyPicker) {
      if (mappedInput.wasReleased(MappedInputManager::Button::Up) ||
          mappedInput.wasReleased(MappedInputManager::Button::Left)) {
        selectedPickerDiff = (selectedPickerDiff == 0) ? 2 : selectedPickerDiff - 1;
        requestUpdate();
      } 
      else if (mappedInput.wasReleased(MappedInputManager::Button::Down) ||
               mappedInput.wasReleased(MappedInputManager::Button::Right)) {
        selectedPickerDiff = (selectedPickerDiff + 1) % 3;
        requestUpdate();
      } 
      else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
        showDifficultyPicker = false;
        newGame(static_cast<Difficulty>(selectedPickerDiff));
        requestUpdate();
      } 
      else if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
        if (board[0][0] != 0 || solution[0][0] != 0) {
          showDifficultyPicker = false;
          requestUpdate();
        } else {
          finish();
        }
      }
      return;
    }

    // Save game state strictly when cleanly backing out of an active game
    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      if (!gameOver) {
        saveGameState();
      }
      finish();
      return;
    }

    if (gameOver) {
      if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
        showDifficultyPicker = true;
        requestUpdate();
      }
      return;
    }

    if (mappedInput.wasReleased(MappedInputManager::Button::Down)) {
      if (cursorRow == 9) {
        cursorRow = 10; // "new game" -> "hint"
      } else if (cursorRow == 10) {
        cursorRow = 9;  // "hint" wraps -> "new game"
      } else {
        cursorCol = (cursorCol == 8) ? 0 : cursorCol + 1;
      }
      requestUpdate();
    } 

    else if (mappedInput.wasReleased(MappedInputManager::Button::Right)) {
      if (cursorRow >= 9) {
        cursorRow = 0;
        cursorCol = 4;
      } else if (cursorRow == 8) {
        cursorRow = 9; // Grid bottom -> "new game" button
      } else {
        cursorRow++;
      }
      requestUpdate();
    }

    else if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
      if (cursorRow >= 9) {
        cursorRow = 8;
        cursorCol = 4;
      } else if (cursorRow == 0) {
        cursorRow = 9; // Grid top wraps -> "new game" button
      } else {
        cursorRow--;
      }
      requestUpdate();
    }

    else if (mappedInput.wasReleased(MappedInputManager::Button::Up)) {
      if (cursorRow == 10) {
        cursorRow = 9;  // "hint" -> "new game"
      } else if (cursorRow == 9) {
        cursorRow = 10; // "new game" wraps -> "hint"
      } else {
        cursorCol = (cursorCol == 0) ? 8 : cursorCol - 1;
      }
      requestUpdate();
    } 
    else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      if (cursorRow == 9) { // new game button
        showDifficultyPicker = true;
        requestUpdate();
      } else if (cursorRow == 10) { // hint button
        applyHint();
        checkWinCondition();
        requestUpdate();
      } else if (!initialBoard[cursorRow][cursorCol] && !hintBoard[cursorRow][cursorCol]) {
        board[cursorRow][cursorCol] = (board[cursorRow][cursorCol] + 1) % 10;
        checkWinCondition();
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

    char diffBuf[32];
    snprintf(diffBuf, sizeof(diffBuf), "%s", 
             currentDifficulty == Difficulty::Easy ? "EASY" :
             (currentDifficulty == Difficulty::Medium ? "MEDIUM" : "HARD"));
    GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, "Sudoku",
                   gameOver ? "SOLVED!" : diffBuf);

    const int footerHeight = 48;
    const int headerBottomY = metrics.topPadding + metrics.headerHeight;
    const int availableHeight = pageHeight - headerBottomY - footerHeight - 38;
    const int availableWidth = pageWidth - 16;

    int cellSize = std::min(availableWidth / 9, availableHeight / 9);
    int gridPixelSize = cellSize * 9;

    const int startX = (pageWidth - gridPixelSize) / 2;
    const int startY = headerBottomY + 4;
    const int fontYOffset = 14;

    // 1. Render 9x9 Grid Cells & Numbers (1-thickness grid)
    for (int r = 0; r < 9; ++r) {
      for (int c = 0; c < 9; ++c) {
        int x = startX + c * cellSize;
        int y = startY + r * cellSize;
        int val = board[r][c];

        bool isSelected = (r == cursorRow && c == cursorCol && !showDifficultyPicker && !gameOver);
        bool isGiven = initialBoard[r][c];
        bool isHint = hintBoard[r][c];

        if (isSelected) {
          renderer.fillRect(x, y, cellSize, cellSize);
        } else if (isHint) {
          // Dark Gray for Hinted cells
          renderer.fillRectDither(x, y, cellSize, cellSize, Color::DarkGray);
          renderer.drawRect(x, y, cellSize, cellSize, true);
        } else if (isGiven) {
          // Light Gray for Initial Givens
          renderer.fillRectDither(x, y, cellSize, cellSize, Color::LightGray);
          renderer.drawRect(x, y, cellSize, cellSize, true);
        } else {
          renderer.drawRect(x, y, cellSize, cellSize);
        }

        if (val > 0) {
          char valStr[2] = { static_cast<char>('0' + val), '\0' };
          int textW = renderer.getTextWidth(font, valStr);
          int textX = x + (cellSize - textW) / 2;
          int textY = y + (cellSize / 2) - fontYOffset;

          if (isSelected || isHint) {
            // White text for selected or dark gray hint cells
            renderer.drawText(font, textX, textY, valStr, false);
          } else {
            // Standard black text
            renderer.drawText(font, textX, textY, valStr, true);
          }
        }
      }
    }

    // 2. Draw 5-thickness lines for 3x3 Block Dividers and Outer Boarder
    for (int i = 0; i <= 3; ++i) {
      int posX = startX + i * (cellSize * 3);
      int posY = startY + i * (cellSize * 3);
      
      // Vertical 5-thick lines (offsets -2, -1, 0, +1, +2)
      for (int off = -2; off <= 2; ++off) {
        renderer.drawLine(posX + off, startY, posX + off, startY + gridPixelSize);
      }

      // Horizontal 5-thick lines (offsets -2, -1, 0, +1, +2)
      for (int off = -2; off <= 2; ++off) {
        renderer.drawLine(startX, posY + off, startX + gridPixelSize, posY + off);
      }
    }

    // 3. Render Bottom Content (Buttons during gameplay OR "You Won!!!" when solved)
    const int btnY = startY + gridPixelSize + 6;

    if (gameOver) {
      renderer.drawCenteredText(font, btnY + (cellSize / 2) - fontYOffset, "You Won!!!");
    } else {
      const int btnPad = 8;
      const int btnW = (gridPixelSize - btnPad) / 2;
      const int btnH = cellSize;
      const int btn1X = startX;
      const int btn2X = startX + btnW + btnPad;

      bool isBtn1Selected = (cursorRow == 9 && !showDifficultyPicker);
      bool isBtn2Selected = (cursorRow == 10 && !showDifficultyPicker);

      // Button 1: "New Game"
      int text1W = renderer.getTextWidth(font, "New Game");
      int text1X = btn1X + (btnW - text1W) / 2;
      int text1Y = btnY + (btnH / 2) - fontYOffset;

      if (isBtn1Selected) {
        renderer.fillRect(btn1X, btnY, btnW, btnH);
        renderer.drawText(font, text1X, text1Y, "New Game", false);
      } else {
        renderer.drawRect(btn1X, btnY, btnW, btnH);
        renderer.drawText(font, text1X, text1Y, "New Game", true);
      }

      // Button 2: "Hint"
      int text2W = renderer.getTextWidth(font, "Hint");
      int text2X = btn2X + (btnW - text2W) / 2;
      int text2Y = btnY + (btnH / 2) - fontYOffset;

      if (isBtn2Selected) {
        renderer.fillRect(btn2X, btnY, btnW, btnH);
        renderer.drawText(font, text2X, text2Y, "Hint", false);
      } else {
        renderer.drawRect(btn2X, btnY, btnW, btnH);
        renderer.drawText(font, text2X, text2Y, "Hint", true);
      }
    }

    // 4. Modal Difficulty Selection Dialog
    if (showDifficultyPicker) {
      const int modalW = pageWidth - 20;
      const int modalH = 100;
      const int modalX = (pageWidth - modalW) / 2;
      const int modalY = (pageHeight - modalH) / 2 - 20;

      renderer.fillRect(modalX, modalY, modalW, modalH);
      renderer.drawRect(modalX + 2, modalY + 2, modalW - 4, modalH - 4, false);

      renderer.drawCenteredText(font, modalY + 20, "SELECT DIFFICULTY", false);

      const int sectionW = modalW / 3;
      const int diffY = modalY + 60;

      const char* labels[3] = {"Easy", "Medium", "Hard"};

      for (int i = 0; i < 3; ++i) {
        char itemBuf[32];
        if (i == selectedPickerDiff) {
          snprintf(itemBuf, sizeof(itemBuf), "[ %s ]", labels[i]);
        } else {
          snprintf(itemBuf, sizeof(itemBuf), "%s", labels[i]);
        }

        int itemX = modalX + i * sectionW + (sectionW / 2);
        int textW = renderer.getTextWidth(font, itemBuf);
        renderer.drawText(font, itemX - (textW / 2), diffY, itemBuf, false);
      }
    }

    // 5. Footer Hints
    if (showDifficultyPicker) {
      const auto labels = mappedInput.mapLabels("Cancel", "Select", "<-", "->");
      GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
    } else if (gameOver) {
      const auto labels = mappedInput.mapLabels(tr(STR_BACK), "New Game", "", "");
      GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
    } else {
      const auto labels = mappedInput.mapLabels(tr(STR_BACK), "Select", "Up", "Down");
      GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
    }

    renderer.displayBuffer();
  }

private:
  uint8_t board[9][9] = {};
  uint8_t solution[9][9] = {};
  bool initialBoard[9][9] = {};
  bool hintBoard[9][9] = {};

  int cursorRow = 0;
  int cursorCol = 0;
  bool gameOver = false;
  bool showDifficultyPicker = false;
  int selectedPickerDiff = 0;
  Difficulty currentDifficulty = Difficulty::Easy;

  static constexpr uint32_t GAME_MAGIC = 0x5355444F; // 'SUDO'
  static constexpr char stateFile[] = "/.smudge/sudoku/state.bin";

  void ensureDirectoriesExist() {
    if (!Storage.exists("/.smudge")) {
      Storage.mkdir("/.smudge");
    }
    if (!Storage.exists("/.smudge/sudoku")) {
      Storage.mkdir("/.smudge/sudoku");
    }
  }

  void saveGameState() {
    ensureDirectoriesExist();

    HalFile file;
    if (!Storage.openFileForWrite("Sudoku", stateFile, file)) {
      return;
    }

    serialization::writePod(file, GAME_MAGIC);
    serialization::writePod(file, static_cast<uint8_t>(currentDifficulty));
    serialization::writePod(file, static_cast<uint8_t>(gameOver ? 1 : 0));
    serialization::writePod(file, static_cast<uint8_t>(cursorRow));
    serialization::writePod(file, static_cast<uint8_t>(cursorCol));

    for (int r = 0; r < 9; ++r) {
      for (int c = 0; c < 9; ++c) {
        serialization::writePod(file, board[r][c]);
        serialization::writePod(file, solution[r][c]);
        serialization::writePod(file, static_cast<uint8_t>(initialBoard[r][c] ? 1 : 0));
        serialization::writePod(file, static_cast<uint8_t>(hintBoard[r][c] ? 1 : 0));
      }
    }

    file.close();
  }

  bool loadGameState() {
    if (!Storage.exists(stateFile)) {
      return false;
    }

    HalFile file;
    if (!Storage.openFileForRead("Sudoku", stateFile, file)) {
      return false;
    }

    uint32_t magic = 0;
    if (!serialization::tryReadPod(file, magic) || magic != GAME_MAGIC) {
      file.close();
      Storage.remove(stateFile);
      return false;
    }

    uint8_t rawDiff = 0, rawGameOver = 0, rawRow = 0, rawCol = 0;
    if (!serialization::tryReadPod(file, rawDiff) ||
        !serialization::tryReadPod(file, rawGameOver) ||
        !serialization::tryReadPod(file, rawRow) ||
        !serialization::tryReadPod(file, rawCol)) {
      file.close();
      Storage.remove(stateFile);
      return false;
    }

    currentDifficulty = static_cast<Difficulty>(rawDiff);
    gameOver = (rawGameOver != 0);
    cursorRow = rawRow;
    cursorCol = rawCol;

    for (int r = 0; r < 9; ++r) {
      for (int c = 0; c < 9; ++c) {
        uint8_t isGiven = 0, isHint = 0;
        if (!serialization::tryReadPod(file, board[r][c]) ||
            !serialization::tryReadPod(file, solution[r][c]) ||
            !serialization::tryReadPod(file, isGiven) ||
            !serialization::tryReadPod(file, isHint)) {
          file.close();
          Storage.remove(stateFile);
          return false;
        }
        initialBoard[r][c] = (isGiven != 0);
        hintBoard[r][c] = (isHint != 0);
      }
    }

    file.close();
    return true;
  }

  void newGame(Difficulty diff) {
    currentDifficulty = diff;
    selectedPickerDiff = static_cast<int>(diff);
    gameOver = false;
    cursorRow = 0;
    cursorCol = 0;

    generateSudoku(diff);
  }

  void applyHint() {
    std::vector<std::pair<int, int>> emptyCells;
    std::vector<std::pair<int, int>> wrongCells;

    for (int r = 0; r < 9; ++r) {
      for (int c = 0; c < 9; ++c) {
        if (!initialBoard[r][c] && !hintBoard[r][c]) {
          if (board[r][c] == 0) {
            emptyCells.push_back({r, c});
          } else if (board[r][c] != solution[r][c]) {
            wrongCells.push_back({r, c});
          }
        }
      }
    }

    if (!emptyCells.empty()) {
      int idx = std::rand() % emptyCells.size();
      auto cell = emptyCells[idx];
      board[cell.first][cell.second] = solution[cell.first][cell.second];
      hintBoard[cell.first][cell.second] = true;
    } else if (!wrongCells.empty()) {
      int idx = std::rand() % wrongCells.size();
      auto cell = wrongCells[idx];
      board[cell.first][cell.second] = solution[cell.first][cell.second];
      hintBoard[cell.first][cell.second] = true;
    }
  }

  bool isSafe(uint8_t grid[9][9], int row, int col, uint8_t num) {
    for (int x = 0; x < 9; ++x) {
      if (grid[row][x] == num || grid[x][col] == num) return false;
    }

    int startRow = row - row % 3;
    int startCol = col - col % 3;
    for (int r = 0; r < 3; ++r) {
      for (int c = 0; c < 3; ++c) {
        if (grid[r + startRow][c + startCol] == num) return false;
      }
    }
    return true;
  }

  bool fillGrid(uint8_t grid[9][9], int row = 0, int col = 0) {
    if (row == 9) return true;
    if (col == 9) return fillGrid(grid, row + 1, 0);

    uint8_t nums[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    
    for (int i = 8; i > 0; --i) {
      int j = std::rand() % (i + 1);
      uint8_t temp = nums[i];
      nums[i] = nums[j];
      nums[j] = temp;
    }

    for (int i = 0; i < 9; ++i) {
      uint8_t num = nums[i];
      if (isSafe(grid, row, col, num)) {
        grid[row][col] = num;
        if (fillGrid(grid, row, col + 1)) return true;
        grid[row][col] = 0;
      }
    }
    return false;
  }

  void generateSudoku(Difficulty diff) {
    for (int r = 0; r < 9; ++r) {
      for (int c = 0; c < 9; ++c) {
        solution[r][c] = 0;
        board[r][c] = 0;
        initialBoard[r][c] = false;
        hintBoard[r][c] = false;
      }
    }

    // 1. Generate Solved Grid
    fillGrid(solution);

    // Copy to board
    for (int r = 0; r < 9; ++r) {
      for (int c = 0; c < 9; ++c) {
        board[r][c] = solution[r][c];
      }
    }

    // 2. Remove items based on difficulty target
    int removeCount = (diff == Difficulty::Easy) ? 35 : ((diff == Difficulty::Medium) ? 45 : 52);

    while (removeCount > 0) {
      int r = std::rand() % 9;
      int c = std::rand() % 9;
      if (board[r][c] != 0) {
        board[r][c] = 0;
        removeCount--;
      }
    }

    // Mark givens
    for (int r = 0; r < 9; ++r) {
      for (int c = 0; c < 9; ++c) {
        initialBoard[r][c] = (board[r][c] != 0);
      }
    }
  }

  void checkWinCondition() {
    for (int r = 0; r < 9; ++r) {
      for (int c = 0; c < 9; ++c) {
        if (board[r][c] == 0 || board[r][c] != solution[r][c]) {
          return;
        }
      }
    }
    gameOver = true;
  }
};