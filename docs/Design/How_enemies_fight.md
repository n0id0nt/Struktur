# Seven Gods RPG — Enemy Types and Combo Patterns

## Overview

Enemies never improvise. Each one picks whole chains from a small, fixed combo menu, always opens the same way, and usually follows a combo with the same next one. A player who has met an enemy before can recognise a combo from its first move and pick the right answer at their very next selection stop.

There are 29 enemy types from the character combat doc, rebuilt for the animation-timed system and grouped into four tabs: Early, Regional (early-mid), Mid, and Late (god domains). Each entry has stats, moves, and a combo menu with weights per battle phase, a usual follow-up, and how to answer each combo.

## What the player learns, per enemy

1. The opener. Every enemy starts every fight with the same combo.

2. The tells. Each combo starts with a move, or a themed Hold animation, that no other combo of that enemy starts with.

3. The rhythm. After a combo, the enemy usually (40–70%) goes into the same follow- up.

4. The phase shift. Below 50% HP, and for some enemies below 20%, the menu weights change in a fixed, learnable way.

Stats were rescaled from the character combat doc to the tiers in the weapon, move and armour types doc, so an early enemy is a fair fight for an early party.

## How enemy behaviour works

An enemy's whole AI is a combo menu plus three small tables: phase weights, a follow-up for each combo, and optional triggers. It never builds chains move by move.

## Combo menu

Each enemy has 2–5 named combos. A combo is a complete, pre-authored chain that obeys the player's chain rules: at least 2 moves, reaching the 4 s budget (the move that crosses it is last), and never Hold after Hold, including across combos. Every chain in this doc was checked against those rules: they run 4.0–5.9 s, median 4.4 s.

## Choosing the next combo

When an enemy's combo ends, it picks the next one in this order:

1. First chain of the fight: always the Opener (the Opening column at 100%).

2. Trigger: if one of its trigger conditions is true (e.g. "an enemy is beside it"), take that trigger's combo at the listed chance.

3. Follow-up: roll the finished combo's follow-up chance (e.g. Worry 60%). On success, take that combo.

4. Phase weights: otherwise pick by the weights for its current phase. Healthy is above 50% HP, Wounded is 50% or below, and Desperate (only some enemies) is 20% or below.

5. Legality: skip any combo that would put a Hold after a Hold, or that can't be afforded. If none can be afforded, the enemy plays its Recover chain (Hold 2.0 s → cheapest move → Hold 2.0 s), which every enemy shares and players learn as a universal opening.

6. No triple repeats: the same combo can't run three times in a row unless it is its own listed follow-up.

## Worked example: Corrupted Thornwolf

The wolf opens with Stalk. After Stalk it goes into Worry 70% of the time; otherwise it rolls Healthy weights (Worry 50%, Hit and Run 50%). So Stalk is followed by Worry 85% of the time (70% + 30% × 50%). A player who sees Lope and a growling Circle at the start knows a Pounce, two Bites and a Leap Back are coming, and that a Bite-heavy combo almost always follows.

## Rules that keep enemies readable

Predictability comes from a few hard rules every enemy follows.

- Unique openers. No two combos of one enemy start with the same move or Hold animation, except the deliberate feint pairs on late enemies. This was checked for all 29.

- Themed Hold animations as tells. Every enemy's Hold is a recognisable idle (the wolf's growling Circle, the Guardian's Grind, the Artillery Demon's targeting glow). A combo that opens with a Hold is identified by it.

- Feint pairs (late game only). Five late enemies have two combos that share an opener and branch on the second move. That tests whether the player has learned the second move, not just the first.

- No cancelling. Enemies obey the same no-cancel rule as the player, so a combo always plays out once started.

- Triggers are positional and visible. The only conditions are things on screen: someone standing beside it, someone guarding in front of it, an ally dying. No hidden state.

- Group sync. Some groups move together (Legionnaires in lockstep, Student Enforcers copying each other, Conformity Wraiths in sync). One enemy's tell then tells you about the whole group.

- Read becomes knowledge. Read still shows more of an enemy's chain, but against these fixed menus, recognising the first move already tells you the whole chain. Read matters most against unfamiliar enemies and late feint pairs.

## Monster moves

