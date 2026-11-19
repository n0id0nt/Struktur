# Seven Gods RPG — Animation-Timed Battle System Design

## Overview

Combat is a shared timeline of fixed-length animations on a 3 × 4 arena. Each combatant commits to a chain of moves (at least 2, roughly 4 s long) at a time, and skill comes from timing those chains against what the enemy has committed to.

This replaces the tick-based system in comprehensive_combat_design.md . Four things from that doc are removed outright:

- Speed as a stat or modifier. No weapon, armour, buff or exhaustion state changes a move's duration.

- Target selection. Attacks and heals hit cells, not characters.

- Abstract time units. All timings are authored in seconds and match the animation exactly. Implementation maps them to frames.

- Frozen animations. Nothing ever pauses mid-animation. Stagger plays the move's own damage animation, then the move resumes from a set-back point.

## Design pillars

1. Commitment is readable. Once a move starts, its impact and completion times are known to both sides.

2. Timing beats stats. A well-timed block, dodge or Hold should beat a bigger number.

3. Positioning is light, timing is heavy. The arena is deliberately shallow; movement sets up timing rather than replacing it.

4. Loadout is a plan. 6 learned moves, weapon, armour and their effects push each combatant toward a play style.

5. Solvable is fine. Full information about committed moves is intended. Depth comes from planning a whole chain ahead under that information.

## Core loop: chains on a shared timeline

Every combatant plays a committed chain of moves back to back, and the clock stops only when someone's chain runs out and they must pick the next one. Chains are measured in time, not move count, so slow movesets aren't punished by committing to 3 long moves. All timings are in seconds.

1. Battle starts at a selection screen. Every combatant builds a chain.

2. The clock runs. All chains play simultaneously.

3. When a combatant's last move completes, the clock stops and that combatant builds their next chain. AI picks instantly.

4. The clock resumes. Repeat until one side is defeated.

The selection stop is the only time the clock stops. No animation ever freezes mid-move.

## Chain rules

- Chain budget: a chain has at least 2 moves, and the player keeps adding moves until their authored lengths add up to the budget (4 s placeholder, final value set by tuning). The move that reaches or crosses the budget is the last one. A fast moveset fills it with many short moves; a slow one with 2.

- Choices are the combatant's 6 loadout moves, Hold, and Use Item.

- No cancelling. A chain always plays out, however badly it's going. A bad plan is punished, not rescued.

- Hold can't follow Hold, including across a chain boundary.

- Stamina is checked at selection. A chain is legal if its total cost is no more than current stamina plus what earlier Holds in the chain will recover. Each move pays when it starts.

- If a move can't be paid when it starts (e.g. Gold drained you mid-chain), it becomes a Falter: a 0.5 s stumble with no effect.

- Simultaneous completions: everyone whose chain ends at the same moment picks blind.

## What a combatant can see

Everyone sees each combatant's active move (name, types, target cells, impact time, defensive or dodge window, time remaining), plus HP, stamina and position. How much of an enemy's chain is visible depends on the observing combatant's Read level, a property gained through learned skills:

| Read | Sees of each enemy's chain |
| --- | --- |
| 0 | Active move only |
| 1 | Active move + next move |
| 2 | The whole chain |

## Hold

Hold is always available. The player picks its length: 0.2, 0.4, 0.6, 0.8, 1.2 or 2.0 s.

- No types and no stamina cost. It's the one move that skips the weapon and armour tax.

- Stamina only recovers during Hold, at the combatant's Stamina recovery rate. Being hit during Hold only plays the Hold damage animation, so the Hold ends that much later. There's no set-back.

## Move timeline anatomy

| 0 | s | -------- | impact | time | ------- | completion | time |
| --- | --- | --- | --- | --- | --- | --- | --- |
| [ 0 | s | wind-up ---- start 0% | ][ hit ^ damage, ---- ramp | ][ peak 100% | recovery heal or item ---- end ramp 0% | ] effect lands ---- completion | here |

## Stagger: damage animation, then resume

Every move defines its own damage animation with a fixed length (e.g. 0.3 s for a jab, 0.5 s for a heavy swing). When a hit staggers a combatant, the damage animation plays, then their move resumes forward from a set-back point. The attacker's stagger S (in seconds) decides how far back:

| Target is… | Reaction (default) |
| --- | --- |
| Before impact time | Damage animation, then resume S earlier, never before 0 s |
| Past impact time | Damage animation, then resume S earlier, never before impact time. The impact does not fire again. |
| Inside a defensive window | Skip forward by the move's own Skip time, never past completion. No damage animation. |
| In a defend move's recovery | Damage animation, then resume S earlier, never before the window's end, so the window never reopens |
| In Hold | Damage animation only. No set-back. |
| Inside a dodge window | Not hit |

Time lost to a stagger = damage animation length + set-back. A 0.4 s set-back on a move with a 0.3 s damage animation costs 0.7 s.

The defensive skip is a fixed amount per move, not tied to the attacker. A blocked hit eats into the rest of your defence but brings your next move sooner. A long guard with a short skip can absorb several hits; a parry with a long skip is a one-shot tempo gain. Each move can override these defaults through its Hit reaction field (see Moves). Diminishing returns apply to the set-back. Each stagger received within 2 s of the last sets back 50%, then 25%, then 0%. The damage animation still plays each time.

## Arena

The base arena is 3 lanes × 4 rows, with each side starting on its own 2 rows. It's kept shallow on purpose so positioning sets up timing instead of becoming the fight.

| Lane | Ally back | Ally front | Enemy front | Enemy back |
| --- | --- | --- | --- | --- |
| A | A1 | A2 | A3 | A4 |
| B | B1 | B2 | B3 | B4 |
| C Starting layout: everyone starts in their side's back row (row 1 for allies, row 4 for enemies), one per lane where possible. Custom sizes: an encounter can set any lanes × rows and which rows belong to each side. Large enemies take up a footprint of several cells, and each cell can have its own hit zone (e.g. a boss's weak point). One occupant per cell, except multi-cell footprints. | C1 | C2 | C3 | C4 |

## Movement

Every movement move has a destination pattern on top of its range, which controls how much choice the player gets:

| Pattern | Player picks? | Example |
| --- | --- | --- |
| Fixed | No: one forced destination | Dash: exactly 2 cells forward |
| Any within range | Yes: any cell in range | Quick Step: 1 cell in any of 8 directions |
| Restricted set A legal destination is unoccupied and not claimed. A cell is claimed as soon as another combatant's movement move targeting it starts, and released when that combatant arrives. Either side may move into the other side's half if the cell is free. Legality is checked again at the move time. Each move also sets a blocked behaviour: Stay (default, the mover doesn't move) or Stop short (fixed paths like Dash stop at the last free cell along the path). Stamina is spent either way. | Yes: only listed offsets | Slip: 1 cell diagonally only. Shuffle: 1 lane left or right only. |

## Targeting is positional

Nobody picks a target or a direction: every attack, heal or thrown item has a fixed aim, and its stencil decides which cells it covers.

- Reach N hits the cell N rows ahead in the user's lane. Reach 1 is the adjacent cell. First hits the nearest occupied cell ahead in the lane.

- Around uses a stencil centred on the user, so it can cover any mix of directions: behind, beside, ahead or all of them. This is how spins, back kicks and two-way cleaves work.

- Self, for personal heals and buffs.

| Around example | Cells covered |
| --- | --- |
| Spin Slash | All 8 neighbouring cells at 100% |
| Back Kick | The cell directly behind |
| Twin Cleave | The cells directly ahead and behind |
| Shoulder Check | The cells left and right in the same row |

Around attacks are how a combatant deals with someone who has crossed behind them. Damage hits enemies only, and heals hit allies only. The cells are locked when the move starts, but resolve against whoever is in them at impact time, so moving out of a telegraphed cell dodges it.

## Moves

A move is a data record tied to one animation plus its damage animation. Its type flags (Attack, Defend, Dodge, Heal, in any combination) decide which other fields are required.

## Loadout

- Each combatant brings 6 moves into battle, plus Hold and Use Item, which everyone always has.

- Moves are learned in the world, e.g. taught by NPCs. A learned move can only be put in the loadout while a compatible weapon is equipped.

- The loadout and gear are locked for the battle.

## Move fields

| All times are in seconds | from the start | of | the move. |
| --- | --- | --- | --- |
| Field | Required when |  | Notes |
| Types | Always |  | Any combination of Attack, Defend, Dodge, Heal |
| Completion time | Always |  | Total animation length. Never modified. |
| Damage animation | Always |  | Its own animation and fixed length, played when staggered |
| Stamina cost | Always |  | Base cost. Weapon and armour tax are added on top. |
| Hit reaction | Always |  | Default by type (Core loop table), or a per-move override |
| Compatible weapon type | Optional |  | e.g. Sword, Spear, Hammer, Bow, Wand, Staff |
| Movement | Optional |  | Range, destination pattern, blocked behaviour, move time |
| Base damage | Attack |  | Can be 0 for pure stagger moves |
| Base heal | Heal |  | HP restored at 100% falloff |
| Impact time | Attack, Heal |  | Before completion |
| Stagger (s) | Attack |  | Set-back applied to every enemy hit, scaled by falloff |
| Aim + stencil | Attack, Heal |  | Reach N / First / Around / Self; % per covered cell |
| Base defence | Defend |  | Value at peak |
| Defensive window | Defend |  | Start, peak and end times |
| Skip time | Defend |  | How far the move jumps forward when it blocks a hit Page 7 of 24 |
| Field | Required when | Notes |  |
| Dodge window | Dodge | Start and end times. Full evasion inside, none outside. |  |

## Types

- Attack — physical with a physical weapon (scales with Attack), or a spell with a magic weapon (scales with Mana).

- Defend — guard or parry. Defence ramps linearly 0% → 100% at peak → 0%.

- Dodge — a binary evasion window.

- Heal — restores HP to allies in the stencil at impact time. Needs a magic weapon and scales with Mana.

Types combine freely, e.g. a shield bash (Attack + Defend), a lunge (Attack + Dodge + movement), or a warding heal (Heal + Defend). Each component keeps its own window.

## Guard range (proposed)

A Defend move can have a guard range: a stencil of ally cells it also protects, relative to the user, with a % per cell. When an ally in a covered cell is hit during the guard's window, the stronger of the ally's own block and the cover applies (guard defence × g × cell %). If the cover is used, the guarder takes the skip and the ally isn't staggered.

| Example | Guard range |
| --- | --- |
| Guard, Shield Bash (shield wall) | Cells either side, 75% |
| Brace (bodyguard) | Cell directly behind, 100% |
| Guarded Hold | Cells either side, 50% |
| Riposte (parry) | None: stays a personal timing skill |

## Hit reaction overrides

Most moves use the defaults. Overrides give individual moves their own character:

| Reaction | Behaviour | Example use |
| --- | --- | --- |
| Set back (default) | Damage animation, then resume S earlier (clamped) | Most attacks, heals, items |
| Skip (default in window) | Jump forward by the move's Skip time | Guards |
| Steadfast | Set-back at 50% | Heavy two-handed swings |
| Unflinching | Damage animation only after impact time, no set-back | Committed finishers |
| Deflect | Skip, and the attacker takes S as a set- back | A parry, at its peak only |
| Absorb | No skip; S becomes stamina | Mage wards |

## Range stencils

For Reach and First aims, the aim cell always takes 100% and the stencil lists other cells relative to it: +1 depth is one cell further along the lane, ±1 side is the neighbouring lanes. Around stencils list cells relative to the user instead.

| Example | Aim | Depth +1 | Side ±1 | Diagonal / other |
| --- | --- | --- | --- | --- |
| Sword cut | Reach 1 | — | — | — |
| Spear thrust | Reach 1 | 50% | — | — |
| Hammer slam | Reach 1 | — | 40% | — |
| Fireball (staff) | First | 30% | 30% | 15% diagonals |
| Spin Slash | Around | — | — | All 8 neighbours 100% |
| Mending Light (wand) | Around, allies | — | — | All 8 neighbours 60% |

Falloff scales damage, healing and stagger alike.

## Weapons, armour and stats

Gear sets element, power, mana, a stamina tax on every move, and two custom effects. It never touches move timing.

This doc reads "custom effect" listed twice as two slots, a primary and a secondary effect, both drawn from the shared pool.

## Weapon

| Field | Purpose |
| --- | --- |
| Weapon type | Gates which moves can be equipped. Physical: Sword, Spear, Hammer, Bow. Magic: Wand, Staff. |
| Element | One of the seven elements |
| Base power | Added to the move's base damage or base heal |
| Stamina cost | Added to every move except Hold |
| Range modifier | A stencil merged onto the move's stencil |
| Primary / secondary effect | From the pool, weapon versions |

Range modifier rule: take the higher % per cell. A modifier can add cells or raise %, but it never changes the aim cell or lowers a value.

## Armour

| Field | Purpose |
| --- | --- |
| Element | Resistances, plus synergy with the weapon element |
| Base defence | Passive mitigation, always on |
| Mana modifier | Optional. Only some armour has one, and it can raise or lower Mana (e.g. +15 on robes, −10 on plate). |
| Stamina cost | Added to every move except Hold, including movement |
| Primary / secondary effect | From the pool, armour versions |

Because both taxes apply to every move, guarding and repositioning are never free. Movement moves get a base cost of 0–2 so their total stays low.

## Combatant stats

| Stat | Effect |
| --- | --- |
| Attack | Scales physical damage: × (1 + Attack / 100) |
| Mana | Scales spell damage and healing: × (1 + Mana / 100). Magic moves use Mana in place of Attack and still cost stamina. |
| Defence | Passive mitigation, added to armour base defence |
| Health | HP pool |
| Stamina | Maximum stamina |
| Stamina recovery | Stamina per second during Hold |

There is no Speed stat, and nothing else can scale time.

## Stamina economy

Every move except Hold pays its base cost plus both gear taxes, and stamina comes back only during Hold. Hold is therefore both the timing tool and the refuel.

| \text{Cost} | = | \text{Move | cost} | + | \text{Weapon | cost} | + | \text{Armour | cost} |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| \qquad (\text{Hold} Chain cost is checked at selection, counting what earlier Holds in the chain will recover. Each move pays when it starts. Because Hold can't follow Hold, at most every other move in a chain can be a Hold. No-deadlock constraint (tuning rule): a 2.0 s Hold at the lowest Stamina recovery in the game must recover at least the cost of the cheapest move plus maximum gear tax. That guarantees At 0 stamina, a combatant takes +50% stagger set-back. |  | = 0) Hold → | move → |  | Hold is always legal. |  |  |  |  |

## Items

Using an item is a move like any other. It has its own animation, and the item's effect lands only at its effect time.

- Use Item is always available in a chain slot, alongside Hold. Picking it opens the inventory.

- Each item type has a completion time, an effect time, a damage animation, an aim (Self, Reach N, First or Around) and a stencil, so thrown items work positionally. It pays the weapon and armour tax like any move.

- The item is consumed at its effect time. If the user is defeated before then, the item isn't lost.

- There's no per-battle limit. Items are kept in check by gold cost and use time: healing items should be expensive enough that they don't replace healers.

| Item | Completion | Effect time | Aim | Effect |
| --- | --- | --- | --- | --- |
| Health Potion | 1.3 s | 1.0 s | Self | Heal 30% max HP |
| Thrown Tonic | 1.5 s | 1.1 s | Reach 1 ally | Heal 25% max HP |
| Fire Flask | 1.7 s | 1.1 s | First; side ±1 30% | 20 damage + Burn |
| Smelling Salts | 1.0 s | 0.7 s | Self | Clears statuses |

## Elements

Each of the seven elements belongs to one of the seven gods, so a loadout's elements say whose power a character is drawing on. Weapons apply the element's offensive effect on hit, armour gives its defensive effect, and the pairing of the two creates a third.

## The seven elements

| Element | God | Theme | Replaces |
| --- | --- | --- | --- |
| Flame | Isaren | Revolution that spreads | Fire |
| Frost | Maioren | Stasis, the past holding on | Ice |
| Storm | Noeth | Precision, chained logic | Lightning |
| Iron | Kratheon | Order, force, immovability | Earth |
| Wild | Agria | Untamed movement, adaptation | Wind |
| Light | Kerys | Faith, healing, certainty | Light |
| Gold Element effects | Ploutar | Extraction, everything has a price | Dark |
| Element | Weapon effect (on hit) | Armour effect | Matched set (weapon = armour) |
| Flame | Burn: 2% max HP per second for 3 s, stacks to 3 | Hits that land during your defensive window at ≥ 75% Burn the attacker | Burn ticks also drain 2 stamina |
| Frost | Chill: −50% stamina recovery for 4 s | Attackers that hit you are Chilled | 3 Chill stacks → Frozen: next set-back taken × 2 |
| Storm | Arc: 30% of damage jumps to cells beside the aim cell | A successful dodge stores 1 Charge (max 3); your next attack gets +20% stagger per Charge | Arcs carry stagger too |
| Iron | Quake: +30% stagger | +25% defence at peak | Set-backs you take are halved while any Defend move is active |
| Wild | Gust: pushes the target 1 cell directly away from the attacker if the cell is free | Your first movement move after a dodge costs no armour stamina | A Gust into an occupied or edge cell deals +50% stagger instead |
| Light | Radiance: heal 10% of damage dealt | A block within 0.1 s of peak heals 5% max HP | Overheal becomes a shield that lasts 5 s |
| Element | Weapon effect (on hit) | Armour effect | Matched set (weapon = armour) |
| Gold | Drain: steal 5 stamina per hit | Attackers that hit you pay +20% stamina on their next move | Stamina stolen above your max becomes bonus damage on your next hit |

## Cross-element pairs

Some mismatched pairs unlock a named synergy, which rewards building around two gods. Draft set:

| Weapon | Armour | Synergy |
| --- | --- | --- |
| Flame | Wild | Wildfire: when you move, Burn spreads from any burning enemy beside you to its neighbouring cells |
| Frost | Iron | Permafrost: Chilled attackers that hit your guard take the stagger they would have dealt |
| Storm | Wild | Stormstep: a dodge that moves you also fires a 0- damage Arc that staggers |
| Iron | Flame | Forge: Quake hits on Burning targets refresh Burn to full |
| Light | Gold | Tithe: Drain also heals; Radiance also steals stamina |

## Elemental reactions (across the party)

Statuses from different allies react when they meet on one target. The reaction consumes both statuses:

- Burn + Chill → Thermal Shock: burst damage + 1.0 s stagger

- Arc onto a Chilled target → Shatter: 2× damage on that arc

- Gust onto a Burning target → Firestorm: Burn spreads to the cell the target is pushed toward

## Resistances

If the armour's element matches the incoming element, that damage is reduced 50%. Opposed gods take +50%: Flame ↔ Frost (change vs permanence), Storm ↔ Iron (reason vs force), Light ↔ Gold (faith vs greed). Wild has no opposite: Agria stands outside the human order the other gods argue over.

## Custom effect pool

Each effect in the pool has a weapon version and an armour version built on the same idea, so mixing gear with the same effect name reads as one build.

Hard rule for every effect: it may change damage, defence, stagger, stamina, windows' strength, statuses or position. It may never change a move's completion, impact or move time, or its playback speed. It may change the size of a set-back or skip.

| Effect | Weapon version | Armour version |
| --- | --- | --- |
| Riposte | Attacks that start within 0.7 s of your block peak deal +30% | A block at peak returns 50% of the blocked hit's stagger to the attacker |
| Momentum | +10% damage per consecutive hit landed, max +40%; resets when you're hit | +10% defence per consecutive successful block, max +40% |
| Opportunist | +50% damage to targets that are in recovery (after impact) | −50% stagger taken while you're in recovery |
| Vanguard | +25% damage from the front row | +20% base defence while in the front row |
| Skirmisher | +25% damage on moves that include movement | Movement moves cost −50% armour stamina |
| Bloodthirst | Heal 5% of damage dealt | Heal 2% max HP on each successful dodge |
| Breaker | +100% damage against targets in a defensive window | Attacks that hit your guard lose 25% of their stagger |
| Tempo | Landing a hit refunds 30% of the move's stamina cost | A successful dodge refunds 50% of the dodge's stamina cost |
| Heavy | +40% stagger, +25% weapon stamina cost | −40% stagger taken, +25% armour stamina cost |
| Guardian | Hits on an enemy in your lane give the ally behind you +15% defence for 3s | When an ally in your lane is hit, you gain 10 stamina |
| Last Stand | +2% damage for each 10% of HP missing | +2% defence for each 10% of HP missing |
| Precision | Damage outside the aim cell × 1.5 | Hits outside the attacker's aim cell deal 0 to you |

Tier gear by effect magnitude (e.g. Momentum I/II/III) instead of adding new effects, so the pool stays learnable.

## Resolution

Effects resolve at impact time against every occupied cell in the stencil: dodge first, then passive defence, then the active block. Stagger then becomes a set-back or a skip depending on the target's state.

## Damage and healing

P is Attack for physical moves and Mana for magic moves.

| \text{Raw} | = | (D_{move} | + | D_{weapon}) | \cdot | \left(1 | + | \frac{P}{100}\right) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| \cdot \text{Falloff} |  |  | \cdot | \text{Element} |  | \cdot | \text{Effects} |  |
| \text{Final} Def_{armour}} \text{Heal} \cdot \text{Falloff} | = | = \max\left(0,\ \; - \; (H_{move} | Def_{move} + \cdot | \text{Raw} \cdot H_{weapon}) \text{Effects} | \cdot g(t)\right) \cdot | \frac{100}{100 \left(1 | + | + Def + \frac{Mana}{100}\right) |

g(t) is the defensive curve at the current time: 0 outside the window, 1 at peak, linear in between. A defender inside a dodge window takes no damage, stagger or on-hit effects. The flat block subtraction is kept deliberately. Fast chip moves are weak into a guard, but they're low-risk and keep forcing the opponent into stagger. Battle simulations will confirm the balance.

## Stagger

| S | = | S_{move} | \cdot | \text{Falloff} | \cdot | \text{Effects} | \cdot | DR | \quad |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| \text{(seconds)} | Set-back = min(S, progress − floor). The floor is impact time once passed, the window end for a defend move in recovery, otherwise 0 s. Time lost = damage animation length + set-back. In Hold, time lost = damage animation length only. Skip (inside a defensive window) = min(the move's Skip time, completion − progress). It doesn't depend on S or on block quality. DR is the diminishing-returns factor on set-back: 1, 0.5, 0.25, 0 for staggers within 2 s of each other. |  |  |  |  |  |  |  |  |

## Order of operations at each moment

1. Apply movement for every move reaching its move time.

2. Resolve every impact or effect time (attacks, heals, items) against the updated positions. Simultaneous impacts all land.

3. Apply hit reactions (damage animation + set-back, or skip) and on-hit statuses.

4. Tick statuses. Add stamina for anyone in Hold.

5. Remove defeated combatants. Their cells become empty.

6. Start the next move in any chain whose current move completed. If a chain's last move completed, stop the clock for selection.

Movement resolving before impact means stepping out at the exact impact time still escapes.

## Progression

Each tier adds new kinds of decision, not just bigger numbers: early moves teach timing, mid moves combine types and free movement, and late moves bend the rules (hit reactions, multi-direction stencils). Gear and monsters step up on the same ladder. These are the tiers used in the simulation (see the Simulation tab).

## Move complexity by tier

| Tier | What's new | Example moves |
| --- | --- | --- |
| Early | Single-type moves. Fixed movement. Reach 1 or First only. | Slash, Jab, Guard, Thrust, Brace, Firebolt, Mend, Step Forward/Back, Sidestep |
| Mid | Combined types (Attack + Dodge, Attack + Defend). Free movement patterns. Reach 2. First hit-reaction override (Absorb). | Lunge, Shield Bash, Long Thrust, Ward, Quick Step (any of 8), Sidestep Dodge |
| Late | Hit-reaction overrides (Deflect, Steadfast, Unflinching). Around stencils. Restricted or forced movement. | Riposte, Vaulting Thrust, Quake Ring, Spin Slash, Twin Cleave, Sanctuary, Slip (diagonal), Dash (2 forward, stop short) |

## Weapons and armour by tier

| Tier | Weapon | Armour |
| --- | --- | --- |
| Early | Base 4–6, tax 2–3. No element, no effects. | Defence 3–5, tax 1–2. No element. A robe may give +5 Mana. |
| Mid | Base 10–13, tax 3–4. Element + 1 effect. | Defence 9–15, tax 2–4. Element + 1 effect. Mana modifiers appear (−5 to +12). |
| Late Combatant stats by tier | Base 20–28, tax 4–6. Element + 2 effects. | Defence 18–30, tax 3–5. Element + 1–2 effects. Mana −10 to +20. |
| Tier | HP Attack / Mana Defence Stamina Recovery /s |  |
| Early | 100 10 5 80 12 |  |
| Mid | 180 25 15 110 16 |  |
| Late | 280 45 30 140 20 |  |

## Monsters by tier

Monsters follow the same move ladder, so a late monster plays with the same tools as a late player.

| Tier | Monsters (from the existing design docs) | Signature |
| --- | --- | --- |
| Early | Thornwolf, Sewer Brute, Twisted Sprite, Stone Guardian (elite) | Fast bites and pounces; one slow telegraphed slam; evasive flyers; a long- window guard |
| Mid | Iron Legionnaire, War Shade, Stone Colossus | Player-style kits (Lunge, Shield Bash); Attack + Dodge strikes; Around attacks |
| Late | Primal Hunter, Logic Engine | Riposte, Spin Slash, Flurry; Read 1–2 (sees your chain) |

## Keeping early monsters relevant

Stats alone make a monster two tiers below the party harmless: a careless late party loses only 1–3% HP to an early pack. Early monsters should therefore carry at least one threat that ignores stats:

- Stagger is already stat-independent, so a wolf can still interrupt a late hero's slow move.

- % max-HP statuses (e.g. a Bleed: 1% max HP per second for 3 s, stacking to 3) only land on hits that weren't blocked, dodged or interrupted. In simulation this kept careful late parties untouched (≤ 0.4% HP) while careless ones lost 7.5% median against the early elite.

- Put these on selected early monsters only. On every early monster, Bleed made early- game fights noticeably harsher for early parties.

## Worked example

A spear user plans a 4 s chain blind, takes a slow Brute's slam for 4 damage instead of 42 by landing her guard on peak, and the guard's skip ends her chain early enough to punish the Brute's recovery.

Setup. Chain budget 4 s. Player: Attack 20, Defence 10, Stamina 100, recovery 10/s. Flame spear (base 10, tax 4) and Iron armour (defence 15, tax 6), so +10 stamina on every move. Brute: Attack 30, Defence 20, armour defence 10, no element. Both start in lane B, back row.

| Move | Types | Base | Key times | Completion | Stagger | Damage anim | Cost (player) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Step Forward | Move (fixed 1 forward) | — | move 0.4 s | 0.7 s | — | 0.3 s | 2 + 10 |
| Spear Thrust | Attack | 25 | impact 0.6 s | 1.2 s | 0.4 s | 0.3 s | 10 + 10 |
| Guard | Defend | def 30 | window 0.1–0.6– 0.9 s, skip 0.4 s | 1.3 s | — | 0.3 s | 6 + 10 |
| Overhead | Attack | 40 | impact 1.8 s | 3.0 s | 0.8 s | 0.4 s | — |

## Slam

Chains built at 0 s (blind):

- Player (4 moves, 4.0 s): Step Forward → Spear Thrust → Hold 0.8 s → Guard

| Brute (3 moves, 4.9 s): Step Forward → Overhead Slam → Hold 1.2 s. The Hold | crosses |
| --- | --- |
| the budget, so it's the last move. |  |
| Time Event |  |
| 0.7 s Both steps complete. Thrust and Slam start (slam impact due 2.5 s). |  |
| 1.3 s Thrust lands: (25 + 10) × 1.2 = 42 raw, × 100/130 = 32 damage, Burn applied. The Brute plays the slam's |  |
| 0.4 s damage animation, then resumes 0.4 s earlier, at 0.2 s progress. Impact moves to 3.3 s, completion to 4.5 s. |  |
| 1.9–2.7 s Thrust completes. Hold 0.8 s recovers 8 stamina. |  |
| 2.7 s Guard starts (peak due 3.3 s). |  |
| 3.3 s The slam hits the guard exactly on peak: g = 1. 52 raw → 41.6 after defence → − 37.5 block (30, +25% from Iron armour at peak) = 4 damage. The guard skips 0.4 s and ends at 3.6 s instead of 4.0 s. |  |
| Time Event |  |
| 3.6 s The player's chain is done and the clock stops. The |  |
| Brute is in recovery until 4.5 s. She builds: Spear |  |
| Thrust → Hold 0.8 s → Guard → Hold 0.8 s (4.1 s). |  |
| 4.2 s Thrust hits the Brute in recovery (slam at 2.7 s progress). The set-back stops at the impact time: |  |
| 0.4 s back to 2.3 s, so the slam can't hit again. |  |
| Damage animation 0.4 s. 32 damage, Brute's slam now completes at 5.3 s. |  |

What this shows:

- The whole first chain was a read. The 0.8 s Hold only lined the guard up because the first thrust delayed the slam. With a 0.6 s Hold the guard would be 0.2 s past peak (g = 0.33) and she'd take 29 damage.

- The skip bought the punish. Without it the chain ends at 4.0 s, and the thrust lands at 4.6 s, after the slam has finished and the Brute is in Hold. It still hits, but a Hold takes no set-back.

- The budget treats both fighters fairly. The player used 4 short moves and the Brute 3 long ones, for similar commitment.

## Simulation summary

About 6,400 simulated fights back the core loop: skill matters a lot, no spam strategy beats careful play, and the progression targets mostly hold. They also exposed three rule

| problems with tested fixes. Full report: | ​Simulation |
| --- | --- |
| Crossed stalemate (must fix). With fixed facing, a combatant standing behind enemies who also can't hit it causes endless fights. Turn to face the nearest enemy at each move start removed every timeout. |  |
| Defence barely matters. Careful players block only ~3% of incoming damage and avoid hits by interrupting and stepping out instead. Stronger guards didn't change this. A Guarded Hold (Hold with partial defence, no stamina recovery) raised it to 11– 14% without making turtling viable. Adding guard range on top (guards also protect nearby allies) took it to 15–20%. |  |
| Stops are frequent. With three characters on 4 s chains, the player stops every ~1.9 s. |  |
| Letting party members whose chains end within 1 s of each other pick at one shared stop stretches that to ~2.8 s. |  |
| Progression: a late party beats early monsters 100% of the time, but careless play only costs 1–3% HP, so it isn't yet annoying. One tier down hits the target (careless loses 12–16%). |  |

## Where the design is weak

The structural questions are settled. What remains is mostly tuning for the simulator, plus a few risks the time-budget chains introduce.

## Resolved

| Weakness | Decision |
| --- | --- |
| Players couldn't wait | Hold, 0.2–2.0 s, never two in a row |
| Too many stops | Chains; the clock stops once per chain |
| Slow moves punished by fixed 3-move chains | Chains are time-budgeted: at least 2 moves, about 4 s |
| Chains going stale | No cancelling. A bad chain plays out and is punished. |
| Solvable with full information | Accepted as intended; chain visibility scales with Read |
| No healing or magic | Heal type; magic weapons scale with Mana; some armour raises or lowers Mana |
| Stagger lengthened defence | Defensive windows skip forward by each move's own Skip time |
| Stagger visuals | Each move has its own damage animation; the move then resumes from the set-back point |
| Turtling was cheap | Both gear taxes on every move except Hold |
| Shallow arena | Accepted; timing over positioning; either side may cross into free cells |
| Crossing and facing | No aim directions to choose. Around stencils (spins, back kicks, two-way cleaves) cover multiple directions. |
| Move source undefined | 6 learned moves + Hold + Use Item; equipping needs a compatible weapon |
| Items vs heals | Items are expensive and take time; no per- battle cap |
| Party size | Set per encounter, with custom grid sizes |

## Accepted, verify by simulation

1. Flat block vs chip moves. Kept as a defined weakness of fast moves.

2. Loadout combinatorics. Seven god elements × two gear slots × 12 effects. Iron + Heavy + Vanguard is the first suspect for a dominant front-liner.

3. Hold-only recovery and the Hold menu. Strong implicit tempo knobs.

4. Chain budget. 4 s is a placeholder; the sim should sweep it against fight length and stops per minute.

- Recommendation for all four: a headless AI-vs-AI simulator over loadout grids. The rules are deterministic and time-based, so it's cheap to build and batch.

5. UI and readability. Deferred until the rules settle.

## Risks

| 6. | Overrunning the budget. The move that crosses the budget is the last one, so a 3 s slam picked at 3.9 s makes a 6.9 s chain. Slow movesets could use this to stretch commitment, or get stuck in it. Recommendation: allow the overrun for now, and have the sim report the distribution of real chain lengths. If it's a problem, require the last move to start before the budget, or cap the overrun at, say, 1 s. |
| --- | --- |
| 7. | Hold as filler. Players may pad chains with Holds to reach the budget, making chains feel like waiting. The no-consecutive-Hold rule limits this to every other move. |
| 8. | No cancelling makes stagger stronger. With no way out, a combatant hit early in a long chain loses its whole plan. That's intended, but it raises the value of fast stagger moves. |
| 9. | Damage-animation lock. Diminishing returns cut the set-back to 0, but the damage animation still plays on every hit. Three allies jabbing one enemy could keep it in damage animations indefinitely. Recommendation: after the 3rd stagger within 2 s, the target ignores damage animations for 1 s. |
| 10. | Around attacks may dominate. A Spin Slash covers 8 cells at full value, which in a 3 × 4 arena is most of the action. Give Around stencils lower %, higher stamina or longer wind-ups than single-cell equivalents. |
| 11. | Planned movement fails often. Destinations are picked a whole chain early. Fixed patterns like Dash rely on Stop short. Recommendation: the selection UI shows projected positions at each chain step. |
| 12. | Read is very strong. Seeing a whole enemy chain (Read 2) makes most fights trivial. Recommendation: Read 2 is rare and late; bosses can carry a Guile property that lowers enemies' Read against them. |

## Open questions

- Adopt turn-to-face (face the nearest enemy along the lane at each move start)? The simulation says yes: it removes the crossed stalemate.

- Add Guarded Hold so defence has a role?

- Batch party selection (members ending within 1 s pick at one stop)?

- Which early monsters get a % max-HP status like Bleed?

- Are one-tier-up encounters meant to be near-unwinnable?

- Final chain budget (4 s placeholder) and whether the last move may overrun it. Where Read comes from: a learned passive skill, a stat, or gear?

- Damage-animation immunity after repeated staggers (1 s proposed; simulation says optional).

- Item pricing targets relative to healer power.

- Final element names and themes for each god.
