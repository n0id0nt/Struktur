# Seven Gods RPG — Elements and Custom Effects

## Overview

Elements and custom effects are the two layers gear adds on top of weapon and armour types. Elements say which god's power a character draws on and create statuses that combine across the party. Custom effects are build modifiers that grow from rank I to III as the player nears the late game. Neither may ever change a move's timing. This builds on the battle system doc (rules, stagger, guard range) and the weapon, move and armour types doc (base stats, quality tiers).

## The layers, in the order they unlock

| Layer | What it is | First appears |
| --- | --- | --- |
| Weapon element | Applies the element's status or effect on hit | Mid game |
| Armour element | Triggers a defensive effect, and resists its own element | Mid game |
| Matched set | Weapon and armour share an element: a stronger bonus | Mid game |
| Cross pair | Weapon and armour of two different elements: a named synergy or Echo | Late-mid game |
| Party reaction | Two allies' statuses meet on one enemy and react | Mid game, once two elements are in the party |
| Custom effects | Pool of 16 build effects, each with a weapon and an armour version | Rare early, common mid, two per item late |
| Resonance | Same custom effect on weapon and armour: one rank stronger | Late game |

## Design rules

1. Timing is untouchable. Elements and effects change damage, defence, stagger size,

skip size, stamina, statuses and position, never impact, completion or move times.

2. Every bonus adds, then caps. Percentage bonuses from effects add together and are

capped, so late builds get strong without multiplying out of control (see Stacking rules).

3. Readable statuses. Only three statuses persist on a target (Burn, Chill, Frozen).

Everything else happens instantly on the hit, so the battlefield stays legible.

## The seven elements

Each element belongs to one god, and its mechanics express that god's idea. Three opposed pairs mirror the gods' conflicts; Wild stands outside them.

| Element | God | Idea | Mechanical identity | Opposed to |
| --- | --- | --- | --- | --- |
| Flame | Isaren | Revolution that spreads | Damage over time that spreads between enemies | Frost |
| Frost | Maioren | The past holding on | Slows stamina recovery; sets up Frozen | Flame |
| Storm | Noeth | Chained logic, precision | Damage that jumps to neighbouring cells | Iron |
| Iron | Kratheon | Order and force | Bigger stagger, stronger guards | Storm |
| Wild | Agria | Untamed movement | Pushes enemies; rewards dodging | — |
| Light | Kerys | Faith and certainty | Healing from damage and blocks | Gold |
| Gold | Ploutar | Everything has a price | Steals and taxes stamina | Light |

## Element effects

Every element has three effects: one on the weapon (fires on a damaging hit), one on the armour (fires on a trigger), and a matched-set bonus when both share the element. Each has a mid-game and a stronger late-game value; early gear carries no element.

## Weapon effects (on a damaging hit)

| Element | Effect | Mid | Late |
| --- | --- | --- | --- |
| Flame | Burn: % max HP per second for 3 s, stacks to 3 | 1.5% per stack | 2% per stack |
| Frost | Chill: stamina recovery reduced for 4 s, stacks to 3 | −35% | −50% |
| Storm | Arc: part of the damage jumps to the cells beside the aim cell | 20% | 30% |
| Iron | Quake: bonus stagger | +20% | +30% |
| Wild | Gust: push the target 1 cell away from you if the cell is free | Push 1 | Push 1; a blocked push adds +0.1 s set-back |
| Light | Radiance: heal a share of damage dealt | 6% | 10% |
| Gold | Drain: steal stamina | 3 per hit | 5 per hit |

## Armour effects (on a trigger)

| Element | Trigger | Mid | Late |
| --- | --- | --- | --- |
| Flame | A hit lands on your guard at ≥ 75% defence | Attacker gains 1 Burn stack | 2 Burn stacks |
| Frost | An attacker damages you | Attacker gains 1 Chill stack | 1 stack, and their next set-back on you is halved |
| Storm | You dodge an attack | Store 1 Charge (max 2): next attack +15% stagger per Charge | Max 3, +20% per Charge |
| Iron | Your Defend move is at peak | +15% defence | +25% defence |
| Wild | You dodge an attack | Your next Move costs no armour tax | Next Move or Dodge costs no armour tax |
| Light | You block within 0.1 s of peak | Heal 3% max HP | Heal 5% max HP |
| Gold | An attacker damages you | Their next move costs +10% stamina | +20% |

## Matched set (weapon element = armour element)

