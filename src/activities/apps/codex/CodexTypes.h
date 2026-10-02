#pragma once
#include <cstdint>
#include <string>

namespace codex {

enum class Meter : uint8_t {
  None = 0,
  Blade,  // Direct physical attacks
  Ward,   // Defensive shields, wards, barriers
  Script  // Magic, debuffs, card draw, ink manipulation
};

enum class CardType : uint8_t { Attack, Skill, Power };

enum class CoupletType : uint8_t {
  None = 0,
  AfterBlade,  // Triggers if previous card played was Blade
  AfterWard,   // Triggers if previous card played was Ward
  AfterScript  // Triggers if previous card played was Script
};

enum class CardEffect : uint8_t {
  Damage,
  Block,
  DamageAndBlock,
  DrawCards,
  GainInk,
  ApplyWeak,
  ApplyVulnerable,
  ApplyPoison,
  BlotCard,         // 0 Cost: Draw 2 cards, gain 3 Ward
  BloodInk,         // 0 Cost: Lose 3 HP, gain 2 Ink
  PowerStrength,    // +2 damage to all attacks for rest of combat
  PowerRetainWard,  // Ward does not degrade at end of round
  PowerCardDraw,    // Draw +1 extra card at start of each turn
  SeverVulnerable,  // Deal 14 damage (+10 if Vulnerable)
  ParryThrust,      // Gain 5 Ward, deal 5 damage
  DivineAegis,      // Gain 15 Ward, retain Ward this turn
  ReboundWard,      // Gain 8 Ward (couplet: deal damage equal to Ward)
  InkSiphon,        // Deal 7 damage (+1 Ink if Weak)
  PowerTranscendence,// Power: All Couplets are always active [1-Use]
  TranscribeCard    // Draw 2 cards, exhaust 1 card
};

enum class IntentType : uint8_t { Attack, Defend, AttackAndDefend, BuffStrength, DebuffPlayer };

enum class RoomType : uint8_t { Combat = 0, Elite, Scriptorium, Merchant, Mystery, Boss };

struct CardDef {
  uint8_t id;
  const char* name;
  int8_t cost;
  Meter meter;
  CardType type;
  CardEffect effect;
  int16_t baseValue;
  CoupletType coupletCond;
  int16_t coupletBonus;
  const char* desc;
  const char* coupletDesc;
};

struct StatusEffects {
  int8_t weak = 0;          // Deals 25% less damage
  int8_t vulnerable = 0;    // Takes 50% more damage
  int8_t poison = 0;        // Takes X damage at end of turn, reduces by 1
  int8_t strength = 0;      // Adds +X to all attacks
  bool retainWard = false;  // Ward does not halve at end of round
  int8_t extraDraw = 0;     // Extra cards drawn at start of each turn
  bool transcendence = false; // All Couplet requirements satisfied
  int8_t combatTurnCount = 0; // Turns in current combat
  int8_t scriptCardsPlayedThisTurn = 0; // Script cards played this turn
};

struct EnemyState {
  const char* name;
  int16_t hp;
  int16_t maxHp;
  int16_t ward;
  IntentType intent;
  int16_t intentValue;
  int16_t intentExtra;
  StatusEffects status;
  uint8_t moveCount;
  uint8_t spriteId;
  bool isElite;
  bool isBoss;
};

struct RelicDef {
  uint8_t id;
  const char* name;
  const char* desc;
};

}  // namespace codex
