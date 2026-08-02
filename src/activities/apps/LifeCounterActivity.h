#pragma once

#include "activities/Activity.h"
#include "components/UITheme.h"
#include <Serialization.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

class LifeCounterActivity : public Activity {
public:
  LifeCounterActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("LifeCounter", renderer, mappedInput) {}

  ~LifeCounterActivity() override {
    saveGameState();
  }

  void onEnter() override {
    Activity::onEnter();
    
    // Auto-load saved state; default to 20 if file missing or invalid
    if (!loadGameState()) {
      lifeTotal = 20;
    }
    requestUpdate();
  }

  void loop() override {
    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      saveGameState();
      finish();
      return;
    }

    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      lifeTotal = 20;
      requestUpdate();
      return;
    }

    // Single clicks for incrementing/decrementing
    if (mappedInput.wasReleased(MappedInputManager::Button::Up)) {
      lifeTotal -= 5;
      requestUpdate();
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Down)) {
      lifeTotal += 5;
      requestUpdate();
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
      lifeTotal -= 1;
      requestUpdate();
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Right)) {
      lifeTotal += 1;
      requestUpdate();
    }

    // Rapid increment logic on button hold
    handleButtonHold(MappedInputManager::Button::Up, -5);
    handleButtonHold(MappedInputManager::Button::Down, 5);
    handleButtonHold(MappedInputManager::Button::Left, -1);
    handleButtonHold(MappedInputManager::Button::Right, 1);
  }

  void render(RenderLock&& lock) override {
    renderer.clearScreen();

    const int pageWidth = renderer.getScreenWidth();
    const int pageHeight = renderer.getScreenHeight();

    const auto& metrics = UITheme::getInstance().getMetrics();
    const auto font = SETTINGS.getReaderFontId();

    // 1. Standard Header
    GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, "Life Counter");

    // 2. Extra Large Heart Setup
    const int centerX = pageWidth / 2;
    const int centerY = pageHeight / 2 - 10;
    const int heartRadius = 165; 

    // Draw side button hints (-5 on left edge, +5 on right edge)
    const int sideHintY = centerY - 14; 
    renderer.drawText(font, 10, sideHintY, "-5");
    renderer.drawText(font, pageWidth - 36, sideHintY, "+5");

    // 3. Render Gray Filled Heart & Black Outline using identical parametric points
    drawFilledAndOutlinedHeart(renderer, centerX, centerY, heartRadius, 5);

    // 4. Life Total Display centered inside Heart
    char lifeStr[16];
    snprintf(lifeStr, sizeof(lifeStr), "%d", lifeTotal);
    
    renderer.drawCenteredText(font, centerY - 14, lifeStr);

    // 5. Footer Hints
    const auto labels = mappedInput.mapLabels(tr(STR_BACK), "Reset", "-1", "+1");
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

    renderer.displayBuffer();
  }

