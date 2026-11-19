# Seven Gods RPG — Weapon, Move and Armour Types

## Overview

A weapon type decides which moves a character can equip, how hard they hit, how much stagger they cause and how much stamina every move costs. It never changes a move's timing. Heavier weapons hit harder and stagger more per swing but cost more stamina per point of damage. Lighter weapons are stamina-efficient but chip and barely stagger. This doc builds on Seven Gods RPG — Animation-Timed Battle System Design (timings in seconds, chains, Hold, stencils, set-back, guard range). It defines types only; specific named weapons with elements and effects come later.

## Principles

1. Time belongs to moves, never to weapons. A claymore feels slow because claymore moves have long animations, not because the claymore slows anything.

2. Weight shows up as stamina tax and stagger. Every equipped item adds its tax to every move except Hold, so a heavy kit drains stamina whatever you do.

3. A move's animation never changes between its variants. Tier II and III versions hit harder or gain a property, but impact and completion times stay the same, so a read learned early stays valid all game.

4. Specific weapons scale power by quality, not tax. A late-game claymore hits four times harder than an early one but still costs 7 stamina per move.

## Equipment slots

Each combatant has a main hand, an off-hand and one armour piece. Two-handed weapons fill both hands.

| Slot | Can hold |
| --- | --- |
| Main hand | Any one-handed weapon, or a two-handed weapon (fills the off-hand too) |
| Off-hand | A shield, a parrying dagger, a wand (spellblade), or nothing |
| Armour | One armour type (see Armour types) |

Two-handed: long sword, claymore, spear, pike, all bows, crossbow, magic staff, quarterstaff.

One-handed: knife, stiletto, rapier, sabre, arming sword, short spear, wand, rod, gauntlets.

Stamina tax stacks: main-hand tax + off-hand tax + armour tax is added to every move except Hold.

Move access: a move can be equipped if any held item matches its weapon list. Damage uses the matching item's power (main hand first).

Combo moves: sword-and-shield moves need a one-handed blade (dagger, light sword, arming sword or short spear) in the main hand and a shield.

Loadout: 6 moves chosen from what the equipment allows, plus Hold and Use Item, as in the battle system doc.

No dual-wielding beyond the parrying dagger and the off-hand wand. Keeps move access readable.

## Weapon types

Eleven weapon families plus unarmed, with 25 subtypes, from 0-tax knives to the 7-tax claymore. Power is shown at early-game quality; tax and stagger multiplier belong to the type and never change.

## Stat meanings

- Power: added to each move's base damage (or base heal). Early quality ×1, mid ×2, late ×4.

- Tax: stamina added to every move while equipped.

- Stagger ×: multiplies the set-back the weapon's attacks cause.

- Scales with: Attack, or Mana for magic.

