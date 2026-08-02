#pragma once

#include "activities/Activity.h"
#include "components/UITheme.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

enum class DiceType { D2_COIN, D4, D6, D8, D10, D12, D20, COUNT };

struct Die {
  const char* name;
  int sides;
};

static const Die kDice[] = {
    {"Coin", 2}, {"d4", 4}, {"d6", 6}, {"d8", 8}, {"d10", 10}, {"d12", 12}, {"d20", 20},
};

inline DiceType nextDie(DiceType current) {
  return static_cast<DiceType>((static_cast<int>(current) + 1) % static_cast<int>(DiceType::COUNT));
}

class DiceSimActivity : public Activity {
public:
  DiceSimActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("DiceSim", renderer, mappedInput) {}

  void onEnter() override {
    Activity::onEnter();
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    requestUpdate();
  }

  void loop() override {
    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      finish();
      return;
    }

    if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
      selectedDie = nextDie(selectedDie);
      currentRoll = 0; // Clear display roll when changing dice
      requestUpdate();
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      resetRolls();
      requestUpdate();
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Right)) {
      rollDice();
      requestUpdate();
    }
  }

  void render(RenderLock&& lock) override {
    renderer.clearScreen();

    const int pageWidth = renderer.getScreenWidth();
    const int pageHeight = renderer.getScreenHeight();

    const auto& metrics = UITheme::getInstance().getMetrics();
    const auto font = SETTINGS.getReaderFontId();

    // 1. Standard Header
    GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, "Dice",
                   kDice[static_cast<int>(selectedDie)].name);

    // 2. Formatted Multi-Line History
    int currentY = metrics.topPadding + metrics.headerHeight + 16;
    const int lineHeight = 34;

    if (!rollHistory.empty()) {
      // Assemble tokens
      std::vector<std::string> tokens;
      for (const auto& entry : rollHistory) {
        char buf[32];
        if (std::strcmp(entry.dieName, "Coin") == 0) {
          snprintf(buf, sizeof(buf), "%s[%s]", entry.dieName, entry.value == 1 ? "H" : "T");
        } else {
          snprintf(buf, sizeof(buf), "%s[%d]", entry.dieName, entry.value);
        }
        tokens.push_back(buf);
      }
      char totalBuf[20];
      snprintf(totalBuf, sizeof(totalBuf), "= %d", runningTotal);
      tokens.push_back(totalBuf);

      // Wrap lines (4 per line)
      const size_t maxTokensPerLine = 4;
      std::string lineStr = "";
      size_t countInLine = 0;

      for (size_t i = 0; i < tokens.size(); ++i) {
        if (countInLine > 0) {
          lineStr += " ";
        }
        lineStr += tokens[i];
        countInLine++;

        if (countInLine >= maxTokensPerLine || i == tokens.size() - 1) {
          renderer.drawCenteredText(font, currentY, lineStr.c_str());
          currentY += lineHeight;
          lineStr = "";
          countInLine = 0;
        }
      }
    } else {
      renderer.drawCenteredText(font, currentY, "Press Roll to start");
    }

    // 3. Visual Die / Coin Shape (Rendered with 3px thick stroke)
    const int centerX = pageWidth / 2;
    const int size = 56;
    const int bottomMargin = 110;
    const int centerY = pageHeight - bottomMargin - size;
    const int lineThickness = 3;

    switch (selectedDie) {
      case DiceType::D2_COIN: {
        drawThickCircle(renderer, centerX, centerY, size, 5);
        break;
      }
      case DiceType::D4: {
        drawThickLine(renderer, centerX, centerY - size, centerX - size, centerY + size, lineThickness);
        drawThickLine(renderer, centerX - size, centerY + size, centerX + size, centerY + size, lineThickness);
        drawThickLine(renderer, centerX + size, centerY + size, centerX, centerY - size, lineThickness);
        break;
      }
      case DiceType::D6: {
        const int left = centerX - size;
        const int right = centerX + size;
        const int top = centerY - size;
        const int bottom = centerY + size;

        drawThickLine(renderer, left, top, right, top, lineThickness);
        drawThickLine(renderer, right, top, right, bottom, lineThickness);
        drawThickLine(renderer, right, bottom, left, bottom, lineThickness);
        drawThickLine(renderer, left, bottom, left, top, lineThickness);
        break;
      }
      case DiceType::D8: {
        drawThickLine(renderer, centerX, centerY - size, centerX + size, centerY, lineThickness);
        drawThickLine(renderer, centerX + size, centerY, centerX, centerY + size, lineThickness);
        drawThickLine(renderer, centerX, centerY + size, centerX - size, centerY, lineThickness);
        drawThickLine(renderer, centerX - size, centerY, centerX, centerY - size, lineThickness);
        break;
      }
      case DiceType::D10: {
        const int waistY = centerY - (size / 5);
        const int waistX = size * 9 / 10;
        const int bottomY = centerY + size + 5;
        
        drawThickLine(renderer, centerX, centerY - size, centerX + waistX, waistY, lineThickness);
        drawThickLine(renderer, centerX + waistX, waistY, centerX, bottomY, lineThickness);
        drawThickLine(renderer, centerX, bottomY, centerX - waistX, waistY, lineThickness);
        drawThickLine(renderer, centerX - waistX, waistY, centerX, centerY - size, lineThickness);
        break;
      }
      case DiceType::D12: {
        const int p1x = centerX,                p1y = centerY - size;
        const int p2x = centerX + (size * 95 / 100), p2y = centerY - (size * 31 / 100);
        const int p3x = centerX + (size * 59 / 100), p3y = centerY + (size * 81 / 100);
        const int p4x = centerX - (size * 59 / 100), p4y = centerY + (size * 81 / 100);
        const int p5x = centerX - (size * 95 / 100), p5y = centerY - (size * 31 / 100);

        drawThickLine(renderer, p1x, p1y, p2x, p2y, lineThickness);
        drawThickLine(renderer, p2x, p2y, p3x, p3y, lineThickness);
        drawThickLine(renderer, p3x, p3y, p4x, p4y, lineThickness);
        drawThickLine(renderer, p4x, p4y, p5x, p5y, lineThickness);
        drawThickLine(renderer, p5x, p5y, p1x, p1y, lineThickness);
        break;
      }
      case DiceType::D20: {
        const int hx1 = centerX,                hy1 = centerY - size;
        const int hx2 = centerX + (size * 87 / 100), hy2 = centerY - (size / 2);
        const int hx3 = centerX + (size * 87 / 100), hy3 = centerY + (size / 2);
        const int hx4 = centerX,                hy4 = centerY + size;
        const int hx5 = centerX - (size * 87 / 100), hy5 = centerY + (size / 2);
        const int hx6 = centerX - (size * 87 / 100), hy6 = centerY - (size / 2);

        drawThickLine(renderer, hx1, hy1, hx2, hy2, lineThickness);
        drawThickLine(renderer, hx2, hy2, hx3, hy3, lineThickness);
        drawThickLine(renderer, hx3, hy3, hx4, hy4, lineThickness);
        drawThickLine(renderer, hx4, hy4, hx5, hy5, lineThickness);
        drawThickLine(renderer, hx5, hy5, hx6, hy6, lineThickness);
        drawThickLine(renderer, hx6, hy6, hx1, hy1, lineThickness);

        drawThickLine(renderer, hx1, hy1, hx3, hy3, lineThickness);
        drawThickLine(renderer, hx3, hy3, hx5, hy5, lineThickness);
        drawThickLine(renderer, hx5, hy5, hx1, hy1, lineThickness);
        break;
      }
      default:
        break;
    }

    // 4. Current Roll centered inside die frame
    if (currentRoll > 0) {
      const int fontHeightOffset = 14; 
      const int textY = centerY - fontHeightOffset;

      char diceBuf[16];
      if (selectedDie == DiceType::D2_COIN) {
        snprintf(diceBuf, sizeof(diceBuf), "%s", currentRoll == 1 ? "Heads" : "Tails");
      } else {
        snprintf(diceBuf, sizeof(diceBuf), "%d", currentRoll);
      }
      renderer.drawCenteredText(font, textY, diceBuf);
    }

    // 5. Selected Die Text Label below shape
    const int labelY = centerY + size + 20;
    char selectedBuf[32];
    snprintf(selectedBuf, sizeof(selectedBuf), "%s", kDice[static_cast<int>(selectedDie)].name);
    renderer.drawCenteredText(font, labelY, selectedBuf);

    // 6. Footer Button Hints
    const auto labels = mappedInput.mapLabels(tr(STR_BACK), "Reset", "Dice", "Roll");
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

    renderer.displayBuffer();
  }

