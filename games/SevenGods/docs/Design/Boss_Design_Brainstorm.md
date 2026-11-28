# Seven Gods RPG — Boss Design Brainstorm

## Overview

A boss is a group of enemies with one mind. It is built from parts: a core whose HP is the win condition, plus limbs, eyes, wings or helpers. Each part has its own cells, HP, defence and combo line. All parts pick their lines together at one decision point, so the whole boss produces one stop, and the player reads one combo, not five separate enemies. Everything else is the regular combat system: chains follow the 4 s budget, nothing cancels, and each stage has an opener, weights and follow-ups like the enemy combo menus. Unlike regular enemies, bosses can have fully custom moves and custom rules (see Shared boss rules). What changes is the size of the grid, the bodies on it, and one twist per boss.

## Design pillars

1. Many enemies, one read. Parts move and act independently, but their lines start together and end within 1 s of each other.

2. Timing first. Damage comes in Exposed windows, authored into specific moves. Bosses guard a lot, with guard ranges that cover other parts. Players win by hitting in windows and not swinging into guards.

3. Readable. Fixed openers, shadow marks on targeted cells, fixed eye orders, mirrored limb pairs. Every combo has an answer, and the tables below list it.

4. One twist each. The twist bends a rule of normal combat (reach, positioning, targeting, who can be hit) without adding a new control.

5. Three stages. Each stage starts with a transformation and swaps in a new combo menu. A stage can be triggered by HP or by a condition, such as breaking both wings. Lead-up bosses get one phase (two at most), and Isaren gets four.

## The four headline bosses

| Boss | Twist | What it tests | Element (weak to) | Stage of game |
| --- | --- | --- | --- | --- |
| The Ledger | Tentacles guard | Target priority, guard | Gold | Mid, tier II |
| Kraken (octopus) | the body; break, outwait or slip past them | timing, Flame/Frost cauterising | (Light) |  |
| The Iron Colossus (giant) | Shadow marks; empty the cells, then close in on the lowered head | Reading area attacks, footwork, punishing recoveries | Iron (Storm) | Early–mid, tier I–II; reprise late |
| The Hymnal Roc (bird) | Off-grid in the Sky; only on the grid when attacking, and every swoop leaves a different- sized window | Pre-positioning, window length, killing helpers to open windows | Light (Gold) | Mid–late, tier II–III |
| The Hundred- Eyed Archive | Weak points open on specific faces; | Circling, approach angles, splitting the | Storm (Iron) | Late, tier III |
| (3×3 eyes) | you must stand in the eye's gaze to hit it, then get out | party, door timing |  |  |

Every combo below was checked by the boss chain validator ( /home/claude/bosses ). It applies the enemy rules: at least 2 moves, 4 s budget on the lead line, no Hold after a Hold. It adds one boss rule: every line ends within 1 s of the others. HP numbers are first guesses sized against the enemy doc (Bandit Leader 130, City Guard 160) for a party of 3 and need simulating.

## Shared boss rules

These rules add to the battle system doc. Each boss below uses them and adds its own twist.

## Custom moves and custom rules

- Boss moves are fully custom. They don't have to be reskinned human moves; any types, timings and aim shapes are allowed. Some bosses below still name a human move as a timing template, but that's a convenience, not a rule.

- No move cap. Six moves per form or phase is a guide for keeping patterns learnable, but bosses can have more, and can swap to a different move set for each stage or form.

- Custom rules are allowed per boss and are listed at the top of each design, for example ignoring stamina, Granite Skin, Momentum or Throne Barrier. The chain budget, the single stop per combo and the no-cancel rule stay, so every boss is still read the same way.

## Grids and off-grid zones

- Boss grids run 4–7 lanes by 6–7 rows for a party of 3, and bigger for 8-fighter god fights. The party still starts in row 1.

- Off-grid zones are named areas outside the grid where boss parts can sit: the Deep, the Backdrop, the Sky, the Crown. The party can't enter them, and attacks can't reach them (one optional exception is Skyshot, on the Roc). A part only becomes hittable when it puts a cell on the grid, usually to attack. This is the rule that keeps a boss out of reach for parts of the fight.

- Big bodies claim cells at the start of a move, like normal movement claims. A character standing in a cell a big part lands on is pushed to the nearest free cell. If the move is an attack, they take its hit as well.

## Parts

- Every part has cells, HP, defence, element, a hit reaction and a line. The core is the win condition. Other parts spill 25–30% of the damage they take into the core, so no hit is wasted, but hitting the core directly is always better.

- One hit per part. An attack whose stencil covers several cells of one part hits it once, at its best falloff. A Spin Slash next to a 3×3 body does not hit it three times. Breaking a part (0 HP) cancels its line for the rest of the combo and gives the core a Break stagger (1.5–3 s, set per boss). The part either frees its cells or leaves an obstacle. Whether it regrows is set per boss: regrows after N stops, regrows at the next stage, or never.

- Pushes don't move multi-cell parts. One-cell parts (tentacle tips, Cantors) can be pushed.

- Facing: parts face the party half by default. A part's facing decides directional moves (Heel Grind hits behind a foot) and guard ranges. The Archive faces outward on all four sides.

## Combos as lines

- A boss combo is one line per part, all starting at the same decision point. Mirrored limb pairs share one line.