| Element | Mid | Late |
| --- | --- | --- |
| Flame | Burn ticks also drain 1 stamina | Drain 2 stamina; Burn stacks to 4 |
| Frost | 3 Chill stacks become Frozen: next set-back taken ×1.5 | ×2 |
| Storm | Arcs carry 50% of the hit's stagger | 100% |
| Iron | Set-backs you take are reduced 30% while any Defend move is active | 50% |
| Wild | A push into an occupied cell or the edge adds +30% stagger instead | +50% |
| Light | Overheal becomes a shield for 5 s, max 10% max HP | Max 20% max HP |
| Gold | Stamina stolen above your max becomes bonus damage on your next hit, max +10 | Max +25 |

## Statuses

Only three statuses stay on a target. Every other element effect resolves instantly, on the hit or the trigger.

| Status | Source | Effect | Duration | Max stacks |
| --- | --- | --- | --- | --- |
| Burn | Flame weapon, Flame armour trigger, Echo | % max HP per second per stack | 3s | 3 (4 with a late Flame matched set) |
| Chill | Frost weapon, Frost armour trigger, Echo | Stamina recovery reduced (applies during Hold) | 4s | 3 |
| Frozen | 3 Chill stacks, Frost matched set only | Next set-back taken ×1.5 (late ×2), then ends | Until the next set- back | 1 |

Charge (Storm armour) is a buff on the wearer, not a status on enemies.

## Stacking rules

- Each new application adds 1 stack and refreshes the duration of the whole stack. Burn ticks every 0.1 s, pro-rated, so damage doesn't jump in chunks.

- Statuses from different sources share one stack: two Flame allies fill the same Burn bar.

- If Burn and Chill would both sit on one target, they react instead (Thermal Shock, see Party reactions).

- Cleansed by items such as Smelling Salts, and by any future cleanse moves.

## How elements combine

Every one of the 49 weapon × armour pairings does something: 7 matched sets, 14 named synergies, 6 Paradox pairs between opposed gods, and a general Echo rule for the other 22.

## The pairing grid

Rows are the weapon element, columns the armour element.

## Weapon ↓ Flame Frost Storm Iron Wild Light Gold

## / Armour

## →

| Flame | Matched | Paradox: Temper | Echo | Echo | Wildfire | Beacon | Echo |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Frost | Paradox: Quench | Matched | Cold Snap | Permafrost | Echo | Echo | Echo |
| Storm | Echo | Echo | Matched | Paradox: Grounded | Stormstep | Echo | Overload |
| Iron | Forge | Shatterpoint | Paradox: Magnetize | Matched | Echo | Echo | Echo |
| Wild | Echo | Echo | Squall | Echo | Matched | Echo | Scatter |
| Light | Echo | Echo | Echo | Bastion | Second Wind | Matched | Paradox: Tithe |
| Gold | Echo | Frozen Assets | Echo | Toll | Echo | Paradox: Indulgence | Matched |

## Echo (the default for unnamed pairs)

When your armour's element effect triggers, your weapon element also fires once on that enemy at reduced strength. Every pairing contributes something, even unplanned ones.

| Weapon element | Echo effect |
| --- | --- |
| Flame | 1 Burn stack |
| Frost | 1 Chill stack |
| Storm | 10% of the triggering hit's damage arcs to the cells beside the enemy |
| Iron | Your next attack on that enemy +10% stagger |
| Wild | Push the enemy 1 cell if free |
| Light | Heal 2% max HP |
| Gold | Steal 2 stamina |
| Named synergies |  |
| Synergy Weapon + armour | Effect |
| Wildfire Flame + Wild | When you move, Burn spreads from any burning enemy beside you to its neighbouring enemies (1 stack) |
| Beacon Flame + Light | Burn ticks on enemies you burned heal you for 20% of the tick |
| Permafrost Frost + Iron | Chilled attackers that hit your guard take the stagger they would have dealt, as set-back |
| Cold Snap Frost + Storm | Spending Charges also applies 1 Chill stack per Charge spent |
| Stormstep Storm + Wild | A dodge that moves you fires a 0-damage Arc that sets back the cells beside your landing cell by 0.2 s |
| Overload Storm + Gold | Every enemy hit by your Arc loses 2 stamina |
| Forge Iron + Flame | Quake hits on Burning targets refresh Burn to full duration |
| Shatterpoint Iron + Frost | Quake hits on a Frozen target deal +50% damage and consume Frozen |
| Squall Wild + Storm | A Gust push that collides releases an Arc (20% of the hit) to the cells beside the collision |
| Scatter Wild + Gold | Every successful push steals 3 stamina |
| Bastion Light + Iron | Radiance overheal becomes +10% defence for 3 s |
| Second Wind Light + Wild | Radiance heals double on your first hit after a dodge |
| Frozen Assets Gold + Frost | Drain steals double from Chilled enemies |
| Toll Gold + Iron | A block at peak also Drains the attacker, as if you'd hit them |

