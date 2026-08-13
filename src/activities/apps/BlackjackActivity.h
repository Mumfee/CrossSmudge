#pragma once

#include "activities/Activity.h"
#include "components/UITheme.h"
#include "FsHelpers.h"
#include <Serialization.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

#include "assets/clubs.h"
#include "assets/diamonds.h"
#include "assets/hearts.h"
#include "assets/spades.h"

class BlackjackActivity : public Activity {
public:
  BlackjackActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Blackjack", renderer, mappedInput) {}

  ~BlackjackActivity() override {
    saveGameState();
  }

  void onEnter() override {
    Activity::onEnter();
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    if (!loadGameState()) {
      bankroll = 50;
      currentBet = 5;
      chipIndex = 1; // Default to $5 chip
      initShoe();
    }
    requestUpdate();
  }

  void loop() override {
    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      saveGameState();
      finish();
      return;
    }

    if (gameState == Phase::BETTING) {
      if (mappedInput.wasReleased(MappedInputManager::Button::Up)) {
        chipIndex = (chipIndex + 1) % kChipCount;
        requestUpdate();
      } else if (mappedInput.wasReleased(MappedInputManager::Button::Down)) {
        chipIndex = (chipIndex - 1 + kChipCount) % kChipCount;
        requestUpdate();
      } else if (mappedInput.wasReleased(MappedInputManager::Button::Right)) {
        int chipVal = kChipValues[chipIndex];
        if (currentBet + chipVal <= bankroll) {
          currentBet += chipVal;
          requestUpdate();
        }
      } else if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
        int chipVal = kChipValues[chipIndex];
        if (currentBet - chipVal >= 1) {
          currentBet -= chipVal;
          requestUpdate();
        }
      } else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
        if (currentBet > 0 && currentBet <= bankroll) {
          startHand();
          requestUpdate();
        }
      }
      return;
    }

    if (gameState == Phase::INSURANCE_PROMPT) {
      if (mappedInput.wasReleased(MappedInputManager::Button::Left) || 
          mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
        int insCost = currentBet / 2;
        if (insCost <= bankroll) {
          insuranceBet = insCost;
          bankroll -= insCost;
        }
        resolveInsuranceAndStartPlayerTurn();
        requestUpdate();
      } else if (mappedInput.wasReleased(MappedInputManager::Button::Right)) {
        insuranceBet = 0;
        resolveInsuranceAndStartPlayerTurn();
        requestUpdate();
      }
      return;
    }

    if (gameState == Phase::PLAYER_TURN) {
      bool canDouble = (playerHands[activeHandIdx].cards.size() == 2 && bankroll >= currentBet);
      bool canSplit = (playerHands[activeHandIdx].cards.size() == 2 &&
                       cardValue(playerHands[activeHandIdx].cards[0]) == cardValue(playerHands[activeHandIdx].cards[1]) &&
                       bankroll >= currentBet && playerHands.size() < 4);

      if (canDouble && (mappedInput.wasReleased(MappedInputManager::Button::Up) ||
                        mappedInput.wasReleased(MappedInputManager::Button::Down))) {
        bankroll -= currentBet;
        playerHands[activeHandIdx].bet *= 2;
        dealCardToHand(playerHands[activeHandIdx]);
        
        if (getHandScore(playerHands[activeHandIdx]) > 21) {
          playerHands[activeHandIdx].busted = true;
        }
        advancePlayerTurn();
        requestUpdate();
        return;
      }

      if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
        dealCardToHand(playerHands[activeHandIdx]);
        if (getHandScore(playerHands[activeHandIdx]) >= 21) {
          if (getHandScore(playerHands[activeHandIdx]) > 21) {
            playerHands[activeHandIdx].busted = true;
          }
          advancePlayerTurn();
        }
        requestUpdate();
        return;
      }

      if (mappedInput.wasReleased(MappedInputManager::Button::Right)) {
        advancePlayerTurn();
        requestUpdate();
        return;
      }

      if (canSplit && mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
        performSplit();
        requestUpdate();
        return;
      }
      return;
    }

    if (gameState == Phase::ROUND_OVER) {
      if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
        reviewHandIdx++;
        
        if (reviewHandIdx < playerHands.size()) {
          requestUpdate();
          return;
        }

        if (bankroll <= 0) {
          bankroll = 50;
        }
        currentBet = std::min(currentBet, bankroll);
        if (currentBet <= 0) currentBet = 1;
        
        if (needReshuffle) {
          initShoe();
        }
        
        gameState = Phase::BETTING;
        statusMessage = "";
        requestUpdate();
      }
      return;
    }
  }

  void render(RenderLock&& lock) override {
    renderer.clearScreen();

    const int pageWidth = renderer.getScreenWidth();
    const int pageHeight = renderer.getScreenHeight();

    const auto& metrics = UITheme::getInstance().getMetrics();
    const auto font = SETTINGS.getReaderFontId();
    const int textHeight = renderer.getLineHeight(font);

    // 1. HEADER
    char headerRight[32];
    snprintf(headerRight, sizeof(headerRight), "Bank: $%d", bankroll);
    GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, "Blackjack", headerRight);

    const int topBound = metrics.topPadding + metrics.headerHeight;
    const int bottomBound = pageHeight - metrics.buttonHintsHeight;
    const int availableHeight = bottomBound - topBound;

    // Reshuffle Banner
    if (needReshuffle) {
      renderer.drawCenteredText(font, topBound + availableHeight/2 - textHeight/2, "Cut card reached!");
      renderer.drawCenteredText(font, topBound + availableHeight/2 + textHeight/2, "Reshuffling next round.");
    }

    // ----------------------------------------------------
    // 2. DEALER SECTION
    // ----------------------------------------------------
    int dealerScore = getHandScore(dealerCards);
    bool hasHiddenCard = (dealerCards.size() >= 2 && (gameState == Phase::PLAYER_TURN || gameState == Phase::INSURANCE_PROMPT));

    char dealerHeader[64];
    if (dealerCards.empty()) {
      snprintf(dealerHeader, sizeof(dealerHeader), "Dealer");
    } else if (hasHiddenCard) {
      snprintf(dealerHeader, sizeof(dealerHeader), "Dealer (?)");
    } else if (dealerScore > 21) {
      snprintf(dealerHeader, sizeof(dealerHeader), "Dealer (%d) [BUST]", dealerScore);
    } else {
      snprintf(dealerHeader, sizeof(dealerHeader), "Dealer (%d)", dealerScore);
    }

    int dealerTextY = topBound + static_cast<int>(availableHeight * 0.2);
    renderer.drawCenteredText(font, dealerTextY, dealerHeader);

    int dealerCardY = dealerTextY + textHeight + 8;

    if (dealerCards.empty()) {
      int handWidth = (2 * kCardWidth) + kCardGap;
      int startX = (pageWidth - handWidth) / 2;
      drawEmptyCardSlot(startX, dealerCardY);
      drawEmptyCardSlot(startX + kCardWidth + kCardGap, dealerCardY);
    } else {
      int numCards = static_cast<int>(dealerCards.size());
      int handWidth = numCards * kCardWidth + (numCards - 1) * kCardGap;
      int startX = (pageWidth - handWidth) / 2;

      for (size_t i = 0; i < dealerCards.size(); ++i) {
        int cardX = startX + static_cast<int>(i) * (kCardWidth + kCardGap);
        bool isFaceDown = (i == 1 && hasHiddenCard);
        drawCardShape(cardX, dealerCardY, dealerCards[i], isFaceDown);
      }
    }

    // ----------------------------------------------------
    // 3. PLAYER SECTION
    // ----------------------------------------------------
    int playerCardY = topBound + static_cast<int>(availableHeight * 0.5);

    char playerHeader[128] = "Player";
    if (!playerHands.empty()) {
      if (gameState == Phase::PLAYER_TURN || gameState == Phase::INSURANCE_PROMPT) {
        size_t h = activeHandIdx;
        int score = getHandScore(playerHands[h]);
        if (playerHands.size() > 1) {
          snprintf(playerHeader, sizeof(playerHeader), "Hand %d of %d (%d)%s",
                   static_cast<int>(h + 1), static_cast<int>(playerHands.size()), score,
                   playerHands[h].busted ? " [BUST]" : "");
        } else {
          snprintf(playerHeader, sizeof(playerHeader), "Player (%d)%s", score,
                   playerHands[h].busted ? " [BUST]" : "");
        }
      } else { // ROUND_OVER View
        if (playerHands.size() == 1) {
          int score = getHandScore(playerHands[0]);
          if (playerHands[0].busted) {
            snprintf(playerHeader, sizeof(playerHeader), "Player (%d) [BUST]", score);
          } else if (score == 21 && playerHands[0].cards.size() == 2) {
            snprintf(playerHeader, sizeof(playerHeader), "Player (%d) [BLACKJACK!]", score);
          } else {
            snprintf(playerHeader, sizeof(playerHeader), "Player (%d)", score);
          }
        } else {
          playerHeader[0] = '\0';
          for (size_t h = 0; h < playerHands.size(); ++h) {
            int score = getHandScore(playerHands[h]);
            char handBuf[32];
            snprintf(handBuf, sizeof(handBuf), "H%d: %d%s  ", static_cast<int>(h + 1), score, playerHands[h].busted ? "(Bust)" : "");
            strcat(playerHeader, handBuf);
          }
        }
      }
    }

    if (playerHands.empty()) {
      int handWidth = (2 * kCardWidth) + kCardGap;
      int startX = (pageWidth - handWidth) / 2;

      drawEmptyCardSlot(startX, playerCardY);
      drawEmptyCardSlot(startX + kCardWidth + kCardGap, playerCardY);
    } else {
      size_t h = (gameState == Phase::PLAYER_TURN || gameState == Phase::INSURANCE_PROMPT) ? activeHandIdx : 0;
      int numCards = static_cast<int>(playerHands[h].cards.size());
      int handWidth = numCards * kCardWidth + (numCards - 1) * kCardGap;
      int startX = (pageWidth - handWidth) / 2;

      for (size_t c = 0; c < playerHands[h].cards.size(); ++c) {
        drawCardShape(startX + static_cast<int>(c) * (kCardWidth + kCardGap), playerCardY, playerHands[h].cards[c], false);
      }
    }

    renderer.drawCenteredText(font, playerCardY + kCardHeight + 8, playerHeader);

    // ----------------------------------------------------
    // 4. STATUS & BET READOUT
    // ----------------------------------------------------
    int statusY = bottomBound - textHeight * 2;

    if (gameState == Phase::BETTING) {
      char betBuf[64];
      snprintf(betBuf, sizeof(betBuf), "Bet: $%d   (Chip: $%d)", currentBet, kChipValues[chipIndex]);
      renderer.drawCenteredText(font, statusY, betBuf);
    } else if (gameState == Phase::ROUND_OVER) {
      size_t h = reviewHandIdx;
      if (h < playerHands.size()) {
        renderer.drawCenteredText(font, statusY, playerHands[h].resultText.c_str());
      } else if (!statusMessage.empty()) {
        renderer.drawCenteredText(font, statusY, statusMessage.c_str());
      }
    } else if (!statusMessage.empty()) {
      renderer.drawCenteredText(font, statusY, statusMessage.c_str());
    }

    // ----------------------------------------------------
    // 5. FOOTER HINTS
    // ----------------------------------------------------
    if (gameState == Phase::BETTING) {
      const auto labels = mappedInput.mapLabels(tr(STR_BACK), "Deal", "- Bet", "+ Bet");
      GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
    } else if (gameState == Phase::INSURANCE_PROMPT) {
      const auto labels = mappedInput.mapLabels("Insure", "No Ins", "Insure", "No Ins");
      GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
    } else if (gameState == Phase::PLAYER_TURN) {
      bool canDouble = (playerHands[activeHandIdx].cards.size() == 2 && bankroll >= currentBet);
      bool canSplit = (playerHands[activeHandIdx].cards.size() == 2 &&
                       cardValue(playerHands[activeHandIdx].cards[0]) == cardValue(playerHands[activeHandIdx].cards[1]) &&
                       bankroll >= currentBet && playerHands.size() < 4);

      std::string btn2Label = canSplit ? "Split" : "";
      const auto labels = mappedInput.mapLabels(tr(STR_BACK), btn2Label.c_str(), "Hit", "Stand");
      GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
    } else { // ROUND_OVER
      const auto labels = mappedInput.mapLabels(tr(STR_BACK), "Next", "", "");
      GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
    }

    renderer.displayBuffer();
  }

