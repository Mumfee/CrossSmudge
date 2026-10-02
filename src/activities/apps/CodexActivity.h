#pragma once

#include <HalStorage.h>
#include <Serialization.h>

#include <algorithm>
#include <cstdio>
#include <string>

#include "activities/Activity.h"
#include "codex/CodexCards.h"
#include "codex/CodexEvents.h"
#include "codex/CodexMonsters.h"
#include "codex/CodexSprites.h"
#include "codex/CodexState.h"
#include "codex/CodexTypes.h"
#include "components/UITheme.h"
#include "components/icons/codex.h"
#include "fontIds.h"

class CodexActivity : public Activity {
 public:
  enum class Screen : uint8_t { Title, Combat, Reward, Scriptorium, DeckView, GameOver, Victory, Shop };

  CodexActivity(GfxRenderer& renderer, MappedInputManager& mappedInput) : Activity("Codex", renderer, mappedInput) {}

  ~CodexActivity() override {
    if (state.inRun && stateDirty_) {
      saveGameState();
    } else if (!state.inRun) {
      deleteSaveState();
    }
  }

  void onEnter() override {
    Activity::onEnter();
    screen = Screen::Title;
    titleIndex = 0;
    stateDirty_ = false;
    loadGameState();
    requestUpdate();
  }

  void onExit() override {
    Activity::onExit();
    if (state.inRun && stateDirty_) {
      saveGameState();
    } else if (!state.inRun) {
      deleteSaveState();
    }
  }

  void loop() override {
    switch (screen) {
      case Screen::Title:
        loopTitle();
        break;
      case Screen::Combat:
        loopCombat();
        break;
      case Screen::Reward:
        loopReward();
        break;
      case Screen::Scriptorium:
        loopScriptorium();
        break;
      case Screen::Shop:
        loopShop();
        break;
      case Screen::DeckView:
        loopDeckView();
        break;
      case Screen::GameOver:
      case Screen::Victory:
        loopEndScreen();
        break;
    }
  }

  void render(RenderLock&& lock) override {
    renderer.clearScreen();
    switch (screen) {
      case Screen::Title:
        renderTitle();
        break;
      case Screen::Combat:
        renderCombat();
        break;
      case Screen::Reward:
        renderReward();
        break;
      case Screen::Scriptorium:
        renderScriptorium();
        break;
      case Screen::Shop:
        renderShop();
        break;
      case Screen::DeckView:
        renderDeckView();
        break;
      case Screen::GameOver:
        renderGameOver();
        break;
      case Screen::Victory:
        renderVictory();
        break;
    }
    renderer.displayBuffer();
  }

 private:
  Screen screen = Screen::Title;
  Screen savedScreen_ = Screen::Combat;
  Screen previousScreen = Screen::Title;
  enum class CardListTab : uint8_t { Deck, Discard, Relics, Menu };
  CardListTab cardListTab = CardListTab::Deck;
  int cardListPage = 0;
  int deckMenuIndex = 0;

  codex::RunState state;

  int titleIndex = 0;
  int rewardIndex = 0;
  uint8_t currentEventId = 0;
  int eventChoiceIndex = 0;
  bool eventResolved = false;
  uint16_t visitedEvents = 0;
  int deckScrollOffset = 0;
  int handY_ = 360;
  int cardH_ = 225;

  struct ShopState {
    static constexpr int CARD_COUNT = 3;
    static constexpr int RELIC_COUNT = 2;

    uint8_t cards[CARD_COUNT] = {0, 0, 0};
    int16_t cardPrices[CARD_COUNT] = {50, 50, 50};
    bool cardSold[CARD_COUNT] = {false, false, false};

    uint8_t relics[RELIC_COUNT] = {0, 0};
    int16_t relicPrices[RELIC_COUNT] = {110, 110};
    bool relicSold[RELIC_COUNT] = {false, false};

    int16_t healPrice = 35;
    int16_t healAmount = 12;
    bool healSold = false;

    int16_t purgePrice = 50;
    bool purgeSold = false;
  };

  ShopState shopState;
  int shopIndex = 0;
  bool shopPurgeMode = false;
  int shopPurgeIndex = 0;

  const char* stateFile = "/.smudge/codex/run.state";
  static constexpr uint32_t GAME_MAGIC = 0x434F4458;  // "CODX"
  static constexpr uint16_t SAVE_VERSION = 5;
  bool stateDirty_ = false;

  void ensureDirectoriesExist() {
    if (!Storage.exists("/.smudge")) {
      Storage.mkdir("/.smudge");
    }
    if (!Storage.exists("/.smudge/codex")) {
      Storage.mkdir("/.smudge/codex");
    }
  }

  void deleteSaveState() {
    if (Storage.exists(stateFile)) {
      Storage.remove(stateFile);
    }
  }

  void drawCornerFiligree(int cx, int cy, bool flipX, bool flipY) {
    int sx = flipX ? -1 : 1;
    int sy = flipY ? -1 : 1;
    // Central corner diamond
    renderer.fillRect(cx - (flipX ? 2 : 0), cy - (flipY ? 2 : 0), 3, 3, true);
    // Inner framing bracket
    renderer.drawLine(cx + (sx * 3), cy, cx + (sx * 7), cy, true);
    renderer.drawLine(cx, cy + (sy * 3), cx, cy + (sy * 7), true);
    // Corner leaf accent
    renderer.drawPixel(cx + (sx * 4), cy + (sy * 4), true);
  }

  void drawCodexPageFrame(int w, int h, int topY, int bottomY) {
    // 1. Medieval bound spine ribbing (left margin: x=7 to 11)
    renderer.drawLine(8, topY, 8, bottomY, true);
    renderer.drawLine(10, topY, 10, bottomY, true);
    for (int y = topY + 25; y < bottomY - 20; y += 42) {
      // Leather binding cord / stitch rib
      renderer.fillRect(2, y - 2, 6, 5, true);
      renderer.drawLine(8, y, 14, y, true);
    }

    // 2. Parchment deckle edge (right margin: x=w-11 to w-8)
    renderer.drawLine(w - 11, topY, w - 11, bottomY, true);
    renderer.drawLine(w - 9, topY, w - 9, bottomY, true);
    // Subtle deckle edge fiber dots
    for (int y = topY + 15; y < bottomY - 15; y += 8) {
      renderer.drawPixel(w - 5, y, true);
      if ((y / 8) % 2 == 0) {
        renderer.drawPixel(w - 6, y, true);
        renderer.drawPixel(w - 4, y + 1, true);
      }
    }

    // 3. Top and bottom horizontal page border rules
    renderer.drawLine(14, topY, w - 15, topY, true);
    renderer.drawLine(14, topY + 2, w - 15, topY + 2, true);

    renderer.drawLine(14, bottomY, w - 15, bottomY, true);
    renderer.drawLine(14, bottomY - 2, w - 15, bottomY - 2, true);

    // 4. Four corner flourishes
    drawCornerFiligree(11, topY, false, false);
    drawCornerFiligree(w - 12, topY, true, false);
    drawCornerFiligree(11, bottomY, false, true);
    drawCornerFiligree(w - 12, bottomY, true, true);
  }

  void drawManuscriptDivider(int y, int w, int margin = 24) {
    int midX = w / 2;
    // Center diamond boss
    renderer.fillRect(midX - 3, y - 3, 7, 7, true);
    renderer.drawPixel(midX, y, false);
    // Flanking diamond accents
    renderer.fillRect(midX - 16, y - 2, 5, 5, true);
    renderer.fillRect(midX + 12, y - 2, 5, 5, true);
    // Outer dots
    renderer.drawPixel(midX - 26, y, true);
    renderer.drawPixel(midX + 26, y, true);
    // Horizontal lines
    renderer.drawLine(margin, y, midX - 32, y, true);
    renderer.drawLine(midX + 32, y, w - margin, y, true);
  }

  void drawManuscriptMiniatureFrame(int x, int y, int w, int h) {
    // Outer border
    renderer.drawRect(x, y, w, h, true);
    // Inner border
    renderer.drawRect(x + 3, y + 3, w - 6, h - 6, true);
    // Corner rosettes
    renderer.fillRect(x + 1, y + 1, 2, 2, true);
    renderer.fillRect(x + w - 3, y + 1, 2, 2, true);
    renderer.fillRect(x + 1, y + h - 3, 2, 2, true);
    renderer.fillRect(x + w - 3, y + h - 3, 2, 2, true);
    // Plate indentation shadow (bottom & right)
    renderer.drawLine(x + 2, y + h + 1, x + w + 1, y + h + 1, true);
    renderer.drawLine(x + w + 1, y + 2, x + w + 1, y + h + 1, true);
  }

  void saveGameState() {
    if (!state.inRun) {
      deleteSaveState();
      return;
    }

    ensureDirectoriesExist();

    HalFile file;
    if (!Storage.openFileForWrite("Codex", stateFile, file)) {
      return;
    }

    stateDirty_ = false;
    serialization::writePod(file, GAME_MAGIC);
    serialization::writePod(file, SAVE_VERSION);

    Screen sToSave = screen;
    if (sToSave == Screen::DeckView) {
      sToSave = previousScreen;
    } else if (sToSave == Screen::Title) {
      sToSave = savedScreen_;
    }
    savedScreen_ = sToSave;
    serialization::writePod(file, static_cast<uint8_t>(sToSave));

    serialization::writePod(file, state.floor);
    serialization::writePod(file, state.maxFloor);
    serialization::writePod(file, state.gold);
    serialization::writePod(file, state.playerHp);
    serialization::writePod(file, state.playerMaxHp);
    serialization::writePod(file, state.playerWard);
    serialization::writePod(file, state.bonusWard);
    serialization::writePod(file, state.permStrength);
    serialization::writePod(file, state.baseMaxInk);
    serialization::writePod(file, state.ink);
    serialization::writePod(file, state.maxInk);
    serialization::writePod(file, static_cast<uint8_t>(state.lastPlayedMeter));
    serialization::writePod(file, state.playerStatus.weak);
    serialization::writePod(file, state.playerStatus.vulnerable);
    serialization::writePod(file, state.playerStatus.poison);
    serialization::writePod(file, state.playerStatus.strength);
    serialization::writePod(file, static_cast<uint8_t>(state.playerStatus.retainWard ? 1 : 0));
    serialization::writePod(file, state.playerStatus.extraDraw);
    serialization::writePod(file, static_cast<int8_t>(state.selectedHandIndex));

    // Deck
    serialization::writePod(file, static_cast<uint16_t>(state.deck.size()));
    for (uint8_t c : state.deck) serialization::writePod(file, c);

    // Relics
    serialization::writePod(file, static_cast<uint16_t>(state.relics.size()));
    for (uint8_t r : state.relics) serialization::writePod(file, r);

    // Draw pile
    serialization::writePod(file, static_cast<uint16_t>(state.drawPile.size()));
    for (uint8_t c : state.drawPile) serialization::writePod(file, c);

    // Hand
    serialization::writePod(file, static_cast<uint16_t>(state.hand.size()));
    for (uint8_t c : state.hand) serialization::writePod(file, c);

    // Discard pile
    serialization::writePod(file, static_cast<uint16_t>(state.discardPile.size()));
    for (uint8_t c : state.discardPile) serialization::writePod(file, c);

    // Card rewards
    serialization::writePod(file, static_cast<uint16_t>(state.cardRewards.size()));
    for (uint8_t c : state.cardRewards) serialization::writePod(file, c);

    // Enemy state
    serialization::writePod(file, state.enemy.hp);
    serialization::writePod(file, state.enemy.maxHp);
    serialization::writePod(file, state.enemy.ward);
    serialization::writePod(file, state.enemy.moveCount);
    serialization::writePod(file, state.enemy.spriteId);
    serialization::writePod(file, static_cast<uint8_t>(state.enemy.isElite ? 1 : 0));
    serialization::writePod(file, static_cast<uint8_t>(state.enemy.isBoss ? 1 : 0));
    serialization::writePod(file, static_cast<uint8_t>(state.enemy.intent));
    serialization::writePod(file, state.enemy.intentValue);
    serialization::writePod(file, state.enemy.intentExtra);
    serialization::writePod(file, state.enemy.status.weak);
    serialization::writePod(file, state.enemy.status.vulnerable);
    serialization::writePod(file, state.enemy.status.poison);
    serialization::writePod(file, state.enemy.status.strength);

    // UI & Event state
    serialization::writePod(file, static_cast<int8_t>(rewardIndex));
    serialization::writePod(file, currentEventId);
    serialization::writePod(file, static_cast<int8_t>(eventChoiceIndex));
    serialization::writePod(file, static_cast<uint8_t>(eventResolved ? 1 : 0));
    serialization::writePod(file, visitedEvents);

    // Shop state
    for (int i = 0; i < ShopState::CARD_COUNT; ++i) {
      serialization::writePod(file, shopState.cards[i]);
      serialization::writePod(file, shopState.cardPrices[i]);
      serialization::writePod(file, static_cast<uint8_t>(shopState.cardSold[i] ? 1 : 0));
    }
    for (int i = 0; i < ShopState::RELIC_COUNT; ++i) {
      serialization::writePod(file, shopState.relics[i]);
      serialization::writePod(file, shopState.relicPrices[i]);
      serialization::writePod(file, static_cast<uint8_t>(shopState.relicSold[i] ? 1 : 0));
    }
    serialization::writePod(file, shopState.healPrice);
    serialization::writePod(file, static_cast<uint8_t>(shopState.healSold ? 1 : 0));
    serialization::writePod(file, shopState.purgePrice);
    serialization::writePod(file, static_cast<uint8_t>(shopState.purgeSold ? 1 : 0));
    serialization::writePod(file, static_cast<int8_t>(shopIndex));

    file.close();
  }