## Paradox pairs (opposed gods)

Wearing an opposed god's armour unlocks a strong effect with a real drawback: a deliberate risk-reward build.

| Paradox | Weapon + armour | Gain | Drawback |
| --- | --- | --- | --- |
| Temper | Flame + Frost | Your own hits on a Chilled target trigger Thermal Shock alone | Your Frost armour trigger no longer fires |
| Quench | Frost + Flame | Your own hits on a Burning target trigger Thermal Shock alone | Your Flame armour trigger no longer fires |
| Grounded | Storm + Iron | Your Arcs also carry Quake (+20% stagger) | Your Defend moves lose 15% defence |
| Magnetize | Iron + Storm | Each Charge also adds +10% damage | Max 2 Charges |
| Tithe | Light + Gold | Radiance also steals 2 stamina; Drain also heals 2% max HP | Your Heal moves are 20% weaker |
| Indulgence | Gold + Light | Drain also heals 2% max HP per hit | Light armour's block heal is halved |

Opposed pairs also take +50% damage from each other's attacks (see Resistances), so a Paradox build fights one of its own gods at a disadvantage.

## Party reactions

When one ally's status is on an enemy and another ally's element hits it, the two react. Reactions consume the status and need two different combatants, except Temper and Quench. That gives a party of different gods a reason to coordinate timing: land the Frost hit, then the Flame hit.

| Reaction | Status on target | Hit by | Effect | Consumes |
| --- | --- | --- | --- | --- |
| Thermal Shock | Burn | Frost (or Chill on a Burning target) | Burst of 5% max HP (late 7%) + 1.0 s set-back | Burn and Chill |
| Shatter | Chill | Storm Arc | That Arc deals double damage | Chill |
| Avalanche | Frozen | Iron Quake | Stagger of that hit +100% | Frozen |
| Ignition | Burn | Storm Arc | Every enemy the Arc hits gains 1 Burn stack | — |
| Eruption | Burn | Iron Quake | Remaining Burn damage dealt instantly; 50% of it to the cells beside | Burn |
| Firestorm | Burn | Wild Gust | Burn spreads to the cell the target is pushed toward | — |
| Whiteout | Chill | Wild Gust | 1 Chill stack to every enemy beside the landing cell | — |
| Judgement | Burn | Light Radiance | Remaining Burn damage dealt instantly; the attacker heals that much | Burn |
| Frostbite | Chill | Gold Drain | Drain steals double; Chill refreshes | — |

Spreading reactions (Ignition, Firestorm, Whiteout) can't trigger themselves again from the stacks they apply, so one hit can't chain across the whole board.

## Resistances

Armour element decides resistance. Same element halves; opposed god makes it half again worse; Wild is neutral to everything.

| Armour vs incoming element | Damage | Status duration |
| --- | --- | --- |
| Same element | −50% | Halved |
| Opposed element (Flame ↔ Frost, Storm ↔ Iron, Light ↔ Gold) | +50% | ×1.5 |
| Any other | Normal | Normal |
| Wild armour, or no element | Normal against all | Normal |

An attack's element is its weapon's element. Monsters use the same system, so a Kratheon legion in Iron armour shrugs off Iron attacks and is weak to Storm. Element choice becomes part of preparing for a region or god fight.

## Custom effect pool

Sixteen effects, each with a weapon version and an armour version built on the same idea. The twelve from the battle system doc are kept; four are new for the weapon families (Piercing, Focus, Sentinel, Reprisal). Values grow roughly ×1 / ×1.5 / ×2 from rank I to III, matching move variants, and some rank III versions add a breakpoint (marked ★).

