#pragma once

#include <cstdint>
#include <vector>

#include "CodexCards.h"
#include "CodexTypes.h"

namespace codex {

struct EventChoice {
  const char* label;        // Action title, e.g. "Pry the Lead Seals"
  const char* effectDesc;   // Explicit consequence, e.g. "Lose 8 HP. Gain an ancient Relic."
  int16_t hpChange;         // Negative for damage, positive for heal
  int16_t maxHpChange;      // +/- max HP
  int16_t goldChange;       // Negative for cost, positive for reward
  int8_t permStrength;      // Permanent Strength bonus
  int8_t maxInkChange;      // Permanent Max Ink bonus
  int8_t nextCombatWard;    // Ward bonus for the next combat
  uint8_t giveCardId;       // 0xFF for none
  uint8_t giveRelicId;      // 0xFF for none, 0xFE for random unowned relic
  bool fullHeal;            // If true, restores HP to max
  const char* outcomeText;  // Narrative description of the result
};

struct EventDef {
  uint8_t id;
  const char* title;
  const char* subtitle;
  const char* narrative[3];  // Up to 3 lines of descriptive atmospheric prose
  uint8_t choiceCount;
  EventChoice choices[3];
};

static constexpr EventDef kEvents[] = {
    // 0: The Cursed Reliquary (High risk / relic gamble)
    {0,
     "The Cursed Reliquary",
     "Sunken Alcove",
     {"An iron coffer rests bound in blackened chains and sealed with lead.",
      "A faint rhythmic hum vibrates from within the chilled metal.",
      "The runes warn of an ancient heretic's worldly vanity."},
     3,
     {{"Break Lead Seals", "Lose 8 HP. Gain an ancient Relic.", -8, 0, 0, 0, 0, 0, 0xFF, 0xFE, false,
       "The lead cracks! Black vapor scorches your flesh, but you salvage an ancient treasure."},
      {"Say Warding Prayer", "Gain 8 Ward for your next battle.", 0, 0, 0, 0, 0, 8, 0xFF, 0xFF, false,
       "You recite stanzas of preservation. A protective silver veil settles over your soul."},
      {"Leave Carefully", "Step away without disturbing the seals.", 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, false,
       "You heed the warning runes and slip back into the darkened corridor."}}},

    // 1: The Scriptorium Sanctuary (Classic safe rest / meditate / study)
    {1,
     "The Scriptorium",
     "Quiet Sanctuary",
     {"You step into an untouched cloister where warm tallow candles flicker.",
      "Neat stacks of vellum, dry quills, and polished desks line the room.",
      "The air smells of cedar, dried lavender, and sacred stillness."},
     3,
     {{"Rest Upon Benches", "Heal 18 HP.", 18, 0, 0, 0, 0, 0, 0xFF, 0xFF, false,
       "You wrap yourself in your cloak and sleep peacefully away from danger."},
      {"Meditate on Illuminations", "+5 Max HP and heal 5 HP.", 5, 5, 0, 0, 0, 0, 0xFF, 0xFF, false,
       "Studying the sacred geometries broadens your spirit and fortifies your body."},
      {"Transcribe Warding Runes", "Add Parchment Ward to your grimoire.", 0, 0, 0, 0, 0, 0, CARD_DEFEND, 0xFF, false,
       "You take up a reed pen and copy ancient verses of shielding into your book."}}},

    // 2: The Alchemist's Crucible (Dangerous mutagenic trade)
    {2,
     "The Alchemist's Crucible",
     "Abandoned Laboratory",
     {"A massive brass alembic gently hisses atop glowing embers.",
      "Glass phials hold viscous black ichor and sparkling crushed minerals.",
      "A scorched journal reads: 'Strength to he who endures the burning.'"},
     3,
     {{"Drink Black Ichor", "+1 Perm Strength! Lose 8 HP and -4 Max HP.", -8, -4, 0, 1, 0, 0, 0xFF, 0xFF, false,
       "Agony sears your chest! Your muscles knot with raw power as corruption hardens your blood."},
      {"Scrape Silver Dust", "Gain 30 Gold.", 0, 0, 30, 0, 0, 0, 0xFF, 0xFF, false,
       "You scrape precious precipitated flakes of alchemical silver into your purse."},
      {"Quench the Embers", "Step away safely.", 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, false,
       "You snuff the coals and leave the toxic fumes behind."}}},

    // 3: Ambush in the Dark Corridor (Pure hazard / hostile encounter)
    {3,
     "Corridor Ambush",
     "Shadowed Passage",
     {"Rusty caltrops clatter across the stone! Hooded cutpurses drop",
      "from iron grates above, brandishing hooked blades and demanding tribute.",
      "Their leader sneers: 'Your gold or your entrails, scribe!'"},
     3,
     {{"Fight Your Way Out!", "Lose 10 HP. Plunder 35 Gold from the fallen.", -10, 0, 35, 0, 0, 0, 0xFF, 0xFF, false,
       "You cut down the brigands in a ferocious clash, looting their purses amidst your bleeding wounds."},
      {"Bribe Cutpurses", "Pay 20 Gold. Escape unharmed.", 0, 0, -20, 0, 0, 0, 0xFF, 0xFF, false,
       "You toss a pouch of clinking coins. The thieves scramble for the gold while you slip away."},
      {"Flee Blindly!", "Lose 8 HP and drop 10 Gold in panic.", -8, 0, -10, 0, 0, 0, 0xFF, 0xFF, false,
       "You dash into the darkness! Concealed wall spikes tear your clothes and coins scatter into the gloom."}}},

    // 4: The Whispering Lectern (Cursed forbidden knowledge)
    {4,
     "The Whispering Lectern",
     "Desecrated Shrine",
     {"A pedestal carved from bone holds an open grimoire bound in leathery hide.",
      "The pages whisper blasphemies in forgotten tongues, urging you closer.",
      "Dark crimson sigils throb upon the brittle parchment."},
     3,
     {{"Read Crimson Verses", "-5 Max HP. Gain Illumination: Fury card.", -5, -5, 0, 0, 0, 0, CARD_POWER_FURY, 0xFF,
       false, "The unholy runes burn into your mind. Your wrath ignites with terrible fury at the cost of your soul."},
      {"Burn Foul Manuscript", "Purge dark malice: Heal 14 HP.", 14, 0, 0, 0, 0, 0, 0xFF, 0xFF, false,
       "You cast a torch upon the tome. The shrieking flames collapse into soothing, purifying ash."},
      {"Avert Your Gaze", "Walk past without reading.", 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, false,
       "You cover your ears and hurry past the insidious whispering."}}},

    // 5: The Wandering Peddler (Merchant economy / gamble)
    {5,
     "The Wandering Peddler",
     "Wayside Vendor",
     {"A hunchbacked merchant sits atop crates stamped with royal customs seals.",
      "He adjusts brass spectacles and unrolls a velvet cloth of oddities.",
      "'Rare curiosities for a brave delver, if you have coin to spare!'"},
     3,
     {{"Buy Antiquity (35 Gold)", "Pay 35 Gold. Gain an ancient Relic.", 0, 0, -35, 0, 0, 0, 0xFF, 0xFE, false,
       "The merchant bows low and hands you an intricately carved antiquity wrapped in linen."},
      {"Buy Tonic (20 Gold)", "Pay 20 Gold. Heal 20 HP.", 20, 0, -20, 0, 0, 0, 0xFF, 0xFF, false,
       "You swallow the fragrant herbal elixir. Warm vitality knits your deepest cuts."},
      {"Rob the Peddler!", "Steal 45 Gold! Suffer 8 damage from hidden trap.", -8, 0, 45, 0, 0, 0, 0xFF, 0xFF, false,
       "You snatch his strongbox, but a poisoned spring-dart strikes your neck as he screams curses!"}}},

    // 6: The Scribe's Blood Altar (Sacrificial trade / max ink)
    {6,
     "The Scribe's Blood Altar",
     "Subterranean Vault",
     {"A monolithic black marble altar stands drenched in centuries of dried ink.",
      "An inscription carved in jagged Latin proclaims:",
      "'Pour out thy vital humors, and the font shall never run dry.'"},
     3,
     {{"Offer Living Blood", "Lose 12 HP. Gain +1 Permanent Max Ink!", -12, 0, 0, 0, 1, 0, 0xFF, 0xFF, false,
       "The marble drinks greedily. An inexhaustible current of ink surges through your spirit!"},
      {"Sacrifice Gold (30 Gold)", "Pay 30 Gold. Restore HP to full.", 0, 0, -30, 0, 0, 0, 0xFF, 0xFF, true,
       "Gilded smoke billows from the brazier. A miraculous soothing warmth mends your every wound."},
      {"Refuse Sacrilege", "Leave the unholy altar.", 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, false,
       "You refuse the dark bargain and depart into the cold corridor."}}},

    // 7: The Torture Vaults (Dangerous scavenge / injury)
    {7,
     "The Torture Vaults",
     "Iron Oubliette",
     {"Rusted iron cages hang from chains above pools of stagnant black water.",
      "Discarded breastplates and skeletal remains litter the damp stone.",
      "Valuable arms and pouches lie tangled in razor-sharp barbs."},
     3,
     {{"Pry Out Steel Blade", "Gain Heavy Pommel card. Take 6 damage from barbs.", -6, 0, 0, 0, 0, 0, CARD_POMMEL, 0xFF,
       false, "You wrench a weighted steel pommel from a cage, shredding your hands on barbed iron."},
      {"Loot Fallen Purses", "Gain 25 Gold.", 0, 0, 25, 0, 0, 0, 0xFF, 0xFF, false,
       "You gingerly cut away coin purses from the fallen warriors."},
      {"Avoid the Barbs", "Pass through without touching anything.", 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, false,
       "You tread carefully around the rusted instruments and continue your journey."}}},

    // 8: Fountain of Shimmering Ink (Blessing / Flow / Ward)
    {8,
     "Fountain of Shimmering Ink",
     "Celestial Spring",
     {"A carved marble angel pours a continuous stream of glowing celestial ink",
      "into an ornate baptismal font. Luminous motes of light dance in the mist.",
      "A profound sense of clarity and purpose fills the chamber."},
     3,
     {{"Bathe in the Waters", "Heal 15 HP and gain +3 Max HP.", 15, 3, 0, 0, 0, 0, 0xFF, 0xFF, false,
       "The crystalline ink revitalizes your weary frame, filling you with enduring strength."},
      {"Imbue Your Grimoire", "Add Illumination: Flow card to grimoire.", 0, 0, 0, 0, 0, 0, CARD_POWER_FLOW, 0xFF,
       false, "Glowing celestial verses illuminate your pages, granting boundless turn-by-turn insight."},
      {"Drink Blessed Ink", "Gain 12 Ward for your next battle.", 0, 0, 0, 0, 0, 12, 0xFF, 0xFF, false,
       "The sweet font fills you with steadfast resolve. An impenetrable aura envelops your mind."}}},
};

constexpr size_t kEventCount = sizeof(kEvents) / sizeof(kEvents[0]);

}  // namespace codex