  bool loadGameState() {
    if (!Storage.exists(stateFile)) {
      return false;
    }

    HalFile file;
    if (!Storage.openFileForRead("Codex", stateFile, file)) {
      return false;
    }

    uint32_t magic = 0;
    if (!serialization::tryReadPod(file, magic) || magic != GAME_MAGIC) {
      file.close();
      Storage.remove(stateFile);
      return false;
    }

    uint16_t version = 0;
    if (!serialization::tryReadPod(file, version) || version != SAVE_VERSION) {
      file.close();
      Storage.remove(stateFile);
      return false;
    }

    uint8_t savedScreen = 0;
    if (!serialization::tryReadPod(file, savedScreen) || !serialization::tryReadPod(file, state.floor) ||
        !serialization::tryReadPod(file, state.maxFloor) || !serialization::tryReadPod(file, state.gold) ||
        !serialization::tryReadPod(file, state.playerHp) || !serialization::tryReadPod(file, state.playerMaxHp) ||
        !serialization::tryReadPod(file, state.playerWard) || !serialization::tryReadPod(file, state.bonusWard) ||
        !serialization::tryReadPod(file, state.permStrength) || !serialization::tryReadPod(file, state.baseMaxInk) ||
        !serialization::tryReadPod(file, state.ink) || !serialization::tryReadPod(file, state.maxInk)) {
      file.close();
      Storage.remove(stateFile);
      return false;
    }

    uint8_t rawMeter = 0;
    if (!serialization::tryReadPod(file, rawMeter)) {
      file.close();
      Storage.remove(stateFile);
      return false;
    }
    state.lastPlayedMeter = static_cast<codex::Meter>(rawMeter);

    if (!serialization::tryReadPod(file, state.playerStatus.weak) ||
        !serialization::tryReadPod(file, state.playerStatus.vulnerable) ||
        !serialization::tryReadPod(file, state.playerStatus.poison) ||
        !serialization::tryReadPod(file, state.playerStatus.strength)) {
      file.close();
      Storage.remove(stateFile);
      return false;
    }

    uint8_t rawRetainWard = 0;
    int8_t rawHandIdx = 0;
    if (!serialization::tryReadPod(file, rawRetainWard) ||
        !serialization::tryReadPod(file, state.playerStatus.extraDraw) ||
        !serialization::tryReadPod(file, rawHandIdx)) {
      file.close();
      Storage.remove(stateFile);
      return false;
    }
    state.playerStatus.retainWard = (rawRetainWard != 0);
    state.selectedHandIndex = rawHandIdx;

    auto readVector = [&](std::vector<uint8_t>& vec) -> bool {
      uint16_t size = 0;
      if (!serialization::tryReadPod(file, size)) return false;
      vec.resize(size);
      for (uint16_t i = 0; i < size; ++i) {
        if (!serialization::tryReadPod(file, vec[i])) return false;
      }
      return true;
    };

    if (!readVector(state.deck) || !readVector(state.relics) || !readVector(state.drawPile) ||
        !readVector(state.hand) || !readVector(state.discardPile) || !readVector(state.cardRewards)) {
      file.close();
      Storage.remove(stateFile);
      return false;
    }

    uint8_t rawElite = 0, rawBoss = 0, rawIntent = 0;
    if (!serialization::tryReadPod(file, state.enemy.hp) || !serialization::tryReadPod(file, state.enemy.maxHp) ||
        !serialization::tryReadPod(file, state.enemy.ward) || !serialization::tryReadPod(file, state.enemy.moveCount) ||
        !serialization::tryReadPod(file, state.enemy.spriteId) || !serialization::tryReadPod(file, rawElite) ||
        !serialization::tryReadPod(file, rawBoss) || !serialization::tryReadPod(file, rawIntent) ||
        !serialization::tryReadPod(file, state.enemy.intentValue) ||
        !serialization::tryReadPod(file, state.enemy.intentExtra) ||
        !serialization::tryReadPod(file, state.enemy.status.weak) ||
        !serialization::tryReadPod(file, state.enemy.status.vulnerable) ||
        !serialization::tryReadPod(file, state.enemy.status.poison) ||
        !serialization::tryReadPod(file, state.enemy.status.strength)) {
      file.close();
      Storage.remove(stateFile);
      return false;
    }

    state.enemy.isElite = (rawElite != 0);
    state.enemy.isBoss = (rawBoss != 0);
    state.enemy.intent = static_cast<codex::IntentType>(rawIntent);

    if (state.enemy.spriteId < sizeof(codex::kMonsterTemplates) / sizeof(codex::kMonsterTemplates[0])) {
      state.enemy.name = codex::kMonsterTemplates[state.enemy.spriteId].name;
    } else {
      state.enemy.name = "Marginalia Fiend";
    }

    int8_t rawRewardIdx = 0, rawEventChoiceIdx = 0;
    uint8_t rawEventResolved = 0;
    serialization::tryReadPod(file, rawRewardIdx);
    serialization::tryReadPod(file, currentEventId);
    serialization::tryReadPod(file, rawEventChoiceIdx);
    serialization::tryReadPod(file, rawEventResolved);
    serialization::tryReadPod(file, visitedEvents);
    rewardIndex = rawRewardIdx;
    eventChoiceIndex = rawEventChoiceIdx;
    eventResolved = (rawEventResolved != 0);
    if (currentEventId >= codex::kEventCount) currentEventId = 0;

    // Shop state
    for (int i = 0; i < ShopState::CARD_COUNT; ++i) {
      uint8_t cSold = 0;
      serialization::tryReadPod(file, shopState.cards[i]);
      serialization::tryReadPod(file, shopState.cardPrices[i]);
      serialization::tryReadPod(file, cSold);
      shopState.cardSold[i] = (cSold != 0);
    }
    for (int i = 0; i < ShopState::RELIC_COUNT; ++i) {
      uint8_t rSold = 0;
      serialization::tryReadPod(file, shopState.relics[i]);
      serialization::tryReadPod(file, shopState.relicPrices[i]);
      serialization::tryReadPod(file, rSold);
      shopState.relicSold[i] = (rSold != 0);
    }
    uint8_t hSold = 0, pSold = 0;
    int8_t sIdx = 0;
    serialization::tryReadPod(file, shopState.healPrice);
    serialization::tryReadPod(file, hSold);
    serialization::tryReadPod(file, shopState.purgePrice);
    serialization::tryReadPod(file, pSold);
    serialization::tryReadPod(file, sIdx);
    shopState.healSold = (hSold != 0);
    shopState.purgeSold = (pSold != 0);
    shopIndex = sIdx;

    file.close();

    state.inRun = true;
    stateDirty_ = false;
    savedScreen_ = static_cast<Screen>(savedScreen);
    return true;
  }

  // --- Title Screen ---

  void loopTitle() {
    const int totalOptions = state.inRun ? 4 : 3;

    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      finish();
      return;
    }

