#pragma once

#include "activities/Activity.h"
#include "components/UITheme.h"
#include "FsHelpers.h"
#include <Serialization.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

class TetrisActivity : public Activity {
public:
  TetrisActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Tetris", renderer, mappedInput) {}

  ~TetrisActivity() override {
    // Only save when exiting or entering sleep
    if (!isGameOver && !showResetConfirm) {
      saveGameState();
    } else {
      deleteSaveState();
    }
  }

  void onEnter() override {
    Activity::onEnter();
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // Load file only once on initial entry
    if (!loadGameState()) {
      startNewGame();
    }
    lastFallTime = millis();
    lastDropHoldTime = millis();
    requestUpdate();
  }

  void loop() override {
    // 1. Reset Confirmation Modal Handling
    if (showResetConfirm) {
      if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
        showResetConfirm = false;
        requestUpdate();
        return;
      }
      if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
        showResetConfirm = false;
        startNewGame();
        requestUpdate();
        return;
      }
      return;
    }

    // 2. Standard Exit
    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      if (!isGameOver) {
        saveGameState();
      } else {
        deleteSaveState();
      }
      finish();
      return;
    }

    // 3. Game Over Restart
    if (isGameOver) {
      if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
        startNewGame();
        requestUpdate();
      }
      return;
    }

    // 4. Line Clear Flash Animation Wait
    if (isClearingLines) {
      if (millis() - clearStartTime >= kClearDelayMs) {
        finishLineClear();
        requestUpdate();
      }
      return;
    }

    // 5. In-Game Controls
    if (mappedInput.wasReleased(MappedInputManager::Button::Down)) {
      // Prompt Reset Confirmation
      showResetConfirm = true;
      requestUpdate();
      return;
    }

    if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
      if (canMove(currentPiece, pieceX - 1, pieceY, currentRotation)) {
        pieceX--;
        requestUpdate();
      }
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Right)) {
      if (canMove(currentPiece, pieceX + 1, pieceY, currentRotation)) {
        pieceX++;
        requestUpdate();
      }
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Up)) {
      // Down Button = Rotate Piece
      int nextRot = (currentRotation + 1) % 4;
      if (canMove(currentPiece, pieceX, pieceY, nextRot)) {
        currentRotation = nextRot;
        requestUpdate();
      } else if (canMove(currentPiece, pieceX - 1, pieceY, nextRot)) { // Wall-kick left
        pieceX--;
        currentRotation = nextRot;
        requestUpdate();
      } else if (canMove(currentPiece, pieceX + 1, pieceY, nextRot)) { // Wall-kick right
        pieceX++;
        currentRotation = nextRot;
        requestUpdate();
      }
    }

    // 6. Hold-to-Drop (Continuous Fast Soft Drop)
    if (mappedInput.isPressed(MappedInputManager::Button::Confirm)) {
      if (millis() - lastDropHoldTime >= kFastDropIntervalMs) {
        lastDropHoldTime = millis();
        if (canMove(currentPiece, pieceX, pieceY + 1, currentRotation)) {
          pieceY++;
          score += 1;
          lastFallTime = millis();
          requestUpdate();
        } else {
          lockPiece();
          requestUpdate();
        }
      }
    }

    // 7. Standard Gravity / Automatic Fall
    uint32_t fallInterval = getFallSpeed();
    if (millis() - lastFallTime >= fallInterval) {
      lastFallTime = millis();
      if (canMove(currentPiece, pieceX, pieceY + 1, currentRotation)) {
        pieceY++;
        requestUpdate();
      } else {
        lockPiece();
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
    const int textLineHeight = renderer.getLineHeight(font);

    const int topBound = metrics.topPadding + metrics.headerHeight;
    const int bottomBound = pageHeight - metrics.buttonHintsHeight;
    const int availableHeight = bottomBound - topBound - 12;

    // 1. Fill Main Background with Light Gray Dither
    renderer.fillRectDither(0, topBound, pageWidth, bottomBound - topBound, Color::LightGray);

    // 2. Header with Score, Lines, Level
    char headerRight[64];
    snprintf(headerRight, sizeof(headerRight), "Score:%d | Lns:%d | Lvl:%d", score, linesCleared, level);
    GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, "Tetris", headerRight);

    // Calculate grid tile dimensions
    const int blockSize = std::min(availableHeight / kGridHeight, (pageWidth - 110) / kGridWidth);
    const int boardPixelW = blockSize * kGridWidth;
    const int boardPixelH = blockSize * kGridHeight;
    const int boardX = 20; 
    const int boardY = topBound + 6 + (availableHeight - boardPixelH) / 2;

    // 3. Draw Solid White Board Area & Boundary
    renderer.fillRect(boardX, boardY, boardPixelW, boardPixelH, false);
    renderer.drawRect(boardX - 2, boardY - 2, boardPixelW + 4, boardPixelH + 4, true);

    // 4. Draw Grid Cells
    for (int y = 0; y < kGridHeight; ++y) {
      if (isClearingLines && linesToClear[y]) {
        renderer.fillRect(boardX, boardY + (y * blockSize), boardPixelW, blockSize, true);
        continue;
      }

      for (int x = 0; x < kGridWidth; ++x) {
        int cellX = boardX + (x * blockSize);
        int cellY = boardY + (y * blockSize);

        if (grid[y][x] > 0) {
          drawBlock(cellX, cellY, blockSize, grid[y][x]);
        } else {
          // Background dot grid
          renderer.drawPixel(cellX + blockSize / 2, cellY + blockSize / 2);
        }
      }
    }

    // 5. Draw Current Falling Piece
    if (!isGameOver && !isClearingLines) {
      const auto& shape = kTetrominoes[currentPiece][currentRotation];
      for (int py = 0; py < 4; ++py) {
        for (int px = 0; px < 4; ++px) {
          if (shape[py][px] != 0) {
            int targetGridY = pieceY + py;
            int targetGridX = pieceX + px;

            if (targetGridY >= 0 && targetGridY < kGridHeight && targetGridX >= 0 && targetGridX < kGridWidth) {
              int blockX = boardX + (targetGridX * blockSize);
              int blockY = boardY + (targetGridY * blockSize);
              drawBlock(blockX, blockY, blockSize, currentPiece + 1);
            }
          }
        }
      }
    }

    // 6. Sidebar - Next Piece Box Preview
    const int sidebarX = boardX + boardPixelW + 16;
    int sidebarY = boardY + 12;

    renderer.drawText(font, sidebarX, sidebarY, "Next");
    sidebarY += textLineHeight + 6;

    const int previewBoxSize = 56;
    renderer.fillRoundedRect(sidebarX, sidebarY, previewBoxSize, previewBoxSize, 4, Color::White);
    renderer.drawRoundedRect(sidebarX, sidebarY, previewBoxSize, previewBoxSize, 1, 4, true);

    // Center piece inside preview box
    const int pBlockSize = 10;
    const auto& nextShape = kTetrominoes[nextPiece][0];

    int minX = 4, maxX = -1, minY = 4, maxY = -1;
    for (int py = 0; py < 4; ++py) {
      for (int px = 0; px < 4; ++px) {
        if (nextShape[py][px] != 0) {
          minX = std::min(minX, px);
          maxX = std::max(maxX, px);
          minY = std::min(minY, py);
          maxY = std::max(maxY, py);
        }
      }
    }

    int shapePixelW = (maxX - minX + 1) * pBlockSize;
    int shapePixelH = (maxY - minY + 1) * pBlockSize;
    int startPreviewX = sidebarX + (previewBoxSize - shapePixelW) / 2;
    int startPreviewY = sidebarY + (previewBoxSize - shapePixelH) / 2;

    for (int py = minY; py <= maxY; ++py) {
      for (int px = minX; px <= maxX; ++px) {
        if (nextShape[py][px] != 0) {
          int bx = startPreviewX + (px - minX) * pBlockSize;
          int by = startPreviewY + (py - minY) * pBlockSize;
          drawBlock(bx, by, pBlockSize, nextPiece + 1);
        }
      }
    }

    // 7. Modal Confirmation Dialog
    if (showResetConfirm) {
      const int modalW = pageWidth - 60;
      const int modalH = 60;
      const int modalX = (pageWidth - modalW) / 2;
      const int modalY = (pageHeight - modalH) / 2 - 20;

      renderer.fillRect(modalX, modalY, modalW, modalH);
      renderer.drawRect(modalX + 2, modalY + 2, modalW - 4, modalH - 4, false);
      renderer.drawCenteredText(font, modalY + (modalH / 2) - 18, "RESET GAME?", false);
    } else if (isGameOver) {
      int modalW = 180;
      int modalH = 70;
      int modalX = (pageWidth - modalW) / 2;
      int modalY = boardY + (boardPixelH - modalH) / 2;

      renderer.fillRoundedRect(modalX, modalY, modalW, modalH, 6, Color::White);
      renderer.drawRoundedRect(modalX, modalY, modalW, modalH, 2, 6, true);
      renderer.drawCenteredText(font, modalY + 16, "GAME OVER");
      renderer.drawCenteredText(font, modalY + 40, "Press Drop to Play");
    }

    // 8. Footer Button Hints
    if (showResetConfirm) {
      const auto labels = mappedInput.mapLabels("Cancel", "Confirm", "", "");
      GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
    } else if (isGameOver) {
      const auto labels = mappedInput.mapLabels(tr(STR_BACK), "Restart", "", "");
      GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
    } else {
      const auto labels = mappedInput.mapLabels(tr(STR_BACK), "Drop", "Left", "Right");
      GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
    }

    renderer.displayBuffer();
  }