private:
  int lifeTotal = 20;

  static constexpr uint32_t GAME_MAGIC = 0x4C494645; // 'LIFE'
  static constexpr char stateFile[] = "/.smudge/lifecounter/state.bin";

  void ensureDirectoriesExist() {
    if (!Storage.exists("/.smudge")) {
      Storage.mkdir("/.smudge");
    }
    if (!Storage.exists("/.smudge/lifecounter")) {
      Storage.mkdir("/.smudge/lifecounter");
    }
  }

  void saveGameState() {
    ensureDirectoriesExist();

    HalFile file;
    if (!Storage.openFileForWrite("LifeCounter", stateFile, file)) {
      return;
    }

    serialization::writePod(file, GAME_MAGIC);
    serialization::writePod(file, static_cast<int32_t>(lifeTotal));

    file.close();
  }

  bool loadGameState() {
    if (!Storage.exists(stateFile)) {
      return false;
    }

    HalFile file;
    if (!Storage.openFileForRead("LifeCounter", stateFile, file)) {
      return false;
    }

    uint32_t magic = 0;
    if (!serialization::tryReadPod(file, magic) || magic != GAME_MAGIC) {
      file.close();
      Storage.remove(stateFile);
      return false;
    }

    int32_t savedLife = 20;
    if (!serialization::tryReadPod(file, savedLife)) {
      file.close();
      Storage.remove(stateFile);
      return false;
    }

    file.close();

    lifeTotal = static_cast<int>(savedLife);
    return true;
  }

  // Track press times per button
  uint32_t pressStartTimes[8] = {0};
  uint32_t lastRepeatTime = 0;

  const uint32_t holdDelayMs = 400;     // Delay before repeat starts
  const uint32_t repeatIntervalMs = 80; // Fast repeat speed

  void handleButtonHold(MappedInputManager::Button btn, int delta) {
    const size_t btnIdx = static_cast<size_t>(btn);
    if (btnIdx >= 8) return;

    const uint32_t now = millis();

    if (mappedInput.isPressed(btn)) {
      if (pressStartTimes[btnIdx] == 0) {
        pressStartTimes[btnIdx] = now;
      } else if (now - pressStartTimes[btnIdx] >= holdDelayMs) {
        if (now - lastRepeatTime >= repeatIntervalMs) {
          lifeTotal += delta;
          lastRepeatTime = now;
          requestUpdate();
        }
      }
    } else {
      pressStartTimes[btnIdx] = 0;
    }
  }

  uint32_t millis() {
    using namespace std::chrono;
    return static_cast<uint32_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
  }

  void drawThickLine(GfxRenderer& renderer, int x1, int y1, int x2, int y2, int thickness) {
    const int halfThick = thickness / 2;
    for (int dx = -halfThick; dx <= halfThick; ++dx) {
      for (int dy = -halfThick; dy <= halfThick; ++dy) {
        renderer.drawLine(x1 + dx, y1 + dy, x2 + dx, y2 + dy);
      }
    }
  }

  void drawFilledAndOutlinedHeart(GfxRenderer& renderer, int cx, int cy, int size, int thickness) {
    const int numSamples = 360;
    
    struct Point { int x, y; };
    std::vector<Point> points;
    points.reserve(numSamples + 1);

    int minY = 10000, maxY = -10000;

    for (int i = 0; i < numSamples; ++i) {
      const float t = (2.0f * M_PI * i) / numSamples;
      const float heartX = 16.0f * std::pow(std::sin(t), 3);
      const float heartY = -(13.0f * std::cos(t) - 5.0f * std::cos(2.0f * t) - 2.0f * std::cos(3.0f * t) - std::cos(4.0f * t));

      const int px = cx + static_cast<int>((heartX / 16.0f) * size);
      const int py = cy + static_cast<int>((heartY / 16.0f) * size);

      points.push_back({px, py});

      if (py < minY) minY = py;
      if (py > maxY) maxY = py;
    }

    for (int y = minY; y <= maxY; ++y) {
      std::vector<int> nodeX;

      size_t j = points.size() - 1;
      for (size_t i = 0; i < points.size(); ++i) {
        if ((points[i].y < y && points[j].y >= y) || (points[j].y < y && points[i].y >= y)) {
          int x = points[i].x + (y - points[i].y) * (points[j].x - points[i].x) / (points[j].y - points[i].y);
          nodeX.push_back(x);
        }
        j = i;
      }

      std::sort(nodeX.begin(), nodeX.end());

      for (size_t k = 0; k + 1 < nodeX.size(); k += 2) {
        for (int x = nodeX[k]; x <= nodeX[k + 1]; ++x) {
          if ((x + y) % 2 == 0) {
            renderer.drawPixel(x, y);
          }
        }
      }
    }

    for (size_t i = 0; i < points.size(); ++i) {
      size_t nextIdx = (i + 1) % points.size();
      drawThickLine(renderer, points[i].x, points[i].y, points[nextIdx].x, points[nextIdx].y, thickness);
    }
  }
};