- The lead line (the core's, usually) follows the normal chain rule: at least 2 moves, and the move that reaches 4 s is the last one, with overrun allowed.

- Every other line has at least 2 moves and must end within 1.0 s of the others, so the whole boss produces a single stop (this is the 1 s stop-batching rule). A part that finishes early idles until the stop. That idle gap is a deliberate free-hit opening. No Hold after a Hold inside a line, or across a listed follow-up. Boss Holds use the standard Hold lengths.

- Bosses don't track stamina by default (a custom rule). Rest is written into the menus as Holds, Shutters and guard-heavy combos, so a random Recover chain never breaks predictability.

- Menu per stage: the opener (always the first combo of a stage, with a unique tell), then positional triggers, then the follow-up roll, then weights. This works like the enemy menus, but stages replace the Healthy and Wounded phases, and Stage 3 can add a Desperate (≤ 10%) table.

## Exposed windows and guards

- Exposed is the boss version of a weakness window. It is authored into a move (usually from impact to completion) or into a Hold (marked Exposed in the move lists). While Exposed, a part takes ×1.5 damage and loses Steadfast and Unflinching, so it can be staggered.

- Boss guards are normal defensive moves (Blade Block, Shield Wall, Hanging Guard, Ward) with guard ranges that cover other parts. An inner tentacle guards the Maw, and a hand guards the head from its side. Swinging into a guard peak feeds its skip, the same as against any guard.

- Each combo table lists every line's Openings (Exposed) and Guarded (defensive window) times, in seconds from the stop. The real puzzle is where an opening isn't covered by a guard.

## Telegraphs

- Shadow marks: cells a heavy attack will hit are shaded. When they appear is a difficulty dial set per stage: at chain commit (gentle), at move start (harder), or not at all for feints.

- Read levels still apply. Read 0 sees shadow marks. Read 1 also sees the move names for every line. Read 2 sees full line timings, including Openings.

### Stage transitions

- Triggers: core HP at 66% and 33%, or a condition set per boss (both wings broken, every eye on one side destroyed). Whichever comes first.

- No cancelling: if a trigger happens mid-combo, that combo finishes, and the transformation plays as the next chain.

- A transformation is a 3–4 s scripted chain: immune for the first part and Exposed for the last 1–1.5 s, a reward for being in position. The grid can change, new parts arrive, broken parts may regrow, and statuses on regrown parts are cleared.

- Proposal: the core can't drop below the next threshold until that transformation has played. Excess damage is lost, so a burst can't skip a stage's content (unless a condition skip is intended, as on the Roc).

## The Ledger Kraken

Octopus · Gold · mid game (tier II moves) · party of 3 · grid 5 lanes × 6 rows plus the Deep (off-grid)

Twist: the body is guarded by its own limbs. The Maw (the core, the win condition) sits at the back of the grid with most of its bulk in the Deep, an off-grid pool behind row 6. Its tentacles are separate enemies with their own HP, movement and lines. Inner tentacles throw guard ranges over the Maw; outer tentacles hunt. You reach the Maw by breaking a tentacle, waiting for one to submerge, or hitting during a tentacle's Exposed recovery after a Slam.

- Grid: lanes 1–5, rows 1–3 are the party half and rows 4–6 the boss half. The Deep runs behind row 6 and along both side edges (off-grid columns 0 and 6) from Stage 2. Tentacle roots: each tentacle is rooted at a cell and can only occupy cells within 2 of it. Outer roots are at lanes 1 and 5, row 5. Inner roots are at lanes 2 and 4, row 5. Regrowth: a broken tentacle regrows at 60% HP after 3 boss stops. A tentacle that is Burning or Frozen when it breaks is cauterised and stays broken for the rest of the stage. Flame and Frost players get a real job here.

- Break stagger: breaking a tentacle staggers the Maw for 1.5 s and cancels its line for the rest of the combo.

- Spill: 25% of damage dealt to a tentacle also hits the Maw, so no hit is wasted.

Moves (custom boss moves; "Based on" only names the human move whose timings were used as a starting template)

| Boss move | Part | Based on | Types | Completion | Key times | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| Ink Jet | Maw | Volley | Attack | 2 | impact 1.3 | Gold hit on a marked band of 3 cells, 2 rows in front of the Maw. |
| Undertow | Maw | Palm Strike | Attack | 0.9 | impact 0.45 | Pulls everyone in row 4 one cell toward the Maw (the push reversed). |
| Gilded Shell | Maw | Blade Block | Defend | 1.6 | window 0.2–0.7–1.2 | Gold plates close over the eye. |
| Beak Snap | Maw | Thrust | Attack | 1.2 | impact 0.6 | Exposed from impact to end. Stage 2–3. Reaches the row in front of the Maw. |
| Gulp | Maw | Throw | Attack | 1.3 | impact 0.7 | Stage 3. Drags the target into the cell in front of the beak. |
| Tentacle Slam | Tentacle | Overhead Cleave | Attack | 1.7 | impact 0.9 | Exposed from impact to end. The tip lies on its cell after impact. |
| Lash | Tentacle | Slash | Attack | 1 | impact 0.5 |  |
| Tentacle Sweep | Tentacle | Wide Sweep | Attack | 1.9 | impact 1.0 | Hits the target cell and both sides. |
| Coil Guard | Tentacle | Shield Wall | Defend | 2 | window 0.1– 0.8–1.6 | Guard range covers the Maw cells behind the tentacle. |
| Seize | Tentacle | Throw | Attack | 1.3 | impact 0.7 | Drags the target 2 cells toward the Maw. |
| Submerge | Tentacle | Roll | Move + Dodge | 0.8 | move 0.3 | Drops into the Deep: can't be hit and doesn't guard until it Emerges. |
| Emerge | Tentacle | Step | Move | 0.6 | move 0.3 | Returns to any free cell within 2 of its root. |
| Flick | Tentacle | Jab | Attack | 0.45 | impact 0.2 |  |
| Deep Strike | Deep arm | Overhead Cleave | Attack | 1.7 | impact 0.9 | Exposed from impact to end. An off-grid arm slams a marked edge cell. It can be hit only from impact to end. |
| Deep Sweep | Deep arm | Wide Sweep | Attack | 1.9 | impact 1.0 | Sweeps a whole edge lane from off-grid. It can't be hit. |

Hold animations — Maw: Appraise (Exposed); Outer tentacles: Writhe; Inner tentacles: Writhe; Rear tentacles: Lurk; Deep arms: Lurk.

### Stage 1 — Hoard (100–66% Maw HP)

The Maw fills row 6, lanes 2–4, with its eye just above the water. Four tentacles: an outer pair hunts and an inner pair guards. Each pair moves as a mirror, so one line describes both.

| Part | Cells | HP | Defence | Hit reaction | Notes |
| --- | --- | --- | --- | --- | --- |
| Maw (core) | row 6, lanes 2–4 | 1,100 | 18 | Unflinching | Win condition. Weak to Light (Light↔Gold). |
| Outer tentacle ×2 | 1 cell each, rooted at lanes 1 and 5 | 140 | 10 | Normal (staggerable) | Hunters |
| Inner tentacle ×2 | 1 cell each, rooted at lanes 2 and 4 | 160 | 12 | Steadfast while guarding | Guardians |

## Combo lines (numbers in brackets are start times in seconds)

| Combo | Part | Line | Ends | Openings | Guarded |
| --- | --- | --- | --- | --- | --- |
| A. Rising Tide | Maw Outer tentacles Inner tentacles | Ink Jet (0.0) → Appraise 1.2 s (2.0) → Ink Jet (3.2) Emerge (0.0) → Tentacle Slam (0.6) → Lash (2.3) → Lash (3.3) Coil Guard (0.0) → Coil Guard (2.0) → Flick (4.0) | 5.2 s 4.3 s 4.5 s | 2.0–3.2 1.5–2.3 — | — — 0.1–1.6, 2.1– 3.6 |
| B. Grasping Pincer | Maw Outer tentacles Inner tentacles | Undertow (0.0) → Appraise 0.8 s (0.9) → Ink Jet (1.7) → Undertow (3.7) Tentacle Sweep (0.0) → Coil Guard (1.9) Seize (0.0) → Lash (1.3) → Seize (2.3) | 4.6 s 3.9 s 3.6 s | 0.9–1.7 — — | — 2.0–3.5 — |
| C. Appraisal | Maw Inner tentacles Outer tentacles | Gilded Shell (0.0) → Appraise 2 s (1.6) → Ink Jet (3.6) Submerge (0.0) → Writhe 2 s (0.8) → Emerge (2.8) → Tentacle Slam (3.4) Lash (0.0) → Tentacle Sweep (1.0) → Tentacle Slam (2.9) | 5.6 s 5.1 s 4.6 s | 1.6–3.6 4.3–5.1 3.8–4.6 | 0.2–1.2 — — |
| D. Coin Squall | Maw Outer tentacles Inner tentacles | Ink Jet (0.0) → Ink Jet (2.0) Coil Guard (0.0) → Lash (2.0) → Flick (3.0) Seize (0.0) → Lash (1.3) → Lash (2.3) | 4.0 s 3.5 s 3.3 s | — — — | — 0.1–1.6 — |
| Combo | When | Usual follow- up | Read it → answer it |  |  |
| A. Rising | Opener | (100%) Appraisal | Tell: the inner tentacles |  |  |
| Tide Combo When B. Grasping Weights 35%; Trigger: Pincer someone stands next to the Maw C. Appraisal Weights 25% D. Coin Weights 40% Squall |  | (60%) Usual follow- Read it → answer it up Coin Squall Tell: the water drains toward (60%) the Maw (Undertow). Anyone in row 4 gets pulled in for the Seizes, so step back to row 3 before 0.45 s. The inner tentacles are attacking, not guarding, so the Maw is unguarded from 0.9–1.7 s. Grasping Tell: the inner tentacles dive. Pincer (50%) The big window: the Maw is Exposed and unguarded from 1.6 to 3.6 s. The inner tentacles resurface at 3.1 s and Slam at 4.3 s, so be clear of their cells by then. The outer Sweep (impact 2.0) covers the lanes beside the Maw, so approach through lane 3. Appraisal Two Ink Jets mark bands 2 (60%) rows out: stand in row 1 or right up against the tentacles. It's a good stop to Hold and recover stamina. | cross in front of the eye. The Maw is open during Appraise but guarded, so don't chase it. Punish the outer tips during their Slam recovery (1.5–2.3 s). |  |  |

### Stage 2 — Margin Call (66–33%)

Transformation (3.5 s, immune for the first 2 s, then Exposed for 1.5 s): the Kraken hauls itself forward. The Maw now fills rows 5–6, lanes 2–4, and row 6 floods. Broken tentacles regrow. Two rear tentacles coil in the side Deep (off-grid columns 0 and 6) and slam the edge lanes. Outer pairs now sweep both sides at once, leaving lane 3 as the only safe lane.

| Part | Cells | HP | Defence | Hit reaction | Notes |
| --- | --- | --- | --- | --- | --- |
| Maw (core) | rows 5–6, lanes 2–4 | continues | 18 | Unflinching | Beak Snap now reaches row 4 |
| Outer / inner tentacles | as before | 120 / 140 | 10 / 12 | as before | Thinner after regrowing |
| Rear tentacle ×2 | off-grid side columns | 120 | 10 | Normal | Can be hit only during Deep Strike recovery |

## Combo lines (numbers in brackets are start times in seconds)

| Combo | Part | Line | Ends | Openings | Guarded |
| --- | --- | --- | --- | --- | --- |
| E. Margin Call | Maw Outer tentacles Inner tentacles Rear tentacles | Gilded Shell (0.0) → Beak Snap (1.6) → Beak Snap (2.8) Tentacle Sweep (0.0) → Tentacle Sweep (1.9) Coil Guard (0.0) → Lash (2.0) → Flick (3.0) Deep Strike (0.0) → Lurk 0.8 s (1.7) → Deep Strike (2.5) | 4.0 s 3.8 s 3.5 s 4.2 s | 2.2–2.8, 3.4– 4.0 — — 0.9–1.7, 3.4– 4.2 | 0.2–1.2 — 0.1–1.6 — |
| F. Foreclosure | Maw Outer tentacles Inner tentacles Rear tentacles | Undertow (0.0) → Ink Jet (0.9) → Undertow (2.9) → Beak Snap (3.8) Seize (0.0) → Writhe 0.8 s (1.3) → Seize (2.1) → Lash (3.4) Tentacle Slam (0.0) → Lash (1.7) → Tentacle Slam (2.7) Deep Strike (0.0) → Lurk 1.2 s (1.7) → Deep Strike (2.9) | 5.0 s 4.4 s 4.4 s 4.6 s | 4.4–5.0 — 0.9–1.7, 3.6– 4.4 0.9–1.7, 3.8– 4.6 | — — — — |
| G. Audit | Maw Inner tentacles Outer tentacles Rear tentacles | Appraise 2 s (0.0) → Gilded Shell (2.0) → Ink Jet (3.6) Submerge (0.0) → Writhe 1.2 s (0.8) → Emerge (2.0) → Tentacle Slam (2.6) → Lash (4.3) Tentacle Sweep (0.0) → Lash (1.9) → Tentacle Sweep (2.9) Deep Strike (0.0) → Lurk 1.2 s (1.7) → Deep Strike (2.9) | 5.6 s 5.3 s 4.8 s 4.6 s | 0.0–2.0 3.5–4.3 — 0.9–1.7, 3.8– 4.6 | 2.2–3.2 — — — |
| H. Dividend | Maw Outer tentacles Inner tentacles Rear tentacles | Ink Jet (0.0) → Ink Jet (2.0) Coil Guard (0.0) → Seize (2.0) Coil Guard (0.0) → Coil Guard (2.0) Deep Strike (0.0) → Deep Strike (1.7) | 4.0 s 3.3 s 4.0 s 3.4 s | — — — 0.9–1.7, 2.6– 3.4 | — 0.1–1.6 0.1–1.6, 2.1– 3.6 — |
| Combo | When | Usual follow- up | Read it → answer it |  |  |
| E. Margin | Opener | (100%) Foreclosure | Pincer sweeps hit at 1.0 s |  |  |
| Call |  | (60%) | and 2.9 s on both sides at once. Stay in lane 3, but out of Beak Snap reach (row 4) until 4.0 s. The rear tips are Exposed on the edge lanes from 0.9–1.7 s and 3.4–4.2 s. |  |  |
| F. | Weights 40%; Trigger: two | Audit (55%) | Two Undertows pull row 4 |  |  |
| Foreclosure | or more party members share a lane |  | toward the Maw, straight into the inner Slams. Stay in row 2 and punish the inner tips (Slam recovery 0.9–1.7 s and 3.6–4.4 s): that is how you break the guards. |  |  |
| G. Audit | Weights 25% | Dividend (50%) | Opens with Appraise: the Maw is Exposed at once, and the inner guards are gone from 0.8–2.6 s. The window is 0.8–2.0 s, then Gilded Shell closes it. It's short, so pre-position in lane 3, row 4 at the end of the previous combo. |  |  |
| Combo | When | Usual follow- up |  | Read it → answer it |  |
| H. Dividend | Weights 35% | Foreclosure (60%) |  | Everything guards and the Maw shoots: no Maw window. Kill rear tips (Exposed 0.9–1.7 s and 2.6– 3.4 s) with ranged or edge- lane fighters. |  |

### Stage 3 — Liquidation (33–0%)

Transformation (4 s; Exposed for the last 1.5 s): the Kraken drags every tentacle into the Deep and rears up into rows 4–5, lanes 2–4. The Maw is now always on the grid and always reachable. The tentacles become deep arms that strike the edge lanes from off-grid. Gilded Shell becomes its main defence, so this stage is about timing hits around the Shell.

| Part | Cells | HP | Defence | Hit reaction | Notes |
| --- | --- | --- | --- | --- | --- |
| Maw (core) | rows 4–5, lanes 2–4 | continues | 18 (40 in Shell) | Unflinching | Beak reaches row 3 |
| Deep arms | off-grid edges | shared | 10 | Normal | Exposed only in Deep Strike |
| ×4 |  | 300 |  |  | recovery. Breaking all of them removes the edge threat. |
| Combo lines (numbers in |  | brackets are start | times in seconds) |  |  |
| Combo | Part | Line |  | Ends | Openings Guarded |
| I. Liquidation | Maw Deep arms | Gilded Shell (0.0) → Gilded Shell (1.6) → Ink Jet (3.2) Deep Strike (0.0) → Lurk 0.8 s (1.7) → Deep Sweep (2.5) |  | 5.2 s 4.4 s | — 0.2–1.2, 1.8– 2.8 0.9–1.7 — |
| J. Fire Sale | Maw Deep arms | Beak Snap (0.0) → Beak Snap (1.2) → Gulp (2.4) → Beak Snap (3.7) Deep Sweep (0.0) → Lurk 1.2 s (1.9) → Deep Strike (3.1) |  | 4.9 s 4.8 s | 0.6–1.2, 1.8–2.4, — 4.3–4.9 4.0–4.8 — |
| K. Last Appraisal | Maw | Undertow (0.0) → Ink Jet (0.9) → Appraise 2 s (2.9) |  | 4.9 s | 2.9–4.9 — |
| Combo | Part Deep arms | Line Lurk 1.2 s (0.0) → Deep Strike (1.2) → Deep Strike (2.9) |  | Ends 4.6 s | Openings Guarded 2.1–2.9, 3.8–4.6 — |
| L. Insolvency | Maw Deep arms | Ink Jet (0.0) → Beak Snap (2.0) → Beak Snap (3.2) Deep Strike (0.0) → Deep Sweep (1.7) |  | 4.4 s 3.6 s | 2.6–3.2, 3.8–4.4 — 0.9–1.7 — |
| Combo | When |  | Usual follow- up |  | Read it → answer it |
| I. Liquidation | Opener (100%); Weights 30% |  | Fire Sale (60%) |  | Double Shell: don't hit into it. Hit the Maw at 3.2 s, when the Shell ends and Ink Jet starts winding up (impact at 4.5 s). |
| J. Fire Sale | Weights 45% |  | Last Appraisal (55%) |  | Beak Snaps leave short openings (0.6–1.2 s and 1.8–2.4 s). Gulp at 2.4 s drags someone in front of the final Snap, so guard or Parry at 4.3 s. |
| K. Last | Weights 25%; |  | Fire Sale |  | Ends in a 2 s Appraise: the |
| Appraisal | Desperate (≤ 10%) 40% |  | (50%) |  | Maw is Exposed from 2.9 s to the stop. This is your burst window. Watch the edges for Deep Strikes at 2.1 s and 3.8 s. |
| L. Insolvency | Desperate (≤ 10%) 60% |  | Last Appraisal (50%) |  | Desperate. No Shell: trade hits during Beak Snap recoveries. |

## The Iron Colossus

Giant · Iron · early–mid game (tier I–II moves) · party of 3 · grid 5 lanes × 6 rows plus the Backdrop (off-grid, above the boss half)

Twist: read the shadow, empty the cells, then close in. The Colossus stands over the boss half. Only its feet touch the grid; its body, arms and head are in the Backdrop. Every big attack throws a shadow mark onto the cells it will hit. A Bend means a hand is about to sweep a whole band of cells, and afterwards the head is low enough to hit. The fight is a rhythm: read the mark, leave, then step in during the recovery.

- Feet are separate enemies (Iron plate, high defence, Steadfast). Breaking a foot makes the Colossus Kneel: a 3 s Break stagger with the head on the grid and Exposed. Once per stage, the foot then recovers at 50% HP with its plate gone. Head (core) is reachable only when it's on the grid: during a Bend Sweep or Hammer Fists recovery, a Kneel, or all of Stage 3.

- Tremor: cells next to a landing Stomp take stagger but no damage. Standing beside a foot is risky but rewarding.

- Shadow marks: Stage 1 marks the cells at chain commit, visible at every Read level. Stage 2 marks them only when the move starts (see the feint).

- Spill: 30% of foot and hand damage reaches the head.

Moves (custom boss moves; "Based on" only names the human move whose timings were used as a starting template)

| Boss move | Part | Based on | Types | Completion | Key times | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| Stride | Legs | Step | Move | 0.6 | move 0.3 | Moves the whole footprint 1 cell, any orthogonal. A character in a landing cell is pushed 1 cell. |
| Stomp | Legs | Crushing Overhead | Attack | 2.4 | impact 1.3 | Exposed from impact to end. The raised foot leaves its cell, then lands on a marked cell: 100% there, Tremor on the 8 around it. The Exposed part is the foot. |
| Kick | Legs | Shove | Attack | 0.8 | impact 0.4 | Push 1. |
| Heel Grind | Legs | Back Kick | Attack | 0.9 | impact 0.45 | Hits the cell behind a foot, so it punishes anyone circling. |
| Plant | Legs | Brace | Defend | 1.1 | window 0.1–0.5–0.8 | Feet dig in. |
| Bend Sweep | Body | Wide Sweep | Attack | 1.9 | impact 1.0 | Exposed from impact to end. Hand sweeps a marked band: 3 lanes of one row (Stage 1) or 2 rows (Stage 2). The Exposed part is the head, which drops onto the grid at row 4, lane 3. |
| Hammer Fists | Body | Overhead Cleave | Attack | 1.7 | impact 0.9 | Exposed from impact to end. Both fists slam 2 marked cells. The Exposed parts are the hands, which stay on the grid; the head is not. |
| Iron Guard | Body | Blade Block | Defend | 1.6 | window 0.2–0.7–1.2 | Arms crossed over the head. |
| Plate Hurl | Body | Volley | Attack | 2 | impact 1.3 | Stage 2. Hurls armour plates. Each plate lands on a marked cell and stays as an obstacle. |
| False Bend | Body | Hanging Guard | Defend | 1.4 | window 0.1–0.6–1.1 | Stage 2 feint: bends one way with no shadow mark, guarding the head. |
| Pillar | Planted | Shield Wall | Defend | 2 | window | Stage 2. The planted hand |
| Guard | hand |  |  |  | 0.1–0.8–1.6 | guards its lane and the head behind it. |
| Knuckle Crush | Hand | Crushing Overhead | Attack | 2.4 | impact 1.3 | Exposed from impact to end. Leaves Rubble on the cell for 2 stops (Stage 3). |
| Palm Sweep | Hand | Wide Sweep | Attack | 1.9 | impact 1.0 |  |
| Flick | Hand | Palm Strike | Attack | 0.9 | impact 0.45 | Push 1. |
| Hand Guard | Hand | Blade Block | Defend | 1.6 | window 0.2–0.7–1.2 | Guard range covers the head on that hand's side only. |
| Slag Spit | Head | Blast | Attack (Mana) | 1.8 | impact 1.0 | Iron splash: 30% / 15% around the target. |
| Headbutt | Head | Uppercut | Attack | 1.1 | impact 0.6 | Exposed from impact to end. |

Hold animations — Body: Bellow; Legs: Settle; Planted hand: Grip; Head: Gasp (Exposed); Left hand: Rest; Right hand: Rest.

### Stage 1 — March (100–66% head HP)

Feet in row 5, lanes 2 and 4. The body line drives the attacks and the legs line moves.

| Part | Cells | HP | Defence | Hit reaction | Notes |
| --- | --- | --- | --- | --- | --- |
| Head (core) | off-grid; row 4, lane 3 when low | 700 | 10 | Normal when Exposed | Win condition. Weak to Storm (Storm↔Iron). |
| Left foot / right foot | 1 cell each | 180 each | 30 | Steadfast | Breaking one makes it Kneel |

## Combo lines (numbers in brackets are start times in seconds)

| Combo | Part | Line | Ends | Openings | Guarded |
| --- | --- | --- | --- | --- | --- |
| A. Advance | Body Legs | Bellow 1.2 s (0.0) → Iron Guard (1.2) → Bend Sweep (2.8) Stride (0.0) → Stride (0.6) → Stomp (1.2) → Plant (3.6) | 4.7 s 4.7 s | 3.8–4.7 2.5–3.6 | 1.4–2.4 3.7–4.4 |
| B. Stamp | Body Legs | Iron Guard (0.0) → Bellow 0.8 s (1.6) → Hammer Fists (2.4) Stomp (0.0) → Stomp (2.4) | 4.1 s 4.8 s | 3.3–4.1 1.3–2.4, 3.7– 4.8 | 0.2–1.2 — |
| C. Low Sweep | Body Legs | Bend Sweep (0.0) → Bend Sweep (1.9) → Iron Guard (3.8) Plant (0.0) → Kick (1.1) → Plant (1.9) → Kick (3.0) → Stride (3.8) | 5.4 s 4.4 s | 1.0–1.9, 2.9– 3.8 — | 4.0–5.0 0.1–0.8, 2.0– 2.7 |
| D. Trample | Body Legs | Iron Guard (0.0) → Iron Guard (1.6) → Hammer Fists (3.2) Kick (0.0) → Heel Grind (0.8) → Stomp (1.7) | 4.9 s 4.1 s | 4.1–4.9 3.0–4.1 | 0.2–1.2, 1.8– 2.8 — |
| Combo |  | When Usual follow- up |  | Read it → answer it |  |
| A. Advance |  | Opener (100%); Low Sweep Weights 25% (60%) |  | Bellow, then two Strides toward you, then a Stomp mark (impact 2.5 s), then a Bend mark across a row (impact 3.8 s). Leave both marks, then hit the head 3.8–4.7 s. |  |
| B. Stamp |  | Weights 40% Low Sweep (50%) |  | Arms crossed means two Stomps (impacts 1.3 and 3.7 s) plus fists at 3.3 s. The hands are Exposed 3.3– 4.1 s and the feet after each Stomp. This is how you chip a foot to break it. |  |
| C. Low Sweep |  | Weights 35% Stamp (60%) |  | The first Bend marks the near row (impact 1.0 s), the second the far row (impact 2.9 s). Step back, then forward. The head is Exposed 1.0– 1.9 s and 2.9–3.8 s. |  |
| D. Trample |  | Trigger: someone next Stamp (50%) to a foot |  | Punishes anyone hugging a foot: Kick, Heel Grind behind, then a Stomp. Back off to Reach 2, wait for the Stomp landing (3.0 s), and hit the foot until 4.1 s. |  |

### Stage 2 — Siege (66–33%)

Transformation (4 s; immune, then Exposed for 1 s): it tears off its plate armour and plants its left hand on the grid as a pillar (row 4, lane 1). Plates thrown during this stage become cover: a Bend Sweep stops at a plate cell, and two sweeps destroy it. Shadow marks now appear only when a move starts, so the False Bend feint (a bend with no mark) needs reading. At Read 2 you see it at chain commit.

| Part | Cells | HP | Defence | Hit reaction | Notes |
| --- | --- | --- | --- | --- | --- |
| Head (core) | as before | continues | 10 | Normal when Exposed |  |
| Feet | as before | 140 each (no plate) | 18 | Steadfast |  |
| Planted hand | row 4, lane 1 | 200 | 20 | Steadfast | Breaking it frees lane 1 and the Colossus loses Pillar Guard |

## Combo lines (numbers in brackets are start times in seconds)

| Combo | Part | Line | Ends | Openings | Guarded |
| --- | --- | --- | --- | --- | --- |
| E. Plate Throw | Body Legs Planted hand | Plate Hurl (0.0) → Bellow 1.2 s (2.0) → Bend Sweep (3.2) Plant (0.0) → Stomp (1.1) → Kick (3.5) Pillar Guard (0.0) → Pillar Guard (2.0) → Flick (4.0) | 5.1 s 4.3 s 4.9 s | 4.2–5.1 2.4–3.5 — | — 0.1–0.8 0.1–1.6, 2.1– 3.6 |
| F. Feint Bend | Body Legs Planted hand | False Bend (0.0) → Bend Sweep (1.4) → Iron Guard (3.3) Stride (0.0) → Plant (0.6) → Stomp (1.7) Knuckle Crush (0.0) → Flick (2.4) → Flick (3.3) | 4.9 s 4.1 s 4.2 s | 2.4–3.3 3.0–4.1 1.3–2.4 | 0.1–1.1, 3.5– 4.5 0.7–1.4 — |
| G. Siege Line | Body Legs Planted hand | Hammer Fists (0.0) → Hammer Fists (1.7) → Bellow 1.2 s (3.4) Kick (0.0) → Stomp (0.8) → Plant (3.2) Pillar Guard (0.0) → Knuckle Crush (2.0) | 4.6 s 4.3 s 4.4 s | 0.9–1.7, 2.6– 3.4 2.1–3.2 3.3–4.4 | — 3.3–4.0 0.1–1.6 |
| Combo | When | Usual follow-up | Read it → answer it |  |  |
| E. Plate | Opener (100%); Weights 25% | Feint Bend | Plates land at 1.3 s. Get behind |  |  |
| Throw |  | (60%) | one: the 2-row Bend (impact 4.2 s) stops at plates. Then hit the head 4.2–5.1 s from beside the plate. |  |  |
| Combo | When | Usual follow-up | Read it → answer it |  |  |
| F. Feint | Weights 40% | Siege Line | It bends left with no mark (the |  |  |
| Bend |  | (55%) | fake), then the real Bend marks the right side at 1.4 s (impact 2.4 s). Don't move on the unmarked bend. The head is Exposed 2.4–3.3 s. |  |  |
| G. Siege | Weights 35%; Trigger: | Feint Bend | Fists smash cover. Bellow (3.4 |  |  |
| Line | someone shelters behind a plate for 2 stops in a row | (60%) | s) calls 2 raiders if fewer than 2 are on the field: kill them or they block cells. The hands are Exposed 0.9–1.7 s and 2.6– 3.4 s. |  |  |

### Stage 3 — Collapse (33–0%)

Transformation (4 s): the fortress starts to fall and the Colossus drops to both knees for good. The head is now on the grid (row 5, lane 3, core always reachable) and its two hands become separate enemies. Every Knuckle Crush leaves Rubble (a blocked cell for 2 stops), so the grid closes up. Keep a path open to the head.

| Part | Cells | HP | Defence | Hit reaction | Notes |
| --- | --- | --- | --- | --- | --- |
| Head (core) | row 5, lane 3 | continues | 10 | Steadfast | Exposed during Gasp and Headbutt recovery |
| Left hand / right hand | 1 cell each, beside the head | 200 each | 16 | Steadfast | Hand Guard covers the head from that side only |

## Combo lines (numbers in brackets are start times in seconds)

| Combo | Part | Line | Ends | Openings | Guarded |
| --- | --- | --- | --- | --- | --- |
| H. Collapse | Head Left hand Right hand | Gasp 2 s (0.0) → Slag Spit (2.0) → Headbutt (3.8) Hand Guard (0.0) → Knuckle Crush (1.6) Knuckle Crush (0.0) → Hand Guard (2.4) | 4.9 s 4.0 s 4.0 s | 0.0–2.0, 4.4– 4.9 2.9–4.0 1.3–2.4 | — 0.2–1.2 2.6–3.6 |
| I. Rubble Rain | Head Left hand Right hand | Slag Spit (0.0) → Slag Spit (1.8) → Headbutt (3.6) Knuckle Crush (0.0) → Flick (2.4) → Flick (3.3) Flick (0.0) → Knuckle Crush (0.9) → Flick (3.3) | 4.7 s 4.2 s 4.2 s | 4.2–4.7 1.3–2.4 2.2–3.3 | — — — |
| J. Kneeling Guard | Head Left hand Right hand | Headbutt (0.0) → Gasp 2 s (1.1) → Slag Spit (3.1) Palm Sweep (0.0) → Flick (1.9) → Palm Sweep (2.8) Hand Guard (0.0) → Hand Guard (1.6) → Flick (3.2) | 4.9 s 4.7 s 4.1 s | 0.6–1.1, 1.1–3.1 — — | — — 0.2–1.2, 1.8– 2.8 |
| K. Last Stand | Head Left hand Right hand | Slag Spit (0.0) → Headbutt (1.8) → Headbutt (2.9) Knuckle Crush (0.0) → Knuckle Crush (2.4) Knuckle Crush (0.0) → Palm Sweep (2.4) | 4.0 s 4.8 s 4.3 s | 2.4–2.9, 3.5– 4.0 1.3–2.4, 3.7– 4.8 1.3–2.4 | — — — |
| Combo | When | Usual follow- up | Read it → answer it |  |  |
| H. Collapse | Opener (100%); Weights 25% | Rubble Rain (60%) | Opens Exposed (Gasp, 0–2 s), but the left hand guards it from that side, so come from the right after the right hand's Crush lands (1.3 s). |  |  |
| I. Rubble | Weights 40% | Kneeling | Two Crushes at 1.3 s and 2.2 s |  |  |
| Rain |  | Guard (55%) | leave Rubble. Watch where the path closes. Hit the hands during their Crush recoveries. |  |  |
| Combo | When | Usual follow- Read it → answer it up |  |  |  |
| J. Kneeling | Weights 35%; | Rubble Rain The head is Exposed 1.1–3.1 s. |  |  |  |
| Guard | Desperate (≤ 10%) | 40% (50%) The right hand guards that side, and the left hand sweeps at 1.0 s and 3.8 s. Slip in from the left between the sweeps (1.1–2.8 s). |  |  |  |
| K. Last Stand | Desperate (≤ 10%) | 60% Kneeling Desperate. No guards at all. Guard (50%) Trade hits on the head between Headbutts. |  |  |  |

## The Hymnal Roc

Bird · Light · mid–late game (tier II–III moves) · party of 3 · grid 4 lanes × 6 rows plus the Sky (off-grid, above every cell)

Twist: the boss is only on the grid when it attacks, and every attack ends in a weakness window. The Roc spends most of a chain in the Sky, where it can't be hit. It comes down to Dive (long window), Rake along a lane (short window), or rain feathers (no window). The window length is what tells the moves apart. You learn which swoop gives which window, pre-position for it, and burst.

- Shadow: a Dive throws its 2×2 landing shadow when the Dive starts. A Rake lights the whole lane it will travel.

- Wings are separate enemies (own HP) on the grid whenever the Roc is. Breaking a wing grounds it: Ascend becomes Scramble, a Roll with no Sky. Breaking both wings in Stage 1 or 2 skips straight to Stage 3. Conditions as well as HP can move a stage on. Grounded trigger: whenever a chain ends with the Roc on the grid, its next combo is the grounded take-off combo for that stage. Landing into a stop is always a bonus window.

- Skyshot (optional rule): First-aim projectiles (bows, Spark, Arcane Bolt) can hit the Roc in the Sky at 40% if it is above the shooter's lane. This gives ranged builds a role without removing the windows.

- Spill: 30% of wing damage reaches the body.

Moves (custom boss moves; "Based on" only names the human move whose timings were used as a starting template)

| Boss move | Part | Based on | Types | Completion | Key times | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| Dive | Body | Crushing Overhead | Attack | 2.4 | impact 1.3 | Exposed from impact to end. From the Sky onto its marked 2×2 shadow; it stays grounded until it Ascends. |
| Rake Pass | Body | Lunge | Attack + Dodge + Move | 1.1 | move 0.3, impact 0.5 | Exposed from impact to end. Swoops down a lit lane, hitting every cell in it, and lands at the lane's end. A short window. |
| Feather Volley | Body | Volley | Attack | 2 | impact 1.3 | From the Sky. Light feathers on marked cells. No window. |
| Ascend | Body | Pole Vault | Move + Dodge | 0.8 | move 0.4, dodge 0.1– 0.4 | Back into the Sky, with a dodge window. |
| Peck | Body | Stab | Attack | 0.5 | impact 0.25 | Grounded only. |
| Wing Slash | Body / wings | Slash | Attack | 1 | impact 0.5 | Grounded: hits the cells beside the footprint. |
| Wing Buffet | Body / wings | Palm Strike | Attack | 0.9 | impact 0.45 | Gust pushes a row back 1. |
| Wing Shield | Body / wings | Hanging Guard | Defend | 1.4 | window 0.1– 0.6–1.1 | Wings wrap the body. |
| Shriek | Body | Blast | Attack (Mana) | 1.8 | impact 1.0 | Light burst around a marked cell. |
| Beak Thrust | Body | Long Thrust | Attack | 1.3 | impact 0.7 | Exposed from impact to end. Stage 3. Reach 2. |
| Leap | Body | Pole Vault | Move + Dodge | 0.8 | move 0.4, dodge 0.1– 0.4 | Stage 3. A short hop into the Sky. |
| Chirp Bolt | Cantor | Spark | Attack (Mana) | 0.7 | impact 0.35 | Page 22 of 59 |
| Dart | Cantor | Lunge | Attack + Dodge + Move | 1.1 | move 0.3, impact 0.5 | Exposed from impact to end. A short swoop. |
| Harmony Ward | Cantor | Ward | Defend (Absorb) | 1.2 | window 0.1– 0.5–0.9 | Absorb ward cast on the Roc. It covers the Roc's landing. |
| Flit | Cantor | Roll | Move + Dodge | 0.8 | move 0.3 |  |

Hold animations — Roc: Glide (in the Sky); Cantors: Hover; Body (Stage 3): Pant (Exposed); Wings: Fold.

### Stage 1 — Circling (100–66% body HP)

One line: the Roc and its wings act as one animation, but the wing cells have their own HP. Landed, it fills a 2×2: the body is the back two cells and the wings the front two.

| Part | Cells | HP | Defence | Hit reaction | Notes |
| --- | --- | --- | --- | --- | --- |
| Body (core) | 2×2 when grounded (back cells) | 1,000 | 14 | Normal when Exposed, else Unflinching | Win condition. Weak to Gold (Light↔Gold). |
| Left wing / right wing | front cells of the footprint | 220 each | 12 | Normal | Breaking one grounds it |

## Combo lines (numbers in brackets are start times in seconds)

| Combo | Part | Line | Ends | Openings | Guarded |
| --- | --- | --- | --- | --- | --- |
| A. Descent | Roc | Glide 1.2 s (0.0) → Dive (1.2) → Ascend (3.6) | 4.4 s | 2.5–3.6 | — |
| B. Rake and Rise | Roc | Rake Pass (0.0) → Rake Pass (1.1) → Wing Shield (2.2) → Ascend (3.6) | 4.4 s | 0.5–1.1, 1.6– 2.2 | 2.3–3.3 |
| C. Hymn of Feathers | Roc | Feather Volley (0.0) → Glide 0.8 s (2.0) → Dive (2.8) | 5.2 s | 4.1–5.2 | — |
| D. Take Wing Combo When A. Descent Opener (100%); Weights (from the Sky) 25% B. Rake and Weights (from the Sky) Rise 40% C. Hymn of Weights (from the Sky) Feathers 35% D. Take Wing Trigger: it ended the last chain grounded | Roc | Peck (0.0) → Wing Slash (0.5) → Wing Shield (1.5) → Peck (2.9) → Ascend (3.4) Usual follow-up Read it → answer it Rake and Rise Shadow at 1.2 s, landing at (60%) 2.5 s. It's Exposed 2.5–3.6 s: stand diagonal to the shadow so you're out of it but next to the landing. Hymn of Feathers Two lit lanes, each with a (55%) short 0.6 s window where it lands at the end of the lane. Stand at a lane's far end to be first in. After the second Rake it wraps its wings (guarded 2.3–3.3 s), so hit the gap 3.3–3.6 s before it Ascends. — (always Feathers land at 1.3 s, then grounded: Take a Dive lands at 4.1 s and Wing) stays grounded into the stop (Exposed 4.1–5.2 s, then Take Wing). Rake and Rise It's grounded and on the (50%) grid until it Ascends at 3.4 s (in the Sky by 3.8 s). Wing Slash hits beside it at 1.0 s, so stay diagonal or at Reach 2, and don't swing into Wing Shield (1.6–2.6 s). Hit the wings now: breaking one means it can't Ascend. | 4.2 s | — | 1.6–2.6 |

### Stage 2 — Choir (66–33%)

Transformation (4 s, airborne, so Skyshot only): the Roc sings and two Cantors descend: small 1-cell fliers with their own HP and lines. While a Cantor lives, Harmony Ward covers the Roc's landings and shrinks its windows. The twist grows: to open the big bird you first kill the little ones, which are Exposed only after their own Darts.

| Part | Cells | HP | Defence | Hit reaction | Notes |
| --- | --- | --- | --- | --- | --- |
| Body (core) | as before | continues | 14 | as before | Feathers burn: Light damage |
| Wings | as before | regrow to 160 | 12 | Normal |  |
| Cantor ×2 | 1 cell, mostly in the Sky | 90 each | 6 | Normal | Mirrored pair |

## Combo lines (numbers in brackets are start times in seconds)

| Combo | Part | Line | Ends | Openings | Guarded |
| --- | --- | --- | --- | --- | --- |
| E. Choir Descends | Roc Cantors | Glide 1.2 s (0.0) → Feather Volley (1.2) → Dive (3.2) Dart (0.0) → Chirp Bolt (1.1) → Flit (1.8) → Hover 1.2 s (2.6) → Harmony Ward (3.8) | 5.6 s 5.0 s | 4.5–5.6 0.5–1.1 | — 3.9–4.7 |
| F. Antiphon | Roc Cantors | Rake Pass (0.0) → Ascend (1.1) → Rake Pass (1.9) → Ascend (3.0) → Dive (3.8) Dart (0.0) → Flit (1.1) → Dart (1.9) → Chirp Bolt (3.0) → Hover 1.2 s (3.7) → Harmony Ward (4.9) | 6.2 s 6.1 s | 0.5–1.1, 2.4–3.0, 5.1–6.2 0.5–1.1, 2.4–3.0 | — 5.0–5.8 |
| G. Radiant Shriek | Roc Cantors | Shriek (0.0) → Glide 0.8 s (1.8) → Feather Volley (2.6) Dart (0.0) → Dart (1.1) → Chirp Bolt (2.2) → Flit (2.9) | 4.6 s 3.7 s | — 0.5–1.1, 1.6–2.2 | — — |
| H. Rise Again Combo When E. Choir Opener (100%); Descends Weights (from the Sky) 30% F. Antiphon Weights (from the Sky) 40% G. Radiant Weights (from the Sky) Shriek 30% H. Rise Again Trigger: it ended the last chain grounded | Roc Cantors | Wing Slash (0.0) → Wing Buffet (1.0) → Wing Slash (1.9) → Ascend (2.9) → Shriek (3.7) Harmony Ward (0.0) → Chirp Bolt (1.2) → Chirp Bolt (1.9) → Flit (2.6) → Dart (3.4) Usual follow-up Read it → answer it Rise Again (100%, The Dive lands at 4.5 s, but grounded) the Ward (3.8–5.0 s) absorbs it, so wait until 4.7 s and hit the Roc until 5.6 s. The Cantors are Exposed 0.5–1.1 s after their opening Dart. Rise Again (100%, With both Cantors alive, grounded) the final Dive (landing 5.1 s) is warded almost the whole way. Kill the Cantors in their Dart windows (0.5–1.1 s, 2.4–3.0 s), and the dive window opens fully. Antiphon (60%) Pure pressure from the Sky with no Roc window. This combo is for killing Cantors (Exposed 0.5–1.1 s and 1.6–2.2 s). Radiant Shriek Grounded start. The Ward (55%) covers it until 1.2 s, then the Roc is on the grid until it Ascends at 2.9 s (gone by 3.3 s). Hit the wings, and watch the Buffet push at 1.45 s. A Cantor Darts at 3.4 s and is Exposed 3.9– 4.5 s. | 5.5 s 4.5 s | — 3.9–4.5 | — 0.1–0.9 |

### Stage 3 — Fallen Choir (33–0%, or both wings broken)

Transformation (3 s; Exposed for the last 1.5 s): the Roc crashes and stays grounded for good, filling a 2×2 in rows 5–6, lanes 2–3. Its wings now fight as separate limbs with their own line. The twist flips: it can still Leap for one Dive per chain, and that Dive is the only time it leaves reach.

| Part | Cells | HP | Defence | Hit reaction | Notes |
| --- | --- | --- | --- | --- | --- |
| Body (core) | rows 5–6, lanes 2–3 | continues | 14 | Steadfast |  |
| Wings | beside the body | regrow to 160 (stay broken if broken in Stage 2) | 12 | Normal | Broken wings leave Wings lines idle |

## Combo lines (numbers in brackets are start times in seconds)

| Combo | Part | Line | Ends | Openings | Guarded |
| --- | --- | --- | --- | --- | --- |
| I. Crash | Body Wings | Shriek (0.0) → Beak Thrust (1.8) → Peck (3.1) → Peck (3.6) Wing Shield (0.0) → Wing Slash (1.4) → Wing Buffet (2.4) | 4.1 s 3.3 s | 2.5–3.1 — | — 0.1–1.1 |
| J. Last Flight | Body Wings | Leap (0.0) → Dive (0.8) → Beak Thrust (3.2) Wing Buffet (0.0) → Wing Slash (0.9) → Wing Shield (1.9) → Wing Buffet (3.3) | 4.5 s 4.2 s | 2.1–3.2, 3.9–4.5 — | — 2.0–3.0 |
| K. Plucked | Body Wings | Beak Thrust (0.0) → Peck (1.3) → Pant 1.2 s (1.8) → Beak Thrust (3.0) Wing Slash (0.0) → Wing Slash (1.0) → Wing Shield (2.0) | 4.3 s 3.4 s | 0.7–1.3, 1.8–3.0, 3.7– 4.3 — | — 2.1–3.1 |
| L. Requiem | Body Wings | Shriek (0.0) → Shriek (1.8) → Peck (3.6) Wing Buffet (0.0) → Wing Slash (0.9) → Wing Buffet (1.9) → Wing Slash (2.8) | 4.1 s 3.8 s | — — | — — |
| Combo | When | Usual follow- Read it → answer it up |  |  |  |
| I. Crash | Opener (100%); Weights 25% | Plucked The Beak Thrust reaches 2 cells. (60%) It's Exposed 2.5–3.1 s after it. |  |  |  |
| Combo | When | Usual follow- Read it → answer it up |  |  |  |
| J. Last | Weights 35%; Desperate | Plucked Leap, shadow, then landing at 2.1 |  |  |  |
| Flight | (≤ 10%) 40% | (55%) s with a full Exposed window to 3.2 s, then a Beak Thrust (Exposed 3.9–4.5 s). With both wings broken it can't Leap, and this combo leaves the menu (its weight goes to Plucked). |  |  |  |
| K. Plucked | Weights 40% | Last Flight Pant at 1.8–3.0 s is Exposed, but (50%) Wing Shield (2.0–3.4 s) wraps it from the sides. Attack from the front or back. |  |  |  |
| L. Requiem | Desperate (≤ 10%) 60% | Last Flight Desperate. Two Light bursts: (50%) spread out. |  |  |  |

## The Hundred-Eyed Archive

Many-eyed weak-point boss · Storm · late game (tier III moves) · party of 3 · grid 5 lanes × 7 rows plus the Crown (off-grid, above the body)

Twist: you can only hurt it from the right cell at the right moment. The Archive is a 3×3 block in the middle of the arena (lanes 2–4, rows 4–6) with a ring of free cells around it, so the party can circle it. Eight eyes sit on its outer faces: front, back, left and right, plus the four corners, which face diagonally. Closed, the body is armoured (only 10% of damage gets through). An open eye takes full damage, from the ring cell it faces (Reach 1), from 2 away with a Reach 2 weapon, or by First-aim down its gaze line.

- Gaze: an open eye looks straight out (corner eyes look diagonally). Anyone in that line becomes Marked, and the eye's Glare fires at Marked cells. To hit an eye you must stand in its gaze: get in, hit during the window, and dodge, guard or step out for the Glare.

- Eye order: eyes open clockwise from the front (Front, Front-right, Right … Front-left), skipping destroyed ones. "Eye n" means the next in order, and "Eye n+1" the one after. The order is fixed, so you can pre-position for it.

- Destroying an eye (60 HP, damage to an open eye also hits the core in full) blinds that face for good. That face no longer gazes, its Lash stops, and its body cell becomes a Scar, hittable at any time for 50%. Progress carves out safe sides.

Moves (custom boss moves; "Based on" only names the human move whose timings were used as a starting template)

| Boss | Part | Based on | Types | Completion | Key times | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| move |  |  |  |  |  |  |
| Pulse | Body | Whirlwind | Attack | 2.2 | impact 1.2 | Storm discharge on every ring cell, 70%. |
| Lash | Body | Long Thrust | Attack | 1.3 | impact 0.7 | A tendril from a face with an open or recently open eye: Reach 2. |
| Index Sweep | Body | Wide Sweep | Attack | 1.9 | impact 1.0 | Tendrils sweep the 3 ring cells along one side, the side of the eye that is opening. |
| Shutter | Body | Shield Wall | Defend | 2 | window 0.1– 0.8–1.6 | All eyes shut, armour up. |
| Turn | Body | Step | Move | 0.6 | move 0.3 | Stage 2. Rotates the block a quarter turn, so every eye moves to a new face. |
| Glare | Eye | Arcane Bolt | Attack (Mana) | 1.4 | impact 0.8 | Exposed for the whole move. The eye stays open through the wind-up, fires a beam along its gaze line at Marked cells at impact, then shuts at the end. |
| Storm Lance | Core Eye | Aimed Shot | Attack | 1.6 | impact 1.0 | Stage 3. Pierces down lane 3. |
| Recall | Core | Blast | Attack | 1.8 | impact 1.0 | Stage 3. A burst centred |
| Pulse | Eye |  | (Mana) |  |  | on a Marked cell. |
| Open | Pages | Step | Move | 0.6 | move 0.3 | Stage 3. The outer pages slide out 1 lane, opening gaps beside the Core Eye. If they are already open, they stay open. |
| Close move | Pages | Shield Charge | Attack + Defend + Move | 1.2 | move 0.3, impact 0.6 | Stage 3. The pages slam shut, hitting and pushing anyone in a gap. |
| Riffle | Pages | Wide Sweep | Attack | 1.9 | impact 1.0 | Stage 3. Page edges sweep the outer ring. |
| Bind | Pages | Shield Wall | Defend | 2 | window 0.1– 0.8–1.6 | Stage 3. Pages shut, guarding the Core Eye. |

Hold animations — Body: Hum; every eye (including the Core Eye): Stare (Exposed); Pages: Rustle.

### Stage 1 — Watch (100–66% core HP)

One eye opens at a time, in order. Each eye's window is split by a body attack, so stand in the ring only while it's safe.

| Part | Cells | HP | Defence | Hit reaction | Notes |
| --- | --- | --- | --- | --- | --- |
| Archive (core) | 3×3, lanes 2– 4, rows 4–6 | 1,400 | 40 closed (10% gets through) | Unflinching | Win condition. Weak to Iron (Storm↔Iron). |
| Eye ×8 | one per outer face | 60 each | 0 when open | Staggers the core 0.5 s when hit open | Open during Stare and Glare |

## Combo lines (numbers in brackets are start times in seconds)

| Combo | Part | Line | Ends | Openings | Guarded |
| --- | --- | --- | --- | --- | --- |
| A. Awaken | Body Eyes | Pulse (0.0) → Index Sweep (2.2) Stare 1.2 s (0.0) → Glare (1.2) → Stare 0.8 s (2.6) → Glare (3.4) | 4.1 s 4.8 s | — 0.0–1.2, 1.2–2.6, 2.6–3.4, 3.4–4.8 | — — |
| B. Cross- Reference | Body Eyes | Lash (0.0) → Lash (1.3) → Index Sweep (2.6) Stare 1.2 s (0.0) → Glare (1.2) → Stare 1.2 s (2.6) → Glare (3.8) | 4.5 s 5.2 s | — 0.0–1.2, 1.2–2.6, 2.6–3.8, 3.8–5.2 | — — |
| C. Shut Archive | Body Eyes | Shutter (0.0) → Pulse (2.0) idle 2 s (0.0) → Stare 1.2 s (2.0) → Glare (3.2) | 4.2 s 4.6 s | — 2.0–3.2, 3.2–4.6 | 0.1–1.6 — |
| D. Total Recall | Body Eyes | Pulse (0.0) → Pulse (2.2) Stare 0.8 s (0.0) → Glare (0.8) → Stare 0.8 s (2.2) → Glare (3.0) | 4.4 s 4.4 s | — 0.0–0.8, 0.8–2.2, 2.2– 3.0, 3.0–4.4 | — — |
| Combo | When | Usual follow-up Read it → answer it |  |  |  |
| A. Awaken | Opener (100%); Weights 35% | Cross- Eye n is open 0–2.6 s, but Reference Pulse hits the whole ring at (60%) 1.2 s. Hit from Reach 2 early, or step in after the Pulse and dodge or guard its Glare at 2.0 s. Eye n+1 is open 2.6–4.8 s, and the Sweep on its side lands at 3.2 s. Step in after the Sweep, then dodge the Glare at 4.2 s. |  |  |  |
| B. Cross- Reference | Weights 40% | Shut Archive Opens Eye n and then the (50%) opposite eye (the back one if n is the front). One fighter goes round the back early. The Lashes reach 2 cells from the open face. |  |  |  |
| C. Shut Archive | Weights 25% | Awaken (60%) All eyes shut, and the ring gets Pulsed at 3.2 s. Leave the ring and Hold to recover stamina. Eye n peeks from 2.0–4.6 s for anyone with range. |  |  |  |
| D. Total Recall | Trigger: two or more party members in the ring | Shut Archive Punishes crowding the (60%) ring: two Pulses at 1.2 s and 3.4 s. Eye n is open 0–2.2 s (Glare at 1.6 s) and eye n+1 2.2–4.4 s (Glare at 3.8 s). Hit after each Pulse. |  |  |  |

### Stage 2 — Index (66–33%)

Transformation (3 s, Shuttered): the Archive grows tendrils into the four corner ring cells, which become hazards, and starts to Turn. Eyes now open two at a time on opposite faces with offset Glares, and a Turn mid-chain moves an open eye to a new face. The party must split up and track rotation.

| Part | Cells | HP | Defence | Hit reaction | Notes |
| --- | --- | --- | --- | --- | --- |
| Archive | as before | continues | 40 | Unflinching | Corner ring cells deal Storm damage on |
| (core) |  |  | closed |  | entry |
| Eyes | rotate with Turn | as before |  |  | Destroyed eyes stay destroyed; their Scars rotate too |

## Combo lines (numbers in brackets are start times in seconds)

| Combo | Part | Line | Ends | Openings | Guarded |
| --- | --- | --- | --- | --- | --- |
| E. Re-Index | Body Eye n Eye opposite | Turn (0.0) → Pulse (0.6) → Turn (2.8) → Lash (3.4) Stare 1.2 s (0.0) → Glare (1.2) → Stare 1.2 s (2.6) → Glare (3.8) Stare 1.2 s (0.0) → Glare (1.2) → Stare 1.2 s (2.6) → Glare (3.8) | 4.7 s 5.2 s 5.2 s | — 0.0–1.2, 1.2–2.6, 2.6– 3.8, 3.8–5.2 0.0–1.2, 1.2–2.6, 2.6– 3.8, 3.8–5.2 | — — — |
| F. Concordance | Body Eye n Eye opposite | Index Sweep (0.0) → Index Sweep (1.9) → Lash (3.8) Stare 2 s (0.0) → Glare (2.0) → Glare (3.4) Glare (0.0) → Stare 2 s (1.4) → Glare (3.4) | 5.1 s 4.8 s 4.8 s | — 0.0–2.0, 2.0–3.4, 3.4– 4.8 0.0–1.4, 1.4–3.4, 3.4– 4.8 | — — — |
| G. Redaction Pulse Combo When E. Re-Index Opener (100%); Weights 30% F. Concordance Weights 40%; Trigger: the whole party on one side of the block G. Redaction Weights 30% Pulse | Body Eye n Eye opposite | Shutter (0.0) → Pulse (2.0) idle 2 s (0.0) → Stare 1.2 s (2.0) → Glare (3.2) idle 2 s (0.0) → Stare 1.2 s (2.0) → Glare (3.2) Usual follow-up Read it → answer it Concordance Two Turns (at 0 s and 2.8 (60%) s) each rotate the block a quarter turn clockwise, so an open front eye moves to the right face, then the back. Follow your eye around the ring, and avoid the Pulse at 1.8 s. Redaction Pulse Offset eyes: the opposite (55%) eye fires first (0.8 s) and the front eye's 2 s Stare begins at once. Staggered windows mean the two fighters take turns. Re-Index (50%) Shut, then Pulse at 3.2 s, with both eyes opening at 2.0 s. Wait outside the ring, then step in at 3.3 s to hit before the Glares at 4.0 s. | 4.2 s 4.6 s 4.6 s | — 2.0–3.2, 3.2–4.6 2.0–3.2, 3.2–4.6 | 0.1–1.6 — — |

### Stage 3 — Redact (33–0%)

Transformation (4 s; Exposed for the last 1.5 s): the block splits into three 1×3 pages (columns). The middle page sinks, and the Core Eye descends from the Crown (off-grid) into its centre cell (row 5, lane 3). The two outer pages are separate enemies with one shared line. They slide open to expose the Core Eye through the gaps (lanes 2 and 4) and slam shut on anyone still inside. It is a door-timing fight.

| Part | Cells | HP | Defence | Hit reaction | Notes |
| --- | --- | --- | --- | --- | --- |
| Core Eye | row 5, lane 3 (Crown | continues | 0 (no | Steadfast | Takes full damage whenever |
| (core) | above) |  | armour left) |  | reached, through a gap or down lane 3. Exposed (bonus) during Stare and Glare. |
| Outer pages ×2 | 1×3 each, lanes 2 and 4 (1 and 5 when open) | 300 each | 24 | Unflinching | Breaking one leaves its gap open for good |

## Combo lines (numbers in brackets are start times in seconds)

| Combo | Part | Line | Ends | Openings | Guarded |
| --- | --- | --- | --- | --- | --- |
| H. Unbound | Core Eye Pages | Stare 1.2 s (0.0) → Storm Lance (1.2) → Glare (2.8) Open (0.0) → Riffle (0.6) → Close (2.5) | 4.2 s 3.7 s | 0.0–1.2, 2.8– 4.2 — | — — |
| I. Marginalia | Core Eye Pages | Glare (0.0) → Glare (1.4) → Recall Pulse (2.8) Bind (0.0) → Open (2.0) → Close (2.6) → Open (3.8) | 4.6 s 4.4 s | 0.0–1.4, 1.4– 2.8 — | — 0.1–1.6 |
| J. Final Edition | Core Eye Pages | Recall Pulse (0.0) → Stare 2 s (1.8) → Storm Lance (3.8) Riffle (0.0) → Open (1.9) → Riffle (2.5) → Close (4.4) | 5.4 s 5.6 s | 1.8–3.8 — | — — |
| K. Burn the | Core | Recall Pulse (0.0) → Recall Pulse (1.8) → | 5.0 s | 3.6–5.0 | — |
| Archive Combo When H. Unbound Opener (100%); Weights 30% I. Marginalia Weights 40% J. Final Edition Weights 30%; Desperate (≤ 10%) 40% K. Burn the Desperate (≤ 10%) Archive 60% | Eye Pages | Glare (3.6) Open (0.0) → Close (0.6) → Open (1.8) → Close (2.4) → Open (3.6) Usual follow- Read it → answer it up Marginalia The gaps open at 0.3 s, and (60%) the Core Eye is Exposed until 1.2 s. Storm Lance pierces lane 3 at 2.2 s, so attack from the gaps, not the front. The pages slam at 3.1 s: be out by 2.8 s. Final Edition A short gap at 2.3–2.8 s, then (50%) the pages reopen at 4.1 s into the stop, so the next combo starts with the gaps open. Marginalia The reward. The gaps are (55%) open 2.2–4.7 s and the Core Eye Stares 1.8–3.8 s, so burst from the gaps 2.2–3.8 s. The Riffle at 3.5 s hits the outer ring, not the gaps. Final Edition Desperate. The doors cycle (50%) every 1.8 s and each gap is only 0.6 s long (0.3–0.9 s, 2.1– 2.7 s, then 3.9 s into the stop). Pick one and commit. | 4.2 s | — | — |

## Lead-up bosses

Every section of the story gets a smaller lead-up boss before its main boss. Lead-ups are lighter:

- They can be humanoid and follow the regular battle rules, or borrow one boss element: parts, shadow marks, an entourage or a custom rule.

- One phase. Two only when the second phase is a real twist (the Drum-Major's broken cadence).

- Each previews one skill the main boss will test, so the main fight builds on something the player has already practised.

| Section | Lead-up boss | Main boss | Rules | Phases | Twist | Previews |
| --- | --- | --- | --- | --- | --- | --- |
| Prologue: Haven (the old shrine) | The Warden | Master Finn (sparring test) | Boss elements, standard 3 × 4 grid | 1 (two forms) | Splits into 3 shards and Reforms | Windows, guards, multiple enemies |
| The Borderlands (Agria) | Hask the Gatebreaker | Warlord Grimm (Iron Colossus) | Regular + entourage | 1 | War horn calls raiders; marked heavy swings | Shadow marks, punishing recoveries |
| Coinmere (Ploutar) | The Bailiff | The Ledger Kraken in Blackwood's vault | Regular + one part | 1 | Writ marks a fighter to be dragged in; the ledger part cancels writs | Source parts and guarding parts |
| Lumenvale (Kerys) | Brother Ashlock, the Purifier | High Inquisitor Mordecai and his Herald (Hymnal Roc) | Regular + cell hazards | 1 | Censers leave purity flames that deny cells | Reading safe cells, Light pressure |
| The Iron | Drum-Major | General | Custom | 2 | On-beat | Timing |
| March (Kratheon) | Varro | Thorne's War Wagon | moves + parts |  | moves, then off-beat when the drums break | adaptation, crew parts |
| The Rimeholds (Maioren) | The Twin Proctors | The Frost Conclave (Chorus of Five) | Regular + linked HP | 1 | Shared HP; full damage only when both are hit together | Synchrony |
| Noeth's Spire (Noeth) | The Hollow Herald | The Hundred- Eyed Archive | Regular + custom rule | 1 | Its signature combo copies the player's last chain | Isaren's mirror phase |
| The | The Pantheon | The six god | Boss | 1 | Its element | Element |
| Gathering | Gate | fights | elements, 8 |  | rotates each | coverage |
| (the six) |  |  | fighters |  | stop; only the opposed element hurts | across 8 fighters |
| The Ashen Heart (final) | The Warden, Last Watch | Isaren | Boss elements, 8 fighters | 1 (two forms) | The tutorial Warden returns with Sunder Wide and Reform anywhere | Bookends the game |
| Optional: sea routes | — | The Tollkeeper (Captain Bram's harbour quest) | Regular + guard parts | 1 | Adds a gold plate each stop that guards him | A side boss, not on the main path |

## The Warden

Living statue · Iron · tutorial lead-up boss (before Master Finn) · player + Kira · standard 3 lanes × 4 rows grid

The Warden guards the old shrine in the tutorial (it's the big cousin of the Stone Guardian enemy). The player and Kira fight it just before Master Finn. It teaches the boss vocabulary on the normal 3 × 4 grid: slow, heavy, readable hits, guards you mustn't swing into, Exposed windows, and one enemy that becomes three.

- Twist: Sunder and Reform. Merged, the Warden is one statue in one cell. Sunder splits it into three shards (left, Heart, right) across its row, each its own enemy with its own line. Reform pulls them back together. Its moves switch between the two forms, so its menu has two halves.

- Granite Skin (custom rule): defence 30, so normal hits barely scratch it, except while Exposed (defence 5). This is the tutorial's main lesson: wait for the window. Slow and heavy: every attack has a long wind-up (impact 1.2–1.8 s) and hits for about 40% of a starting fighter's HP. Shadow marks show at chain commit.

- One HP pool (300). Damage to any shard comes off the pool. The side shards also have 60 HP of their own. Breaking a side shard crumbles it: its line goes idle, the Warden Reforms without that arm, and it loses Ground Slam, which needs both arms. With both side shards broken it can't Sunder any more.

- Easy by design: no stamina (custom rule), no feints, and it never stays split for more than three combos: Three Stones, at most one Crossfire, then always Reform. Kira calls out each lesson the first time it happens ("It's wide open after the punch!").

## Moves (custom boss moves)

| Move | Part | Types | Completion | Key times | Aim | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| Stone Fist | Merged | Attack | 2.2 | impact 1.5 | Reach 1 | Exposed from impact to end. Heavy. The fist stays in the ground after impact. |
| Ground Slam | Merged | Attack | 2.6 | impact 1.8 | Whole row in front (3 cells) | Exposed from impact to end. Heavy. Marks the whole row in front; step back a row. |
| Bastion | Merged | Defend | 2 | window 0.2–1.0–1.8 | Self + both sides | Arms crossed. Swinging into the peak feeds its skip. |
| Lumber | Merged | Move | 1 | move 0.5 | 1 cell, any orthogonal |  |
| Sunder | Merged | Move | 1.8 | split 1.2 | Splits into 3 shards across its row | At 1.2 s it splits into Left shard / Heart / Right shard across its row. Anyone in those cells is pushed back 1. |
| Shard Slam | Side shards | Attack | 1.8 | impact 1.2 | Reach 1 | Exposed from impact to end. |
| Shard Roll | Side shards | Move | 1 | move 0.5 | 1 cell, any orthogonal |  |
| Gather | Side shards | Move | 1 | move 0.5 | Toward the Heart | Exposed for the whole move. Rolls back toward the Heart for Reform. |
| Heart Pulse | Heart | Attack | 2 | impact 1.4 | All 8 around, 50% | Punishes anyone standing next to the Heart. |
| Heart | Heart | Defend | 1.6 | window | Self + adjacent | Guard range covers the |
| Guard |  |  |  | 0.2–0.8–1.4 | shards | shards beside it. |
| Reform Hold animations — Warden: | Heart | Move | 2 Settle; Heart: Rattle; | merge 1.6 | Shards fly into the Heart's cell Side shards: Rattle. | Exposed for the whole move. The big window: the glowing core rune is exposed for the whole Reform; the shards lock back in at 1.6 s. |

## Merged form

One statue, starting in row 4, lane 2.

## Combo lines (numbers in brackets are start times in seconds)

| Combo | Part | Line | Ends | Openings | Guarded |
| --- | --- | --- | --- | --- | --- |
| A. Awakening | Warden | Settle 2 s (0.0) → Lumber (2.0) → Stone Fist (3.0) | 5.2 s | 4.5–5.2 | — |
| B. Fist and Wall | Warden | Bastion (0.0) → Stone Fist (2.0) | 4.2 s | 3.5–4.2 | 0.2–1.8 |
| C. Earthshaker | Warden | Lumber (0.0) → Ground Slam (1.0) → Settle 0.8 s (3.6) | 4.4 s | 2.8–3.6 | — |
| D. Sunder | Warden | Stone Fist (0.0) → Sunder (2.2) | 4.0 s | 1.5–2.2 | — |
| Combo | When | Usual follow-up Read it → answer it |  |  |  |
| A. Awakening | Opener (100%) | Fist and Wall Lesson 1, the window. It (60%) wakes slowly. Hits during the Settle barely scratch it (Granite Skin). The punch lands at 4.5 s. Step aside, then hit the stuck fist 4.5–5.2 s for full damage. |  |  |  |
| B. Fist and Wall | Weights 40% | Sunder (50%) Lesson 2, the guard. Don't swing into Bastion (0.2–1.8 s). Wait for the punch (impact 3.5 s), then hit 3.5–4.2 s. |  |  |  |
| C. | Weights 35%; Trigger: a | Sunder (50%) Lesson 3, marked cells. |  |  |  |
| Earthshaker | fighter next to it for 2 stops in a row | The whole front row lights up. Step back a row before 2.8 s, then step in for the window 2.8–3.6 s. |  |  |  |
| D. Sunder | Weights 25% | Three Stones Punch first (Exposed 1.5– (100%) (other 2.2 s), then it cracks apart form) at 3.4 s. Kira: "It's splitting! Watch all three!" |  |  |  |

## Split form

Three shards in a row. The Heart sits where the statue stood. The side shards are mirrored and share one line.

## Combo lines (numbers in brackets are start times in seconds)

| Combo | Part | Line | Ends | Openings | Guarded |
| --- | --- | --- | --- | --- | --- |
| E. Three Stones | Heart Side shards | Heart Guard (0.0) → Heart Pulse (1.6) → Rattle 0.8 s (3.6) Shard Roll (0.0) → Shard Slam (1.0) → Shard Slam (2.8) | 4.4 s 4.6 s | — 2.2–2.8, 4.0– 4.6 | 0.2–1.4 — |
| F. Crossfire | Heart Side shards | Heart Pulse (0.0) → Rattle 1.2 s (2.0) → Heart Guard (3.2) Shard Slam (0.0) → Shard Roll (1.8) → Shard Slam (2.8) | 4.8 s 4.6 s | — 1.2–1.8, 4.0– 4.6 | 3.4–4.6 — |
| G. Reform | Heart Side shards | Heart Pulse (0.0) → Reform (2.0) Shard Slam (0.0) → Shard Roll (1.8) → Gather (2.8) | 4.0 s 3.8 s | 2.0–4.0 1.2–1.8, 2.8–3.8 | — — |
| Combo | When | Usual follow-up Read it → answer it |  |  |  |
| E. Three | Opener | Crossfire (60%) Lesson 4, several enemies. The side |  |  |  |
| Stones | (100%) | shards roll toward you and slam at 2.2 s and 4.0 s. The Heart guards first. Break a side shard in its slam windows (2.2–2.8 s, 4.0–4.6 s), and stay out of the Heart's Pulse ring at 3.0 s. |  |  |  |
| F. Crossfire | Weights 50% | Reform (100%) Side slams at 1.2 s and 4.0 s, the Heart Pulse at 1.4 s. The Heart then rests (2.0–3.2 s) before guarding: hit it, or the side shards after each slam. |  |  |  |
| G. Reform | Weights 50% | Fist and Wall (60%) Lesson 5, the transformation (other form) window. After the Pulse, the shards fly home (2.8–3.8 s) and the Heart rune is exposed 2.0–4.0 s. This is the best damage of the fight. It ends merged. |  |  |  |

## The Warden, Last Watch

Living statue · Iron · lead-up boss before Isaren · 8 fighters · grid 5 lanes × 8 rows The same statue stands on the road into the Ashen Heart. It opens with the exact Awakening from the tutorial, so veterans recognise it instantly, then shows what it can do with an end-game team in front of it. One phase.

- Same rules, tier III numbers: Granite Skin (defence 60, 10 while Exposed), one HP pool, side shards that break, no stamina.

- New: Sunder Wide. The shards land on three different rows, not one. Eight fighters have to split into three groups.

- New: Reform anywhere. The Warden Reforms in the cell of whichever shard has the most HP left. Where it ends up depends on where the party spreads its damage, which makes the merge predictable but player-controlled.

- Story beat: the Heart rune now glows with a soul fragment, foreshadowing Isaren's first phase.

Moves: the same custom moves as the tutorial Warden, at tier III damage.

## Merged form

Starts in row 8, lane 3.

## Combo lines (numbers in brackets are start times in seconds)

| Combo | Part | Line | Ends | Openings | Guarded |
| --- | --- | --- | --- | --- | --- |
| A. Awakening (again) | Warden | Settle 2 s (0.0) → Lumber (2.0) → Stone Fist (3.0) | 5.2 s | 4.5–5.2 | — |
| B. Bastion Line | Warden | Bastion (0.0) → Ground Slam (2.0) | 4.6 s | 3.8–4.6 | 0.2–1.8 |
| D. Sunder Wide | Warden | Ground Slam (0.0) → Sunder (2.6) | 4.4 s | 1.8–2.6 | — |
| Combo | When | Usual follow-up Read it → answer it |  |  |  |
| A. Awakening | Opener | Sunder Wide (70%) Identical to the tutorial. It's a |  |  |  |
| (again) | (100%) | free read for anyone who remembers. |  |  |  |
| B. Bastion Line | Weights 50% | Sunder Wide (60%) Guard, then a row slam (impact 3.8 s). Leave the row, then hit 3.8–4.6 s. |  |  |  |
| D. Sunder Wide | Weights 50% | Three Watches Slam window 1.8–2.6 s, then the (100%) (other form) shards scatter to three rows at 3.8 s. |  |  |  |

## Split form (shards on three rows)

Each shard has its own row, so each group of fighters reads one shard.

## Combo lines (numbers in brackets are start times in seconds)

| Combo | Part | Line | Ends | Openings | Guarded |
| --- | --- | --- | --- | --- | --- |
| E. Three Watches | Heart Side shards | Heart Guard (0.0) → Heart Pulse (1.6) → Heart Pulse (3.6) Shard Slam (0.0) → Shard Roll (1.8) → Shard Slam (2.8) → Shard Roll (4.6) | 5.6 s 5.6 s | — 1.2–1.8, 4.0– 4.6 | 0.2–1.4 — |
| G. Reform Anywhere | Heart Side shards | Heart Pulse (0.0) → Reform (2.0) Shard Slam (0.0) → Shard Roll (1.8) → Gather (2.8) | 4.0 s 3.8 s | 2.0–4.0 1.2–1.8, 2.8– 3.8 | — — |
| Combo | When | Usual follow-up | Read it → answer it |  |  |
| E. Three Watches | Opener (100%); Weights 40% | Reform Anywhere (70%) | Side slams at 1.2 s and 4.0 s, Heart Pulses at 3.0 s and 5.0 s. Damage the shard you want it to Reform away from. |  |  |
| G. Reform | Weights 60% | Bastion Line (60%) | The Warden Reforms in the |  |  |
| Anywhere |  | (other form) | healthiest shard's cell, and the other shards Gather toward it (2.8–3.8 s). The rune is exposed 2.0–4.0 s wherever the Heart is. |  |  |

## The other lead-ups

Short designs. Signature chains use the 4 s budget and human-move timings unless marked custom.

Hask the Gatebreaker (Borderlands; claymore bandit and Grimm's gate captain)

- Twist: blowing his war horn (a Hold) calls 2 Desperate Raiders if fewer than 2 are up. His heavy swings show shadow marks, the first time the player sees them.

- Signature, "Horn and Hammer": Rally Horn 1.2 s → Crushing Overhead (marked, impact 2.5 s, Exposed after) → Shove. 4.4 s.

- Answer: kill raiders while he blows the horn, leave the mark, and punish the overhead's recovery.

The Bailiff (Coinmere; Blackwood's debt collector, here for Marcus's grandmother's debt)

- Twist: Writ of Distraint (custom, 0.8 s) marks a fighter, and his Throw always drags the marked fighter to him. His ledger sits on a lectern as a 1-cell part (80 HP). Breaking it voids every writ and stops new ones, a small version of Blackwood's signet ring. Signature, "Distraint": Writ → Block → Throw (the marked fighter) → Pommel Strike. 4.0 s.

- Answer: the marked fighter guards; someone else breaks the ledger.

## Brother Ashlock, the Purifier (Lumenvale; Mordecai's inquisitor)

- Twist: two censers on chains swing in Reach 2 arcs. Every cell a censer hits burns with purity flame (Light hazard) for 2 stops. The grid shrinks, then recovers.

- Signature, "Purge": Censer Swing → Censer Swing → Ward → Spark. 4.3 s.

- Answer: keep a path of clean cells; hit him while his Ward is down.

## The Tollkeeper (optional harbour boss; Captain Bram's side quest)

- Twist: Plate Up (custom, 1.0 s) places a gold plate, a 1-cell part (40 HP), next to him. Its guard range covers him. Plates stack up to 4, and every plate adds defence. Break plates as fast as he makes them, a small version of the Kraken's guarding tentacles. Signature, "Collect": Plate Up → Guarded Thrust → Shield Bash → Guarded Thrust. 4.7 s.

- Answer: break plates first; never let him reach 4.

Drum-Major Varro (Iron March; General Thorne's drill master; two phases)

- Phase 1, Cadence (custom): every move is exactly 1.0 s with impact at 0.5 s, so every chain is four beats with hits at 0.5, 1.5, 2.5 and 3.5 s. Two drummers (1-cell parts, 60 HP) keep the beat.

- Phase 2, Broken Cadence: at 50% HP, or when both drummers are broken (a

- condition skip), his moves become 1.5 s with impact at 1.0 s. The hits move off-beat (1.0, 2.5, 4.0 s). Players who learned the rhythm have to unlearn it.

- Answer: in Phase 1, dodge on the beat; break the drummers early to choose when the rhythm changes.

## The Twin Proctors (Rimeholds; keepers of Maioren's Hall of Lineage)

- Twist: two linked humanoids share one HP pool. A hit deals full damage only if the other twin is hit within 0.5 s; otherwise 50%. Twin B runs the same chain as Twin A, starting 0.5 s later, a small version of the Chorus of Five's canon.

- Signature, "Recitation": Slash → Parry → Slash → Riposte → Slash. 4.4 s, offset by 0.5 s on Twin B.

- Answer: pair up, one fighter per twin, and hit Twin B 0.5 s after Twin A.

The Hollow Herald (Noeth's Spire; the six's recurring fragment-hunter, finally cornered)

- Twist (custom rule): its signature combo, Precedent, copies the player character's previous chain, same moves and timings, from the mirrored cell. Its other combos are fixed. A preview of Isaren's mirror phase.

- Answer: end your chains with something you'd be happy to face next stop.

The Pantheon Gate (before the gods; a 2×2 shrine construct, 8 fighters)

- Twist: six god sigils light up in a fixed order, one per stop, and the Gate takes that sigil's element. Hits with the opposed element (Flame↔Frost, Storm↔Iron,

- Light↔Gold) deal ×2 and stagger; everything else deals 50%. The Wild sigil has no opposite, so on that stop it deals extra damage and the party guards.

- Answer: the eight fighters rotate who attacks based on the next sigil. The UI shows the next two sigils.

## More boss concepts

Shorter brainstorms, with no combo tables yet. Each follows the shared rules and has a twist the four headline bosses don't use.

## The Chorus of Five — linked bodies

Light · 5 humanoid figures in row 5 of a 5 × 6 grid

- Twist: Synchrony. The five share one HP pool. A hit deals full damage only if another member is hit within 0.5 s; alone, it deals 25%. This is the boss-system version of Master Finn's "hit both hands together". It needs two fighters with matched timing. Stage 1, Unison: all five run the same line at the same time. Syncing is easy, but they also guard at the same time.

- Stage 2, Canon: the same line, but each member starts 0.4 s after the one to its left, so guards and openings ripple down the row. The puzzle is finding the two neighbours whose openings overlap.

- Stage 3, Discord: the link breaks. Each survivor gets its own HP (a share of what's left) and its own short menu. It's chaotic but individually weak, the payoff for the tight Stage 2.

## The Hourglass Ossuary — your last chain comes back

Frost · core behind an ice wall at the back of a 5 × 6 grid; one bone Ancestor per party member

- Twist: Precedent. The Ancestors' signature combo copies each party member's previous chain, translated into the Ancestor's reskins at the same timings (an attack for an attack, a Hold for a Hold, a guard for a guard) and started from the mirrored cell. It's fully predictable, because you wrote it. A greedy all-attack chain comes back at you; a chain with a long Hold gives you a free window next time.

- Stage 1, Ancestry: Ancestors use fixed menus; the core is out of reach behind the ice wall. Flame melts wall cells and Storm cracks them.

- Stage 2, Precedent: the copy combo joins the menu at 40% weight. The party starts planning one chain ahead.

- Stage 3, Stasis: every cell an Ancestor ends a chain on freezes into a wall for 3 stops, so the grid shrinks while the core walks out. No move speed changes: frozen means blocked, not slowed.

## The Rootmother — the grid is the boss

Wild · the bulb (core) lives off-grid under the boss half

- Twist: roots erupt in cells as 1-cell parts with their own HP and simple lines, and can grow walls between cells. Connected roots form a network: Burn spreads along connected roots, and cutting a root splits the network.

- Stage 1, Thicket: root walls close lanes. Cut a path, or burn the network back. Stage 2, Bloom: flowers open on root cells and heal the bulb each stop. Kill them, or burn the root they grow from.

- Stage 3, Uprooted: the Rootmother stands up as a 3×2 walking body, dragging its remaining roots behind it as a moving hazard trail.

## The Toll Serpent — a conveyor body

Gold · the body coils around the off-grid perimeter and threads through the grid

- Twist: the serpent follows a fixed, visible path. Body segments on the grid are walls that slide one cell along the path each Step, pushing anyone in the way. Only the head and tail attack. Each segment has its own HP, and cutting one shortens the loop and reroutes the path.

- Stage 1, Coil: the path loops through the boss half.

- Stage 2, Constrict: the path crosses the party half, and three segments abreast squeeze the open cells.

- Stage 3, Shed: the old skin stays behind as static walls, and the fresh serpent takes a new path through what's left.

## The War Wagon — crew stations

## Iron · a 2×3 vehicle with crew parts

- Twist: each crew member runs one system. Driver (moves and rams), Gunner

- (artillery on marked cells), Standard-bearer (guard range over the crew), and the commander. Killing a crew member switches that system off for the rest of the stage. Stage 1, Deployment: the wagon drives across lanes. Marks show its ram path. Stage 2, Dismount: the broken wagon becomes cover in the middle of the grid. The commander and two elites fight around it.

- Stage 3, Duel: the commander alone, a human-sized boss with the tightest guard timings in the game. The contrast after two big-body stages is the point.

## Human-sized bosses: the entourage twist

Not every boss is a monster. Master Finn, the Bailiff, Brother Ashlock and Drum-Major Varro are human-sized. They meet the "fought as multiple enemies" rule through linked entourages: a 1-cell boss plus guards or devices that are parts of the same boss and share its decision point. Each entourage has a source part, such as the Bailiff's ledger, Ashlock's censers or Varro's drummers. Breaking the source stops that entourage from being replaced. Stages work the same way.

## Isaren — the final boss

Final boss · 8 fighters · grid 6 lanes × 8 rows plus the Halo (off-grid ring around the boss half) · lead-up: The Warden, Last Watch

The final boss is the one exception to the 3-stage rule: four combat phases, then The Choice as the non-combat ending. Each phase tests something different. First the team dodges while focusing one target, then it fights a weakened version of itself, then it duels one overwhelming fighter, and finally it refuses power together.

| Phase | Story beat (plot outline) | What it is | Twist | Ends when |
| --- | --- | --- | --- | --- |
| 1. Benevolent | Homecoming: he | The Throne | Each shard hunts | The Heart's |
| Authority | splits his soul again, mocking his old shattering | Heart and 8 shards, the 8 fragments the party carried | one fighter; hit the Heart while dodging yours | pool is empty |
| 2. Necessary | "Why do you need | The shards | Clones use each | All 8 clones |
| Control | each other? I can be all of you." | become clones of the team | fighter's own style; the team is weakened; you can't beat your own clone | are defeated |
| 3. The Last Fighter | He fights as the mortal he was before godhood | One human- sized fighter, extremely strong | A duellist with Momentum: passivity makes it stronger | Its pool is empty |
| 4. The Two | His philosophy: | Isaren | The Barrier only | Its pool is |
| Tiers | everyone equal beneath him, and him above | enthroned | breaks to Unison; safety beneath him and a "necessary" sacrifice must both be refused | empty |
| The Letting | Finn and the player | Non-combat | — | He lets go |
| Go | talk him down, and he breaks godhood | finale |  |  |

## Whole-fight custom rules

- Ignores stamina throughout (the Phase 2 clones are the exception: they are you). Each phase has its own HP pool. Nothing carries over, and no burst can skip a phase. Transitions are 4 s scripted chains: immune, then Exposed for the last 1.5 s. The exception is Phase 1→2, where the fragments dive into the fighters' shadows and nothing is left to hit.

## Moves (custom boss moves)

| Move | Part | Types | Completion | Key times | Aim | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| Perfect Solution | Throne Heart | Attack | 2.4 | impact 1.6 | 2 marked lanes, full length | Beams down two marked lanes. |
| Demonstrated | Throne | Attack | 2 | impact 1.2 | Ring: every cell | Punishes fighters |
| Success | Heart |  |  |  | within 2 of the Heart | crowding the Heart. |
| Benevolent Ward | Throne Heart | Defend | 1.8 | window 0.2–0.9–1.6 | Self |  |
| Descend | Fragment | Attack + Move | 1.4 | move 0.4, impact 0.8 | The bound fighter's cell (marked at move start) | Exposed from impact to end. Drops from the Halo onto its bound fighter's cell; on the grid only from impact to end. |
| Echo Bolt | Fragment | Attack | 1 | impact 0.6 | First, at the bound fighter | Fired from the Halo; can't be answered except by dodging or guarding. |
| Return | Fragment | Move | 0.6 | move 0.3 | Back to the Halo (off-grid) |  |
| Verdict | Isaren | Attack | 0.9 | impact 0.45 | Reach 1 | Fast cut. |
| Sentence | Isaren | Attack | 1.6 | impact 1.0 | Reach 2, depth 50% | Exposed from impact to end. Heavy thrust. |
| Equal Measure | Isaren | Attack | 1.8 | impact 1.0 | All 8 around | Spin: the answer to being surrounded. |
| Absolute Guard | Isaren | Defend | 0.8 | window 0.05–0.2– 0.5 | Self, Deflect | A tiny parry window with Deflect. |
| Step of Judgment | Isaren | Move + Dodge | 0.7 | move 0.3, dodge 0.0–0.4 | Blinks next to its target | Blinks next to the fighter who dealt it the most damage last stop. |
| Crown Breaker | Isaren | Attack | 2.6 | impact 1.8 | Reach 1 plus the whole row, marked | Exposed from impact to end. Enormous. Marked at chain commit. |
| Thousand Decisions | Isaren | Attack | 1.4 | impact 0.3 | Reach 1, 3 hits (0.3 / 0.7 / 1.1 s) | Three hits; guard all three or step out. |
| Offer Shelter | Enthroned | Special | 1.5 | effect 0.5 | 2 marked cells | See Shelter. |
| Throne Barrier | Enthroned | Defend | 2 | window 0.1–1.0–1.9 | Self (raises the Barrier) | Raises the Barrier again. |
| Mark Sacrifice | Enthroned | Special | 1 | effect 0.5 | One fighter | See Greater Good. |
| Judgment | Enthroned | Attack | 3 | impact 2.6 | The Sacrifice's cell (follows them) | Lethal to one fighter alone; shared by guards. |
| Benevolent | Enthroned | Attack | 1.6 | impact 1.0 | 2 marked lanes |  |

## Light

Hold animations — Throne Heart: Reflect (Exposed); fragment choirs: Circle; Isaren: Composure; Isaren (enthroned): Plead (Exposed).

## Phase 1 — Benevolent Authority: the Heart and eight fragments

The Throne Heart (core, 2×2 at rows 7–8, lanes 3–4) is always on the grid. Eight soul fragments, one bound to each fighter, circle in the Halo, an off-grid ring around the boss half. A fragment's attacks only ever aim at its own fighter's cell. So every player watches the Heart plus their own fragment: eight hunters, but each person reads one. For readability the fragments fly as two choirs of four that share lines. The job is to hit the Heart while dodging your fragment. Fragments can be destroyed in their Exposed windows (180 HP each) and re-form after 2 stops, which buys their fighter breathing room. The phase ends when the Heart's phase pool is empty.

| Part | Cells | HP | Defence | Hit reaction | Notes |
| --- | --- | --- | --- | --- | --- |
| Throne Heart (core) | rows 7–8, lanes 3–4 | phase pool 3,200 | 30 | Unflinching | Exposed during Reflect |
| Soul fragment | off-grid Halo; 1 cell | 180 each | 15 | Normal | Each bound to one fighter; |
| ×8 | when Descending |  |  |  | re-forms after 2 stops |

## Combo lines (numbers in brackets are start times in seconds)

| Combo | Part | Line | Ends | Openings | Guarded |
| --- | --- | --- | --- | --- | --- |
| A. Perfect Solutions | Throne Heart Choir A (fragments 1–4) Choir B (fragments 5–8) | Benevolent Ward (0.0) → Perfect Solution (1.8) Descend (0.0) → Return (1.4) → Echo Bolt (2.0) → Echo Bolt (3.0) Circle 1.2 s (0.0) → Descend (1.2) → Return (2.6) → Echo Bolt (3.2) | 4.2 s 4.0 s 4.2 s | — 0.8–1.4 2.0–2.6 | 0.2–1.6 — — |
| B. Demonstrated Success | Throne Heart Choir A (fragments 1–4) Choir B (fragments 5–8) | Demonstrated Success (0.0) → Reflect 1.2 s (2.0) → Perfect Solution (3.2) Echo Bolt (0.0) → Circle 0.8 s (1.0) → Descend (1.8) → Return (3.2) → Descend (3.8) Descend (0.0) → Return (1.4) → Circle 1.2 s (2.0) → Descend (3.2) → Return (4.6) | 5.6 s 5.2 s 5.2 s | 2.0–3.2 2.6–3.2, 4.6– 5.2 0.8–1.4, 4.0– 4.6 | — — — |
| C. Divine Shelter | Throne Heart Choir A (fragments 1–4) Choir B (fragments 5–8) | Benevolent Ward (0.0) → Benevolent Ward (1.8) → Demonstrated Success (3.6) Descend (0.0) → Return (1.4) → Descend (2.0) → Return (3.4) → Echo Bolt (4.0) Echo Bolt (0.0) → Descend (1.0) → Return (2.4) → Descend (3.0) → Return (4.4) | 5.6 s 5.0 s 5.0 s | — 0.8–1.4, 2.8– 3.4 1.8–2.4, 3.8– 4.4 | 0.2–1.6, 2.0–3.4 — — |
| Combo |  | When Usual follow-up |  | Read it → answer it |  |
| A. Perfect |  | Opener (100%); Demonstrated |  | The Heart guards to 1.6 |  |
| Solutions |  | Weights 30% Success (60%) |  | s, then beams two lanes at 3.4 s. It's hittable 1.6–4.2 s if you're not in a beam lane. Choir A drops at 0.8 s and Choir B at 2.0 s: sidestep your own mark, then hit your fragment if you want it gone. |  |
| B. Demonstrated Success |  | Weights 40%; Divine Shelter Trigger: three or more (50%) fighters next to the Heart |  | A ring pulse at 1.2 s, then Reflect: the Heart is Exposed 2.0– 3.2 s. Choir A drops into that window at 2.6 s, so its fighters have to dodge mid-burst. Choir B drops at 0.8 s and 4.0 s. |  |
| C. Divine Shelter |  | Weights 30% Perfect Solutions (60%) |  | The Heart shuts itself for 3.6 s: a fragment- hunting combo. Each choir descends twice, so destroy fragments in their windows to make the next combos safer. |  |

## Phase 2 — Necessary Control: the mirror team

Transformation: each fragment dives into its bound fighter's shadow and rises as a clone of that fighter in the mirrored cell across the midline. The Throne Heart goes dark and can't be targeted. This is "I can be all of you" turned on the player: Isaren becomes the team.

- The team is weakened. Each fighter's soul fragment now powers their clone, so every fighter has −20% Attack, Mana and Defence until their own clone is defeated. They get it back the moment it falls, so early clone kills snowball the team back to strength. The clones are you. Each has the original's 6 moves, weapon, armour, element and full- strength stats, with 60% of its HP. Clones follow the regular battle rules, stamina and Hold included, so they can be out-paced like any human enemy.

- Predictable by construction. Each clone's combo menu is built from its original's own play in Phase 1. Its opener is that fighter's most-used chain, and its other combos are their next two most-used chains, weighted by use. A player who spammed one chain faces that chain; a player who varied faces variety. Each clone's opener shows at Read 0.

- Knows You (custom rule). A clone takes 50% damage from its own original and is never staggered by them. You can't beat yourself, so fighters swap opponents: the spear user takes the archer's clone, and so on. No one wins this phase alone. The mirror pull. Each clone aims at its original first, so players have to reposition to swap partners. That movement is the skill test.

- Ends when all eight clones are defeated. Their fragments fly back into the Heart, which collapses for Phase 3.

## Phase 3 — The Last Fighter: one humanoid, extremely strong

The fragments fly back into the Heart, and it collapses into a single human-sized figure in one cell. Isaren sheds his godhood's grandeur and fights as the mortal he was before it, the best duellist in the game: fast cuts, a tiny parry with Deflect, a blink, a spin for anyone who surrounds it, and one enormous marked attack. Eight fighters against one cell means crowding, and Equal Measure punishes it.

Custom rules: ignores stamina. Unflinching except while Exposed. Momentum: each combo in which it takes no Exposed hit adds +10% damage (max +50%), and an Exposed hit resets it. Passive play gets more dangerous every stop. Step of Judgment always blinks next to the fighter who dealt it the most damage last stop, so the party can choose who it chases.

| Part | Cells | HP | Defence | Hit reaction | Notes |
| --- | --- | --- | --- | --- | --- |
| Isaren | 1 cell | phase pool | 35 | Unflinching except while | Deflects hits into Absolute |
| (core) |  | 2,600 |  | Exposed | Guard's peak |

## Combo lines (numbers in brackets are start times in seconds)

| Combo | Part | Line | Ends | Openings | Guarded |
| --- | --- | --- | --- | --- | --- |
| E. Unbound | Isaren | Step of Judgment (0.0) → Verdict (0.7) → Verdict (1.6) → Sentence (2.5) | 4.1 s | 3.5–4.1 | — |
| F. Absolute | Isaren | Absolute Guard (0.0) → Verdict (0.8) → Absolute Guard (1.7) → Thousand Decisions (2.5) → Verdict (3.9) | 4.8 s | — | 0.1–0.5, 1.8– 2.2 |
| G. Equal Measure | Isaren | Step of Judgment (0.0) → Equal Measure (0.7) → Composure 0.8 s (2.5) → Crown Breaker (3.3) | 5.9 s | 5.1–5.9 | — |
| H. Mercy | Isaren | Sentence (0.0) → Composure 1.2 s (1.6) → Sentence (2.8) | 4.4 s | 1.0–1.6, 3.8– 4.4 | — |
| K. Final Authority | Isaren | Crown Breaker (0.0) → Equal Measure (2.6) | 4.4 s | 1.8–2.6 | — |
| Combo | When | Usual follow-up | Read it → answer it |  |  |
| E. Unbound | Opener (100%); Weights 20% | Absolute (60%) | It blinks to last stop's top damage dealer, cuts twice (impacts 1.15 s and 2.05 s: guard them), then thrusts 2 cells deep at 3.5 s. Exposed 3.5–4.1 s. |  |  |
| F. Absolute | Weights 35% | Mercy (50%) | Two parries (0.05–0.5 s and 1.75–2.2 s) bait swings, so don't attack on them. No Exposed window; hit between the parries (0.5–1.7 s) or after the flurry. |  |  |
| G. Equal | Weights 20%; Trigger: three | Absolute | A spin at 1.7 s hits everyone |  |  |
| Measure | or more fighters next to it | (55%) | next to it, then a short Composure, then Crown Breaker (marked row, impact 5.1 s). Clear the row; Exposed 5.1–5.9 s: everyone in. |  |  |
| Combo | When | Usual Read it → answer it follow-up |  |  |  |
| H. Mercy | Weights 25%; Desperate (≤ 15%) 40% | Unbound The rest combo: two thrusts, (50%) each with an Exposed window (1.0–1.6 s and 3.8–4.4 s). Resetting Momentum here is the priority. |  |  |  |
| K. Final Authority | Desperate (≤ 15%) 60% | Mercy Desperate: the big marked (50%) hit (Exposed 1.8–2.6 s), then a spin at 3.6 s. Get in, burst, and get out before 3.6 s. |  |  |  |

## Phase 4 — The Two Tiers

Isaren rises onto the throne and stops trying to win by force. This phase is his two-tier world as a fight: perfect safety for everyone beneath him, paid for by letting him rule. Both offers are traps you win by refusing them together.

- Throne Barrier (custom rule): while it stands, Isaren takes no damage. It breaks only through Unison: hits from 4 or more different fighters inside one 1.0 s span. Then it stays down until the next Throne Barrier move. It's horizontal cooperation as a mechanic, and no single hero can do it alone.

- Shelter: Offer Shelter lights 2 cells. A fighter who ends a chain in a Shelter takes no damage for the next combo, but every hit they land heals Isaren. Safety under a ruler is paid for by making the ruler stronger. The offer is real and it's tempting. Refusing it is the right play.

- Greater Good: Mark Sacrifice picks one fighter, and Judgment follows them and lands on their cell. Alone, it downs them, and Isaren drops the Barrier as a reward, the utilitarian deal. Or the others stand next to the marked fighter and guard: Judgment's damage is split between everyone whose guard covers that cell, the shared burden. Small pool (900). When it empties, the fight moves to The Choice.

| Part | Cells | HP | Defence | Hit reaction | Notes |
| --- | --- | --- | --- | --- | --- |
| Isaren, enthroned (core) Combo lines (numbers in | 1 cell on the throne, row 8, lane 3 brackets are start times in seconds) | phase pool 900 | 20 | Steadfast | Barrier until Unison |
| Combo Part | Line |  |  |  | Ends Openings Guarded |
| J. Shelter Isaren (enthroned) | Offer Shelter (0.0) → Throne Barrier (1.5) → Benevolent Light (3.5) |  |  |  | 5.1 s — 1.6–3.4 |
| L. Greater Isaren Good (enthroned) | Mark Sacrifice (0.0) → Throne Barrier (1.0) → Judgment (3.0) |  |  |  | 6.0 s — 1.1–2.9 |
| M. Plea Isaren (enthroned) | Throne Barrier (0.0) → Offer Shelter (2.0) → Plead 1.2 s (3.5) |  |  |  | 4.7 s 3.5–4.7 0.1–1.9 |
| Combo When | Usual follow- up |  | Read it → answer it |  |  |
| J. Shelter Opener (100%); | Greater Good |  | Shelters appear at 0.5 s. Leave |  |  |
| Weights 35% | (60%) |  | them. Beams at 4.5 s. The Barrier is raised again at 1.5 s, so plan a Unison for after it. |  |  |
| L. Greater Weights 35% Good | Plea (50%) |  | A fighter is marked at 0.5 s and Judgment lands at 5.6 s. Gather the guards around them by then, or accept the deal. |  |  |
| M. Plea Weights 30% | Shelter (60%) |  | Plead (3.5–4.7 s) is Exposed, the best moment for the Unison burst. The Shelters offered at 2.5 s sit right next to the throne, as a final temptation. |  |  |

## The Letting Go

Not combat, and not a choice: this is the only ending. Beaten and finally listening, Isaren is confronted by Finn as a friend. Then the player says back to him everything the companions learned. He breaks the power that made him a god and returns the world's life to the land (see the plot outline). What happened in Phase 4 colours his lines: Shelters taken, sacrifices refused, Unisons achieved. Isaren can quote them back ("You stood in my shelter three times. And still you came").

## Mapping to the story bosses

Updated to match the revised plot outline. Every region has a lead-up boss and a main boss (the champion its god has blessed), and its god is fought later, in Act 4, with that region's companion at the heart of the plan. Three of the god fights reuse their champion's template at god scale: the champions were made in their god's image.

| Region | God | Lead-up | Main boss (template) | Key companion | God fight in Act 4 (template) |
| --- | --- | --- | --- | --- | --- |
| Haven (prologue) | — | The Warden | Master Finn, sparring | Kira | — |
| The | Agria | Hask the | Warlord Grimm (Iron | Vera | Agria (Rootmother) |
| Borderlands | (Wild) | Gatebreaker | Colossus) |  |  |
| Coinmere | Ploutar (Gold) | The Bailiff | The Ledger Kraken, guarding Blackwood's vault under the upper city | Marcus | Ploutar (Toll Serpent) |
| Lumenvale | Kerys (Light) | Brother Ashlock | High Inquisitor Mordecai and his Herald (a lesser Hymnal Roc) | Elena | Kerys (the full Hymnal Roc) |
| The Iron | Kratheon | Drum-Major | General Thorne's War | Alex, with | Kratheon (Iron |
| March | (Iron) | Varro | Wagon | Marcus | Colossus at god scale) |
| The | Maioren | The Twin | The Frost Conclave | Jakob | Maioren (Hourglass |
| Rimeholds | (Frost) | Proctors | (Chorus of Five) |  | Ossuary) |
| Noeth's Spire | Noeth (Storm) | The Hollow Herald | The Hundred-Eyed Archive | Kira | Noeth (the Archive at god scale) |
| The Gathering | the six | The Pantheon Gate | — | everyone | the six fights above |
| The Ashen Heart | Isaren (Flame) | The Warden, Last Watch | Isaren, four phases | everyone, with Finn | — |
| Optional | — | — | The Tollkeeper (Captain Bram's harbour quest) | — | — |

Teaching order: the Warden and Finn teach the basics. The Colossus teaches reading marks and punishing recoveries, and the Kraken teaches target priority and guards. The Roc teaches windows of different lengths, the War Wagon crew parts, the Conclave synchrony, and the Archive approach angles and door timing. The god fights then remix each region's lesson for 8 fighters, and Isaren tests all of them at once.

## Weak points and open questions

## Where the design is weakest

1. Reading several lines at once. A Kraken Stage 2 combo runs four lines at once. The combat logic is fine (one stop), but the UI has to show four timelines, or the tells become memorisation. Suggestion: Read 1 shows only the lead line plus any line that will hit the reader's own cell; Read 2 shows everything.

2. Spill rate is a balance lever with two bad ends. At 0% spill, hitting limbs feels wasted; at 50%, nobody bothers with the core's windows. 25–30% is a guess and needs the simulator.

3. Off-grid time can feel like dead time. If the Roc spends 60% of a chain in the Sky, melee fighters spend that time waiting. Mitigations: Skyshot for ranged builds, the Hold being the only stamina recovery (so waiting is never wasted), and helpers (Cantors) to kill during the gaps. It still needs playtesting.

4. Exposed ×1.5 plus stagger can snowball. A party that staggers the core during an Exposed window could chain staggers through the whole window. The existing diminishing returns (1 / 0.5 / 0.25 / 0 within 2 s) should apply to boss parts too. Proposal: count them per part.

5. Party size. Every number here assumes 3 fighters. God fights are 8 against 1, and on a 5-lane grid that is gridlock. Those bosses need 7–9 lanes and more parts (for example, two pairs of Colossus feet), not just more HP.

6. Stage skipping by burst (proposal: an HP gate until the transformation plays) conflicts with the Roc's intended condition skip. Decide whether gates apply only to HP thresholds.

7. Frozen on big bodies. Freezing a whole 3×3 core would be a huge, cheesy stun. Proposal: Frozen only applies to 1-cell parts; on bigger parts it becomes a Chill-style slow on that part's stagger recovery. That depends on the elements doc, so it needs checking against those rules.

8. Validator coverage. It checks timing legality only. It doesn't yet simulate positions, so "stand in lane 3" safety claims in the tells were worked out by hand and should go through the battle sim.

## Questions for you

- Is the Warden right as the tutorial's lead-up before Master Finn, or should it replace him as the tutorial's main boss?

- Is one shared Read level for all boss lines right, or should each line need its own Read (seeing a hidden off-grid line could be an upgrade)?

- Three god fights (Kratheon, Kerys, Noeth) reuse their champion's template at god scale. Is a reprise enough, or should any of them get a new template?

- Should breaking optional parts (feet, wings, eyes) give rewards (materials, unique custom-effect drops), so part-breaking is worth doing beyond speed?

- Next step: I can extend the battle simulator to multi-cell bosses (parts, spill, Exposed, single-stop batching) and run planner vs masher parties against the Colossus and the Kraken to tune HP and spill.
