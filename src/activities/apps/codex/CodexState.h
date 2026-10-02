#pragma once
#include <algorithm>
#include <random>
#include <vector>

#include "CodexCards.h"
#include "CodexMonsters.h"
#include "CodexTypes.h"

namespace codex {

struct RunState {
  int16_t playerHp = 50;
  int16_t playerMaxHp = 50;
  int16_t gold = 50;
  uint8_t floor = 1;
  uint8_t maxFloor = 15;
  bool inRun = false;

  int16_t bonusWard = 0;
  int8_t permStrength = 0;
  int8_t baseMaxInk = 3;

  std::vector<uint8_t> deck;
  std::vector<uint8_t> relics;

  // Combat runtime state
  int8_t ink = 3;
  int8_t maxInk = 3;
  int16_t playerWard = 0;
  Meter lastPlayedMeter = Meter::None;
  StatusEffects playerStatus;
  EnemyState enemy;

  std::vector<uint8_t> drawPile;
  std::vector<uint8_t> hand;
  std::vector<uint8_t> discardPile;

  // Rewards
  std::vector<uint8_t> cardRewards;

  // UI / Combat feedback
  std::string combatLog = "Encounter begins!";
  int selectedHandIndex = 0;

  void initNewRun() {
    playerHp = 50;
    playerMaxHp = 50;
    gold = 50;
    floor = 1;
    maxFloor = 15;
    inRun = true;
    bonusWard = 0;
    permStrength = 0;
    baseMaxInk = 3;
    relics.clear();
    relics.push_back(0);  // Silver Quill: +1 Ink on turn 1

    // Starter deck: 4 Strikes, 4 Defends, 1 Blot Out, 1 Pommel
    deck = {CARD_STRIKE, CARD_STRIKE, CARD_STRIKE, CARD_STRIKE, CARD_DEFEND,
            CARD_DEFEND, CARD_DEFEND, CARD_DEFEND, CARD_BLOT,   CARD_POMMEL};
  }

  bool hasRelic(uint8_t id) const {
    for (uint8_t r : relics) {
      if (r == id) return true;
    }
    return false;
  }

  int8_t giveRandomRelic() {
    std::vector<uint8_t> unowned;
    for (uint8_t r = 0; r < RELIC_COUNT; ++r) {
      if (!hasRelic(r)) unowned.push_back(r);
    }
    if (unowned.empty()) {
      gold += 35;
      return -1;
    }
    std::random_device rd;
    std::mt19937 g(rd());
    std::uniform_int_distribution<size_t> dist(0, unowned.size() - 1);
    uint8_t id = unowned[dist(g)];
    relics.push_back(id);
    return static_cast<int8_t>(id);
  }

  bool giveRelic(uint8_t id) {
    if (!hasRelic(id)) {
      relics.push_back(id);
      return true;
    }
    gold += 25;
    return false;
  }

  void startCombat(bool isElite = false, bool isBoss = false) {
    enemy = spawnMonster(floor, isElite, isBoss);
    playerWard = (hasRelic(1) ? 6 : 0) + bonusWard;
    bonusWard = 0;
    maxInk = baseMaxInk;
    ink = (hasRelic(0) ? 1 : 0) + maxInk;
    lastPlayedMeter = Meter::None;
    playerStatus = {};
    playerStatus.strength = permStrength;
    playerStatus.combatTurnCount = 1;
    playerStatus.scriptCardsPlayedThisTurn = 0;

    drawPile = deck;
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(drawPile.begin(), drawPile.end(), g);

    hand.clear();
    discardPile.clear();

    // Draw initial hand
    int drawCount = hasRelic(3) ? 5 : 4;  // Scholar's Ring
    drawCards(drawCount);

    combatLog = std::string("Faced with ") + enemy.name + "!";
    selectedHandIndex = 0;
  }

  void drawCards(int count) {
    for (int i = 0; i < count; ++i) {
      if (drawPile.empty()) {
        if (discardPile.empty()) break;
        drawPile = discardPile;
        discardPile.clear();
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(drawPile.begin(), drawPile.end(), g);
      }
      if (!drawPile.empty()) {
        hand.push_back(drawPile.back());
        drawPile.pop_back();
      }
    }
  }