private:
  struct RollEntry {
    const char* dieName;
    int value;
  };

  DiceType selectedDie = DiceType::D6;
  int currentRoll = 0;
  int runningTotal = 0;
  std::vector<RollEntry> rollHistory;

  void rollDice() {
    const auto& die = kDice[static_cast<int>(selectedDie)];
    currentRoll = (rand() % die.sides) + 1;
    
    rollHistory.push_back({die.name, currentRoll});
    runningTotal += currentRoll;
  }

  void resetRolls() {
    currentRoll = 0;
    runningTotal = 0;
    rollHistory.clear();
  }

  void drawThickLine(GfxRenderer& renderer, int x1, int y1, int x2, int y2, int thickness = 2) {
    for (int dx = 0; dx < thickness; ++dx) {
      for (int dy = 0; dy < thickness; ++dy) {
        renderer.drawLine(x1 + dx, y1 + dy, x2 + dx, y2 + dy);
      }
    }
  }

  void drawThickCircle(GfxRenderer& renderer, int cx, int cy, int radius, int thickness) {
    const int innerR = radius - (thickness / 2);
    const int outerR = radius + (thickness / 2);

    const int innerR2 = innerR * innerR;
    const int outerR2 = outerR * outerR;

    // Scan vertical bounding box and fill horizontal pixel spans
    for (int y = -outerR; y <= outerR; ++y) {
      const int y2 = y * y;
      for (int x = -outerR; x <= outerR; ++x) {
        const int dist2 = x * x + y2;
        // Fill every pixel that falls solidly between inner and outer radius squared
        if (dist2 >= innerR2 && dist2 <= outerR2) {
          renderer.drawPixel(cx + x, cy + y);
        }
      }
    }
  }
};