Every enemy move is a human move from the weapon, move and armour types doc. Humanoid enemies use the moves directly with real gear. Monsters use reskins: a human move renamed and re-animated to use a body part.

## The reskin rule

A monster move keeps its base move's types, impact, completion and move times, windows, stencil and stamina cost. Only these may change:

- Name and animation (Stab → Bite, Lunge → Pounce, Roll → Leap Back, Spin Slash → Savage Spin).

- Damage source: the monster's natural weapon power replaces weapon power. Scaling stat: Attack or Mana, set per monster (e.g. the Logic Engine's Arc Field is Spin Slash scaled by Mana).

- Element: the monster's attack element applies its on-hit effect.

- Damage animation: each monster has its own.

One monster move has an extra effect: the Sewer Devourer's Toxic Bite adds Bleed, the % max-HP status proposed in the battle simulation. The "Value" column marks any such additions.

## Natural weapons vs gear

| Enemy kind | Weapon | Armour | Stamina tax |
| --- | --- | --- | --- |
| Monster (beast, spirit, construct, fae, demon, swarm) | Natural weapon: power and scaling stat | Defence and stagger resistance built into its stats; element as its body element | Natural tax 0– 2 |
| Humanoid (bandits, guards, soldiers, god- domain humans) | A weapon type, with an off-hand if any | An armour type | Taxes from the types doc |

Humanoid gear quality follows the enemy's tier. Early enemies use early-quality gear, regional and mid enemies mid quality (power and defence ×2), and late enemies late quality (power ×4, defence ×3.5, already shown in their stats).

## How enemy complexity grows

Later enemies don't just hit harder. They have bigger menus, follow up less reliably, use higher-tier move variants, and late ones add feint pairs, so the knowledge the player needs deepens alongside their own kit.

