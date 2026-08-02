#pragma once

#include "activities/Activity.h"
#include "components/UITheme.h"
#include "components/icons/rosary.h"
#include <cmath>
#include <cstdio>
#include <vector>

class RosaryActivity : public Activity {
public:
  RosaryActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Rosary", renderer, mappedInput) {}

  void onEnter() override {
    Activity::onEnter();
    currentBeadIndex = 0;
    requestUpdate();
  }

  void loop() override {
    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      finish();
      return;
    }

    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      currentBeadIndex = 0;
      requestUpdate();
      return;
    }

    if (mappedInput.wasReleased(MappedInputManager::Button::Right) ||
        mappedInput.wasReleased(MappedInputManager::Button::Down)) {
      if (currentBeadIndex + 1 < totalSteps) {
        currentBeadIndex++;
        requestUpdate();
      }
    } 
    
    else if (mappedInput.wasReleased(MappedInputManager::Button::Left) ||
             mappedInput.wasReleased(MappedInputManager::Button::Up)) {
      if (currentBeadIndex > 0) {
        currentBeadIndex--;
        requestUpdate();
      }
    }
  }

  void render(RenderLock&& lock) override {
    renderer.clearScreen();

    const int pageWidth = renderer.getScreenWidth();
    const int pageHeight = renderer.getScreenHeight();

    const auto& metrics = UITheme::getInstance().getMetrics();

    // 1. Header
    GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, "Rosary");

    // 2. Build/Recalculate layout dynamically based on actual screen dimensions
    buildRosaryBeads(pageWidth, pageHeight, metrics.topPadding + metrics.headerHeight, metrics.buttonHintsHeight);

    // 3. Connecting Lines
    const size_t loopStart = 4;
    for (size_t i = loopStart; i < beads.size(); ++i) {
      size_t nextIdx = (i + 1 == beads.size()) ? loopStart : i + 1;
      renderer.drawLine(beads[i].x, beads[i].y, beads[nextIdx].x, beads[nextIdx].y);
    }

    // Drop strand line
    renderer.drawLine(beads[4].x, beads[4].y, beads[0].x, beads[0].y);

    // Cross String
    const int crossTopY = beads[0].y + 10;
    renderer.drawLine(beads[0].x, beads[0].y + beads[0].radius, beads[0].x, crossTopY + 5);

    // 4. Render Crucifix Icon at the base
    drawCrucifix(renderer, beads[0].x, crossTopY + 30);

    // 5. Determine Active Bead Index
    const bool isCompleted = (currentBeadIndex == static_cast<int>(beads.size()));
    const size_t activeBeadIdx = isCompleted ? 4 : static_cast<size_t>(currentBeadIndex);

    // 6. Render All Beads
    for (size_t i = 0; i < beads.size(); ++i) {
      const bool isActive = (i == activeBeadIdx);
      const bool showRing = (i == 4 && isCompleted);
      drawBead(renderer, beads[i].x, beads[i].y, beads[i].radius, isActive, showRing);
    }

    // 7. Footer Hints
    const auto labels = mappedInput.mapLabels(tr(STR_BACK), "Reset", "Prev", "Next");
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

    renderer.displayBuffer();
  }