  void startPlayerTurn() {
    ink = maxInk;
    lastPlayedMeter = Meter::None;

    // Ward degrades by 50% unless RetainWard power or Monk's Rosary is active
    if (!playerStatus.retainWard && !hasRelic(5)) {
      playerWard /= 2;
    }

    // Tick down player statuses
    if (playerStatus.weak > 0) playerStatus.weak--;
    if (playerStatus.vulnerable > 0) playerStatus.vulnerable--;

    playerStatus.combatTurnCount++;
    playerStatus.scriptCardsPlayedThisTurn = 0;

    // Relic 10: Censer of Cleansing - Remove Weak and Vulnerable at start of turn 2
    if (hasRelic(10) && playerStatus.combatTurnCount == 2) {
      playerStatus.weak = 0;
      playerStatus.vulnerable = 0;
      combatLog = "Censer of Cleansing dispels debuffs!";
    }

    // Flow power gives +1 card draw
    int toDraw = 4 + playerStatus.extraDraw;
    // Relic 9: Hourglass of Sand - Every 3 turns, draw 2 additional cards
    if (hasRelic(9) && (playerStatus.combatTurnCount % 3 == 0)) {
      toDraw += 2;
    }
    drawCards(toDraw);
    selectedHandIndex = std::min(selectedHandIndex, std::max(0, static_cast<int>(hand.size()) - 1));
  }

  void endPlayerTurn() {
    // Discard unplayed cards
    for (uint8_t c : hand) {
      discardPile.push_back(c);
    }
    hand.clear();

    // --- ENEMY ACTION ---
    executeEnemyTurn();

    // Check if player died or enemy died
    if (playerHp > 0 && enemy.hp > 0) {
      decideNextIntent(enemy, floor);
      startPlayerTurn();
    }
  }

  void executeEnemyTurn() {
    // Enemy Ward degrades by 50%
    if (!enemy.status.retainWard) {
      enemy.ward /= 2;
    }

    // Apply poison damage to enemy
    if (enemy.status.poison > 0) {
      int pDmg = enemy.status.poison;
      enemy.hp -= pDmg;
      enemy.status.poison--;
      combatLog = std::string(enemy.name) + " suffers " + std::to_string(pDmg) + " Poison!";
      if (enemy.hp <= 0) return;
    }

    // Execute current intent
    switch (enemy.intent) {
      case IntentType::Attack: {
        int dmg = enemy.intentValue;
        if (enemy.status.strength > 0) dmg += enemy.status.strength;
        if (enemy.status.weak > 0) dmg = std::max(1, (dmg * 3) / 4);
        if (playerStatus.vulnerable > 0) dmg = (dmg * 3) / 2;

        int blocked = std::min(playerWard, static_cast<int16_t>(dmg));
        playerWard -= blocked;
        int unblocked = dmg - blocked;
        playerHp -= unblocked;

        combatLog = std::string(enemy.name) + " attacks for " + std::to_string(dmg) + " (" + std::to_string(blocked) +
                    " blocked)!";

        // Relic 8: Barbed Bookmark - When an enemy hits your Ward, deal 3 damage back
        if (blocked > 0 && hasRelic(8)) {
          int bDmg = 3;
          int bBlocked = std::min(enemy.ward, static_cast<int16_t>(bDmg));
          enemy.ward -= bBlocked;
          enemy.hp -= (bDmg - bBlocked);
          combatLog += " Barbed Bookmark reflects 3 dmg!";
        }
        break;
      }
      case IntentType::Defend: {
        enemy.ward += enemy.intentValue;
        combatLog = std::string(enemy.name) + " fortifies for " + std::to_string(enemy.intentValue) + " Ward!";
        break;
      }
      case IntentType::AttackAndDefend: {
        int dmg = enemy.intentValue;
        if (enemy.status.strength > 0) dmg += enemy.status.strength;
        int blocked = std::min(playerWard, static_cast<int16_t>(dmg));
        playerWard -= blocked;
        playerHp -= (dmg - blocked);
        enemy.ward += enemy.intentExtra;

        combatLog = std::string(enemy.name) + " strikes for " + std::to_string(dmg) + " and gains " +
                    std::to_string(enemy.intentExtra) + " Ward!";

        // Relic 8: Barbed Bookmark
        if (blocked > 0 && hasRelic(8)) {
          int bDmg = 3;
          int bBlocked = std::min(enemy.ward, static_cast<int16_t>(bDmg));
          enemy.ward -= bBlocked;
          enemy.hp -= (bDmg - bBlocked);
          combatLog += " Barbed Bookmark reflects 3 dmg!";
        }
        break;
      }
      case IntentType::BuffStrength: {
        enemy.status.strength += enemy.intentValue;
        combatLog = std::string(enemy.name) + " enters a frenzy (+ " + std::to_string(enemy.intentValue) + " Str)!";
        break;
      }
      case IntentType::DebuffPlayer: {
        playerStatus.weak += enemy.intentValue;
        combatLog = std::string(enemy.name) + " casts an enervating hex (Weakened)!";
        break;
      }
    }

    // Tick down enemy weak / vulnerable
    if (enemy.status.weak > 0) enemy.status.weak--;
    if (enemy.status.vulnerable > 0) enemy.status.vulnerable--;
  }