| Family | Subtype | Hands | Scales with | Power | Tax | Stagger × | Special |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Dagger | Knife | 1H | Attack | 3 | 0 | 0.7 | — |
| Dagger | Stiletto | 1H | Attack | 4 | 0 | 0.6 | Backstab ×2.5 instead of ×2 |
| Dagger | Parrying dagger | Off-hand | Attack | 2 | 0 | 0.6 | Unlocks Parry with any 1H main hand |
| Light sword | Rapier | 1H | Attack | 5 | 1 | 0.75 | — |
| Light sword | Sabre | 1H | Attack | 6 | 1 | 0.85 | — |
| Sword | Arming sword | 1H | Attack | 7 | 3 | 1.0 | The baseline weapon |
| Long sword | Long sword | 2H | Attack | 10 | 4 | 1.15 | Can be held 1H with a shield at power 8, without long-sword-only moves |
| Claymore | Claymore | 2H | Attack | 15 | 7 | 1.4 | Side cells of its stencils +15% |
| Shield | Buckler | Off-hand | Attack | 2 | 1 | 1.0 | Guard +6, no guard range. Unlocks Buckler Parry |
| Shield | Heater shield | Off-hand | Attack | 4 | 2 | 1.0 | Guard +12, guard range: sides 50% |
| Shield | Tower shield | Off-hand | Attack | 6 | 4 | 1.0 | Guard +20, guard range: sides 75%, behind 100% |
| Spear | Short spear | 1H | Attack | 6 | 2 | 0.9 | Counts as a blade for sword-and-shield moves |
| Spear | Spear | 2H | Attack | 9 | 3 | 1.0 | Depth cells of its stencils +25% |
| Spear | Pike | 2H | Attack | 11 | 4 | 1.0 | Reach 1 moves deal 50%. Reach 2 moves gain depth +1 at 50% |
| Bow | Short bow (light draw) | 2H | Attack | 5 | 1 | 0.8 | Unlocks Point-blank Shot |
| Bow | Longbow (medium draw) | 2H | Attack | 9 | 3 | 1.0 | — |
| Bow | Warbow (heavy draw) | 2H | Attack | 14 | 6 | 1.3 | Aimed Shot pierces: depth +1 at 50% from tier I |
| Bow | Crossbow | 2H | Attack | 12 | 3 | 1.2 | Must be loaded: Bolt fires one, Reload loads |
| Gauntlets | Hand wraps | 1H | Attack | 2 | 0 | 1.0 | — |
| Gauntlets | Gauntlets | 1H | Attack | 4 | 1 | 1.2 | — |
| Gauntlets | Cestus | 1H | Attack | 6 | 2 | 1.3 | — |
| Wand | Wand | 1H or off- hand | Mana | 4 | 1 | 0.6 | Off-hand use makes a spellblade |
| Wand | Rod | 1H | Mana | 6 | 2 | 0.8 | — |
| Staff | Magic staff | 2H | Mana | 8 | 3 | 0.8 | Physical Staff Strike uses Attack |
| Staff | Quarterstaff | 2H | Attack | 6 | 2 | 1.1 | Physical staff moves only |
| — | Unarmed | — | Attack | 0 | 0 | 1.0 | Universal and gauntlet moves only |

"Sword and shield" isn't a separate item. It's the pairing of a one-handed blade with a shield, which unlocks the combo moves under Shield and sword-and-shield moves.

## How move variants progress

Every move comes in up to three variants: I (early game), II (mid) and III (late). A later variant replaces the earlier one in its loadout slot, is taught later in the world, and keeps the exact same animation and timings.

| Stat | I | II | III |
| --- | --- | --- | --- |
| Base damage / heal / defence | ×1 | ×1.5 | ×2 |
| Stagger | base | +0.05 s | +0.10 s |
| Stamina cost | ×1 | ×1.25 | ×1.5 (rounded) |
| Properties | — | Usually one new property | One more property, or a stronger one |