private:
  int largeBeadRadius = 10;
  int smallBeadRadius = 7;

  struct Bead {
    int x;
    int y;
    int radius;
    bool isOurFather;
  };

  int currentBeadIndex = 0;
  int totalSteps = 0;
  std::vector<Bead> beads;

  void buildRosaryBeads(int pageWidth, int pageHeight, int topOffset, int footerHeight) {
    beads.clear();

    const int centerX = pageWidth / 2;
    const int availableHeight = pageHeight - topOffset - footerHeight;
    
    const int loopCenterY = topOffset + static_cast<int>(availableHeight * 0.32f);

    const float radiusX = (pageWidth / 2.0f) - 62.0f;
    const float radiusY = availableHeight * 0.28f;

    const int centerpieceY = loopCenterY + static_cast<int>(radiusY);

    const int dropGap = 22;
    const int dropSpacing = 18;

    const int startY = centerpieceY + dropGap + (dropSpacing * 3);

    beads.push_back({centerX, startY, largeBeadRadius, true});                       // 0: Bottom Our Father
    beads.push_back({centerX, startY - dropSpacing, smallBeadRadius, false});        // 1: Hail Mary 1
    beads.push_back({centerX, startY - (dropSpacing * 2), smallBeadRadius, false});  // 2: Hail Mary 2
    beads.push_back({centerX, startY - (dropSpacing * 3), smallBeadRadius, false});  // 3: Hail Mary 3

    const int totalLoopBeads = 54; 
    for (int i = 0; i < totalLoopBeads; ++i) {
      float angle = (M_PI / 2.0f) - (2.0f * M_PI * i / totalLoopBeads);

      int bx = centerX + static_cast<int>(radiusX * std::cos(angle));
      int by = loopCenterY + static_cast<int>(radiusY * std::sin(angle));

      bool isSpacer = (i == 0 || i == 11 || i == 22 || i == 33 || i == 44);
      int beadRadius = isSpacer ? largeBeadRadius : smallBeadRadius;

      beads.push_back({bx, by, beadRadius, isSpacer});
    }

    totalSteps = static_cast<int>(beads.size()) + 1;
  }

  void drawBead(GfxRenderer& renderer, int cx, int cy, int radius, bool isActive, bool showRing) {
    if (isActive) {
      for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
          if (x * x + y * y <= radius * radius) {
            renderer.drawPixel(cx + x, cy + y);
          }
        }
      }

      if (showRing) {
        const int ringR = radius + 4;
        const int ringR2 = ringR * ringR;
        const int ringInner2 = (ringR - 2) * (ringR - 2);

        for (int y = -ringR; y <= ringR; ++y) {
          for (int x = -ringR; x <= ringR; ++x) {
            int distSq = x * x + y * y;
            if (distSq <= ringR2 && distSq >= ringInner2) {
              renderer.drawPixel(cx + x, cy + y);
            }
          }
        }
      }
    } else {
      const int innerRadius = radius - 2;
      for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
          int distSq = x * x + y * y;
          if (distSq <= radius * radius && distSq >= innerRadius * innerRadius) {
            renderer.drawPixel(cx + x, cy + y);
          }
        }
      }
    }
  }

  void drawCrucifix(GfxRenderer& renderer, int cx, int cy) {
    const int origWidth = 32;
    const int origHeight = 32;
    
    const int scale = 2; 
    
    const int rotationAngle = 90; 

    const int scaledWidth = origWidth * scale;
    const int scaledHeight = origHeight * scale;

    const int topLeftX = cx - (scaledWidth / 2);
    const int topLeftY = cy - (scaledHeight / 2);

    const int bytesPerRow = origWidth / 8; // 4 bytes per row

    for (int y = 0; y < origHeight; ++y) {
      for (int x = 0; x < origWidth; ++x) {
        int byteIndex = (y * bytesPerRow) + (x / 8);
        int bitIndex = 7 - (x % 8);

        bool isBlack = ((RosaryIcon[byteIndex] >> bitIndex) & 1) == 0;
        if (!isBlack) continue;

        // Map (x, y) coordinates based on rotation angle
        int rx = x;
        int ry = y;

        if (rotationAngle == 90) {
          rx = origHeight - 1 - y;
          ry = x;
        } else if (rotationAngle == 180) {
          rx = origWidth - 1 - x;
          ry = origHeight - 1 - y;
        } else if (rotationAngle == 270) {
          rx = y;
          ry = origWidth - 1 - x;
        }

        // Draw scaled pixel block
        int drawX = topLeftX + (rx * scale);
        int drawY = topLeftY + (ry * scale);

        for (int sx = 0; sx < scale; ++sx) {
          for (int sy = 0; sy < scale; ++sy) {
            renderer.drawPixel(drawX + sx, drawY + sy);
          }
        }
      }
    }
  }
};