| Tier | Enemies | Combos per enemy | Follow-up chance | Move variants | Feint pairs | Positional triggers |
| --- | --- | --- | --- | --- | --- | --- |
| Early | 5 | 3–4 (avg 3.2) | 50–70% (avg 60%) | I | 0 | 0 (Bandit Leader commands raiders) |
| Regional | 8 | 2–3 (avg 2.9) | 50–70% (avg 59%) | I–II | 0 | 1 |
| Mid | 10 | 3 | 50–70% (avg 57%) | II | 0 | 2 |
| Late Early enemies teach the loop: an opener, two or three combos, and obvious Hold windows to punish. Regional enemies each teach one idea: Bleed (Sewer Devourer), stamina drain (Greed Wraith), splitting (Coinspawn), healing allies (Zealot Spirit), shield coverage (Temple Guard). Mid enemies combine ideas and act as groups (formation, synced Assaults, Hive Mind). Late enemies use the full late move set and feints, and each god domain has a twist: lockstep (Kratheon), stamina theft (Ploutar), faith-armoured casters (Kerys), a repeating loop (Maioren), counter-programming (Noeth) and a desperate frenzy (Agria). Roster All 29 enemies with the opener the player sees first. Full entries Regional enemies Enemy Corrupted Thornwolf Twisted Sprite Stone Guardian Desperate Raider Bandit Leader Enemy | 6 , Tier | 4–5 (avg 4.2) M​ id enemies and Tier Kind Early Beast Early Fae Early Construct (elite) Early Human Early Human (elite) Kind Region | 40–60% (avg 51%) L​ ate enemies . Region Haven forests, borderlands Haven forests Ancient Shrine and old sites Borderlands Borderlands Opener: first two moves | III | 5 | 1 are in ​Early enemies , Opener: first two moves Stalk: Lope → Circle 0.6 s Pester: Pixie Bolt → Pixie Bolt Awaken: Grind 1.2 s → Rumble Step Rush: Step → Lunge Take Command: Rally Cry 1.2 s → Step |
| Battle Wraith | Regional | Spirit Borderlands battlefields | Charge of the Dead: Drift → Phantom Strike |  |  |  |
| Sewer Devourer | Regional | Beast Coinmere sewers | Ambush: Lurk 2 s → Ambush Lunge |  |  |  |
| Zealot Spirit | Regional | Spirit Forest of the persecuted | Sanctify: Divine Shell → Righteous Flame |  |  |  |
| Greed Wraith | Regional | Spirit Coinmere trade roads | Appraise: Count coins 1.2 s → Wealth Drain |  |  |  |
| Coinspawn | Regional | Swarm Coinmere trade roads, treasuries | Scatter: Coin Roll → Swarm Nip |  |  |  |
| City Guard | Regional | Human Coinmere | Halt!: Phalanx Step → Block |  |  |  |
| Temple Guard | Regional | Human Cathedral districts | Sacred Line: Shield Wall → Guarded Thrust |  |  |  |
| Inquisitor | Regional | Human Cathedral districts, forest purges | Accusation: Spark → Spark |  |  |  |
| War Shade | Mid | Spirit Iron March | Form Up: Spectral Wall → Ghost March |  |  |  |
| Artillery Demon | Mid | Demon Iron March | Range Finding: Range Find 1.2 s → Shell Burst |  |  |  |
| Doubt Shadow | Mid | Spirit Noeth's Spire | Question Everything: Critical Question → Critical Question |  |  |  |
| Conformity Wraith | Mid | Spirit Noeth's Spire | Assimilate: Hum 0.8 s → Identity Erasure |  |  |  |
| Cartel Soldier | Mid | Human Trade cities | Assess: Hanging Guard → Half-sword Thrust |  |  |  |
| Debt Collector | Mid | Human Port towns, Coinmere slums | Shakedown: Jab → Jab |  |  |  |
| Enemy | Tier | Kind Region | Opener: first two moves |  |  |  |
| Professional Soldier | Mid | Human Iron March | Formation: Phalanx Step → Block |  |  |  |
| Deserter | Mid | Human Iron March, Borderlands | Pick Off: Step → Aimed Shot |  |  |  |
| Student Enforcer | Mid | Human Noeth's Spire | Youthful Energy: Pole Vault → Staff Strike |  |  |  |
| Educational Inspector | Mid | Human Noeth's Spire | Assessment: Barrier → Arcane Bolt |  |  |  |
| Iron Legionnaire | Late | Human Industrial fortress (Kratheon) | Phalanx: Shield Wall → Guarded Thrust |  |  |  |
| Golden Servant | Late | Human Sky palace (Ploutar) | Appraisal: Parry → Riposte |  |  |  |
| True Believer | Late | Human Cathedral of souls (Kerys) | Proclamation: Barrier → Blast |  |  |  |
| Eternal Guard | Late | Spirit Ancestral throne (Maioren) room | Coronation: Blade Block → Pommel Strike |  |  |  |
| Logic Engine | Late | Construct Tower of perfect (Noeth) logic | Initialise: Compute 1.2 s → Paradox Bolt |  |  |  |
| Primal Hunter | Late | Beast Primal grove (Agria) | Scent: Prowl → Prowl |  |  |  |

## New rules and open questions

A few enemy traits need rules the other docs don't have yet; the biggest open question is how much the player should be shown about combos they've learned.

## New rules introduced

1. Combo-menu AI (this doc's core): opener, triggers, follow-ups, phase weights, legality, no triple repeats.

2. Recover chain: every enemy's fallback when it can't afford a combo.

3. Spawning: Coinspawn's Divide creates a new combatant if it isn't damaged during the Hold.

4. Death effect: the Artillery Demon bursts on all 8 cells around it when defeated.

5. Cross-enemy commands and sync: Rally Cry, Coordinated, Peer Pressure, Hive Mind, Perfect Order, Backup.

6. Movement limits: Ancient Duty (stay within 1 cell), Sacred Duty (front row only), Flier (back row only).

7. Loop behaviour: the Eternal Guard's Precedent replaces random choice with a fixed repeat when Wounded.

## Open questions

- Should a bestiary fill in combo names and chains as the player sees them, and show the recognised combo name in battle?

- Are follow-up chances right for the feel you want? Higher makes enemies easier to read, lower makes them feel smarter.

- Should elites and bosses use the same system with more combos, or get scripted phases (like the boss doc's attack patterns)?

- Do allied NPCs (Thomas, Village Guards, Reformed Bandits) use combo menus too when AI-controlled?

- Run each enemy through the battle simulator with its combo menu replacing the planner AI, to check difficulty per tier.