  bool playCard(int handIndex) {
    if (handIndex < 0 || handIndex >= static_cast<int>(hand.size())) return false;
    uint8_t cardId = hand[handIndex];
    if (cardId >= CARD_COUNT) return false;
    const auto& c = kCardCatalog[cardId];

    if (ink < c.cost) {
      combatLog = "Not enough Ink!";
      return false;
    }

    // Check couplet
    bool coupletActive = false;
    if (playerStatus.transcendence) coupletActive = true;
    if (c.coupletCond == CoupletType::AfterBlade && lastPlayedMeter == Meter::Blade) coupletActive = true;
    if (c.coupletCond == CoupletType::AfterWard && lastPlayedMeter == Meter::Ward) coupletActive = true;
    if (c.coupletCond == CoupletType::AfterScript && lastPlayedMeter == Meter::Script) coupletActive = true;

    ink -= c.cost;

    // Relic 7: Obsidian Inkwell - whenever you play 3 Script cards, gain 1 Ink
    if (c.meter == Meter::Script) {
      playerStatus.scriptCardsPlayedThisTurn++;
      if (hasRelic(7) && (playerStatus.scriptCardsPlayedThisTurn % 3 == 0)) {
        ink = std::min(static_cast<int8_t>(maxInk + 1), static_cast<int8_t>(ink + 1));
      }
    }

    // Apply Card Effects
    switch (c.effect) {
      case CardEffect::Damage: {
        if (c.id == CARD_FLURRY) {
          int hits = coupletActive ? 3 : 2;
          int perHit = c.baseValue + playerStatus.strength;
          if (hasRelic(4)) perHit++;
          if (playerStatus.weak > 0) perHit = std::max(1, (perHit * 3) / 4);
          int totDmg = 0;
          for (int h = 0; h < hits; ++h) {
            int curHit = perHit;
            if (enemy.status.vulnerable > 0) curHit = (curHit * 3) / 2;
            int b = std::min(enemy.ward, static_cast<int16_t>(curHit));
            enemy.ward -= b;
            enemy.hp -= (curHit - b);
            totDmg += curHit;
          }
          combatLog = std::string("Cast Flurry (") + std::to_string(totDmg) + " dmg, " + std::to_string(hits) + " hits)!";
          break;
        }

        int dmg = c.baseValue + playerStatus.strength;
        if (hasRelic(4) && c.meter == Meter::Blade) dmg++;  // Whetstone
        if (coupletActive) {
          if (c.id == CARD_SPELLBLADE)
            ink = std::min(static_cast<int8_t>(maxInk + 1), static_cast<int8_t>(ink + 1));
          else
            dmg += c.coupletBonus;
        }
        if (playerStatus.weak > 0) dmg = std::max(1, (dmg * 3) / 4);
        if (enemy.status.vulnerable > 0) dmg = (dmg * 3) / 2;

        int blocked = std::min(enemy.ward, static_cast<int16_t>(dmg));
        enemy.ward -= blocked;
        int unblocked = dmg - blocked;
        enemy.hp -= unblocked;

        combatLog = std::string("Cast ") + c.name + " (" + std::to_string(dmg) + " dmg" +
                    (coupletActive ? ", Couplet!" : "") + ")!";
        break;
      }
      case CardEffect::Block: {
        int w = c.baseValue;
        if (coupletActive) {
          w += c.coupletBonus;
          if (c.id == CARD_RUNIC_BARRIER) enemy.status.weak += 2;
        }
        playerWard += w;
        combatLog = std::string("Inscribed ") + c.name + " (+" + std::to_string(w) + " Ward)!";
        break;
      }
      case CardEffect::BlotCard: {  // 0 Cost: Draw 2 cards, gain 3 Ward
        drawCards(2);
        playerWard += 3;
        if (coupletActive) {
          ink = std::min(static_cast<int8_t>(maxInk + 1), static_cast<int8_t>(ink + 1));
        }
        combatLog = std::string("Blotted! Drew 2, +3 Ward") + (coupletActive ? " (+1 Ink Couplet!)" : ".");
        break;
      }
      case CardEffect::BloodInk: {
        playerHp = std::max(1, playerHp - 3);
        ink = std::min(static_cast<int8_t>(maxInk + 2), static_cast<int8_t>(ink + 2));
        combatLog = "Sacrificed 3 HP for +2 Ink!";
        break;
      }
      case CardEffect::DrawCards: {
        drawCards(c.baseValue);
        combatLog = std::string("Cast ") + c.name + "! Drew " + std::to_string(c.baseValue) + " cards.";
        break;
      }
      case CardEffect::ApplyWeak: {
        enemy.status.weak += 2;
        enemy.status.vulnerable += 2;
        combatLog = "Hex cast! Enemy Weak & Vulnerable.";
        break;
      }
      case CardEffect::ApplyPoison: {
        int p = c.baseValue + (coupletActive ? c.coupletBonus : 0);
        enemy.status.poison += p;
        int dmg = 4 + playerStatus.strength;
        int blocked = std::min(enemy.ward, static_cast<int16_t>(dmg));
        enemy.ward -= blocked;
        enemy.hp -= (dmg - blocked);
        combatLog = "Poisoned Quill! " + std::to_string(p) + " Poison applied.";
        break;
      }
      case CardEffect::ApplyVulnerable: {
        enemy.status.vulnerable += 2;
        int dmg = c.baseValue + playerStatus.strength;
        int blocked = std::min(enemy.ward, static_cast<int16_t>(dmg));
        enemy.ward -= blocked;
        enemy.hp -= (dmg - blocked);
        combatLog = "Corrosive Acid! Enemy Vulnerable.";
        break;
      }
      case CardEffect::DamageAndBlock: {
        int dmg = c.baseValue + playerStatus.strength;
        int blocked = std::min(enemy.ward, static_cast<int16_t>(dmg));
        enemy.ward -= blocked;
        enemy.hp -= (dmg - blocked);
        playerWard += 4;
        combatLog = "Blood Rite: 9 dmg, +4 Ward!";
        break;
      }
      case CardEffect::PowerStrength: {
        playerStatus.strength += 2;
        combatLog = "Illuminated Fury! +2 Attack Strength.";
        break;
      }
      case CardEffect::PowerRetainWard: {
        playerStatus.retainWard = true;
        combatLog = "Illuminated Bastion! Ward persists.";
        break;
      }
      case CardEffect::PowerCardDraw: {
        playerStatus.extraDraw += 1;
        combatLog = "Illuminated Flow! +1 Card Draw each turn.";
        break;
      }
      case CardEffect::SeverVulnerable: {
        int dmg = c.baseValue + playerStatus.strength;
        if (hasRelic(4)) dmg++;  // Whetstone
        if (enemy.status.vulnerable > 0) dmg += 10;
        if (playerStatus.weak > 0) dmg = std::max(1, (dmg * 3) / 4);
        if (enemy.status.vulnerable > 0) dmg = (dmg * 3) / 2;

        int blocked = std::min(enemy.ward, static_cast<int16_t>(dmg));
        enemy.ward -= blocked;
        enemy.hp -= (dmg - blocked);
        combatLog = std::string("Sever struck for ") + std::to_string(dmg) + " dmg!";
        break;
      }
      case CardEffect::ParryThrust: {
        playerWard += 5;
        int dmg = 5 + playerStatus.strength;
        if (hasRelic(4)) dmg++;
        if (coupletActive) dmg += c.coupletBonus;
        if (playerStatus.weak > 0) dmg = std::max(1, (dmg * 3) / 4);
        if (enemy.status.vulnerable > 0) dmg = (dmg * 3) / 2;

        int blocked = std::min(enemy.ward, static_cast<int16_t>(dmg));
        enemy.ward -= blocked;
        enemy.hp -= (dmg - blocked);
        combatLog = std::string("Parry & Thrust (+5 Ward, ") + std::to_string(dmg) + " dmg)!";
        break;
      }
      case CardEffect::DivineAegis: {
        playerWard += c.baseValue;
        playerStatus.retainWard = true;
        combatLog = "Divine Aegis: +15 Ward, Ward retained!";
        break;
      }
      case CardEffect::ReboundWard: {
        playerWard += c.baseValue;
        if (coupletActive) {
          int dmg = std::min(static_cast<int16_t>(12), playerWard);
          dmg += playerStatus.strength;
          if (playerStatus.weak > 0) dmg = std::max(1, (dmg * 3) / 4);
          if (enemy.status.vulnerable > 0) dmg = (dmg * 3) / 2;
          int blocked = std::min(enemy.ward, static_cast<int16_t>(dmg));
          enemy.ward -= blocked;
          enemy.hp -= (dmg - blocked);
          combatLog = std::string("Rebound: +8 Ward & Couplet ") + std::to_string(dmg) + " dmg!";
        } else {
          combatLog = "Rebound: +8 Ward.";
        }
        break;
      }
      case CardEffect::InkSiphon: {
        int dmg = c.baseValue + playerStatus.strength;
        if (playerStatus.weak > 0) dmg = std::max(1, (dmg * 3) / 4);
        if (enemy.status.vulnerable > 0) dmg = (dmg * 3) / 2;
        int blocked = std::min(enemy.ward, static_cast<int16_t>(dmg));
        enemy.ward -= blocked;
        enemy.hp -= (dmg - blocked);
        bool siphoned = false;
        if (enemy.status.weak > 0) {
          ink = std::min(static_cast<int8_t>(maxInk + 1), static_cast<int8_t>(ink + 1));
          siphoned = true;
        }
        combatLog = std::string("Ink Siphon: ") + std::to_string(dmg) + " dmg" + (siphoned ? " (+1 Ink)!" : ".");
        break;
      }
      case CardEffect::PowerTranscendence: {
        playerStatus.transcendence = true;
        combatLog = "Illuminated Transcendence! All Couplets active!";
        break;
      }
      case CardEffect::TranscribeCard: {
        drawCards(2);
        if (!hand.empty()) {
          discardPile.push_back(hand.back());
          hand.pop_back();
        }
        combatLog = "Transcribe: Drew 2 cards, discarded 1.";
        break;
      }
      default:
        break;
    }

    lastPlayedMeter = c.meter;
    if (c.type != CardType::Power) {
      discardPile.push_back(cardId);
    }
    hand.erase(hand.begin() + handIndex);

    if (selectedHandIndex >= static_cast<int>(hand.size())) {
      selectedHandIndex = std::max(0, static_cast<int>(hand.size()) - 1);
    }

    return true;
  }

  void generateCardRewards() {
    cardRewards.clear();
    std::random_device rd;
    std::mt19937 g(rd());
    std::vector<uint8_t> pool;
    for (uint8_t i = 3; i < CARD_COUNT; ++i) pool.push_back(i);
    std::shuffle(pool.begin(), pool.end(), g);
    for (int i = 0; i < 3 && i < static_cast<int>(pool.size()); ++i) {
      cardRewards.push_back(pool[i]);
    }
  }
};

}  // namespace codex