| Effect | Version | Rank I | Rank II | Rank III |
| --- | --- | --- | --- | --- |
| Riposte | Weapon | Attacks starting within 0.7 s of your block peak +20% | +30% | +40% ★ and +0.1 s stagger |
| Riposte | Armour | A block at peak returns 30% of the hit's stagger to the attacker as set- back | 50% | 70% |
| Momentum | Weapon | +6% damage per consecutive hit landed, max 3 stacks; resets when you're hit | +8%, max 4 | +10%, max 5 |
| Momentum | Armour | +6% defence per consecutive successful block, max 3 | +8%, max 4 | +10%, max 5 |
| Opportunist | Weapon | +25% damage to targets in recovery (after their impact) | +40% | +60% |
| Opportunist | Armour | −25% set-back taken while you're in recovery | −40% | −60% |
| Vanguard | Weapon | +15% damage while in your side's front row | +20% | +30% |
| Vanguard | Armour | +15% armour defence while in your side's front row | +20% | +30% ★ and your guard range sides +25% |
| Skirmisher | Weapon | +15% damage on moves that include movement | +25% | +35% |
| Skirmisher | Armour | Move and Dodge moves cost 1 less stamina | 2 less | 3 less (min 0) |
| Bloodthirst | Weapon | Heal 3% of damage dealt | 5% | 8% |
| Bloodthirst | Armour | Heal 1% max HP on each successful dodge | 2% | 3% |
| Breaker | Weapon | +50% damage to targets in a defensive window | +75% | +100% |
| Breaker | Armour | Each hit you block drains 3 stamina from the attacker | 5 | 8 |
| Tempo | Weapon | Landing a hit refunds 15% of the move's stamina cost | 25% | 35% |
| Tempo | Armour | A successful dodge refunds 30% of the dodge's cost | 50% | 70% |
| Heavy | Weapon | +25% stagger; weapon tax +20% | +35% | +50% (tax penalty stays +20%) Page 11 of 17 |
| Heavy | Armour | −25% set-back taken; armour tax +20% | −35% | −50% |
| Guardian | Weapon | Hitting an enemy in your lane gives the ally behind you +10% defence for 3 s | +15% | +20% |
| Guardian | Armour | When an ally in your guard range is hit, gain 5 stamina | 8 | 12 |
| Last Stand | Weapon | +1% damage per 10% HP missing | +1.5% | +2% ★ and +10% stagger below 30% HP |
| Last Stand | Armour | +1% defence per 10% HP missing | +1.5% | +2% |
| Precision | Weapon | Damage on stencil cells other than the aim cell ×1.25 | ×1.5 | ×1.75 |
| Precision | Armour | Hits on you from outside the attacker's aim cell deal −30% | −50% | −70% |
| Piercing | Weapon | Adds depth +1 at 20% to Reach and First stencils (higher % wins) | 30% | 40% |
| Piercing | Armour | −10% damage from First- aimed (ranged) attacks | −20% | −30% |
| Focus | Weapon | Spells and heals +10% if the move started straight after a Hold | +15% | +20% |
| Focus | Armour | −10% set-back taken during Heal and spell moves | −20% | −30% ★ and their damage animation is 0.1 s shorter |
| Sentinel | Weapon / shield | Guard range cells +15% | +25% | +35% |
| Sentinel | Armour | Your block skip +0.1 s | +0.15 s | +0.2 s |
| Reprisal | Weapon | After you're hit, your next attack +15% | +25% | +35% |
| Reprisal | Armour | Attackers that damage you take 5% of that damage back | 8% | 12% |

Focus III's shorter damage animation affects how long a hit staggers you, not any move's own timing. It's the one effect that touches an animation length; cut it if that feels too close to the line.

## How effects grow toward the late game

Effects grow on three axes as the game goes on: rank (values roughly double from I to III), number per item (0 → 1 → 2), and combinations (matched sets, then cross pairs, then Resonance capstones). A late character doesn't just have bigger numbers; they have more interactions running at once.

## By stage

| Stage | Element on gear | Effects per item | Ranks seen | What opens up |
| --- | --- | --- | --- | --- |
| Early | None | 0 (rare finds: 1) | I | Learning moves and timing; no build layer yet |
| Early-mid | First elemental items, one piece | 1 | I | First statuses; first party reactions if two allies carry elements |
| Mid | Weapon and/or armour, mid strength | 1 | I–II | Matched sets; regular party reactions |
| Late-mid | Both pieces | 1–2 | II | Cross-pair synergies and Paradox builds |
| Late | Both pieces, late strength | 2 | II–III | Resonance and capstones; full build identity |
| God artifacts | God's element, late strength | 2 at rank III + 1 unique | III | Unique effects tied to each god's story |

## Effect budget per item

Each rank costs points (I = 1, II = 2, III = 3), and an item's quality sets its budget. This keeps item generation and hand design on one scale.

| Item quality | Budget | Typical result |
| --- | --- | --- |
| Early | 0–1 | Nothing, or one rank I effect |
| Mid | 1–2 | One effect at rank I or II |
| Late-mid | 2–4 | One rank II, or two effects at I–II |
| Late | 4–6 | Two effects at II–III |
| God artifact | 6 + unique | Two rank III effects plus a unique |