private:
  enum class Suit { CLUBS, DIAMONDS, HEARTS, SPADES };
  struct Card {
    int rank; // 1 = Ace, 2-10 = Value, 11 = Jack, 12 = Queen, 13 = King
    Suit suit;
  };

  struct Hand {
    std::vector<Card> cards;
    int bet = 0;
    bool busted = false;
    bool stood = false;
    int payout = 0;
    std::string resultText = "";
  };

  enum class Phase { BETTING, INSURANCE_PROMPT, PLAYER_TURN, DEALER_TURN, ROUND_OVER };

  static constexpr int kCardWidth = 64;
  static constexpr int kCardHeight = 90;
  static constexpr int kCardCornerRadius = 6;
  static constexpr int kCardGap = 8;

  static constexpr int kChipCount = 6;
  const int kChipValues[kChipCount] = {1, 5, 10, 25, 50, 100};
  int chipIndex = 1;

  int bankroll = 50;
  int currentBet = 5;
  int insuranceBet = 0;

  std::vector<Card> shoe;
  size_t shoeIndex = 0;
  size_t cutCardIndex = 0;
  bool needReshuffle = false;

  std::vector<Hand> playerHands;
  size_t activeHandIdx = 0;
  size_t reviewHandIdx = 0;
  std::vector<Card> dealerCards;

  Phase gameState = Phase::BETTING;
  std::string statusMessage = "";

  static constexpr uint32_t GAME_MAGIC = 0x424C4B4A; // 'BLK' 'J'
  static constexpr char stateFile[] = "/.smudge/blackjack/state.bin";

  void drawSuitIcon(int x, int y, Suit suit) {
    const uint8_t* iconData = nullptr;
    bool isRed = (suit == Suit::HEARTS || suit == Suit::DIAMONDS);

    
    switch (suit) {
      case Suit::CLUBS:    iconData = ClubsIcon; break;
      case Suit::DIAMONDS: iconData = DiamondsIcon; break;
      case Suit::HEARTS:   iconData = HeartsIcon; break;
      case Suit::SPADES:   iconData = SpadesIcon; break;
    }

    if (!iconData) return;

    // Bit-level parser for 32x32 inverted bitmap (0 = icon pixel, 1 = background)
    for (int cy = 0; cy < 32; ++cy) {
      for (int cx = 0; cx < 32; ++cx) {
        int byteIdx = (cy * 4) + (cx / 8);
        int bitIdx = 7 - (cx % 8);
        bool isIconPixel = ((iconData[byteIdx] >> bitIdx) & 1) == 0;

        if (isIconPixel) {
          if (isRed) {
            renderer.fillRectDither(x + cx, y + cy, 1, 1, Color::LightGray);
          } else {
            renderer.drawPixel(x + cx, y + cy);
          }
        }
      }
    }
  }

  void drawCardShape(int x, int y, const Card& card, bool faceDown) {
    const auto font = SETTINGS.getReaderFontId();

    renderer.drawRoundedRect(x, y, kCardWidth, kCardHeight, 1, kCardCornerRadius, true);

    if (faceDown) {
      renderer.fillRectDither(x + 4, y + 4, kCardWidth - 8, kCardHeight - 8, Color::LightGray);
      return;
    }

    // Rank text (Top Left)
    std::string rankStr;
    if (card.rank == 1) rankStr = "A";
    else if (card.rank == 11) rankStr = "J";
    else if (card.rank == 12) rankStr = "Q";
    else if (card.rank == 13) rankStr = "K";
    else rankStr = std::to_string(card.rank);

    renderer.drawText(font, x + 6, y + 6, rankStr.c_str());

    // Suit Icon (32x32) centered inside the card
    int iconX = x + (kCardWidth - 32) / 2;
    int iconY = y + (kCardHeight / 2) - 2;

    drawSuitIcon(iconX, iconY, card.suit);
  }

  void drawEmptyCardSlot(int x, int y) {
    renderer.drawRoundedRect(x, y, kCardWidth, kCardHeight, 1, kCardCornerRadius, true);
  }

  void initShoe() {
    shoe.clear();
    for (int d = 0; d < 8; ++d) {
      for (int s = 0; s < 4; ++s) {
        for (int r = 1; r <= 13; ++r) {
          shoe.push_back({r, static_cast<Suit>(s)});
        }
      }
    }

    for (size_t i = shoe.size() - 1; i > 0; --i) {
      size_t j = rand() % (i + 1);
      std::swap(shoe[i], shoe[j]);
    }

    shoeIndex = 0;
    size_t minCut = static_cast<size_t>(shoe.size() * 0.60f);
    size_t maxCut = static_cast<size_t>(shoe.size() * 0.80f);
    cutCardIndex = minCut + (rand() % (maxCut - minCut));
    needReshuffle = false;
  }

  Card drawCard() {
    if (shoeIndex >= shoe.size()) {
      initShoe();
    }
    if (shoeIndex >= cutCardIndex) {
      needReshuffle = true;
    }
    return shoe[shoeIndex++];
  }

  int cardValue(const Card& c) const {
    if (c.rank > 10) return 10;
    if (c.rank == 1) return 11;
    return c.rank;
  }

  int getHandScore(const std::vector<Card>& cards) const {
    int total = 0;
    int aces = 0;
    for (const auto& c : cards) {
      if (c.rank == 1) {
        total += 11;
        aces++;
      } else if (c.rank > 10) {
        total += 10;
      } else {
        total += c.rank;
      }
    }
    while (total > 21 && aces > 0) {
      total -= 10;
      aces--;
    }
    return total;
  }

  int getHandScore(const Hand& hand) const {
    return getHandScore(hand.cards);
  }

  void dealCardToHand(Hand& hand) {
    hand.cards.push_back(drawCard());
  }

  void startHand() {
    bankroll -= currentBet;
    insuranceBet = 0;
    playerHands.clear();
    dealerCards.clear();

    Hand initialHand;
    initialHand.bet = currentBet;
    playerHands.push_back(initialHand);
    activeHandIdx = 0;
    reviewHandIdx = 0;

    dealCardToHand(playerHands[0]);
    dealerCards.push_back(drawCard());
    dealCardToHand(playerHands[0]);
    dealerCards.push_back(drawCard());

    if (dealerCards[0].rank == 1) {
      gameState = Phase::INSURANCE_PROMPT;
      statusMessage = "Dealer shows Ace. Buy Insurance?";
    } else {
      resolveInsuranceAndStartPlayerTurn();
    }
  }

  void resolveInsuranceAndStartPlayerTurn() {
    gameState = Phase::PLAYER_TURN;
    statusMessage = "";

    bool playerBJ = (getHandScore(playerHands[0]) == 21);
    bool dealerBJ = (getHandScore(dealerCards) == 21);

    if (playerBJ || dealerBJ) {
      resolveRoundEnd();
    }
  }

  void performSplit() {
    bankroll -= currentBet;

    Hand secondHand;
    secondHand.bet = currentBet;
    secondHand.cards.push_back(playerHands[activeHandIdx].cards.back());
    playerHands[activeHandIdx].cards.pop_back();

    dealCardToHand(playerHands[activeHandIdx]);
    dealCardToHand(secondHand);

    playerHands.insert(playerHands.begin() + activeHandIdx + 1, secondHand);
  }

  void advancePlayerTurn() {
    activeHandIdx++;
    if (activeHandIdx >= playerHands.size()) {
      playDealerTurnAndFinish();
    }
  }

  void playDealerTurnAndFinish() {
    gameState = Phase::DEALER_TURN;

    bool allBusted = true;
    for (const auto& h : playerHands) {
      if (!h.busted) allBusted = false;
    }

    if (!allBusted) {
      while (getHandScore(dealerCards) < 17) {
        dealerCards.push_back(drawCard());
      }
    }

    resolveRoundEnd();
  }

  void resolveRoundEnd() {
    gameState = Phase::ROUND_OVER;
    reviewHandIdx = 0;

    int dealerScore = getHandScore(dealerCards);
    bool dealerBust = dealerScore > 21;
    bool dealerBJ = (dealerCards.size() == 2 && dealerScore == 21);

    int totalPayout = 0;

    if (insuranceBet > 0 && dealerBJ) {
      totalPayout += insuranceBet * 3;
    }

    for (size_t i = 0; i < playerHands.size(); ++i) {
      auto& h = playerHands[i];
      int pScore = getHandScore(h);
      bool pBJ = (h.cards.size() == 2 && pScore == 21 && playerHands.size() == 1);

      if (h.busted) {
        h.payout = 0;
        h.resultText = "BUST (-" + std::to_string(h.bet) + " chips)";
      } else if (pBJ) {
        if (dealerBJ) {
          h.payout = h.bet; // Push
          h.resultText = "PUSH (Tie)";
        } else {
          h.payout = h.bet + (h.bet * 3 / 2); // BJ 3:2
          h.resultText = "BLACKJACK! (+" + std::to_string(h.payout - h.bet) + " chips)";
        }
      } else if (dealerBJ) {
        h.payout = 0;
        h.resultText = "DEALER BLACKJACK (-" + std::to_string(h.bet) + " chips)";
      } else if (dealerBust || pScore > dealerScore) {
        h.payout = h.bet * 2; // Win
        h.resultText = "WIN! (+" + std::to_string(h.bet) + " chips)";
      } else if (pScore == dealerScore) {
        h.payout = h.bet; // Push
        h.resultText = "PUSH (Tie)";
      } else {
        h.payout = 0;
        h.resultText = "DEALER WINS (-" + std::to_string(h.bet) + " chips)";
      }

      totalPayout += h.payout;
    }

    bankroll += totalPayout;
  }

  void ensureDirectoriesExist() {
    if (!Storage.exists("/.smudge")) {
      Storage.mkdir("/.smudge");
    }
    if (!Storage.exists("/.smudge/blackjack")) {
      Storage.mkdir("/.smudge/blackjack");
    }
  }

  void saveGameState() {
    ensureDirectoriesExist();

    HalFile file;
    if (!Storage.openFileForWrite("Blackjack", stateFile, file)) {
      return;
    }

    serialization::writePod(file, GAME_MAGIC);
    serialization::writePod(file, static_cast<int32_t>(bankroll));
    serialization::writePod(file, static_cast<int32_t>(currentBet));
    serialization::writePod(file, static_cast<int32_t>(chipIndex));
    serialization::writePod(file, static_cast<uint32_t>(shoeIndex));
    serialization::writePod(file, static_cast<uint32_t>(cutCardIndex));
    serialization::writePod(file, static_cast<uint8_t>(needReshuffle ? 1 : 0));

    file.close();
  }

  bool loadGameState() {
    if (!Storage.exists(stateFile)) {
      return false;
    }

    HalFile file;
    if (!Storage.openFileForRead("Blackjack", stateFile, file)) {
      return false;
    }

    uint32_t magic = 0;
    if (!serialization::tryReadPod(file, magic) || magic != GAME_MAGIC) {
      file.close();
      Storage.remove(stateFile);
      return false;
    }

    int32_t rawBank = 50, rawBet = 5, rawChip = 1;
    uint32_t rawShoeIdx = 0, rawCutIdx = 0;
    uint8_t rawReshuffle = 0;

    if (!serialization::tryReadPod(file, rawBank) ||
        !serialization::tryReadPod(file, rawBet) ||
        !serialization::tryReadPod(file, rawChip) ||
        !serialization::tryReadPod(file, rawShoeIdx) ||
        !serialization::tryReadPod(file, rawCutIdx) ||
        !serialization::tryReadPod(file, rawReshuffle)) {
      file.close();
      Storage.remove(stateFile);
      return false;
    }

    bankroll = static_cast<int>(rawBank);
    currentBet = static_cast<int>(rawBet);
    chipIndex = static_cast<int>(rawChip);
    shoeIndex = static_cast<size_t>(rawShoeIdx);
    cutCardIndex = static_cast<size_t>(rawCutIdx);
    needReshuffle = (rawReshuffle != 0);

    file.close();

    if (shoe.empty() || shoeIndex >= shoe.size()) {
      initShoe();
    }

    return true;
  }

  void deleteSaveState() {
    if (Storage.exists(stateFile)) {
      Storage.remove(stateFile);
    }
  }
};