    if (mappedInput.wasReleased(MappedInputManager::Button::Down) ||
        mappedInput.wasReleased(MappedInputManager::Button::Right) ||
        mappedInput.wasReleased(MappedInputManager::Button::PageForward)) {
      titleIndex = (titleIndex + 1) % totalOptions;
      requestUpdate();
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Up) ||
               mappedInput.wasReleased(MappedInputManager::Button::Left) ||
               mappedInput.wasReleased(MappedInputManager::Button::PageBack)) {
      titleIndex = (titleIndex - 1 + totalOptions) % totalOptions;
      requestUpdate();
    }

    int touchedItem = -1;
    if (mappedInput.wasItemTouchedDown(touchedItem)) {
      if (touchedItem >= 0 && touchedItem < totalOptions) {
        titleIndex = touchedItem;
        selectTitleOption(titleIndex);
        return;
      }
    }

    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      selectTitleOption(titleIndex);
    }
  }

  void selectTitleOption(int index) {
    if (state.inRun && index == 0) {
      // Continue Delve
      screen = (savedScreen_ == Screen::Reward || savedScreen_ == Screen::Scriptorium || savedScreen_ == Screen::Shop)
                   ? savedScreen_
                   : Screen::Combat;
      if (state.selectedHandIndex >= static_cast<int>(state.hand.size())) {
        state.selectedHandIndex = 0;
      }
      requestUpdate();
      return;
    }

    int opt = state.inRun ? index - 1 : index;
    if (opt == 0) {
      // New Delve (abandons existing if any)
      deleteSaveState();
      state.initNewRun();
      visitedEvents = 0;
      currentEventId = 0;
      eventChoiceIndex = 0;
      eventResolved = false;
      state.startCombat(false, false);
      stateDirty_ = true;
      screen = Screen::Combat;
      savedScreen_ = Screen::Combat;
      requestUpdate();
    } else if (opt == 1) {
      // View Deck
      if (!state.inRun) state.initNewRun();
      cardListTab = CardListTab::Deck;
      cardListPage = 0;
      previousScreen = Screen::Title;
      screen = Screen::DeckView;
      requestUpdate();
    } else if (opt == 2) {
      if (state.inRun) {
        // Abandon Delve
        state.inRun = false;
        stateDirty_ = false;
        deleteSaveState();
        requestUpdate();
      } else {
        finish();
      }
    } else if (opt == 3) {
      finish();
    }
  }

  void renderTitle() {
    const int w = renderer.getScreenWidth();
    const int h = renderer.getScreenHeight();
    const auto& m = UITheme::getInstance().getMetrics();

    GUI.drawHeader(renderer, Rect{0, m.topPadding, w, m.headerHeight}, "Codex", "Ink & Iron");

    const int topY = m.topPadding + m.headerHeight + 2;
    const int bottomY = h - m.buttonHintsHeight - 2;
    drawCodexPageFrame(w, h, topY, bottomY);

    int midX = w / 2;
    int y = topY + 12;

    // Title banner
    renderer.drawCenteredText(UI_12_FONT_ID, y, "CODEX: INK & IRON", true, EpdFontFamily::BOLD);
    y += renderer.getLineHeight(UI_12_FONT_ID) + 4;
    renderer.drawCenteredText(SMALL_FONT_ID, y, "A Roguelike Deckbuilder for E-Paper", true);
    y += renderer.getLineHeight(SMALL_FONT_ID) + 12;

    // Large woodcut miniature plate (96x96 inside decorative plate frame)
    const int plateW = 106;
    const int plateH = 106;
    const int plateX = (w - plateW) / 2;
    drawManuscriptMiniatureFrame(plateX, y, plateW, plateH);
    renderer.drawIcon(codex::SpriteCodexLarge, plateX + 5, y + 5, 96, 96);
    y += plateH + 12;

    // Manuscript flourish divider
    drawManuscriptDivider(y, w, 28);
    y += 12;

    // Menu buttons
    const int totalOptions = state.inRun ? 4 : 3;
    const int menuHeight = bottomY - y - 6;

    char continueLabel[48];
    if (state.inRun) {
      snprintf(continueLabel, sizeof(continueLabel), "Continue Delve (Page %d/15)", state.floor);
    }

    GUI.drawButtonMenu(
        renderer, Rect{0, y, w, menuHeight}, totalOptions, titleIndex,
        [this, &continueLabel](int i) {
          if (state.inRun) {
            if (i == 0) return (const char*)continueLabel;
            if (i == 1) return "New Delve";
            if (i == 2) return "Inspect Grimoire";
            return "Abandon Delve";
          }
          if (i == 0) return "New Delve";
          if (i == 1) return "Inspect Grimoire";
          return "Exit to Applications";
        },
        nullptr);

    const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  }

  // --- Combat Screen ---

  void loopCombat() {
    // Hold Back for 600ms to Save & Exit to Title
    if (mappedInput.isPressed(MappedInputManager::Button::Back) && mappedInput.getHeldTime() >= 600) {
      mappedInput.suppressNextBackRelease();
      savedScreen_ = Screen::Combat;
      saveGameState();
      stateDirty_ = false;
      screen = Screen::Title;
      requestUpdate();
      return;
    }

    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      // Short press Back: End turn
      state.endPlayerTurn();
      stateDirty_ = true;
      checkCombatResolution();
      requestUpdate();
      return;
    }

    // Top-left button on X3 (Up / PageBack): Open Deck
    if (mappedInput.wasReleased(MappedInputManager::Button::Up) ||
        mappedInput.wasReleased(MappedInputManager::Button::PageBack)) {
      cardListTab = CardListTab::Deck;
      cardListPage = 0;
      previousScreen = Screen::Combat;
      screen = Screen::DeckView;
      requestUpdate();
      return;
    }

    // Top-right button on X3 (Down / PageForward): Open Discard
    if (mappedInput.wasReleased(MappedInputManager::Button::Down) ||
        mappedInput.wasReleased(MappedInputManager::Button::PageForward)) {
      cardListTab = CardListTab::Discard;
      cardListPage = 0;
      previousScreen = Screen::Combat;
      screen = Screen::DeckView;
      requestUpdate();
      return;
    }

    int handSize = static_cast<int>(state.hand.size());

    // Cycle cards in hand with Left / Right
    if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
      if (handSize > 0) {
        state.selectedHandIndex = (state.selectedHandIndex - 1 + handSize) % handSize;
        requestUpdate();
      }
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Right)) {
      if (handSize > 0) {
        state.selectedHandIndex = (state.selectedHandIndex + 1) % handSize;
        requestUpdate();
      }
    }

    // Play card
    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      if (handSize > 0) {
        if (state.playCard(state.selectedHandIndex)) {
          stateDirty_ = true;
          checkCombatResolution();
          requestUpdate();
        } else {
          requestUpdate();
        }
      }
    }

    // Touch card selection
    int tx = 0, ty = 0;
    if (mappedInput.wasScreenTouchDown(tx, ty)) {
      handleCombatTouch(tx, ty);
    }
  }

  void handleCombatTouch(int tx, int ty) {
    const int w = renderer.getScreenWidth();
    const int h = renderer.getScreenHeight();
    const auto& m = UITheme::getInstance().getMetrics();

    // Check header / top corner tap to view Deck / Discard / Relics / Menu
    if (ty < m.topPadding + m.headerHeight + 20) {
      if (tx < w / 4) {
        cardListTab = CardListTab::Deck;
      } else if (tx < w / 2) {
        cardListTab = CardListTab::Discard;
      } else if (tx < (3 * w) / 4) {
        cardListTab = CardListTab::Relics;
      } else {
        cardListTab = CardListTab::Menu;
      }
      cardListPage = 0;
      previousScreen = Screen::Combat;
      screen = Screen::DeckView;
      requestUpdate();
      return;
    }

    // Check if bottom button hints touched
    if (ty >= h - m.buttonHintsHeight - 6) {
      if (tx < w / 4) {
        // End Turn
        state.endPlayerTurn();
        stateDirty_ = true;
        checkCombatResolution();
        requestUpdate();
        return;
      } else if (tx < w / 2) {
        // Play
        if (!state.hand.empty() && state.selectedHandIndex >= 0 &&
            state.selectedHandIndex < static_cast<int>(state.hand.size())) {
          if (state.playCard(state.selectedHandIndex)) {
            stateDirty_ = true;
            checkCombatResolution();
            requestUpdate();
            return;
          }
        }
      } else if (tx < (3 * w) / 4) {
        // < Card
        if (!state.hand.empty()) {
          int count = static_cast<int>(state.hand.size());
          state.selectedHandIndex = (state.selectedHandIndex - 1 + count) % count;
          requestUpdate();
          return;
        }
      } else {
        // Card >
        if (!state.hand.empty()) {
          int count = static_cast<int>(state.hand.size());
          state.selectedHandIndex = (state.selectedHandIndex + 1) % count;
          requestUpdate();
          return;
        }
      }
    }

    // Check if card touched in hand
    if (ty >= handY_ && ty < handY_ + cardH_ && !state.hand.empty()) {
      int count = static_cast<int>(state.hand.size());
      int cardAreaW = w - 36;
      int cardW = cardAreaW / std::max(1, count);
      cardW = std::min(cardW, 110);
      int startX = (w - (cardW * count)) / 2;
      if (tx >= startX && tx < startX + (cardW * count)) {
        int clickedIdx = (tx - startX) / cardW;
        if (clickedIdx >= 0 && clickedIdx < count) {
          if (clickedIdx == state.selectedHandIndex) {
            // Double-tap plays card
            state.playCard(clickedIdx);
            stateDirty_ = true;
            checkCombatResolution();
          } else {
            state.selectedHandIndex = clickedIdx;
          }
          requestUpdate();
        }
      }
    }
  }

  void checkCombatResolution() {
    if (state.enemy.hp <= 0) {
      // Victory!
      if (state.hasRelic(2)) state.playerHp = std::min(state.playerMaxHp, static_cast<int16_t>(state.playerHp + 2));
      state.gold += 15 + (state.floor * 3);
      if (state.hasRelic(6)) state.gold += 15;  // Golden Bookmark: +15 Gold after combat
      state.lastPlayedMeter = codex::Meter::None;

      if (state.enemy.isBoss) {
        state.inRun = false;
        deleteSaveState();
        screen = Screen::Victory;
      } else {
        state.generateCardRewards();
        rewardIndex = 0;
        screen = Screen::Reward;
      }
    } else if (state.playerHp <= 0) {
      state.inRun = false;
      deleteSaveState();
      screen = Screen::GameOver;
    }
  }

  void renderCombat() {
    const int w = renderer.getScreenWidth();
    const int h = renderer.getScreenHeight();
    const auto& m = UITheme::getInstance().getMetrics();

    // 1. Top Ribbon: Chapter 1 Page
    char subHeader[32];
    snprintf(subHeader, sizeof(subHeader), "Ch.1 Pg %d/15", state.floor);
    GUI.drawHeader(renderer, Rect{0, m.topPadding, w, m.headerHeight}, "Codex Delve", subHeader);

    const int topY = m.topPadding + m.headerHeight + 2;
    const int bottomY = h - m.buttonHintsHeight - 2;
    drawCodexPageFrame(w, h, topY, bottomY);

    int y = topY + 6;

    // Player Status Bar
    char pStats[80];
    snprintf(pStats, sizeof(pStats), "HP: %d/%d   Ward: %d   Ink: %d/%d", std::max(0, static_cast<int>(state.playerHp)),
             state.playerMaxHp, state.playerWard, state.ink, state.maxInk);
    renderer.drawCenteredText(UI_12_FONT_ID, y, pStats, true, EpdFontFamily::BOLD);
    y += renderer.getLineHeight(UI_12_FONT_ID) + 4;

    // Player Status Effects (Str, Weak, Vuln, Poison)
    char pStatusBuf[96];
    int pLen = 0;
    if (state.playerStatus.strength != 0) {
      pLen += snprintf(pStatusBuf + pLen, sizeof(pStatusBuf) - pLen, "[Str %+d] ", state.playerStatus.strength);
    }
    if (state.playerStatus.weak > 0) {
      pLen += snprintf(pStatusBuf + pLen, sizeof(pStatusBuf) - pLen, "[Weak %d] ", state.playerStatus.weak);
    }
    if (state.playerStatus.vulnerable > 0) {
      pLen += snprintf(pStatusBuf + pLen, sizeof(pStatusBuf) - pLen, "[Vuln %d] ", state.playerStatus.vulnerable);
    }
    if (state.playerStatus.poison > 0) {
      pLen += snprintf(pStatusBuf + pLen, sizeof(pStatusBuf) - pLen, "[Poison %d] ", state.playerStatus.poison);
    }
    if (pLen > 0) {
      renderer.drawCenteredText(SMALL_FONT_ID, y, pStatusBuf, true, EpdFontFamily::BOLD);
      y += renderer.getLineHeight(SMALL_FONT_ID) + 4;
    }

    // Manuscript flourish divider
    y += 2;
    drawManuscriptDivider(y, w, 24);
    y += 8;

    // 2. Encounter Area (Left: Stats, Intent, Rhythm; Right: Large Woodcut Engraving)
    const int spriteSize = codex::kMonsterSpriteSize;
    const int spritePlateSize = spriteSize + 8;
    const int plateX = w - 22 - spritePlateSize;
    const int plateY = y + 2;

    // Right Column: Monster Sprite Miniature Frame & Dedicated Woodcut Sprite
    uint8_t sId = (state.enemy.spriteId < 7) ? state.enemy.spriteId : 0;
    const uint8_t* monSprite = codex::kMonsterSprites[sId];
    drawManuscriptMiniatureFrame(plateX, plateY, spritePlateSize, spritePlateSize);
    renderer.drawIcon(monSprite, plateX + 4, plateY + 4, spriteSize, spriteSize);

    // Left Column: Enemy Name, HP Bar, Stats, Intent, Rhythm
    const int leftX = 22;
    const int leftW = plateX - leftX - 10;
    int curY = y + 2;

    // Monster Name & Badge (wrapped to fit leftW without clipping the plate)
    char eTitle[64];
    snprintf(eTitle, sizeof(eTitle), "%s %s", state.enemy.name,
             state.enemy.isElite ? "[ELITE]" : (state.enemy.isBoss ? "[BOSS]" : ""));
    auto titleLines = renderer.wrappedText(UI_12_FONT_ID, eTitle, leftW, 2, EpdFontFamily::BOLD);
    for (const auto& tl : titleLines) {
      renderer.drawText(UI_12_FONT_ID, leftX, curY, tl.c_str(), true, EpdFontFamily::BOLD);
      curY += renderer.getLineHeight(UI_12_FONT_ID) - 1;
    }
    curY += 3;

    // Monster HP & Ward Bar
    int barW = leftW;
    int barH = 12;
    renderer.drawRect(leftX, curY, barW, barH, true);
    if (state.enemy.maxHp > 0) {
      int fillW = std::clamp((state.enemy.hp * (barW - 4)) / state.enemy.maxHp, 0, barW - 4);
      renderer.fillRect(leftX + 2, curY + 2, fillW, barH - 4, true);
    }
    curY += barH + 3;

    // HP & Ward Text
    char eHpStr[48];
    snprintf(eHpStr, sizeof(eHpStr), "%d / %d HP  (Ward: %d)", std::max(0, static_cast<int>(state.enemy.hp)),
             state.enemy.maxHp, state.enemy.ward);
    renderer.drawText(SMALL_FONT_ID, leftX, curY, eHpStr, true);
    curY += renderer.getLineHeight(SMALL_FONT_ID) + 3;

    // Enemy Status Effects (Str, Weak, Vuln, Poison)
    char eStatusBuf[96];
    int eLen = 0;
    if (state.enemy.status.strength != 0) {
      eLen += snprintf(eStatusBuf + eLen, sizeof(eStatusBuf) - eLen, "[Str %+d] ", state.enemy.status.strength);
    }
    if (state.enemy.status.weak > 0) {
      eLen += snprintf(eStatusBuf + eLen, sizeof(eStatusBuf) - eLen, "[Weak %d] ", state.enemy.status.weak);
    }
    if (state.enemy.status.vulnerable > 0) {
      eLen += snprintf(eStatusBuf + eLen, sizeof(eStatusBuf) - eLen, "[Vuln %d] ", state.enemy.status.vulnerable);
    }
    if (state.enemy.status.poison > 0) {
      eLen += snprintf(eStatusBuf + eLen, sizeof(eStatusBuf) - eLen, "[Poison %d] ", state.enemy.status.poison);
    }
    if (eLen > 0) {
      renderer.drawText(SMALL_FONT_ID, leftX, curY, eStatusBuf, true, EpdFontFamily::BOLD);
      curY += renderer.getLineHeight(SMALL_FONT_ID) + 3;
    }

    // Intent Indicator
    char intentStr[64];
    switch (state.enemy.intent) {
      case codex::IntentType::Attack:
        snprintf(intentStr, sizeof(intentStr), "Intent: Attacks (%d dmg)", state.enemy.intentValue);
        break;
      case codex::IntentType::Defend:
        snprintf(intentStr, sizeof(intentStr), "Intent: Fortifies (+%d Ward)", state.enemy.intentValue);
        break;
      case codex::IntentType::AttackAndDefend:
        snprintf(intentStr, sizeof(intentStr), "Intent: Strike %d & +%d Ward", state.enemy.intentValue,
                 state.enemy.intentExtra);
        break;
      case codex::IntentType::BuffStrength:
        snprintf(intentStr, sizeof(intentStr), "Intent: Enrages! (+%d Str)", state.enemy.intentValue);
        break;
      case codex::IntentType::DebuffPlayer:
        snprintf(intentStr, sizeof(intentStr), "Intent: Casts Weaken Hex");
        break;
    }
    renderer.drawText(UI_12_FONT_ID, leftX, curY, intentStr, true, EpdFontFamily::BOLD);
    curY += renderer.getLineHeight(UI_12_FONT_ID) + 4;

    // Active Rhythm / Meter Strip
    char rhythmBuf[48];
    if (state.lastPlayedMeter == codex::Meter::Blade) {
      snprintf(rhythmBuf, sizeof(rhythmBuf), "Rhythm: Blade");
    } else if (state.lastPlayedMeter == codex::Meter::Ward) {
      snprintf(rhythmBuf, sizeof(rhythmBuf), "Rhythm: Ward");
    } else if (state.lastPlayedMeter == codex::Meter::Script) {
      snprintf(rhythmBuf, sizeof(rhythmBuf), "Rhythm: Script");
    } else {
      snprintf(rhythmBuf, sizeof(rhythmBuf), "Rhythm: None");
    }
    renderer.drawText(SMALL_FONT_ID, leftX, curY, rhythmBuf, true, EpdFontFamily::BOLD);
    curY += renderer.getLineHeight(SMALL_FONT_ID) + 4;

    // Advance y past the encounter block
    y = std::max(plateY + spritePlateSize, curY) + 6;
    drawManuscriptDivider(y, w, 24);
    y += 8;

    // Selected Card Inspector Box (fits neatly within manuscript margins)
    int handCount = static_cast<int>(state.hand.size());
    if (handCount > 0 && state.selectedHandIndex >= 0 && state.selectedHandIndex < handCount) {
      uint8_t selId = state.hand[state.selectedHandIndex];
      if (selId < codex::CARD_COUNT) {
        int inspX = 16;
        int inspW = w - 32;
        int inspH = 118;
        int inspY = y;
        renderCardInspector(inspX, inspY, inspW, inspH, codex::kCardCatalog[selId], true);
        y = inspY + inspH + 8;
      }
    }

    // 3. Card Hand at Bottom
    const int handY = y + 4;
    const int cardAreaW = w - 36;
    const int availableH = bottomY - handY - 4;
    const int cardH = std::min(availableH, 225);
    handY_ = handY;
    cardH_ = cardH;

    if (handCount > 0) {
      int cardW = cardAreaW / std::max(1, handCount);
      int maxCardW = 110;
      cardW = std::min(cardW, maxCardW);
      int startX = (w - (cardW * handCount)) / 2;

      for (int i = 0; i < handCount; ++i) {
        int cx = startX + (i * cardW);
        bool isSelected = (i == state.selectedHandIndex);
        int cy = isSelected ? handY - 8 : handY;

        uint8_t cId = state.hand[i];
        if (cId < codex::CARD_COUNT) {
          renderCard(cx, cy, cardW - 4, cardH, codex::kCardCatalog[cId], isSelected);
        }
      }
    } else {
      renderer.drawCenteredText(UI_12_FONT_ID, handY + 60, "(Hand empty - press End Turn)", true);
    }

    // Bottom Navigation Button Hints
    const auto labels = mappedInput.mapLabels("End Turn", "Play", "< Card", "Card >");
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  }

  void drawCardHatchPattern(int x, int y, int w, int h, int spacing = 4) {
    for (int py = y; py < y + h; ++py) {
      for (int px = x; px < x + w; ++px) {
        if ((px + py) % spacing == 0) {
          renderer.drawPixel(px, py, true);
        }
      }
    }
  }

  void drawTextWithHalo(int fontId, int x, int y, const char* text, bool isBold = false) {
    auto style = isBold ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;
    // 8-point white knockout halo around text glyphs
    for (int dx = -1; dx <= 1; ++dx) {
      for (int dy = -1; dy <= 1; ++dy) {
        if (dx != 0 || dy != 0) {
          renderer.drawText(fontId, x + dx, y + dy, text, false, style);
        }
      }
    }
    renderer.drawText(fontId, x, y, text, true, style);
  }

  void renderCardInspector(int inspX, int inspY, int inspW, int inspH, const codex::CardDef& sc, bool checkCouplet) {
    drawManuscriptMiniatureFrame(inspX, inspY, inspW, inspH);

    // Divider line below header (with clearance below UI_12_FONT_ID)
    int divY = inspY + 8 + renderer.getLineHeight(UI_12_FONT_ID) + 4;

    // Header: [1 Ink] Name   •   [BLADE]
    const char* meterDesc =
        (sc.meter == codex::Meter::Blade) ? "[BLADE]" : ((sc.meter == codex::Meter::Ward) ? "[WARD]" : "[SCRIPT]");
    char headerBuf[64];
    snprintf(headerBuf, sizeof(headerBuf), "[%d Ink] %s", sc.cost, sc.name);
    int mWidth = renderer.getTextWidth(UI_12_FONT_ID, meterDesc, EpdFontFamily::BOLD);

    // If 1-use Power card, shade title header bar with /// hatching and haloed text!
    if (sc.type == codex::CardType::Power) {
      drawCardHatchPattern(inspX + 4, inspY + 4, inspW - 8, divY - (inspY + 4), 4);
      drawTextWithHalo(UI_12_FONT_ID, inspX + 10, inspY + 8, headerBuf, true);
      drawTextWithHalo(UI_12_FONT_ID, inspX + inspW - mWidth - 12, inspY + 8, meterDesc, true);
    } else {
      renderer.drawText(UI_12_FONT_ID, inspX + 10, inspY + 8, headerBuf, true, EpdFontFamily::BOLD);
      renderer.drawText(UI_12_FONT_ID, inspX + inspW - mWidth - 12, inspY + 8, meterDesc, true, EpdFontFamily::BOLD);
    }

    renderer.drawLine(inspX + 6, divY, inspX + inspW - 6, divY, true);

    // Description
    int dY = divY + 6;
    auto dLines = renderer.wrappedText(SMALL_FONT_ID, sc.desc, inspW - 20, 2);
    for (const auto& dl : dLines) {
      renderer.drawText(SMALL_FONT_ID, inspX + 10, dY, dl.c_str(), true);
      dY += renderer.getLineHeight(SMALL_FONT_ID) + 2;
    }

    // Couplet Banner at bottom of inspector box
    if (sc.coupletDesc != nullptr) {
      bool active = false;
      if (checkCouplet) {
        if (sc.coupletCond == codex::CoupletType::AfterBlade && state.lastPlayedMeter == codex::Meter::Blade)
          active = true;
        if (sc.coupletCond == codex::CoupletType::AfterWard && state.lastPlayedMeter == codex::Meter::Ward)
          active = true;
        if (sc.coupletCond == codex::CoupletType::AfterScript && state.lastPlayedMeter == codex::Meter::Script)
          active = true;
      }

      const char* condStr = (sc.coupletCond == codex::CoupletType::AfterBlade)
                                ? "Blade"
                                : ((sc.coupletCond == codex::CoupletType::AfterWard) ? "Ward" : "Script");

      int bannerH = 26;
      int bannerY = inspY + inspH - bannerH - 6;
      if (active) {
        renderer.fillRect(inspX + 6, bannerY, inspW - 12, bannerH, true);
        char cBuf[96];
        snprintf(cBuf, sizeof(cBuf), "COUPLET READY (%s): %s", condStr, sc.coupletDesc);
        int cW = renderer.getTextWidth(SMALL_FONT_ID, cBuf, EpdFontFamily::BOLD);
        int cTextY = bannerY + (bannerH - renderer.getLineHeight(SMALL_FONT_ID)) / 2;
        renderer.drawText(SMALL_FONT_ID, inspX + (inspW - cW) / 2, cTextY, cBuf, false, EpdFontFamily::BOLD);
      } else {
        renderer.drawRect(inspX + 6, bannerY, inspW - 12, bannerH, true);
        char cBuf[96];
        snprintf(cBuf, sizeof(cBuf), "Couplet (%s): %s", condStr, sc.coupletDesc);
        int cW = renderer.getTextWidth(SMALL_FONT_ID, cBuf);
        int cTextY = bannerY + (bannerH - renderer.getLineHeight(SMALL_FONT_ID)) / 2;
        renderer.drawText(SMALL_FONT_ID, inspX + (inspW - cW) / 2, cTextY, cBuf, true);
      }
    }
  }

  void renderCard(int x, int y, int cardW, int cardH, const codex::CardDef& c, bool isSelected) {
    // Outer Card Border
    renderer.drawRoundedRect(x, y, cardW, cardH, isSelected ? 2 : 1, 4, true);

    // Inner illuminated manuscript border rule (1px inset)
    renderer.drawRect(x + 2, y + 2, cardW - 4, cardH - 4, true);
    // Delicate corner accents
    renderer.drawPixel(x + 4, y + 4, true);
    renderer.drawPixel(x + cardW - 5, y + 4, true);
    renderer.drawPixel(x + 4, y + cardH - 5, true);
    renderer.drawPixel(x + cardW - 5, y + cardH - 5, true);

    // Cost badge top-left
    renderer.fillRoundedRect(x + 4, y + 4, 18, 18, 9, Color::Black);
    char costStr[4];
    snprintf(costStr, sizeof(costStr), "%d", c.cost);
    int costW = renderer.getTextWidth(SMALL_FONT_ID, costStr, EpdFontFamily::BOLD);
    int costTextY = y + 4 + (18 - renderer.getLineHeight(SMALL_FONT_ID)) / 2;
    renderer.drawText(SMALL_FONT_ID, x + 4 + (18 - costW) / 2, costTextY, costStr, false, EpdFontFamily::BOLD);

    // Meter Badge top-right
    const char* meterStr = "Script";
    if (c.meter == codex::Meter::Blade)
      meterStr = "Blade";
    else if (c.meter == codex::Meter::Ward)
      meterStr = "Ward";
    int mWidth = renderer.getTextWidth(SMALL_FONT_ID, meterStr, EpdFontFamily::BOLD);
    renderer.drawText(SMALL_FONT_ID, x + cardW - mWidth - 6, y + 6, meterStr, true, EpdFontFamily::BOLD);

    // Divider line below cost and meter
    int divY = y + 26;
    renderer.drawLine(x + 3, divY, x + cardW - 3, divY, true);

    // Card Name (Wrapped onto 2 lines with regular font to avoid truncation)
    auto nameLines = renderer.wrappedText(SMALL_FONT_ID, c.name, cardW - 6, 2, EpdFontFamily::REGULAR);
    int titleH = static_cast<int>(nameLines.size()) * (renderer.getLineHeight(SMALL_FONT_ID) - 1) + 6;

    // If 1-use Power card, shade title background with diagonal /// hatching!
    if (c.type == codex::CardType::Power) {
      drawCardHatchPattern(x + 3, divY + 1, cardW - 6, titleH, 4);
      renderer.drawLine(x + 3, divY + 1 + titleH, x + cardW - 3, divY + 1 + titleH, true);
    }

    int textY = divY + 4;
    for (const auto& nl : nameLines) {
      int nameW = renderer.getTextWidth(SMALL_FONT_ID, nl.c_str(), EpdFontFamily::BOLD);
      if (c.type == codex::CardType::Power) {
        drawTextWithHalo(SMALL_FONT_ID, x + (cardW - nameW) / 2, textY, nl.c_str(), true);
      } else {
        renderer.drawText(SMALL_FONT_ID, x + (cardW - nameW) / 2, textY, nl.c_str(), true, EpdFontFamily::BOLD);
      }
      textY += renderer.getLineHeight(SMALL_FONT_ID) - 1;
    }
    textY += 3;

    // Card Description (Wrapped cleanly)
    auto lines = renderer.wrappedText(SMALL_FONT_ID, c.desc, cardW - 6, 3);
    for (const auto& l : lines) {
      int lineW = renderer.getTextWidth(SMALL_FONT_ID, l.c_str());
      renderer.drawText(SMALL_FONT_ID, x + (cardW - lineW) / 2, textY, l.c_str(), true);
      textY += renderer.getLineHeight(SMALL_FONT_ID) + 2;
    }

    // Couplet bonus box at bottom of card
    if (c.coupletDesc != nullptr) {
      int coupletBoxH = 44;
      int coupletBoxY = y + cardH - coupletBoxH - 4;
      renderer.drawRect(x + 3, coupletBoxY, cardW - 6, coupletBoxH, true);

      // Check if couplet is actively ready (ONLY in combat)!
      bool active = false;
      if (screen == Screen::Combat) {
        if (c.coupletCond == codex::CoupletType::AfterBlade && state.lastPlayedMeter == codex::Meter::Blade)
          active = true;
        if (c.coupletCond == codex::CoupletType::AfterWard && state.lastPlayedMeter == codex::Meter::Ward)
          active = true;
        if (c.coupletCond == codex::CoupletType::AfterScript && state.lastPlayedMeter == codex::Meter::Script)
          active = true;
      }

      int text1Y = coupletBoxY + 3;
      int text2Y = text1Y + renderer.getLineHeight(SMALL_FONT_ID);

      if (active) {
        renderer.fillRect(x + 4, coupletBoxY + 1, cardW - 8, coupletBoxH - 2, true);
        const char* t1 = "COUPLET";
        int w1 = renderer.getTextWidth(SMALL_FONT_ID, t1, EpdFontFamily::BOLD);
        renderer.drawText(SMALL_FONT_ID, x + (cardW - w1) / 2, text1Y, t1, false, EpdFontFamily::BOLD);

        char t2Buf[24];
        if (c.effect == codex::CardEffect::ApplyPoison) {
          snprintf(t2Buf, sizeof(t2Buf), "+%d Poison", c.coupletBonus);
        } else if (c.effect == codex::CardEffect::Block) {
          snprintf(t2Buf, sizeof(t2Buf), "+%d Ward", c.coupletBonus);
        } else if (c.effect == codex::CardEffect::BlotCard || c.effect == codex::CardEffect::DrawCards) {
          snprintf(t2Buf, sizeof(t2Buf), "+%d Ink", c.coupletBonus);
        } else {
          snprintf(t2Buf, sizeof(t2Buf), "+%d Dmg", c.coupletBonus);
        }
        int w2 = renderer.getTextWidth(SMALL_FONT_ID, t2Buf, EpdFontFamily::BOLD);
        renderer.drawText(SMALL_FONT_ID, x + (cardW - w2) / 2, text2Y, t2Buf, false, EpdFontFamily::BOLD);
      } else {
        const char* t1 = "Couplet:";
        int w1 = renderer.getTextWidth(SMALL_FONT_ID, t1);
        renderer.drawText(SMALL_FONT_ID, x + (cardW - w1) / 2, text1Y, t1, true);

        const char* t2 = (c.coupletCond == codex::CoupletType::AfterBlade)
                             ? "Blade"
                             : ((c.coupletCond == codex::CoupletType::AfterWard) ? "Ward" : "Script");
        int w2 = renderer.getTextWidth(SMALL_FONT_ID, t2);
        renderer.drawText(SMALL_FONT_ID, x + (cardW - w2) / 2, text2Y, t2, true);
      }
    }
  }

  // --- Reward Screen ---

  void loopReward() {
    int total = static_cast<int>(state.cardRewards.size()) + 1;  // +1 to skip

    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      savedScreen_ = Screen::Reward;
      saveGameState();
      stateDirty_ = false;
      screen = Screen::Title;
      requestUpdate();
      return;
    }

    // Top-left on X3: Open Deck
    if (mappedInput.wasReleased(MappedInputManager::Button::Up) ||
        mappedInput.wasReleased(MappedInputManager::Button::PageBack)) {
      cardListTab = CardListTab::Deck;
      cardListPage = 0;
      previousScreen = Screen::Reward;
      screen = Screen::DeckView;
      requestUpdate();
      return;
    }

    // Top-right on X3: Open Menu
    if (mappedInput.wasReleased(MappedInputManager::Button::Down) ||
        mappedInput.wasReleased(MappedInputManager::Button::PageForward)) {
      cardListTab = CardListTab::Menu;
      cardListPage = 0;
      previousScreen = Screen::Reward;
      screen = Screen::DeckView;
      requestUpdate();
      return;
    }

    if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
      rewardIndex = (rewardIndex - 1 + total) % total;
      requestUpdate();
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Right)) {
      rewardIndex = (rewardIndex + 1) % total;
      requestUpdate();
    }

    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      if (rewardIndex < static_cast<int>(state.cardRewards.size())) {
        state.deck.push_back(state.cardRewards[rewardIndex]);
      }
      advanceFloor();
    }

    int tx = 0, ty = 0;
    if (mappedInput.wasScreenTouchDown(tx, ty)) {
      handleRewardTouch(tx, ty);
    }
  }

  void handleRewardTouch(int tx, int ty) {
    const int w = renderer.getScreenWidth();
    const auto& m = UITheme::getInstance().getMetrics();

    // Check header / top corner tap to view Deck / Discard / Relics / Menu
    if (ty < m.topPadding + m.headerHeight + 20) {
      if (tx < w / 4) {
        cardListTab = CardListTab::Deck;
      } else if (tx < w / 2) {
        cardListTab = CardListTab::Discard;
      } else if (tx < (3 * w) / 4) {
        cardListTab = CardListTab::Relics;
      } else {
        cardListTab = CardListTab::Menu;
      }
      cardListPage = 0;
      previousScreen = Screen::Reward;
      screen = Screen::DeckView;
      requestUpdate();
      return;
    }

    int count = static_cast<int>(state.cardRewards.size());
    int cardW = 125;
    int startX = (w - (cardW * count)) / 2;
    int cardsY = m.topPadding + m.headerHeight + 2 + 14 + 22 + 20 + 14;

    // Check cards
    if (ty >= cardsY - 10 && ty <= cardsY + 210) {
      for (int i = 0; i < count; ++i) {
        int cx = startX + (i * cardW);
        if (tx >= cx && tx <= cx + cardW) {
          if (rewardIndex == i) {
            state.deck.push_back(state.cardRewards[i]);
            advanceFloor();
          } else {
            rewardIndex = i;
            requestUpdate();
          }
          return;
        }
      }
    }

    // Check Skip button
    int inspY = cardsY + 208;
    int btnY = inspY + 118 + 18;
    if (ty >= btnY && ty <= btnY + 44) {
      if (rewardIndex == count) {
        advanceFloor();
      } else {
        rewardIndex = count;
        requestUpdate();
      }
    }
  }

  bool canAffordChoice(const codex::EventChoice& c) const {
    if (c.goldChange < 0 && state.gold + c.goldChange < 0) {
      return false;
    }
    return true;
  }

  void applyEventChoice(const codex::EventChoice& c) {
    if (c.fullHeal) {
      state.playerHp = state.playerMaxHp;
    } else {
      if (c.maxHpChange != 0) {
        state.playerMaxHp = std::max<int16_t>(5, state.playerMaxHp + c.maxHpChange);
      }
      if (c.hpChange > 0) {
        state.playerHp = std::min<int16_t>(state.playerMaxHp, state.playerHp + c.hpChange);
      } else if (c.hpChange < 0) {
        state.playerHp += c.hpChange;
      }
    }

    state.gold = std::max<int16_t>(0, state.gold + c.goldChange);
    state.permStrength += c.permStrength;
    state.baseMaxInk += c.maxInkChange;
    state.bonusWard += c.nextCombatWard;

    if (c.giveCardId != 0xFF && c.giveCardId < codex::CARD_COUNT) {
      state.deck.push_back(c.giveCardId);
    }

    if (c.giveRelicId == 0xFE) {
      state.giveRandomRelic();
    } else if (c.giveRelicId != 0xFF && c.giveRelicId < codex::RELIC_COUNT) {
      state.giveRelic(c.giveRelicId);
    }

    eventResolved = true;
    stateDirty_ = true;
  }

  void startRandomEvent() {
    std::vector<uint8_t> unvisited;
    for (uint8_t i = 0; i < codex::kEventCount; ++i) {
      if ((visitedEvents & (1 << i)) == 0) {
        unvisited.push_back(i);
      }
    }
    if (unvisited.empty()) {
      visitedEvents = 0;
      for (uint8_t i = 0; i < codex::kEventCount; ++i) {
        unvisited.push_back(i);
      }
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::uniform_int_distribution<size_t> dist(0, unvisited.size() - 1);
    currentEventId = unvisited[dist(g)];
    visitedEvents |= (1 << currentEventId);

    eventChoiceIndex = 0;
    eventResolved = false;
    screen = Screen::Scriptorium;
    savedScreen_ = Screen::Scriptorium;
    stateDirty_ = true;
    requestUpdate();
  }

  void advanceToNextCombat() { advanceFloor(); }

  void advanceFloor() {
    state.floor++;
    stateDirty_ = true;
    if (state.floor > state.maxFloor) {
      state.inRun = false;
      deleteSaveState();
      screen = Screen::Victory;
      requestUpdate();
      return;
    }

    // Scriptorium Shop appears every 5th page (Page 5 and 10)
    if (state.floor == 5 || state.floor == 10) {
      startShop();
    } else if (state.floor == 3 || state.floor == 7 || state.floor == 9 || state.floor == 13) {
      startRandomEvent();
    } else {
      bool isBoss = (state.floor == 15);
      bool isElite = (state.floor == 8 || state.floor == 11 || state.floor == 14);
      state.startCombat(isElite, isBoss);
      screen = Screen::Combat;
      savedScreen_ = Screen::Combat;
      requestUpdate();
    }
  }

  const uint8_t* getEventSprite(uint8_t eventId) const {
    if (eventId < codex::kEventCount) {
      return codex::kEventSprites[eventId];
    }
    return codex::SpriteCodexLarge;
  }

  int getEventSpriteSize(uint8_t /*eventId*/) const { return codex::kEventSpriteSize; }

  void renderReward() {
    const int w = renderer.getScreenWidth();
    const int h = renderer.getScreenHeight();
    const auto& m = UITheme::getInstance().getMetrics();

    GUI.drawHeader(renderer, Rect{0, m.topPadding, w, m.headerHeight}, "Victory!", "Choose a Verse");

    const int topY = m.topPadding + m.headerHeight + 2;
    const int bottomY = h - m.buttonHintsHeight - 2;
    drawCodexPageFrame(w, h, topY, bottomY);

    int y = topY + 12;
    renderer.drawCenteredText(UI_12_FONT_ID, y, "The beast dissolves into ink droplets.", true, EpdFontFamily::BOLD);
    y += renderer.getLineHeight(UI_12_FONT_ID) + 4;
    renderer.drawCenteredText(SMALL_FONT_ID, y, "Select a new verse to transcribe into your grimoire:", true);
    y += renderer.getLineHeight(SMALL_FONT_ID) + 8;

    drawManuscriptDivider(y, w, 28);
    y += 14;

    int count = static_cast<int>(state.cardRewards.size());
    int cardW = 125;
    int startX = (w - (cardW * count)) / 2;

    for (int i = 0; i < count; ++i) {
      bool isSelected = (i == rewardIndex);
      int cx = startX + (i * cardW);
      int cy = isSelected ? y - 6 : y;
      uint8_t cId = state.cardRewards[i];
      if (cId < codex::CARD_COUNT) {
        renderCard(cx, cy, cardW - 8, 200, codex::kCardCatalog[cId], isSelected);
      }
    }

    // Card detail inspector for highlighted reward card
    int inspX = 16;
    int inspW = w - 32;
    int inspH = 118;
    int inspY = y + 208;

    if (rewardIndex >= 0 && rewardIndex < count) {
      uint8_t selId = state.cardRewards[rewardIndex];
      if (selId < codex::CARD_COUNT) {
        renderCardInspector(inspX, inspY, inspW, inspH, codex::kCardCatalog[selId], false);
      }
    } else {
      // Skip selected
      drawManuscriptMiniatureFrame(inspX, inspY, inspW, inspH);
      int ty = inspY + (inspH - renderer.getLineHeight(UI_12_FONT_ID) - renderer.getLineHeight(SMALL_FONT_ID) - 8) / 2;
      renderer.drawCenteredText(UI_12_FONT_ID, ty, "Skip Verse", true, EpdFontFamily::BOLD);
      char skipDesc[80];
      snprintf(skipDesc, sizeof(skipDesc), "Advance to Page %d without transcribing a new verse.", state.floor + 1);
      renderer.drawCenteredText(SMALL_FONT_ID, ty + renderer.getLineHeight(UI_12_FONT_ID) + 8, skipDesc, true);
    }

    const char* skipLabel = "Skip Card Reward";
    int textW = renderer.getTextWidth(UI_12_FONT_ID, skipLabel, EpdFontFamily::BOLD);
    int btnW = textW + 36;
    int btnH = 36;
    int btnX = (w - btnW) / 2;
    int btnY = inspY + inspH + 18;

    bool skipSelected = (rewardIndex == count);
    if (skipSelected) {
      renderer.fillRoundedRect(btnX, btnY, btnW, btnH, 6, Color::Black);
      int textY = btnY + (btnH - renderer.getLineHeight(UI_12_FONT_ID)) / 2;
      renderer.drawText(UI_12_FONT_ID, btnX + (btnW - textW) / 2, textY, skipLabel, false, EpdFontFamily::BOLD);
    } else {
      renderer.drawRoundedRect(btnX, btnY, btnW, btnH, 1, 6, true);
      int textY = btnY + (btnH - renderer.getLineHeight(UI_12_FONT_ID)) / 2;
      renderer.drawText(UI_12_FONT_ID, btnX + (btnW - textW) / 2, textY, skipLabel, true, EpdFontFamily::BOLD);
    }

    const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_LEFT), tr(STR_DIR_RIGHT));
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  }

  // --- Scriptorium (Rest Site) ---

  // --- Scriptorium (Chamber Event System) ---

  void handleScriptoriumTouch(int tx, int ty) {
    const int w = renderer.getScreenWidth();
    const auto& m = UITheme::getInstance().getMetrics();

    // Check header / top corner tap to view Deck / Discard / Relics / Menu
    if (ty < m.topPadding + m.headerHeight + 20) {
      if (tx < w / 4) {
        cardListTab = CardListTab::Deck;
      } else if (tx < w / 2) {
        cardListTab = CardListTab::Discard;
      } else if (tx < (3 * w) / 4) {
        cardListTab = CardListTab::Relics;
      } else {
        cardListTab = CardListTab::Menu;
      }
      cardListPage = 0;
      previousScreen = Screen::Scriptorium;
      screen = Screen::DeckView;
      requestUpdate();
      return;
    }

    if (currentEventId >= codex::kEventCount) currentEventId = 0;
    const auto& event = codex::kEvents[currentEventId];

    const int topY = m.topPadding + m.headerHeight + 2;
    int y = topY + 6 + renderer.getLineHeight(SMALL_FONT_ID) + 6 + 12;
    int spriteSize = getEventSpriteSize(currentEventId);
    y += (spriteSize + 10) + 10;
    for (int i = 0; i < 3; ++i) {
      if (event.narrative[i] && event.narrative[i][0] != '\0') {
        auto nLines = renderer.wrappedText(SMALL_FONT_ID, event.narrative[i], w - 64, 2);
        for (const auto& nl : nLines) {
          y += renderer.getLineHeight(SMALL_FONT_ID) + 2;
        }
      }
    }
    y += 6 + 14;

    if (!eventResolved) {
      int optW = w - 44;
      int optX = 22;
      int optH = 78;
      int gap = 10;

      for (int i = 0; i < event.choiceCount; ++i) {
        int itemY = y + (i * (optH + gap));
        if (tx >= optX && tx <= optX + optW && ty >= itemY && ty <= itemY + optH) {
          if (eventChoiceIndex == i) {
            const auto& c = event.choices[i];
            if (canAffordChoice(c)) {
              applyEventChoice(c);
            }
          } else {
            eventChoiceIndex = i;
          }
          requestUpdate();
          return;
        }
      }
    } else {
      int resBoxH = 140;
      int resBoxY = y + 4;
      int btnW = 260;
      int btnH = 46;
      int btnX = (w - btnW) / 2;
      int btnY = resBoxY + resBoxH + 24;

      if (tx >= btnX && tx <= btnX + btnW && ty >= btnY && ty <= btnY + btnH) {
        if (state.playerHp <= 0) {
          state.inRun = false;
          deleteSaveState();
          screen = Screen::GameOver;
        } else {
          advanceToNextCombat();
        }
        requestUpdate();
      }
    }
  }

  void loopScriptorium() {
    if (currentEventId >= codex::kEventCount) {
      currentEventId = 0;
    }
    const auto& event = codex::kEvents[currentEventId];
    int choiceCount = event.choiceCount;

    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      if (eventResolved && state.playerHp <= 0) {
        state.inRun = false;
        deleteSaveState();
        screen = Screen::GameOver;
        requestUpdate();
        return;
      }
      savedScreen_ = Screen::Scriptorium;
      saveGameState();
      stateDirty_ = false;
      screen = Screen::Title;
      requestUpdate();
      return;
    }

    // Top-left on X3: Open Deck
    if (mappedInput.wasReleased(MappedInputManager::Button::Up) ||
        mappedInput.wasReleased(MappedInputManager::Button::PageBack)) {
      cardListTab = CardListTab::Deck;
      cardListPage = 0;
      previousScreen = Screen::Scriptorium;
      screen = Screen::DeckView;
      requestUpdate();
      return;
    }

    // Top-right on X3: Open Menu
    if (mappedInput.wasReleased(MappedInputManager::Button::Down) ||
        mappedInput.wasReleased(MappedInputManager::Button::PageForward)) {
      cardListTab = CardListTab::Menu;
      cardListPage = 0;
      previousScreen = Screen::Scriptorium;
      screen = Screen::DeckView;
      requestUpdate();
      return;
    }

    if (!eventResolved) {
      if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
        eventChoiceIndex = (eventChoiceIndex - 1 + choiceCount) % choiceCount;
        requestUpdate();
      } else if (mappedInput.wasReleased(MappedInputManager::Button::Right)) {
        eventChoiceIndex = (eventChoiceIndex + 1) % choiceCount;
        requestUpdate();
      }

      if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
        if (eventChoiceIndex >= 0 && eventChoiceIndex < choiceCount) {
          const auto& c = event.choices[eventChoiceIndex];
          if (canAffordChoice(c)) {
            applyEventChoice(c);
            requestUpdate();
          }
        }
      }
    } else {
      if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
        if (state.playerHp <= 0) {
          state.inRun = false;
          deleteSaveState();
          screen = Screen::GameOver;
        } else {
          advanceToNextCombat();
        }
        requestUpdate();
        return;
      }
    }

    int tx = 0, ty = 0;
    if (mappedInput.wasScreenTouchDown(tx, ty)) {
      handleScriptoriumTouch(tx, ty);
    }
  }

  void renderScriptorium() {
    const int w = renderer.getScreenWidth();
    const int h = renderer.getScreenHeight();
    const auto& m = UITheme::getInstance().getMetrics();

    if (currentEventId >= codex::kEventCount) currentEventId = 0;
    const auto& event = codex::kEvents[currentEventId];

    GUI.drawHeader(renderer, Rect{0, m.topPadding, w, m.headerHeight}, event.title, event.subtitle);

    const int topY = m.topPadding + m.headerHeight + 2;
    const int bottomY = h - m.buttonHintsHeight - 2;
    drawCodexPageFrame(w, h, topY, bottomY);

    int y = topY + 6;

    // Status Bar
    char statBuf[80];
    if (state.playerHp <= 0) {
      snprintf(statBuf, sizeof(statBuf), "HP: 0 / %d (DECEASED)   |   Gold: %d   |   Deck: %zu", state.playerMaxHp,
               state.gold, state.deck.size());
    } else {
      snprintf(statBuf, sizeof(statBuf), "HP: %d/%d   |   Gold: %d   |   Deck: %zu   |   Relics: %zu", state.playerHp,
               state.playerMaxHp, state.gold, state.deck.size(), state.relics.size());
    }
    renderer.drawCenteredText(SMALL_FONT_ID, y, statBuf, true, EpdFontFamily::BOLD);
    y += renderer.getLineHeight(SMALL_FONT_ID) + 6;

    drawManuscriptDivider(y, w, 28);
    y += 12;

    // Miniature Plate
    const uint8_t* eventSprite = getEventSprite(currentEventId);
    int spriteSize = getEventSpriteSize(currentEventId);
    int plateW = spriteSize + 10;
    int plateH = spriteSize + 10;
    int plateX = (w - plateW) / 2;
    drawManuscriptMiniatureFrame(plateX, y, plateW, plateH);
    renderer.drawIcon(eventSprite, plateX + 5, y + 5, spriteSize, spriteSize);
    y += plateH + 10;

    // Atmospheric Narrative Prose
    for (int i = 0; i < 3; ++i) {
      if (event.narrative[i] && event.narrative[i][0] != '\0') {
        auto nLines = renderer.wrappedText(SMALL_FONT_ID, event.narrative[i], w - 64, 2);
        for (const auto& nl : nLines) {
          renderer.drawCenteredText(SMALL_FONT_ID, y, nl.c_str(), true);
          y += renderer.getLineHeight(SMALL_FONT_ID) + 2;
        }
      }
    }
    y += 6;

    drawManuscriptDivider(y, w, 28);
    y += 14;

    if (!eventResolved) {
      int optW = w - 44;
      int optX = 22;
      int optH = 78;
      int gap = 10;

      for (int i = 0; i < event.choiceCount; ++i) {
        const auto& c = event.choices[i];
        bool isSelected = (i == eventChoiceIndex);
        bool affordable = canAffordChoice(c);
        int itemY = y + (i * (optH + gap));

        if (isSelected) {
          renderer.fillRoundedRect(optX, itemY, optW, optH, 6, Color::Black);
          renderer.drawText(UI_12_FONT_ID, optX + 14, itemY + 10, c.label, false, EpdFontFamily::BOLD);
          if (!affordable) {
            char reqBuf[48];
            snprintf(reqBuf, sizeof(reqBuf), "[Requires %d Gold - Cannot Afford]", -c.goldChange);
            renderer.drawText(SMALL_FONT_ID, optX + 14, itemY + 36, reqBuf, false, EpdFontFamily::BOLD);
          } else {
            renderer.drawText(SMALL_FONT_ID, optX + 14, itemY + 36, c.effectDesc, false);
          }
        } else {
          renderer.drawRoundedRect(optX, itemY, optW, optH, 1, 6, true);
          renderer.drawText(UI_12_FONT_ID, optX + 14, itemY + 10, c.label, true, EpdFontFamily::BOLD);
          if (!affordable) {
            char reqBuf[48];
            snprintf(reqBuf, sizeof(reqBuf), "[Requires %d Gold - Cannot Afford]", -c.goldChange);
            renderer.drawText(SMALL_FONT_ID, optX + 14, itemY + 36, reqBuf, true, EpdFontFamily::BOLD);
          } else {
            renderer.drawText(SMALL_FONT_ID, optX + 14, itemY + 36, c.effectDesc, true);
          }
        }
      }

      const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_LEFT), tr(STR_DIR_RIGHT));
      GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
    } else {
      const auto& c = event.choices[eventChoiceIndex];
      int resBoxW = w - 44;
      int resBoxX = 22;
      int resBoxH = 140;
      int resBoxY = y + 4;
      drawManuscriptMiniatureFrame(resBoxX, resBoxY, resBoxW, resBoxH);

      int textY = resBoxY + 14;
      renderer.drawCenteredText(UI_12_FONT_ID, textY, "Consequence", true, EpdFontFamily::BOLD);
      textY += renderer.getLineHeight(UI_12_FONT_ID) + 8;

      auto outLines = renderer.wrappedText(SMALL_FONT_ID, c.outcomeText, resBoxW - 28, 3);
      for (const auto& ol : outLines) {
        renderer.drawCenteredText(SMALL_FONT_ID, textY, ol.c_str(), true);
        textY += renderer.getLineHeight(SMALL_FONT_ID) + 3;
      }

      int btnW = 260;
      int btnH = 46;
      int btnX = (w - btnW) / 2;
      int btnY = resBoxY + resBoxH + 24;

      renderer.fillRoundedRect(btnX, btnY, btnW, btnH, 6, Color::Black);
      const char* continueLabel = (state.playerHp <= 0) ? "Succumb to Wounds" : "Continue Delve";
      int textW = renderer.getTextWidth(UI_12_FONT_ID, continueLabel, EpdFontFamily::BOLD);
      int cTextY = btnY + (btnH - renderer.getLineHeight(UI_12_FONT_ID)) / 2;
      renderer.drawText(UI_12_FONT_ID, btnX + (btnW - textW) / 2, cTextY, continueLabel, false, EpdFontFamily::BOLD);

      const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_CONFIRM), nullptr, nullptr);
      GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
    }
  }

  // --- Scriptorium Merchant (Shop) ---

  void generateShop() {
    std::random_device rd;
    std::mt19937 g(rd());

    // 3 Cards: pick from pool of non-starter cards
    std::vector<uint8_t> cardPool;
    for (uint8_t c = 2; c < codex::CARD_COUNT; ++c) cardPool.push_back(c);
    std::shuffle(cardPool.begin(), cardPool.end(), g);
    for (int i = 0; i < ShopState::CARD_COUNT; ++i) {
      shopState.cards[i] = cardPool[i];
      const auto& cDef = codex::kCardCatalog[cardPool[i]];
      shopState.cardPrices[i] = (cDef.type == codex::CardType::Power) ? 65 : (42 + (cDef.cost * 8));
      shopState.cardSold[i] = false;
    }

    // 2 Relics: pick from unowned relics
    std::vector<uint8_t> relicPool;
    for (uint8_t r = 0; r < codex::RELIC_COUNT; ++r) {
      if (!state.hasRelic(r)) relicPool.push_back(r);
    }
    std::shuffle(relicPool.begin(), relicPool.end(), g);
    for (int i = 0; i < ShopState::RELIC_COUNT; ++i) {
      if (i < static_cast<int>(relicPool.size())) {
        shopState.relics[i] = relicPool[i];
        shopState.relicPrices[i] = 110 + (relicPool[i] % 3) * 15;
        shopState.relicSold[i] = false;
      } else {
        shopState.relics[i] = 0;
        shopState.relicPrices[i] = 999;
        shopState.relicSold[i] = true;
      }
    }

    shopState.healPrice = 35;
    shopState.healAmount = 12;
    shopState.healSold = false;

    shopState.purgePrice = 50;
    shopState.purgeSold = false;

    shopIndex = 0;
    shopPurgeMode = false;
    shopPurgeIndex = 0;

    // Relic 11: Alchemical Flask - Heal 8 HP whenever visiting a Shop
    if (state.hasRelic(11)) {
      state.playerHp = std::min(state.playerMaxHp, static_cast<int16_t>(state.playerHp + 8));
    }
  }

  void startShop() {
    generateShop();
    screen = Screen::Shop;
    savedScreen_ = Screen::Shop;
    stateDirty_ = true;
    requestUpdate();
  }

  void loopShop() {
    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      if (shopPurgeMode) {
        shopPurgeMode = false;
        requestUpdate();
        return;
      }
      savedScreen_ = Screen::Shop;
      saveGameState();
      stateDirty_ = false;
      screen = Screen::Title;
      requestUpdate();
      return;
    }

    // Top-left on X3: Open Deck
    if (mappedInput.wasReleased(MappedInputManager::Button::Up) ||
        mappedInput.wasReleased(MappedInputManager::Button::PageBack)) {
      if (!shopPurgeMode) {
        cardListTab = CardListTab::Deck;
        cardListPage = 0;
        previousScreen = Screen::Shop;
        screen = Screen::DeckView;
        requestUpdate();
        return;
      }
    }

    // Top-right on X3: Open Menu
    if (mappedInput.wasReleased(MappedInputManager::Button::Down) ||
        mappedInput.wasReleased(MappedInputManager::Button::PageForward)) {
      if (!shopPurgeMode) {
        cardListTab = CardListTab::Menu;
        cardListPage = 0;
        previousScreen = Screen::Shop;
        screen = Screen::DeckView;
        requestUpdate();
        return;
      }
    }

    if (shopPurgeMode) {
      int deckSz = static_cast<int>(state.deck.size());
      if (deckSz == 0) {
        shopPurgeMode = false;
        requestUpdate();
        return;
      }
      if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
        shopPurgeIndex = (shopPurgeIndex - 1 + deckSz) % deckSz;
        requestUpdate();
      } else if (mappedInput.wasReleased(MappedInputManager::Button::Right)) {
        shopPurgeIndex = (shopPurgeIndex + 1) % deckSz;
        requestUpdate();
      } else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
        if (state.gold >= shopState.purgePrice && shopPurgeIndex < deckSz) {
          state.gold -= shopState.purgePrice;
          state.deck.erase(state.deck.begin() + shopPurgeIndex);
          shopState.purgeSold = true;
          shopPurgeMode = false;
          stateDirty_ = true;
          requestUpdate();
        }
      }
    } else {
      constexpr int totalItems = 8;
      if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
        shopIndex = (shopIndex - 1 + totalItems) % totalItems;
        requestUpdate();
      } else if (mappedInput.wasReleased(MappedInputManager::Button::Right)) {
        shopIndex = (shopIndex + 1) % totalItems;
        requestUpdate();
      } else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
        if (shopIndex == 7) {
          // Depart Shop
          advanceFloor();
          return;
        } else if (shopIndex >= 0 && shopIndex <= 2) {
          // Buy Card
          int cIdx = shopIndex;
          if (!shopState.cardSold[cIdx] && state.gold >= shopState.cardPrices[cIdx]) {
            state.gold -= shopState.cardPrices[cIdx];
            state.deck.push_back(shopState.cards[cIdx]);
            shopState.cardSold[cIdx] = true;
            stateDirty_ = true;
            requestUpdate();
          }
        } else if (shopIndex == 3 || shopIndex == 4) {
          // Buy Relic
          int rIdx = shopIndex - 3;
          if (!shopState.relicSold[rIdx] && state.gold >= shopState.relicPrices[rIdx]) {
            state.gold -= shopState.relicPrices[rIdx];
            state.giveRelic(shopState.relics[rIdx]);
            shopState.relicSold[rIdx] = true;
            stateDirty_ = true;
            requestUpdate();
          }
        } else if (shopIndex == 5) {
          // Buy Heal
          if (!shopState.healSold && state.gold >= shopState.healPrice) {
            state.gold -= shopState.healPrice;
            state.playerHp = std::min(state.playerMaxHp, static_cast<int16_t>(state.playerHp + shopState.healAmount));
            shopState.healSold = true;
            stateDirty_ = true;
            requestUpdate();
          }
        } else if (shopIndex == 6) {
          // Scrape Parchment (Purge)
          if (!shopState.purgeSold && state.gold >= shopState.purgePrice && !state.deck.empty()) {
            shopPurgeMode = true;
            shopPurgeIndex = 0;
            requestUpdate();
          }
        }
      }
    }

    int tx = 0, ty = 0;
    if (mappedInput.wasScreenTouchDown(tx, ty)) {
      handleShopTouch(tx, ty);
    }
  }

  void handleShopTouch(int tx, int ty) {
    const int w = renderer.getScreenWidth();
    const int h = renderer.getScreenHeight();
    const auto& m = UITheme::getInstance().getMetrics();
    const int bottomY = h - m.buttonHintsHeight - 2;

    // Top ribbon tap
    if (ty < m.topPadding + m.headerHeight + 20) {
      if (tx < w / 2) {
        cardListTab = CardListTab::Deck;
      } else {
        cardListTab = CardListTab::Menu;
      }
      cardListPage = 0;
      previousScreen = Screen::Shop;
      screen = Screen::DeckView;
      requestUpdate();
      return;
    }

    if (shopPurgeMode) {
      int deckSz = static_cast<int>(state.deck.size());
      if (deckSz == 0) return;
      int btnY = bottomY - 50;
      if (ty >= btnY && ty <= btnY + 44) {
        if (state.gold >= shopState.purgePrice && shopPurgeIndex < deckSz) {
          state.gold -= shopState.purgePrice;
          state.deck.erase(state.deck.begin() + shopPurgeIndex);
          shopState.purgeSold = true;
          shopPurgeMode = false;
          stateDirty_ = true;
          requestUpdate();
        }
        return;
      }
      if (tx < w / 3) {
        shopPurgeIndex = (shopPurgeIndex - 1 + deckSz) % deckSz;
        requestUpdate();
      } else if (tx > (2 * w) / 3) {
        shopPurgeIndex = (shopPurgeIndex + 1) % deckSz;
        requestUpdate();
      }
      return;
    }

    // Depart button at bottom
    int depBtnY = bottomY - 50;
    if (ty >= depBtnY && ty <= depBtnY + 44) {
      advanceFloor();
      return;
    }

    // Draught (Heal) hitbox
    if (tx >= 166 && tx <= 456 && ty >= 126 && ty <= 164) {
      if (shopIndex == 5) {
        if (!shopState.healSold && state.gold >= shopState.healPrice) {
          state.gold -= shopState.healPrice;
          state.playerHp = std::min(state.playerMaxHp, static_cast<int16_t>(state.playerHp + shopState.healAmount));
          shopState.healSold = true;
          stateDirty_ = true;
        }
      } else {
        shopIndex = 5;
      }
      requestUpdate();
      return;
    }

    // Scrape Parchment (Purge) hitbox
    if (tx >= 166 && tx <= 456 && ty >= 170 && ty <= 208) {
      if (shopIndex == 6) {
        if (!shopState.purgeSold && state.gold >= shopState.purgePrice && !state.deck.empty()) {
          shopPurgeMode = true;
          shopPurgeIndex = 0;
        }
      } else {
        shopIndex = 6;
      }
      requestUpdate();
      return;
    }

    // Cards (y = 225..420)
    int cardW = 136;
    int cardGap = 8;
    int startCardsX = (w - (3 * cardW + 2 * cardGap)) / 2;
    for (int i = 0; i < 3; ++i) {
      int cx = startCardsX + i * (cardW + cardGap);
      if (tx >= cx && tx <= cx + cardW && ty >= 225 && ty <= 420) {
        if (shopIndex == i) {
          if (!shopState.cardSold[i] && state.gold >= shopState.cardPrices[i]) {
            state.gold -= shopState.cardPrices[i];
            state.deck.push_back(shopState.cards[i]);
            shopState.cardSold[i] = true;
            stateDirty_ = true;
          }
        } else {
          shopIndex = i;
        }
        requestUpdate();
        return;
      }
    }

    // Relics (y = 425..505)
    int relW = 208;
    for (int r = 0; r < 2; ++r) {
      int rx = 24 + r * (relW + 16);
      if (tx >= rx && tx <= rx + relW && ty >= 425 && ty <= 505) {
        if (shopIndex == 3 + r) {
          if (!shopState.relicSold[r] && state.gold >= shopState.relicPrices[r]) {
            state.gold -= shopState.relicPrices[r];
            state.giveRelic(shopState.relics[r]);
            shopState.relicSold[r] = true;
            stateDirty_ = true;
          }
        } else {
          shopIndex = 3 + r;
        }
        requestUpdate();
        return;
      }
    }
  }

  void renderShop() {
    const int w = renderer.getScreenWidth();
    const int h = renderer.getScreenHeight();
    const auto& m = UITheme::getInstance().getMetrics();

    char shopSubHeader[32];
    snprintf(shopSubHeader, sizeof(shopSubHeader), "Ch.1 Pg %d/15", state.floor);
    GUI.drawHeader(renderer, Rect{0, m.topPadding, w, m.headerHeight}, "Scriptorium", shopSubHeader);

    const int topY = m.topPadding + m.headerHeight + 2;
    const int bottomY = h - m.buttonHintsHeight - 2;
    drawCodexPageFrame(w, h, topY, bottomY);

    int y = topY + 6;

    // Status bar
    char statBuf[80];
    snprintf(statBuf, sizeof(statBuf), "HP: %d/%d   |   Gold: %d   |   Deck: %zu   |   Relics: %zu", state.playerHp,
             state.playerMaxHp, state.gold, state.deck.size(), state.relics.size());
    renderer.drawCenteredText(SMALL_FONT_ID, y, statBuf, true, EpdFontFamily::BOLD);
    y += renderer.getLineHeight(SMALL_FONT_ID) + 6;

    drawManuscriptDivider(y, w, 28);
    y += 10;

    if (shopPurgeMode) {
      renderer.drawCenteredText(UI_12_FONT_ID, y, "Scrape Parchment (Purge)", true, EpdFontFamily::BOLD);
      y += renderer.getLineHeight(UI_12_FONT_ID) + 4;
      char purgeInfo[80];
      snprintf(purgeInfo, sizeof(purgeInfo), "Choose a verse to purge from your grimoire (%d Gold).",
               shopState.purgePrice);
      renderer.drawCenteredText(SMALL_FONT_ID, y, purgeInfo, true);
      y += renderer.getLineHeight(SMALL_FONT_ID) + 8;

      drawManuscriptDivider(y, w, 28);
      y += 12;

      if (!state.deck.empty()) {
        if (shopPurgeIndex >= static_cast<int>(state.deck.size())) shopPurgeIndex = 0;
        uint8_t cId = state.deck[shopPurgeIndex];
        int cardW = 140;
        int cardH = 200;
        int cardX = (w - cardW) / 2;
        if (cId < codex::CARD_COUNT) {
          renderCard(cardX, y, cardW, cardH, codex::kCardCatalog[cId], true);
        }
        y += cardH + 12;

        char counterBuf[48];
        snprintf(counterBuf, sizeof(counterBuf), "< Verse %d / %zu >", shopPurgeIndex + 1, state.deck.size());
        renderer.drawCenteredText(UI_12_FONT_ID, y, counterBuf, true, EpdFontFamily::BOLD);
        y += renderer.getLineHeight(UI_12_FONT_ID) + 8;

        int inspX = 24;
        int inspW = w - 48;
        int inspH = 90;
        if (cId < codex::CARD_COUNT) {
          renderCardInspector(inspX, y, inspW, inspH, codex::kCardCatalog[cId], false);
        }
      }

      int btnW = w - 80;
      int btnH = 42;
      int btnX = 40;
      int btnY = bottomY - 48;
      bool canAfford = (state.gold >= shopState.purgePrice);
      renderer.fillRoundedRect(btnX, btnY, btnW, btnH, 4, Color::Black);
      char btnTxt[64];
      snprintf(btnTxt, sizeof(btnTxt), canAfford ? "Scrape Verse (%d Gold)" : "Cannot Afford (%d Gold)",
               shopState.purgePrice);
      renderer.drawCenteredText(UI_12_FONT_ID, btnY + (btnH - renderer.getLineHeight(UI_12_FONT_ID)) / 2, btnTxt,
                                false, EpdFontFamily::BOLD);

      const auto labels = mappedInput.mapLabels(tr(STR_BACK), "Scrape", "< Verse", "Verse >");
      GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
      return;
    }

    // Merchant Area (y = 78..214)
    int plateX = 22;
    int plateY = y;
    drawManuscriptMiniatureFrame(plateX, plateY, 136, 136);
    renderer.drawIcon(codex::SpriteShopMerchant, plateX + 4, plateY + 4, 128, 128);

    // Right of Merchant
    int rightX = 168;
    int rightW = w - rightX - 22;
    auto gLines = renderer.wrappedText(SMALL_FONT_ID,
                                      "Ink, relics, or a clean slate? All knowledge comes with a price, traveler.",
                                      rightW, 3);
    int gy = plateY + 2;
    for (const auto& gl : gLines) {
      renderer.drawText(SMALL_FONT_ID, rightX, gy, gl.c_str(), true);
      gy += renderer.getLineHeight(SMALL_FONT_ID) + 2;
    }

    // Services
    // Draught (Heal)
    int srvW = rightW;
    int srvH = 36;
    int draughtY = plateY + 54;
    bool draughtSel = (shopIndex == 5);
    if (draughtSel) {
      renderer.fillRoundedRect(rightX, draughtY, srvW, srvH, 4, Color::Black);
    } else {
      renderer.drawRoundedRect(rightX, draughtY, srvW, srvH, 1, 4, true);
    }
    char dBuf[48];
    if (shopState.healSold) {
      snprintf(dBuf, sizeof(dBuf), "[SOLD] Scribe's Draught");
    } else {
      snprintf(dBuf, sizeof(dBuf), "Scribe's Draught (+12 HP) [35g]");
    }
    renderer.drawText(SMALL_FONT_ID, rightX + 8, draughtY + 11, dBuf, !draughtSel,
                      draughtSel ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR);

    // Scrape Parchment (Purge)
    int scrapeY = draughtY + srvH + 6;
    bool scrapeSel = (shopIndex == 6);
    if (scrapeSel) {
      renderer.fillRoundedRect(rightX, scrapeY, srvW, srvH, 4, Color::Black);
    } else {
      renderer.drawRoundedRect(rightX, scrapeY, srvW, srvH, 1, 4, true);
    }
    char sBuf[48];
    if (shopState.purgeSold) {
      snprintf(sBuf, sizeof(sBuf), "[SOLD] Scrape Parchment");
    } else {
      snprintf(sBuf, sizeof(sBuf), "Scrape Parchment (Purge) [50g]");
    }
    renderer.drawText(SMALL_FONT_ID, rightX + 8, scrapeY + 11, sBuf, !scrapeSel,
                      scrapeSel ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR);

    y = plateY + 136 + 8;
    drawManuscriptDivider(y, w, 28);
    y += 10;

    // Section 1: Verses for Sale (Cards)
    int cardW = 136;
    int cardH = 150;
    int cardGap = 8;
    int startCardsX = (w - (3 * cardW + 2 * cardGap)) / 2;
    int cardY = y;

    for (int i = 0; i < 3; ++i) {
      int cx = startCardsX + i * (cardW + cardGap);
      bool isSelected = (shopIndex == i);
      uint8_t cId = shopState.cards[i];
      if (cId < codex::CARD_COUNT) {
        renderCard(cx, cardY, cardW, cardH, codex::kCardCatalog[cId], isSelected);
      }
      // Price tag centered under each card
      int tagY = cardY + cardH + 5;
      char tagBuf[32];
      const char* tagStr = nullptr;
      if (shopState.cardSold[i]) {
        tagStr = "[ SOLD ]";
      } else {
        snprintf(tagBuf, sizeof(tagBuf), "[ %d Gold ]", shopState.cardPrices[i]);
        tagStr = tagBuf;
      }
      auto fontStyle = isSelected ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;
      int tw = renderer.getTextWidth(SMALL_FONT_ID, tagStr, fontStyle);
      int tagX = cx + (cardW - tw) / 2;
      renderer.drawText(SMALL_FONT_ID, tagX, tagY, tagStr, true, fontStyle);
    }

    y = cardY + cardH + 26;
    drawManuscriptDivider(y, w, 28);
    y += 8;

    // Section 2: Relics
    int relW = 208;
    int relH = 62;
    int startRelX = 24;
    int relGap = 16;
    int relY = y;

    for (int r = 0; r < 2; ++r) {
      int rx = startRelX + r * (relW + relGap);
      bool isSelected = (shopIndex == 3 + r);
      uint8_t rId = shopState.relics[r];

      if (isSelected) {
        renderer.fillRoundedRect(rx, relY, relW, relH, 4, Color::Black);
      } else {
        renderer.drawRoundedRect(rx, relY, relW, relH, 1, 4, true);
      }

      // Relic Icon (48x48)
      if (rId < codex::RELIC_COUNT) {
        renderer.drawIcon(codex::kRelicSprites[rId], rx + 6, relY + 7, 48, 48);
      }

      const char* rName = (rId < codex::RELIC_COUNT) ? codex::kRelicCatalog[rId].name : "Relic";
      renderer.drawText(SMALL_FONT_ID, rx + 58, relY + 12, rName, !isSelected, EpdFontFamily::BOLD);

      if (shopState.relicSold[r]) {
        renderer.drawText(SMALL_FONT_ID, rx + 58, relY + 34, "[ SOLD ]", !isSelected);
      } else {
        char rpTag[32];
        snprintf(rpTag, sizeof(rpTag), "[ %d Gold ]", shopState.relicPrices[r]);
        renderer.drawText(SMALL_FONT_ID, rx + 58, relY + 34, rpTag, !isSelected, EpdFontFamily::BOLD);
      }
    }

    y = relY + relH + 10;

    // Section 3: Item Inspector Box
    int inspX = 24;
    int inspW = w - 48;
    int inspH = 78;
    int inspY = y;
    drawManuscriptMiniatureFrame(inspX, inspY, inspW, inspH);

    if (shopIndex >= 0 && shopIndex <= 2) {
      uint8_t cId = shopState.cards[shopIndex];
      if (cId < codex::CARD_COUNT) {
        renderCardInspector(inspX + 2, inspY + 2, inspW - 4, inspH - 4, codex::kCardCatalog[cId], false);
      }
    } else if (shopIndex == 3 || shopIndex == 4) {
      int rIdx = shopIndex - 3;
      uint8_t rId = shopState.relics[rIdx];
      const char* rName = (rId < codex::RELIC_COUNT) ? codex::kRelicCatalog[rId].name : "Relic";
      const char* rDesc = (rId < codex::RELIC_COUNT) ? codex::kRelicCatalog[rId].desc : "";
      int ty = inspY + 8;
      renderer.drawCenteredText(UI_12_FONT_ID, ty, rName, true, EpdFontFamily::BOLD);
      ty += renderer.getLineHeight(UI_12_FONT_ID) + 4;
      auto dLines = renderer.wrappedText(SMALL_FONT_ID, rDesc, inspW - 20, 2);
      for (const auto& dl : dLines) {
        renderer.drawCenteredText(SMALL_FONT_ID, ty, dl.c_str(), true);
        ty += renderer.getLineHeight(SMALL_FONT_ID) + 2;
      }
    } else if (shopIndex == 5) {
      int ty = inspY + 14;
      renderer.drawCenteredText(UI_12_FONT_ID, ty, "Scribe's Draught", true, EpdFontFamily::BOLD);
      ty += renderer.getLineHeight(UI_12_FONT_ID) + 4;
      renderer.drawCenteredText(SMALL_FONT_ID, ty, "A soothing tonic that restores 12 Hit Points immediately.", true);
    } else if (shopIndex == 6) {
      int ty = inspY + 14;
      renderer.drawCenteredText(UI_12_FONT_ID, ty, "Scrape Parchment", true, EpdFontFamily::BOLD);
      ty += renderer.getLineHeight(UI_12_FONT_ID) + 4;
      renderer.drawCenteredText(SMALL_FONT_ID, ty, "Erase a flawed verse permanently from your grimoire.", true);
    } else if (shopIndex == 7) {
      int ty = inspY + 14;
      renderer.drawCenteredText(UI_12_FONT_ID, ty, "Turn the Page", true, EpdFontFamily::BOLD);
      ty += renderer.getLineHeight(UI_12_FONT_ID) + 4;
      renderer.drawCenteredText(SMALL_FONT_ID, ty, "Depart the scriptorium and advance onward through Chapter 1.", true);
    }

    // Section 4: Depart Button
    int depBtnW = w - 64;
    int depBtnH = 40;
    int depBtnX = 32;
    int depBtnY = bottomY - 48;
    bool depSel = (shopIndex == 7);
    if (depSel) {
      renderer.fillRoundedRect(depBtnX, depBtnY, depBtnW, depBtnH, 4, Color::Black);
      renderer.drawCenteredText(UI_12_FONT_ID, depBtnY + (depBtnH - renderer.getLineHeight(UI_12_FONT_ID)) / 2,
                                "Turn the Page -> (Depart Shop)", false, EpdFontFamily::BOLD);
    } else {
      renderer.drawRoundedRect(depBtnX, depBtnY, depBtnW, depBtnH, 1, 4, true);
      renderer.drawCenteredText(UI_12_FONT_ID, depBtnY + (depBtnH - renderer.getLineHeight(UI_12_FONT_ID)) / 2,
                                "Turn the Page -> (Depart Shop)", true, EpdFontFamily::BOLD);
    }

    const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_LEFT), tr(STR_DIR_RIGHT));
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  }

  // --- Deck / Discard / Relics / Menu View Screen ---

  void selectDeckMenuOption(int opt) {
    if (opt == 0) {
      // Resume Delve
      screen = previousScreen;
      requestUpdate();
    } else if (opt == 1) {
      // Save & Exit to Title
      savedScreen_ = previousScreen;
      saveGameState();
      stateDirty_ = false;
      screen = Screen::Title;
      titleIndex = 0;
      requestUpdate();
    } else if (opt == 2) {
      // Save & Exit to Applications
      savedScreen_ = previousScreen;
      saveGameState();
      stateDirty_ = false;
      finish();
    } else if (opt == 3) {
      // Abandon Delve
      deleteSaveState();
      state.inRun = false;
      stateDirty_ = false;
      screen = Screen::Title;
      titleIndex = 0;
      requestUpdate();
    }
  }

  void loopDeckView() {
    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      screen = previousScreen;
      requestUpdate();
      return;
    }

    if (cardListTab == CardListTab::Menu) {
      constexpr int kMenuCount = 4;
      if (mappedInput.wasReleased(MappedInputManager::Button::Down) ||
          mappedInput.wasReleased(MappedInputManager::Button::PageForward)) {
        deckMenuIndex = (deckMenuIndex + 1) % kMenuCount;
        requestUpdate();
        return;
      } else if (mappedInput.wasReleased(MappedInputManager::Button::Up) ||
                 mappedInput.wasReleased(MappedInputManager::Button::PageBack)) {
        deckMenuIndex = (deckMenuIndex - 1 + kMenuCount) % kMenuCount;
        requestUpdate();
        return;
      } else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
        selectDeckMenuOption(deckMenuIndex);
        return;
      } else if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
        cardListTab = CardListTab::Relics;
        requestUpdate();
        return;
      } else if (mappedInput.wasReleased(MappedInputManager::Button::Right)) {
        cardListTab = CardListTab::Deck;
        requestUpdate();
        return;
      }
    } else {
      if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
        cardListTab = static_cast<CardListTab>((static_cast<uint8_t>(cardListTab) + 1) % 4);
        cardListPage = 0;
        requestUpdate();
        return;
      }

      // Top-left on X3 (Up / PageBack): previous tab
      if (mappedInput.wasReleased(MappedInputManager::Button::Up)) {
        cardListTab = static_cast<CardListTab>((static_cast<uint8_t>(cardListTab) + 3) % 4);
        cardListPage = 0;
        requestUpdate();
        return;
      }

      // Top-right on X3 (Down / PageForward): next tab
      if (mappedInput.wasReleased(MappedInputManager::Button::Down)) {
        cardListTab = static_cast<CardListTab>((static_cast<uint8_t>(cardListTab) + 1) % 4);
        cardListPage = 0;
        requestUpdate();
        return;
      }

      int total = 0;
      if (cardListTab == CardListTab::Deck) total = static_cast<int>(state.deck.size());
      else if (cardListTab == CardListTab::Discard) total = static_cast<int>(state.discardPile.size());
      else if (cardListTab == CardListTab::Relics) total = static_cast<int>(state.relics.size());
      int maxPage = std::max(0, (total - 1) / 8);

      if (mappedInput.wasReleased(MappedInputManager::Button::Left) ||
          mappedInput.wasReleased(MappedInputManager::Button::PageBack)) {
        if (cardListPage > 0) {
          cardListPage--;
        } else {
          cardListTab = static_cast<CardListTab>((static_cast<uint8_t>(cardListTab) + 3) % 4);
          cardListPage = 0;
        }
        requestUpdate();
        return;
      } else if (mappedInput.wasReleased(MappedInputManager::Button::Right) ||
                 mappedInput.wasReleased(MappedInputManager::Button::PageForward)) {
        if (cardListPage < maxPage) {
          cardListPage++;
        } else {
          cardListTab = static_cast<CardListTab>((static_cast<uint8_t>(cardListTab) + 1) % 4);
          cardListPage = 0;
        }
        requestUpdate();
        return;
      }
    }

    int tx = 0, ty = 0;
    if (mappedInput.wasScreenTouchDown(tx, ty)) {
      const int w = renderer.getScreenWidth();
      const int h = renderer.getScreenHeight();
      const auto& m = UITheme::getInstance().getMetrics();
      int tabsY = m.topPadding + m.headerHeight + 2;

      // Check tab pills touch
      if (ty >= tabsY && ty <= tabsY + 40) {
        constexpr int kTabCount = 4;
        int tabGap = 6;
        int tabW = (w - 36 - (kTabCount - 1) * tabGap) / kTabCount;
        for (int i = 0; i < kTabCount; ++i) {
          int tabX = 18 + i * (tabW + tabGap);
          if (tx >= tabX && tx <= tabX + tabW) {
            cardListTab = static_cast<CardListTab>(i);
            cardListPage = 0;
            deckMenuIndex = 0;
            requestUpdate();
            return;
          }
        }
      }

      // Check menu options touch
      if (cardListTab == CardListTab::Menu) {
        int optW = w - 60;
        int optH = 42;
        int optX = 30;
        int menuY = tabsY + 8 + 28 + 6 + 12 + 10;
        for (int i = 0; i < 4; ++i) {
          int itemY = menuY + i * (optH + 12);
          if (tx >= optX && tx <= optX + optW && ty >= itemY && ty <= itemY + optH) {
            deckMenuIndex = i;
            selectDeckMenuOption(i);
            return;
          }
        }
      }

      // Check bottom bar touch
      if (ty > h - m.buttonHintsHeight - 30) {
        if (tx < w / 4) {
          screen = previousScreen;
          requestUpdate();
          return;
        } else if (tx >= w / 4 && tx < w / 2) {
          if (cardListTab == CardListTab::Menu) {
            selectDeckMenuOption(deckMenuIndex);
          } else {
            cardListTab = static_cast<CardListTab>((static_cast<uint8_t>(cardListTab) + 1) % 4);
            cardListPage = 0;
            requestUpdate();
          }
          return;
        } else if (tx >= w / 2 && tx < 3 * w / 4) {
          if (cardListTab == CardListTab::Menu) {
            deckMenuIndex = (deckMenuIndex - 1 + 4) % 4;
          } else if (cardListPage > 0) {
            cardListPage--;
          } else {
            cardListTab = static_cast<CardListTab>((static_cast<uint8_t>(cardListTab) + 3) % 4);
          }
          requestUpdate();
          return;
        } else {
          if (cardListTab == CardListTab::Menu) {
            deckMenuIndex = (deckMenuIndex + 1) % 4;
          } else {
            const auto& list = (cardListTab == CardListTab::Deck) ? state.deck : state.discardPile;
            int total = static_cast<int>(list.size());
            int maxPage = std::max(0, (total - 1) / 8);
            if (cardListPage < maxPage && cardListTab != CardListTab::Relics) {
              cardListPage++;
            } else {
              cardListTab = static_cast<CardListTab>((static_cast<uint8_t>(cardListTab) + 1) % 4);
              cardListPage = 0;
            }
          }
          requestUpdate();
          return;
        }
      }
    }
  }

  void renderDeckView() {
    const int w = renderer.getScreenWidth();
    const int h = renderer.getScreenHeight();
    const auto& m = UITheme::getInstance().getMetrics();

    char countStr[32];
    if (cardListTab == CardListTab::Deck) {
      snprintf(countStr, sizeof(countStr), "%zu Verses", state.deck.size());
      GUI.drawHeader(renderer, Rect{0, m.topPadding, w, m.headerHeight}, "Grimoire (Deck)", countStr);
    } else if (cardListTab == CardListTab::Discard) {
      snprintf(countStr, sizeof(countStr), "%zu Verses", state.discardPile.size());
      GUI.drawHeader(renderer, Rect{0, m.topPadding, w, m.headerHeight}, "Discard Pile", countStr);
    } else if (cardListTab == CardListTab::Relics) {
      snprintf(countStr, sizeof(countStr), "%zu Relics", state.relics.size());
      GUI.drawHeader(renderer, Rect{0, m.topPadding, w, m.headerHeight}, "Relics Acquired", countStr);
    } else {
      GUI.drawHeader(renderer, Rect{0, m.topPadding, w, m.headerHeight}, "Codex Menu", "Delve Options");
    }

    const int topY = m.topPadding + m.headerHeight + 2;
    const int bottomY = h - m.buttonHintsHeight - 2;
    drawCodexPageFrame(w, h, topY, bottomY);

    int y = topY + 8;

    // Four Tab Pills at top: [ Deck (N) ] [ Discard (N) ] [ Relics (N) ] [ Menu ]
    constexpr int kTabCount = 4;
    int tabGap = 6;
    int tabW = (w - 36 - (kTabCount - 1) * tabGap) / kTabCount;
    int tabH = 28;

    char tabTexts[4][24];
    snprintf(tabTexts[0], sizeof(tabTexts[0]), "Deck (%zu)", state.deck.size());
    snprintf(tabTexts[1], sizeof(tabTexts[1]), "Disc (%zu)", state.discardPile.size());
    snprintf(tabTexts[2], sizeof(tabTexts[2]), "Relics (%zu)", state.relics.size());
    snprintf(tabTexts[3], sizeof(tabTexts[3]), "Menu");

    for (int i = 0; i < kTabCount; ++i) {
      int tabX = 18 + i * (tabW + tabGap);
      bool isSelected = (cardListTab == static_cast<CardListTab>(i));
      if (isSelected) {
        renderer.fillRoundedRect(tabX, y, tabW, tabH, 4, Color::Black);
        renderer.drawText(
            SMALL_FONT_ID, tabX + (tabW - renderer.getTextWidth(SMALL_FONT_ID, tabTexts[i], EpdFontFamily::BOLD)) / 2,
            y + (tabH - renderer.getLineHeight(SMALL_FONT_ID)) / 2, tabTexts[i], false, EpdFontFamily::BOLD);
      } else {
        renderer.drawRoundedRect(tabX, y, tabW, tabH, 1, 4, true);
        renderer.drawText(SMALL_FONT_ID, tabX + (tabW - renderer.getTextWidth(SMALL_FONT_ID, tabTexts[i])) / 2,
                          y + (tabH - renderer.getLineHeight(SMALL_FONT_ID)) / 2, tabTexts[i], true);
      }
    }

    y += tabH + 6;
    drawManuscriptDivider(y, w, 20);
    y += 12;

    if (cardListTab == CardListTab::Deck || cardListTab == CardListTab::Discard) {
      const auto& list = (cardListTab == CardListTab::Deck) ? state.deck : state.discardPile;
      int total = static_cast<int>(list.size());

      if (total == 0) {
        const char* emptyMsg =
            (cardListTab == CardListTab::Deck) ? "(No cards in grimoire)" : "(Discard pile is empty)";
        renderer.drawCenteredText(UI_12_FONT_ID, y + 60, emptyMsg, true);
      } else {
        constexpr int kPerPage = 8;
        int startIdx = cardListPage * kPerPage;
        int endIdx = std::min(total, startIdx + kPerPage);

        for (int i = startIdx; i < endIdx; ++i) {
          uint8_t cId = list[i];
          if (cId < codex::CARD_COUNT) {
            const auto& c = codex::kCardCatalog[cId];
            const char* meterStr =
                (c.meter == codex::Meter::Blade) ? "BLADE" : ((c.meter == codex::Meter::Ward) ? "WARD" : "SCRIPT");

            char titleBuf[64];
            snprintf(titleBuf, sizeof(titleBuf), "[%d Ink] %s   [%s]", c.cost, c.name, meterStr);
            renderer.drawText(SMALL_FONT_ID, 20, y, titleBuf, true, EpdFontFamily::BOLD);
            y += renderer.getLineHeight(SMALL_FONT_ID) + 1;

            char descBuf[128];
            if (c.coupletDesc != nullptr) {
              snprintf(descBuf, sizeof(descBuf), "%s | Couplet: %s", c.desc, c.coupletDesc);
            } else {
              snprintf(descBuf, sizeof(descBuf), "%s", c.desc);
            }
            renderer.drawText(SMALL_FONT_ID, 20, y, descBuf, true);
            y += renderer.getLineHeight(SMALL_FONT_ID) + 4;

            // Subtle hairline between cards
            renderer.drawLine(20, y, w - 20, y, true);
            y += 4;
          }
        }

        int maxPage = (total - 1) / kPerPage;
        if (maxPage > 0) {
          char pageBuf[32];
          snprintf(pageBuf, sizeof(pageBuf), "Page %d / %d", cardListPage + 1, maxPage + 1);
          renderer.drawCenteredText(SMALL_FONT_ID, bottomY - 22, pageBuf, true, EpdFontFamily::BOLD);
        }
      }

      const auto labels = mappedInput.mapLabels(tr(STR_BACK), "Tab >", "< Page", "Page >");
      GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

    } else if (cardListTab == CardListTab::Relics) {
      if (state.relics.empty()) {
        renderer.drawCenteredText(UI_12_FONT_ID, y + 60, "(No relics acquired yet)", true);
      } else {
        int rCount = static_cast<int>(state.relics.size());
        constexpr int kPerPage = 8;
        int startIdx = cardListPage * kPerPage;
        int endIdx = std::min(rCount, startIdx + kPerPage);

        for (int i = startIdx; i < endIdx; ++i) {
          uint8_t rId = state.relics[i];
          const char* rName = "Ancient Artifact";
          const char* rDesc = "A mysterious artifact of the ancient archives.";
          for (const auto& rDef : codex::kRelicCatalog) {
            if (rDef.id == rId) {
              rName = rDef.name;
              rDesc = rDef.desc;
              break;
            }
          }

          // Miniature frame (56x56) and 48x48 relic icon
          drawManuscriptMiniatureFrame(20, y + 4, 56, 56);
          if (rId < codex::RELIC_COUNT) {
            renderer.drawIcon(codex::kRelicSprites[rId], 24, y + 8, 48, 48);
          }

          // Relic Name (Inter 10pt bold)
          renderer.drawText(UI_10_FONT_ID, 84, y + 4, rName, true, EpdFontFamily::BOLD);

          // Relic Description (Inter 8pt regular)
          auto dLines = renderer.wrappedText(SMALL_FONT_ID, rDesc, w - 104, 2);
          int dy = y + 26;
          for (const auto& dl : dLines) {
            renderer.drawText(SMALL_FONT_ID, 84, dy, dl.c_str(), true);
            dy += 18;
          }

          y += 70;
          renderer.drawLine(20, y - 2, w - 20, y - 2, true);
        }

        int maxPage = (rCount - 1) / kPerPage;
        if (maxPage > 0) {
          char pageBuf[32];
          snprintf(pageBuf, sizeof(pageBuf), "Page %d / %d", cardListPage + 1, maxPage + 1);
          renderer.drawCenteredText(SMALL_FONT_ID, bottomY - 22, pageBuf, true, EpdFontFamily::BOLD);
        }
      }

      const auto labels = mappedInput.mapLabels(tr(STR_BACK), "Tab >", "< Page", "Page >");
      GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

    } else {
      // Menu Tab
      static const char* kMenuOptions[] = {
          "Resume Delve",
          "Save & Exit to Title",
          "Save & Exit to Applications",
          "Abandon Delve",
      };
      constexpr int kMenuCount = 4;
      int optW = w - 60;
      int optH = 42;
      int optX = 30;
      int menuY = y + 10;

      for (int i = 0; i < kMenuCount; ++i) {
        int itemY = menuY + i * (optH + 12);
        if (deckMenuIndex == i) {
          renderer.fillRoundedRect(optX, itemY, optW, optH, 4, Color::Black);
          renderer.drawCenteredText(UI_12_FONT_ID, itemY + (optH - renderer.getLineHeight(UI_12_FONT_ID)) / 2,
                                    kMenuOptions[i], false, EpdFontFamily::BOLD);
        } else {
          renderer.drawRoundedRect(optX, itemY, optW, optH, 1, 4, true);
          renderer.drawCenteredText(UI_12_FONT_ID, itemY + (optH - renderer.getLineHeight(UI_12_FONT_ID)) / 2,
                                    kMenuOptions[i], true);
        }
      }

      renderer.drawCenteredText(SMALL_FONT_ID, bottomY - 26, "Delve progress is saved automatically when exiting.",
                                true);

      const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
      GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
    }
  }

  // --- Game Over & Victory Screens ---

  void loopEndScreen() {
    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm) ||
        mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      state.inRun = false;
      deleteSaveState();
      screen = Screen::Title;
      requestUpdate();
    }
  }

  void renderGameOver() {
    const int w = renderer.getScreenWidth();
    const int h = renderer.getScreenHeight();
    const auto& m = UITheme::getInstance().getMetrics();

    GUI.drawHeader(renderer, Rect{0, m.topPadding, w, m.headerHeight}, "Fallen Scribe");

    const int topY = m.topPadding + m.headerHeight + 2;
    const int bottomY = h - m.buttonHintsHeight - 2;
    drawCodexPageFrame(w, h, topY, bottomY);

    int y = h / 3 - 30;
    drawManuscriptDivider(y, w, 32);
    y += 24;

    renderer.drawCenteredText(UI_12_FONT_ID, y, "YOUR INK HAS DRIED", true, EpdFontFamily::BOLD);
    y += renderer.getLineHeight(UI_12_FONT_ID) + 10;
    renderer.drawCenteredText(SMALL_FONT_ID, y, "The marginalia consumes your soul.", true);
    y += renderer.getLineHeight(SMALL_FONT_ID) + 12;

    char fStr[48];
    snprintf(fStr, sizeof(fStr), "Fell on Chapter 1 • Page %d / 15", state.floor);
    renderer.drawCenteredText(UI_12_FONT_ID, y, fStr, true, EpdFontFamily::BOLD);
    y += renderer.getLineHeight(UI_12_FONT_ID) + 16;

    drawManuscriptDivider(y, w, 32);

    renderer.drawCenteredText(SMALL_FONT_ID, bottomY - 24, "Press Confirm to return to Menu", true);

    const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), nullptr, nullptr);
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  }

  void renderVictory() {
    const int w = renderer.getScreenWidth();
    const int h = renderer.getScreenHeight();
    const auto& m = UITheme::getInstance().getMetrics();

    GUI.drawHeader(renderer, Rect{0, m.topPadding, w, m.headerHeight}, "Triumph of the Scribe",
                   "Chapter 1 Completed");

    const int topY = m.topPadding + m.headerHeight + 2;
    const int bottomY = h - m.buttonHintsHeight - 2;
    drawCodexPageFrame(w, h, topY, bottomY);

    int midX = w / 2;
    int y = topY + 24;

    // Woodcut miniature plate of the purified codex
    const int plateW = 106;
    const int plateH = 106;
    const int plateX = (w - plateW) / 2;
    drawManuscriptMiniatureFrame(plateX, y, plateW, plateH);
    renderer.drawIcon(codex::SpriteCodexLarge, plateX + 5, y + 5, 96, 96);
    y += plateH + 16;

    drawManuscriptDivider(y, w, 28);
    y += 20;

    renderer.drawCenteredText(UI_12_FONT_ID, y, "THE CODEX IS PURIFIED", true, EpdFontFamily::BOLD);
    y += renderer.getLineHeight(UI_12_FONT_ID) + 10;
    renderer.drawCenteredText(SMALL_FONT_ID, y, "The evil of Chapter 1 has been bound and sealed.", true);
    y += renderer.getLineHeight(SMALL_FONT_ID) + 8;
    renderer.drawCenteredText(SMALL_FONT_ID, y, "You have inscribed your name into eternity.", true,
                              EpdFontFamily::BOLD);
    y += renderer.getLineHeight(SMALL_FONT_ID) + 16;

    drawManuscriptDivider(y, w, 28);

    renderer.drawCenteredText(SMALL_FONT_ID, bottomY - 24, "Press Confirm to return to Menu", true);

    const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), nullptr, nullptr);
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  }
};