## Resonance

If the weapon and armour carry the same effect, each version acts one rank higher (max III). If both are already rank III, the pair unlocks a capstone instead, a qualitative upgrade only late builds reach.

| Effect | Capstone (both versions at rank III) |
| --- | --- |
| Riposte | Blocks at ≥ 90% count as Deflect, not only peak frames |
| Momentum | Stacks survive the first hit you take |
| Opportunist | Enemies you set back during their wind-up count as "in recovery" for 1 s |
| Vanguard | Front-row bonuses also apply in the enemy half |
| Skirmisher | The first movement move in each chain costs nothing |
| Bloodthirst | Healing above max becomes a 5 s shield, max 10% max HP |
| Breaker | Your hits on a guard deny the defender's skip |
| Tempo | The last move of a chain is fully refunded if it lands |
| Heavy | The tax penalty is removed |
| Guardian | The ally directly behind you takes 15% less damage at all times |
| Last Stand | Below 20% HP, Hold recovers stamina twice as fast |
| Precision | Stencil side cells also apply your on-hit element effect |
| Piercing | The pierced cell also applies your on-hit element effect |
| Focus | Spells started straight after a Hold can't be set back before their impact time |
| Sentinel | Your guard range gains the cell behind you at 100% |
| Reprisal | Returned damage also sets the attacker back 0.2 s |

## Stacking rules and caps

Within each category, all bonuses add together first, then the total is capped, then applied once. That keeps late builds strong without exponential stacking.

| Category | Sources that add together | Cap |
| --- | --- | --- |
| Damage bonus % | All custom effects, Magnetize (the Gold matched bonus is flat damage and sits outside the cap) | +100% |
| Defence bonus % | Momentum (armour), Vanguard, Last Stand, Guardian, Bastion, Iron armour peak | +60% |
| Set-back reduction % | Armour stagger resistance, Heavy, Opportunist (armour), Focus (armour), Iron matched | 75% (everyone stays interruptible) |
| Healing from damage dealt | Radiance, Bloodthirst, Tithe, Indulgence | 15% of the hit |
| Stamina refunds | Tempo, capstones | Never more than the move's own cost |
| Damage reduction vs specific hits | Precision (armour), Piercing (armour) | 70% |

Elemental resistance (±50%) and the defence formula from the battle system doc apply after these caps.

## Worked example: a late Flame duelist

- Weapon: late Flame sword with Momentum III and Opportunist III.

- Armour: late Flame armour with Momentum III and Vanguard II.

- What's active: Flame matched set (Burn ticks drain 2 stamina, Burn stacks to 4). Momentum Resonance, already III + III, gives its capstone: stacks survive the first hit taken.

- A 5-hit streak, the last hit on an enemy in recovery: Momentum +50% (5 × 10%) and Opportunist +60% add to +110%, capped at +100%. The hit does double damage plus Burn, rather than the ×2.4 that multiplying would give.

The cap makes a second damage effect strongest early in a streak and partly wasted at full stacks. That's a real build decision: Momentum + Opportunist peaks fast; Momentum + Bloodthirst sustains.

## Open questions and risks

The system is large: 49 pairings, 9 reactions and 16 effects × 2 versions × 3 ranks. Its biggest risks are readability and testing load, so the first job is simulating the strongest combinations.

## Risks

1. Testing load. Every pairing and capstone needs checking. Recommendation: add elements, reactions and the effect pool to the battle simulator and sweep late-game loadouts for outliers.

2. Readability. Burn, Chill, Frozen, Charges, Echo pops and reactions all happen at once in a 3 × 4 arena. Each needs a distinct, quick visual and sound cue.

3. Echo may feel generic. It guarantees every pairing does something, but players may not notice it. Promote popular Echo pairs to named synergies over time.

4. Thermal Shock may dominate. A Flame + Frost party, or one Paradox character, can trigger it on demand. Watch its frequency in simulation; consider a short per-target cooldown.

5. Caps hide value. Players stacking damage effects past +100% waste slots without knowing. Show the capped total in the equipment screen.

## Open questions

- Are the element names (Flame, Frost, Storm, Iron, Wild, Light, Gold) final, or should they follow god-specific names?

- Should Paradox drawbacks be harsher, or is the +50% opposed-damage weakness enough?

- Keep Focus III's shorter damage animation, or replace it with a non-timing bonus? Per-target cooldown on reactions (e.g. 2 s)?

- Unique god-artifact effects: one per god, designed with the story beats?

- Can monsters carry custom effects, and at what rank per tier?