Properties a variant can gain (all from the battle system's vocabulary, none touch timing):

- extra or stronger stencil cells (a sweep reaching further to the sides, a shot that pierces)

- a wider defend or dodge window around the same peak

- a larger skip on a block

- a hit-reaction override: Deflect, Steadfast, Unflinching, Absorb

- a push of 1 cell, or guard range on a defensive move

Some moves start at II: Spin Slash, Whirlwind, Shield Wall, Twin Cleave, Vaulting Thrust, Volley, Throw, Barrier and Blink. Multi-direction and team-protecting moves are mid-game unlocks, matching the progression in the battle system doc.

Damage grows faster than cost (×2 vs ×1.5 by III), so later variants are more stamina- efficient as well as stronger.

Reading the move tables: times are impact / completion in seconds. Each tier column gives damage / stagger (s), or the defence or heal value, then anything the variant adds. "Cost" is the tier I base stamina cost before tax.

## Universal moves

Available with any weapon or none, so every character always has movement, a basic guard and a way to deal with someone behind them. Damage uses gauntlet power if worn, otherwise 0.

| Move | Types | Times (s) | Aim / pattern | Cost | I | II | III |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Step | Move | move 0.3 / 0.6 | Fixed: 1 forward or 1 back | 1 | — | Any orthogonal cell | Any of 8 cells, dodge 0.0–0.2 |
| Roll | Move + Dodge | move 0.3 / 0.8 | Any adjacent cell | 4 | Dodge 0.0–0.3 | Dodge 0.0–0.4 | Dodge 0.0– 0.5 |
| Brace | Defend | window 0.1– 0.5–0.8 / 1.1 | Self | 3 | Def 8, skip 0.3 | Def 12 | Def 16, skip 0.4 |
| Shove | Attack | 0.4 / 0.8 | Reach 1 | 3 | 4 / 0.50 | 6 / 0.55 | 8 / 0.60, push 1 |
| Back Kick | Attack | 0.45 / 0.9 | Around: cell behind | 4 | 8 / 0.40 | 12 / 0.45, diagonals behind 50% | 16 / 0.50, push 1 |
| Guarded Hold* | Defend | 0.6, 1.2 or 2.0 (flat window) | Self (+ shield's guard range) | 0 | 50% of def 12 | 50% of def 18 | 50% of def 24 |
| Use Item | Item | per item | per item | 0 | — | — | — |

*Guarded Hold is the rule proposed in the battle system's Simulation tab. It pays gear tax and recovers no stamina.

## Blade moves

Blades run from fast, cheap dagger stabs to slow claymore swings that stagger for a full second. Many moves are shared down the size range, so a character moving from sword to long sword keeps part of their kit.

## Dagger

| Move | Weapons | Types | Times (s) | Aim | Cost | I | II | III |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Stab | Dagger, light sword | Attack | 0.25 / 0.5 | Reach 1 | 2 | 8 / 0.20 | 12 / 0.25 | 16 / 0.30, cost stays 2 |
| Backstab | Dagger | Attack | 0.4 / 0.8 | Reach 1; ×2 if the target faces away | 3 | 10 / 0.30 | 15 / 0.35 | 20 / 0.40, ×2.5 from behind |
| Slip Strike | Dagger, light sword | Attack + Dodge + Move | move 0.2, impact 0.45 / 0.9 | Diagonal step, then Reach 1 | 5 | 8, dodge 0.0–0.3 | 12, dodge 0.0–0.35 | 16, dodge 0.0–0.4 |
| Throwing Knife | Dagger | Attack | 0.35 / 0.7 | First | 3 | 6 / 0.15 | 9 / 0.20 | 12 / 0.20, sides 30% (fan) |
| Parry Light sword and sword | Parrying dagger, dagger, light sword | Defend | window 0.05–0.15– 0.25 / 0.6 | Self | 2 | Def 20, skip 0.4 | Def 30, Deflect | Def 40, Deflect, skip 0.5 |
| Slash | Light sword, sword, long sword | Attack | 0.5 / 1.0 | Reach 1 | 6 | 18 / 0.30 | 27 / 0.35 | 36 / 0.40, sides 25% |
| Lunge | Light sword, sword | Attack + Dodge + Move | move 0.3, impact 0.5 / 1.1 | Fixed 1 forward, then Reach 1 | 8 | 20, dodge 0.0–0.3 | 30 | 40, dodge 0.0–0.4, stop short |
| Riposte | Light sword, sword | Defend | window 0.1–0.2– 0.4 / 0.8 | Self | 5 | Def 25, skip 0.5 | Def 35, Deflect | Def 45, Deflect, skip 0.6 |
| Pommel Strike | Sword, long sword, claymore | Attack | 0.35 / 0.7 | Reach 1 | 4 | 8 / 0.50 | 10 / 0.60 | 12 / 0.70, push 1 |
| Cross Cut | Sword, long sword, claymore | Attack | 0.7 / 1.3 | Reach 1, sides 50% | 8 | 16 / 0.35 | 24 / 0.40 | 32 / 0.45, sides 75% |
| Spin Slash Pommel Strike deals little damage but staggers hard: a | Sword, long sword, claymore | Attack | 0.8 / 1.5 | Around: all 8 at 60% | 14 cheap interrupt tool for heavier | — | 30 / 0.40 | 40 / 0.45, 70% |

blades.

## Long sword

| Move | Weapons | Types | Times (s) | Aim | Cost | I | II | III |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Overhead Cleave | Long sword, claymore | Attack | 0.9 / 1.7 | Reach 1 | 14 | 32 / 0.60 | 48 / 0.65 | 64 / 0.70, Steadfast |
| Half-sword Thrust | Long sword | Attack | 0.6 / 1.2 | Reach 1, depth +1 at 40% | 9 | 22 / 0.40 | 33 / 0.45 | 44 / 0.50, Steadfast |
| Hanging Guard Claymore | Long sword | Defend | window 0.1–0.6–1.1 / 1.4 | Self | 5 | Def 20, skip 0.3 | Def 30 | Def 40, guard range sides 50% Page 7 of 18 |
| Wide Sweep | Claymore | Attack | 1.0 / 1.9 | Reach 1, sides 80% | 16 | 34 / 0.60 | 51 / 0.65 | 68 / 0.70, sides 100% |
| Crushing Overhead | Claymore | Attack | 1.3 / 2.4 | Reach 1, sides 30% | 22 | 48 / 0.90 | 72 / 0.95, Steadfast | 96 / 1.00, Unflinching |
| Whirlwind | Claymore | Attack | 1.2 / 2.2 | Around: all 8 at 70% | 20 | — | 60 / 0.60 | 80 / 0.70, Steadfast |
| Blade Block | Claymore | Defend | window 0.2–0.7–1.2 / 1.6 | Self | 6 | Def 25, skip 0.3 | Def 38 | Def 50, skip 0.5 |

## Shield and sword-and-shield moves

Shields are the team-defence family. Each shield adds its guard bonus to the defence of its moves and projects its guard range onto allies, so a tower shield protects the healer behind it. Shield moves use the shield's power for damage.

## Shield (any shield, any or no main-hand weapon)

| Move | Weapons | Types | Times (s) | Aim / range | Cost | I | II | III |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Block | Any shield | Defend | window 0.1– 0.5–0.9 / 1.2 | Self + shield's guard range | 3 | Def 15 + shield, skip 0.3 | Def 22 + shield | Def 30 + shield, skip 0.4 |
| Shield Bash | Any shield | Attack + Defend | guard 0.1– 0.3–0.5, impact 0.6 / 1.1 | Reach 1 | 6 | 10 / 0.60, def 10 | 15 / 0.70, def 15 | 20 / 0.80, def 20, push 1 |
| Shield Charge | Any shield | Attack + Defend + Move | move 0.3, guard 0.0– 0.3–0.5, impact 0.6 / 1.2 | Fixed 1 forward (stop short), Reach 1 | 8 | 12 / 0.80, def 10 | 18 / 0.85, def 15 | 24 / 0.90, def 20, push 1 |
| Shield Wall | Heater, tower | Defend | window 0.1– 0.8–1.6 / 2.0 | Shield's range, sides raised to 100% | 6 | — | Def 27 + shield, skip 0.2 | Def 36 + shield, skip 0.3 |
| Buckler Parry | Buckler | Defend | window 0.05–0.15– 0.25 / 0.6 | Self | 2 | Def 20 + shield, skip 0.4 | Deflect | Deflect, skip 0.6 |

## Sword and shield (one-handed blade + shield)

| Move | Weapons | Types | Times (s) | Aim / range | Cost | I | II | III |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Guarded Thrust | 1H blade + shield | Attack + Defend | guard 0.1– 0.4–0.6, impact 0.7 / 1.3 | Reach 1 | 8 | 16, def 15 | 24, def 22 | 32, def 30, depth +1 at 30% |
| Counter Cut | 1H blade + shield | Attack + Defend | guard 0.1– 0.3–0.5 (skip 0.3), impact 0.8 / 1.4 | Reach 1 | 7 | 18, def 18 | 27, def 27, skip 0.4 | 36, def 36, Deflect |
| Phalanx Step | 1H blade + shield | Defend + Move | move 0.3, guard 0.0– 0.3–0.6 / 0.9 | Fixed 1 forward + shield's guard range | 4 | Def 15 + shield | Def 22 + shield | Def 30 + shield, any orthogonal |

Counter Cut is built on the skip rule. Blocking during its guard window skips the animation forward toward its own impact, so a well-timed block brings the counter-attack out sooner.

## Spear, bow and crossbow moves

Spears own Reach 2 and depth; bows own range, trading damage per second for safety. Bow draw weight (light, medium, heavy) changes power, tax and stagger, not which moves are available, except the short bow's point-blank shot. The crossbow swaps stamina for time: cheap, powerful bolts that need Reload moves.

## Spear (short spear, spear, pike)

| Move | Weapons | Types | Times (s) | Aim | Cost | I | II | III |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Thrust | All spears (pike at 50%) | Attack | 0.6 / 1.2 | Reach 1, depth +1 at 50% | 7 | 20 / 0.40 | 30 / 0.45 | 40 / 0.50, depth 75% |
| Long Thrust | Spear, pike | Attack | 0.7 / 1.3 | Reach 2 | 8 | 18 / 0.40 | 27 / 0.45 | 36 / 0.50, depth +1 at 50% |
| Set Spear | All spears | Defend | window 0.1– 0.6–1.1 / 1.4 | Self | 4 | Def 18, skip 0.3 | Def 27, Deflect | Def 36, Deflect, skip 0.4 |
| Twin Cleave | Spear | Attack | 0.7 / 1.3 | Around: ahead and behind at 100% | 12 | — | 27 / 0.40 | 36 / 0.45 |
| Vaulting Thrust | Short spear, spear | Attack + Dodge + Move | move 0.3, dodge 0.1– 0.4, impact 0.6 / 1.2 | Fixed 1 forward, Reach 1, depth 50% | 10 | — | 33 / 0.45 | 44 / 0.50, Steadfast |

## Bow (short bow, longbow, warbow)

| Move | Weapons | Types | Times (s) | Aim | Cost | I | II | III |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Quick Shot | All bows | Attack | 0.4 / 0.8 | First | 3 | 10 / 0.15 | 15 / 0.20 | 20 / 0.20, cost stays 3 |
| Aimed Shot | All bows | Attack | 1.0 / 1.6 | First | 9 | 26 / 0.40 | 39 / 0.45 | 52 / 0.50, pierces: depth +1 at 50% |
| Retreating Shot | All bows | Attack + Move | move 0.2 (fixed 1 back), impact 0.6 / 1.0 | First | 5 | 12 / 0.20 | 18 / 0.20 | 24 / 0.25, dodge 0.0– 0.2 |
| Volley | Longbow, warbow | Attack | 1.3 / 2.0 | Reach 2, depth +1 and sides at 60% | 12 | — | 27 / 0.30 | 36 / 0.35, 75% |
| Point-blank Shot Crossbow | Short bow | Attack | 0.3 / 0.7 | Reach 1 | 3 | 12 / 0.30 | 18 / 0.35 | 24 / 0.40 |
| Move Weapons | Types | Times (s) | Aim | Cost I | II |  | III |  |
| Bolt Crossbow (must be loaded) | Attack | 0.3 / 0.6 | First | 5 30 / 0.50 | 45 / 0.55 |  | 60 / 0.60, depth +1 at 40% |  |
| Reload Crossbow | — | — / 1.2 | Self | 0 Loads 1 bolt | Loads 1, dodge 0.0– 0.3 |  | Loads 1, dodge 0.0– 0.3, guard window 0.4–0.8–1.1 def 15 |  |

Reload always loads one bolt. Loading more per Reload made the crossbow the strongest late-game weapon by far (108 damage per second), so its variants gain protection instead. An early Reload is 1.2 s with no defence at all, which is the crossbow's real cost.

## Gauntlet, wand and staff moves

Gauntlets are the fastest stagger family, with positional tricks (push, throw). Wands are quick, cheap spells. Staffs are slow, area-wide spells and team heals, plus a physical quarterstaff line.

## Gauntlets (hand wraps, gauntlets, cestus; unarmed at power 0)

| Move | Weapons | Types | Times (s) | Aim | Cost | I | II | III |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Jab | Gauntlets, unarmed | Attack | 0.2 / 0.45 | Reach 1 | 2 | 6 / 0.20 | 9 / 0.25 | 12 / 0.30 |
| Hook | Gauntlets, unarmed | Attack | 0.4 / 0.8 | Reach 1 | 4 | 12 / 0.35 | 18 / 0.40 | 24 / 0.45 |
| Uppercut | Gauntlets | Attack | 0.6 / 1.1 | Reach 1 | 7 | 20 / 0.70 | 30 / 0.75 | 40 / 0.80, Unflinching |
| Palm Strike | Gauntlets | Attack | 0.45 / 0.9 | Reach 1, push 1 | 5 | 10 / 0.50 | 15 / 0.55 | 20 / 0.60, depth +1 at 50% |
| Catch | Gauntlets | Defend | window 0.05–0.15– 0.3 / 0.6 | Self | 2 | Def 15, skip 0.5 | Def 25, Deflect | Def 35, Deflect, skip 0.6 |
| Throw Wand | Gauntlets, unarmed (wand, rod; | Attack wand also | 0.7 / 1.3 off-hand) | Reach 1; moves the target to the cell behind you if free | 8 | — | 15 / 0.60 | 20 / 0.70 |
| Spark | Wand, rod | Attack (Mana) | 0.35 / 0.7 | First | 2 | 8/ 0.15 | 12 / 0.15 | 16 / 0.20 |
| Arcane Bolt | Wand, rod, magic staff | Attack (Mana) | 0.8 / 1.4 | First | 8 | 22 / 0.20 | 33 / 0.25 | 44 / 0.30, depth +1 at 30% |
| Mend | Wand, rod, magic staff | Heal | 0.6 / 1.2 | Self or Reach 1 ally | 6 | Heal 14 | Heal 21 | Heal 28, sides 50% |
| Ward | Wand, rod, magic staff | Defend (Absorb) | window 0.1– 0.5–0.9 / 1.2 | Self | 4 | Def 15 | Def 22 | Def 30, guard range sides 50% |
| Blink | Wand, rod | Move + Dodge | move 0.3, dodge 0.1–0.3 / 0.7 | Any free cell within 2 | 5 | — | Range 2 | Range 2, dodge 0.0– 0.4 |

## Staff (magic staff; quarterstaff for physical moves)

| Move | Weapons | Types | Times (s) | Aim | Cost | I | II | III |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Blast | Magic staff | Attack (Mana) | 1.0 / 1.8 | First; depth and sides 30%, diagonals 15% | 12 | 30 / 0.40 | 45 / 0.45 | 60 / 0.50, 45% and 25% |
| Mending Circle | Magic staff | Heal | 1.0 / 1.8 | Around: self 100%, all 8 at 60% | 12 | Heal 20 | Heal 30, 80% | Heal 40, 100% |
| Barrier | Magic staff | Defend (Absorb) | window 0.2–0.8– 1.4 / 1.8 | Self + guard range all 8 at 75% | 8 | — | Def 18 | Def 24, 100% |
| Staff Strike | Magic staff, quarterstaff | Attack (Attack stat) | 0.4 / 0.8 | Reach 1 | 2 | 6 / 0.40 | 9 / 0.45 | 12 / 0.50 |
| Sweep | Quarterstaff | Attack | 0.6 / 1.2 | Reach 1, sides 60% | 6 | 12 / 0.50 | 18 / 0.55 | 24 / 0.60, push 1 |
| Pole Vault | Quarterstaff | Move + Dodge | move 0.4, dodge 0.1–0.4 / 0.8 | Fixed 2 forward, stop short | 4 | Dodge 0.1–0.4 | Dodge 0.1–0.5 | Lands any orthogonal of the target cell |

## Weapon feel

The numbers above produce a clear trade. Light weapons get 2.5–3.0 damage per stamina early (5–6.4 late) but stagger for 0.1–0.3 s. Heavy weapons get about 2.0 early (3.7–3.9 late) but hit 2–6× harder per swing and stagger up to 1.4 s. Damage per second stays in a narrow band, so no weapon simply out-damages the others; they differ in commitment, stamina and stagger.

Computed for each subtype's signature move, before Attack/Mana scaling and defence, with a typical armour tax of 2.

| Weapon | Signature move | Early: dmg / hit | Early: dmg / s | Early: stamina per use | Early: dmg per stamina | Early: stagger (s) | Late: dmg per stamina | Late: stagger (s) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Knife | Stab | 11 | 22 | 4 | 2.75 | 0.14 | 5.6 | 0.21 |
| Stiletto | Stab | 12 | 24 | 4 | 3.00 | 0.12 | 6.4 | 0.18 |
| Rapier | Slash | 23 | 23 | 9 | 2.56 | 0.22 | 4.7 | 0.30 |
| Sabre | Slash | 24 | 24 | 9 | 2.67 | 0.26 | 5.0 | 0.34 |
| Arming sword | Slash | 25 | 25 | 11 | 2.27 | 0.30 | 4.6 | 0.40 |
| Long sword | Overhead Cleave | 42 | 25 | 20 | 2.10 | 0.69 | 3.9 | 0.80 |
| Claymore | Crushing Overhead | 63 | 26 | 31 | 2.03 | 1.26 | 3.7 | 1.40 |
| Short spear | Thrust | 26 | 22 | 11 | 2.36 | 0.36 | 4.6 | 0.45 |
| Spear | Thrust | 29 | 24 | 12 | 2.42 | 0.40 | 5.1 | 0.50 |
| Pike | Long Thrust | 29 | 22 | 14 | 2.07 | 0.40 | 4.4 | 0.50 |
| Short bow | Quick Shot | 15 | 19 | 6 | 2.50 | 0.12 | 5.7 | 0.20 |
| Short bow | Aimed Shot | 31 | 19 | 12 | 2.58 | 0.32 | 4.2 | 0.40 |
| Longbow | Aimed Shot | 35 | 22 | 14 | 2.50 | 0.40 | 4.6 | 0.50 |
| Warbow | Aimed Shot | 40 | 25 | 17 | 2.35 | 0.52 | 4.9 | 0.65 |
| Crossbow | Bolt + Reload | 42 | 23 | 15 | 2.80 | 0.60 | 6.0 | 0.72 |
| Hand wraps | Hook | 14 | 18 | 6 | 2.33 | 0.35 | 4.0 | 0.45 |
| Gauntlets | Hook | 16 | 20 | 7 | 2.29 | 0.42 | 4.4 | 0.54 |
| Cestus | Hook | 18 | 23 | 8 | 2.25 | 0.45 | 4.8 | 0.58 |
| Wand | Spark | 12 | 17 | 5 | 2.40 | 0.09 | 5.3 | 0.15 |
| Wand | Arcane Bolt | 26 | 19 | 11 | 2.36 | 0.12 | 4.0 | 0.18 |
| Rod | Arcane Bolt | 28 | 20 | 12 | 2.33 | 0.16 | 4.3 | 0.24 |
| Magic staff | Arcane Bolt | 30 | 21 | 13 | 2.31 | 0.16 | 4.5 | 0.24 |
| Quarterstaff | Staff Strike | 12 | 15 | 6 | 2.00 | 0.44 | 5.1 | 0.55 |

Late = tier III move with a late-quality weapon (power ×4).

- Bows by draw weight: heavier draw adds damage, stagger and pierce for more stamina (short 2.58 → warbow 2.35 damage per stamina on Aimed Shot).

- Ranged and magic sit slightly lower on damage per second (15–22 early) in exchange for First aim and safety from the back row.

- The crossbow is the stamina-light ranged option. Its cost is time spent in Reload, not stamina.

- These are first-pass numbers. The next step is running each weapon family through the battle simulator to check that no family dominates in real fights.

## Armour types

Armour trades defence and stagger resistance against stamina tax and Mana. Twelve types run from clothes (no tax, almost no protection) to full plate (7 tax, half of all set- back ignored, −15 Mana). Defence is shown at early quality: mid ×2, late ×3.5. Tax, surcharge, resistance and Mana belong to the type.

## Stat meanings

- Defence: passive mitigation, as in the battle system doc.

- Tax: stamina added to every move except Hold.

- Movement surcharge: extra stamina on Move and Dodge moves only. Positive makes repositioning expensive; negative makes it cheaper.

- Stagger resistance: reduces the set-back you take, not the damage animation. Negative means you're set back further.

- Mana: added to or taken from the wearer's Mana.

| Type | Weight | Defence | Tax | Movement surcharge | Stagger resistance | Mana | Suits |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Clothes | None | 2 | 0 | 0 | 0% | 0 | Civilians, disguise |
| Robes | Light | 3 | 0 | 0 | −10% | +8 | Pure casters, healers |
| Martial garb | Light | 5 | 0 | −1 | 10% | 0 | Gauntlet fighters |
| Battle robes | Light | 7 | 1 | 0 | 0% | +4 | Spellblades |
| Padded (gambeson) | Light | 6 | 1 | 0 | 10% | 0 | Early all-rounders |
| Leather | Light | 8 | 1 | 0 | 5% | 0 | Duelists, archers |
| Hide | Medium | 10 | 2 | 0 | 20% | −2 | Wilds fighters, spears |
| Brigandine | Medium | 12 | 2 | +1 | 15% | −2 | Crossbows, sword users |
| Chain mail | Medium | 16 | 3 | +1 | 20% | −5 | Knights, shield users |
| Scale mail | Heavy | 20 | 4 | +1 | 25% | −8 | Front-liners |
| Half plate | Heavy | 24 | 5 | +2 | 35% | −10 | Pikes, long swords |
| Full plate | Heavy | 30 | 7 | +3 | 50% | −15 | Claymores, tower shields |

Why the extra stats:

- Stagger resistance gives heavy armour its identity under the timing rules. You can't make plate slower, so instead it keeps your slow swings from being interrupted. Movement surcharge stops heavy armour from also being the best at repositioning, so dodging by moving stays a light-armour strength.

- Robes' negative resistance makes casters easy to interrupt, which is their weakness and the front line's reason to protect them (guard range).

Stamina per move adds up fast. A claymore in full plate pays 7 + 7 = 14 on every move before the move's own cost, plus 3 more on any Step or Roll. A rapier in leather pays 1 + 1 = 2.

## Example builds

| The same types combine into very different play | styles; stamina tax per move ranges from |
| --- | --- |
| 1 to 14. |  |
| Build Main hand Off-hand Armour | Tax per Plays like move |
| Duelist Rapier Parrying Leather dagger | 2 Parry and Riposte into Lunge; cheap, precise, little stagger |
| Knight Arming Heater Chain mail sword shield | 8 Block and Counter Cut; covers the allies beside them |
| Juggernaut Claymore — Full plate | 14 One or two huge swings per chain, shrugs off interrupts, lives on Hold |
| Ranger Longbow — Leather | 4 Aimed Shots from the back row, Retreating Shot when rushed |
| Crossbowman Crossbow — Brigandine | 5 Bolt, Reload, Hold rhythm; vulnerable while reloading |
| Pikeman Pike — Half plate | 9 Controls Reach 2 and the cell behind it; weak up close |
| Skirmisher Stiletto Parrying Leather dagger | 1 Slip Strike to flank, Backstab from behind |
| Monk Cestus — Martial garb | 2 Jab-Hook-Uppercut stagger chains, Throw to reposition enemies |
| Spellblade Sabre Wand Battle robes | 3 Slash up close, Spark at range, Ward when pressed |
| Healer Magic staff — Robes | 3 Mending Circle and Barrier; easy to interrupt, needs a guard |
| Warden Short spear Tower shield Scale mail | 10 Shield Wall and Guarded Thrust; protects the healer behind |

## New rules and open questions

Several types need small rules the battle system doc doesn't have yet. Each is listed so it can be accepted, changed or cut.

## New rules introduced

1. Off-hand slot. Main hand + off-hand + armour taxes stack on every move except Hold.

2. Shield guard bonus and guard range come from the shield, which depends on the guard-range proposal in the battle system doc.

| 3. | Stagger resistance (armour): reduces set-back, not the damage animation. |
| --- | --- |
| 4. | Movement surcharge (armour): extra or reduced stamina on Move and Dodge moves. |
| 5. | Crossbow loaded state: Bolt needs a loaded bolt; Reload loads one. |
| 6. | Facing away for Backstab: the attacker is behind the target relative to its facing. |

- Needs the turn-to-face rule settled.

| 7. | Throw moves an enemy to the cell behind the thrower, the first move that relocates someone else. |
| --- | --- |
| 8. | Pike close-range penalty: Reach 1 moves deal 50%. |
| 9. | Quality scaling: weapon power ×1 / ×2 / ×4 and armour defence ×1 / ×2 / ×3.5 from |

- early to late quality; tax never scales.

10. Variant scaling: damage ×1 / ×1.5 / ×2 and cost ×1 / ×1.25 / ×1.5, with timings locked.

## Open questions

- Is dual-wielding wanted beyond the parrying dagger and off-hand wand?

- Should the long sword's one-handed mode exist, or stay two-handed only?

- Does the quarterstaff count as a magic staff for Mana moves, or stay purely physical? Should unarmed characters get Jab and Hook at power 0, or only universal moves? Can Throw send an enemy into the thrower's own half (crossing them over)? Run each weapon family through the battle simulator to check the weapon-feel numbers hold in real fights.
