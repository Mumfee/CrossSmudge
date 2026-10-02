#pragma once
#include <vector>

#include "CodexTypes.h"

namespace codex {

struct MonsterTemplate {
  uint8_t id;
  const char* name;
  int16_t minHp;
  int16_t maxHp;
  bool isElite;
  bool isBoss;
};

static constexpr MonsterTemplate kMonsterTemplates[] = {{0, "Ink Imp", 20, 26, false, false},
                                                        {1, "Stone Gargoyle", 28, 34, false, false},
                                                        {2, "Cursed Scribe", 24, 30, false, false},
                                                        {3, "Parchment Ghoul", 26, 32, false, false},
                                                        {4, "Book Golem", 44, 50, true, false},
                                                        {5, "Marginalia Fiend", 42, 48, true, false},
                                                        {6, "The Grand Inquisitor", 85, 95, false, true},
                                                        {7, "Quill Hound", 22, 28, false, false},
                                                        {8, "Crypt Warden", 32, 38, false, false},
                                                        {9, "Spine Horror", 46, 52, true, false},
                                                        {10, "The Iron Scriptor", 48, 56, true, false},
                                                        {11, "The Arch-Heretic", 85, 95, false, true}};

inline EnemyState spawnMonster(int floor, bool isElite = false, bool isBoss = false) {
  EnemyState enemy;
  const MonsterTemplate* t = nullptr;

  if (isBoss) {
    int bIdx = (floor % 2 == 0) ? 11 : 6; // 6: Grand Inquisitor, 11: Arch-Heretic
    t = &kMonsterTemplates[bIdx];
  } else if (isElite) {
    const uint8_t eliteIds[] = {4, 5, 9, 10}; // Golem, Fiend, Spine Horror, Iron Scriptor
    t = &kMonsterTemplates[eliteIds[floor % 4]];
  } else {
    const uint8_t normalIds[] = {0, 1, 2, 3, 7, 8}; // Imp, Gargoyle, Scribe, Ghoul, Hound, Warden
    t = &kMonsterTemplates[normalIds[(floor - 1) % 6]];
  }

  enemy.name = t->name;
  enemy.maxHp = t->minHp + (floor * 2);
  enemy.hp = enemy.maxHp;
  enemy.ward = 0;
  enemy.moveCount = 0;
  enemy.spriteId = t->id;
  enemy.isElite = t->isElite;
  enemy.isBoss = t->isBoss;
  enemy.status = {};

  // First turn intent
  if (t->id == 0) {  // Ink Imp: fast strike
    enemy.intent = IntentType::Attack;
    enemy.intentValue = 7;
  } else if (t->id == 1) {  // Stone Gargoyle: fortify first
    enemy.intent = IntentType::Defend;
    enemy.intentValue = 8;
  } else if (t->id == 7) {  // Quill Hound: quick snap
    enemy.intent = IntentType::Attack;
    enemy.intentValue = 6;
  } else if (t->id == 8) {  // Crypt Warden: tower guard
    enemy.intent = IntentType::Defend;
    enemy.intentValue = 12;
  } else if (t->id == 9) {  // Spine Horror: multi-needle flurry
    enemy.intent = IntentType::Attack;
    enemy.intentValue = 11;
  } else if (t->id == 10) { // Iron Scriptor: steam vent
    enemy.intent = IntentType::Defend;
    enemy.intentValue = 10;
  } else if (t->id == 6) {  // Grand Inquisitor: heavy opening smash
    enemy.intent = IntentType::AttackAndDefend;
    enemy.intentValue = 10;
    enemy.intentExtra = 8;
  } else if (t->id == 11) { // Arch-Heretic: blasphemous litany
    enemy.intent = IntentType::AttackAndDefend;
    enemy.intentValue = 11;
    enemy.intentExtra = 8;
  } else {
    enemy.intent = IntentType::Attack;
    enemy.intentValue = 6 + floor;
  }

  return enemy;
}

inline void decideNextIntent(EnemyState& enemy, int floor) {
  enemy.moveCount++;
  int turn = enemy.moveCount % 4;

  if (enemy.isBoss) {
    if (enemy.spriteId == 11) { // Arch-Heretic
      if (turn == 1) {
        enemy.intent = IntentType::DebuffPlayer;
        enemy.intentValue = 2; // Weak
      } else if (turn == 2) {
        enemy.intent = IntentType::Attack;
        enemy.intentValue = 16 + (enemy.status.strength * 2);
      } else if (turn == 3) {
        enemy.intent = IntentType::BuffStrength;
        enemy.intentValue = 2;
      } else {
        enemy.intent = IntentType::AttackAndDefend;
        enemy.intentValue = 13 + enemy.status.strength;
        enemy.intentExtra = 12;
      }
      return;
    }

    if (turn == 1) {
      enemy.intent = IntentType::Attack;
      enemy.intentValue = 14 + (enemy.status.strength * 2);
    } else if (turn == 2) {
      enemy.intent = IntentType::Defend;
      enemy.intentValue = 15;
    } else if (turn == 3) {
      enemy.intent = IntentType::BuffStrength;
      enemy.intentValue = 2;  // +2 Strength
    } else {
      enemy.intent = IntentType::AttackAndDefend;
      enemy.intentValue = 12 + enemy.status.strength;
      enemy.intentExtra = 10;
    }
    return;
  }

  if (enemy.isElite) {
    if (enemy.spriteId == 9) { // Spine Horror
      if (turn == 1) {
        enemy.intent = IntentType::Attack;
        enemy.intentValue = 14;
      } else if (turn == 2) {
        enemy.intent = IntentType::Defend;
        enemy.intentValue = 12;
      } else if (turn == 3) {
        enemy.intent = IntentType::DebuffPlayer;
        enemy.intentValue = 2;
      } else {
        enemy.intent = IntentType::AttackAndDefend;
        enemy.intentValue = 10;
        enemy.intentExtra = 6;
      }
      return;
    }
    if (enemy.spriteId == 10) { // Iron Scriptor
      if (turn == 1) {
        enemy.intent = IntentType::Attack;
        enemy.intentValue = 15;
      } else if (turn == 2) {
        enemy.intent = IntentType::AttackAndDefend;
        enemy.intentValue = 9;
        enemy.intentExtra = 9;
      } else if (turn == 3) {
        enemy.intent = IntentType::BuffStrength;
        enemy.intentValue = 2;
      } else {
        enemy.intent = IntentType::Defend;
        enemy.intentValue = 14;
      }
      return;
    }

    // Standard Elites (4 Golem, 5 Fiend)
    if (turn == 1) {
      enemy.intent = IntentType::Attack;
      enemy.intentValue = 12 + floor;
    } else if (turn == 2) {
      enemy.intent = IntentType::AttackAndDefend;
      enemy.intentValue = 8;
      enemy.intentExtra = 8;
    } else {
      enemy.intent = IntentType::BuffStrength;
      enemy.intentValue = 1;
    }
    return;
  }

  // Normal monsters
  if (enemy.spriteId == 0) {  // Ink Imp
    if (turn == 2) {
      enemy.intent = IntentType::DebuffPlayer;
      enemy.intentValue = 2;  // 2 weak
    } else {
      enemy.intent = IntentType::Attack;
      enemy.intentValue = (turn == 1) ? 9 : 6;
    }
  } else if (enemy.spriteId == 1) {  // Gargoyle
    if (turn % 2 == 1) {
      enemy.intent = IntentType::Attack;
      enemy.intentValue = 12 + floor;
    } else {
      enemy.intent = IntentType::Defend;
      enemy.intentValue = 10;
    }
  } else if (enemy.spriteId == 7) {  // Quill Hound
    if (turn == 1) {
      enemy.intent = IntentType::Attack;
      enemy.intentValue = 8 + (floor / 2);
    } else if (turn == 2) {
      enemy.intent = IntentType::DebuffPlayer;
      enemy.intentValue = 2; // Vulnerable
    } else if (turn == 3) {
      enemy.intent = IntentType::BuffStrength;
      enemy.intentValue = 1;
    } else {
      enemy.intent = IntentType::Attack;
      enemy.intentValue = 7;
    }
  } else if (enemy.spriteId == 8) {  // Crypt Warden
    if (turn == 1) {
      enemy.intent = IntentType::Attack;
      enemy.intentValue = 10;
    } else if (turn == 2) {
      enemy.intent = IntentType::AttackAndDefend;
      enemy.intentValue = 7;
      enemy.intentExtra = 8;
    } else if (turn == 3) {
      enemy.intent = IntentType::Defend;
      enemy.intentValue = 12;
    } else {
      enemy.intent = IntentType::Attack;
      enemy.intentValue = 12;
    }
  } else {
    if (turn == 2) {
      enemy.intent = IntentType::AttackAndDefend;
      enemy.intentValue = 7;
      enemy.intentExtra = 5;
    } else {
      enemy.intent = IntentType::Attack;
      enemy.intentValue = 7 + (floor / 2);
    }
  }
}

}  // namespace codex