private:
  enum class PieceStyle {
    SolidBlack,         // 1. I: Solid Black
    SolidWhite,         // 2. O: Solid White Box
    SegmentedWhite,     // 3. T: White Box with Inner Inset Frame
    SolidLightGray,     // 4. L: Solid Light Gray Dither
    SegmentedLightGray, // 5. J: Light Gray Dither with Inset Frame
    SolidDarkGray,      // 6. S: Solid Dark Gray Dither
    SegmentedDarkGray   // 7. Z: Dark Gray Dither with Inset Frame
  };

  static constexpr int kGridWidth = 10;
  static constexpr int kGridHeight = 20;
  static constexpr uint32_t GAME_MAGIC = 0x54455452; // 'TETR'
  static constexpr char stateFile[] = "/.smudge/tetris/state.bin";
  static constexpr uint32_t kClearDelayMs = 150;
  static constexpr uint32_t kFastDropIntervalMs = 50;

  int grid[kGridHeight][kGridWidth] = {{0}};

  int currentPiece = 0;
  int currentRotation = 0;
  int pieceX = 3;
  int pieceY = 0;
  int nextPiece = 0;

  int score = 0;
  int linesCleared = 0;
  int level = 1;
  bool isGameOver = false;
  bool showResetConfirm = false;

  uint32_t lastFallTime = 0;
  uint32_t lastDropHoldTime = 0;

  // Line Clear Animation state
  bool isClearingLines = false;
  uint32_t clearStartTime = 0;
  bool linesToClear[kGridHeight] = {false};

  // 7 Unique Styles mapped to the 7 pieces (index 0..6)
  static constexpr PieceStyle kPieceStyles[7] = {
      PieceStyle::SolidBlack,         // 0 -> I
      PieceStyle::SegmentedLightGray, // 1 -> J
      PieceStyle::SolidLightGray,     // 2 -> L
      PieceStyle::SolidWhite,         // 3 -> O
      PieceStyle::SolidDarkGray,      // 4 -> S
      PieceStyle::SegmentedWhite,     // 5 -> T
      PieceStyle::SegmentedDarkGray   // 6 -> Z
  };

  // 7 Standard Tetrominoes in 4 Rotations
  static constexpr uint8_t kTetrominoes[7][4][4][4] = {
      // I
      {{{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
       {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}},
       {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
       {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}}},
      // J
      {{{1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
       {{0,1,1,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}},
       {{0,0,0,0},{1,1,1,0},{0,0,1,0},{0,0,0,0}},
       {{0,1,0,0},{0,1,0,0},{1,1,0,0},{0,0,0,0}}},
      // L
      {{{0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
       {{0,1,0,0},{0,1,0,0},{0,1,1,0},{0,0,0,0}},
       {{0,0,0,0},{1,1,1,0},{1,0,0,0},{0,0,0,0}},
       {{1,1,0,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}}},
      // O
      {{{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
       {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
       {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
       {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}}},
      // S
      {{{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
       {{0,1,0,0},{0,1,1,0},{0,0,1,0},{0,0,0,0}},
       {{0,0,0,0},{0,1,1,0},{1,1,0,0},{0,0,0,0}},
       {{1,0,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}}},
      // T
      {{{0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
       {{0,1,0,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},
       {{0,0,0,0},{1,1,1,0},{0,1,0,0},{0,0,0,0}},
       {{0,1,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}}},
      // Z
      {{{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
       {{0,0,1,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},
       {{0,0,0,0},{1,1,0,0},{0,1,1,0},{0,0,0,0}},
       {{0,1,0,0},{1,1,0,0},{1,0,0,0},{0,0,0,0}}}
  };

  void drawBlock(int x, int y, int size, int typeId) {
    if (typeId <= 0 || typeId > 7) return;
    PieceStyle style = kPieceStyles[typeId - 1];

    switch (style) {
      case PieceStyle::SolidBlack:
        renderer.fillRect(x, y, size, size, true);
        break;

      case PieceStyle::SolidWhite:
        renderer.drawRect(x, y, size, size, true);
        if (size > 2) {
          renderer.fillRect(x + 1, y + 1, size - 2, size - 2, false);
        }
        break;

      case PieceStyle::SegmentedWhite:
        renderer.drawRect(x, y, size, size, true);
        if (size > 4) {
          renderer.fillRect(x + 1, y + 1, size - 2, size - 2, false);
          renderer.drawRect(x + 2, y + 2, size - 4, size - 4, true); // Inner segment frame
        }
        break;

      case PieceStyle::SolidLightGray:
        renderer.drawRect(x, y, size, size, true);
        if (size > 2) {
          renderer.fillRectDither(x + 1, y + 1, size - 2, size - 2, Color::LightGray);
        }
        break;

      case PieceStyle::SegmentedLightGray:
        renderer.drawRect(x, y, size, size, true);
        if (size > 4) {
          renderer.fillRectDither(x + 1, y + 1, size - 2, size - 2, Color::LightGray);
          renderer.drawRect(x + 2, y + 2, size - 4, size - 4, true); // Inner segment frame
        }
        break;

      case PieceStyle::SolidDarkGray:
        renderer.drawRect(x, y, size, size, true);
        if (size > 2) {
          renderer.fillRectDither(x + 1, y + 1, size - 2, size - 2, Color::DarkGray);
        }
        break;

      case PieceStyle::SegmentedDarkGray:
        renderer.drawRect(x, y, size, size, true);
        if (size > 4) {
          renderer.fillRectDither(x + 1, y + 1, size - 2, size - 2, Color::DarkGray);
          renderer.drawRect(x + 2, y + 2, size - 4, size - 4, true); // Inner segment frame
        }
        break;
    }
  }

  uint32_t getFallSpeed() const {
    return std::max(120, 800 - ((level - 1) * 70));
  }

  void startNewGame() {
    for (int y = 0; y < kGridHeight; ++y) {
      linesToClear[y] = false;
      for (int x = 0; x < kGridWidth; ++x) {
        grid[y][x] = 0;
      }
    }
    score = 0;
    linesCleared = 0;
    level = 1;
    isGameOver = false;
    showResetConfirm = false;
    isClearingLines = false;
    nextPiece = rand() % 7;
    spawnPiece();
  }

  void spawnPiece() {
    currentPiece = nextPiece;
    nextPiece = rand() % 7;
    currentRotation = 0;
    pieceX = 3;
    pieceY = 0;

    if (!canMove(currentPiece, pieceX, pieceY, currentRotation)) {
      isGameOver = true;
    }
  }

  bool canMove(int piece, int testX, int testY, int rotation) const {
    const auto& shape = kTetrominoes[piece][rotation];
    for (int py = 0; py < 4; ++py) {
      for (int px = 0; px < 4; ++px) {
        if (shape[py][px] != 0) {
          int targetX = testX + px;
          int targetY = testY + py;

          if (targetX < 0 || targetX >= kGridWidth || targetY >= kGridHeight) {
            return false;
          }
          if (targetY >= 0 && grid[targetY][targetX] != 0) {
            return false;
          }
        }
      }
    }
    return true;
  }

  void lockPiece() {
    const auto& shape = kTetrominoes[currentPiece][currentRotation];
    for (int py = 0; py < 4; ++py) {
      for (int px = 0; px < 4; ++px) {
        if (shape[py][px] != 0) {
          int targetX = pieceX + px;
          int targetY = pieceY + py;

          if (targetY >= 0 && targetY < kGridHeight && targetX >= 0 && targetX < kGridWidth) {
            grid[targetY][targetX] = currentPiece + 1;
          }
        }
      }
    }

    checkAndStartLineClear();
  }

  void checkAndStartLineClear() {
    int clearedCount = 0;
    for (int y = 0; y < kGridHeight; ++y) {
      bool full = true;
      for (int x = 0; x < kGridWidth; ++x) {
        if (grid[y][x] == 0) {
          full = false;
          break;
        }
      }
      linesToClear[y] = full;
      if (full) clearedCount++;
    }

    if (clearedCount > 0) {
      isClearingLines = true;
      clearStartTime = millis();
    } else {
      spawnPiece();
    }
  }

  void finishLineClear() {
    int cleared = 0;
    for (int y = kGridHeight - 1; y >= 0; --y) {
      if (linesToClear[y]) {
        cleared++;
        for (int pullY = y; pullY > 0; --pullY) {
          for (int x = 0; x < kGridWidth; ++x) {
            grid[pullY][x] = grid[pullY - 1][x];
          }
          linesToClear[pullY] = linesToClear[pullY - 1];
        }
        for (int x = 0; x < kGridWidth; ++x) {
          grid[0][x] = 0;
        }
        linesToClear[0] = false;
        y++; // Re-check this row index
      }
    }

    isClearingLines = false;

    if (cleared > 0) {
      linesCleared += cleared;
      level = (linesCleared / 10) + 1;

      if (cleared == 1) score += 100 * level;
      else if (cleared == 2) score += 300 * level;
      else if (cleared == 3) score += 500 * level;
      else if (cleared >= 4) score += 800 * level;
    }

    spawnPiece();
  }

  void ensureDirectoriesExist() {
    if (!Storage.exists("/.smudge")) {
      Storage.mkdir("/.smudge");
    }
    if (!Storage.exists("/.smudge/tetris")) {
      Storage.mkdir("/.smudge/tetris");
    }
  }

  void saveGameState() {
    ensureDirectoriesExist();

    HalFile file;
    if (!Storage.openFileForWrite("Tetris", stateFile, file)) {
      return;
    }

    serialization::writePod(file, GAME_MAGIC);
    serialization::writePod(file, static_cast<int32_t>(score));
    serialization::writePod(file, static_cast<int32_t>(linesCleared));
    serialization::writePod(file, static_cast<int32_t>(level));
    serialization::writePod(file, static_cast<int32_t>(currentPiece));
    serialization::writePod(file, static_cast<int32_t>(currentRotation));
    serialization::writePod(file, static_cast<int32_t>(pieceX));
    serialization::writePod(file, static_cast<int32_t>(pieceY));
    serialization::writePod(file, static_cast<int32_t>(nextPiece));

    for (int y = 0; y < kGridHeight; ++y) {
      for (int x = 0; x < kGridWidth; ++x) {
        serialization::writePod(file, static_cast<int32_t>(grid[y][x]));
      }
    }

    file.close();
  }

  bool loadGameState() {
    if (!Storage.exists(stateFile)) {
      return false;
    }

    HalFile file;
    if (!Storage.openFileForRead("Tetris", stateFile, file)) {
      return false;
    }

    uint32_t magic = 0;
    if (!serialization::tryReadPod(file, magic) || magic != GAME_MAGIC) {
      file.close();
      Storage.remove(stateFile);
      return false;
    }

    int32_t rScore = 0, rLines = 0, rLevel = 1;
    int32_t rPiece = 0, rRot = 0, rPx = 3, rPy = 0, rNext = 0;

    if (!serialization::tryReadPod(file, rScore) ||
        !serialization::tryReadPod(file, rLines) ||
        !serialization::tryReadPod(file, rLevel) ||
        !serialization::tryReadPod(file, rPiece) ||
        !serialization::tryReadPod(file, rRot) ||
        !serialization::tryReadPod(file, rPx) ||
        !serialization::tryReadPod(file, rPy) ||
        !serialization::tryReadPod(file, rNext)) {
      file.close();
      Storage.remove(stateFile);
      return false;
    }

    score = rScore;
    linesCleared = rLines;
    level = rLevel;
    currentPiece = rPiece;
    currentRotation = rRot;
    pieceX = rPx;
    pieceY = rPy;
    nextPiece = rNext;

    for (int y = 0; y < kGridHeight; ++y) {
      for (int x = 0; x < kGridWidth; ++x) {
        int32_t cell = 0;
        if (!serialization::tryReadPod(file, cell)) {
          file.close();
          Storage.remove(stateFile);
          return false;
        }
        grid[y][x] = cell;
      }
    }

    file.close();
    return true;
  }

  void deleteSaveState() {
    if (Storage.exists(stateFile)) {
      Storage.remove(stateFile);
    }
  }
};