# Seven Gods – First Arc ("The Awakening") Completion Plan

Source script: `first_arc_script.md` (24 scenes). This plan maps every scene onto what the engine and
Wren game code already do and lists what's missing.

**How the plan is used: iteratively.** **Iteration 1** (§4) builds the whole arc as a playable
start-to-end loop using only systems that already exist. Every missing feature appears in it as a
clearly marked **placeholder**: an NPC, sign or door that stands in for the real thing. Each later
iteration (§4.2) adds **one** feature and swaps out the placeholders it covers, so the demo stays
playable after every step. Sections 2–3 are the reference catalogue of what those features are.

---

## 0. Where the project stands today

| Area | What exists | Gap for the arc |
|---|---|---|
| **Overworld** | `ExperimentState` (start state): level streaming, rooms, `DoorWorp` warps, party followers, signs, items, interact prompts | No scene or chapter flow; levels are test/demo spaces (`Level_0–4`, `PlayerTent`, `Tent1–4`) |
| **Levels** | `SevenGods.ldtk` (entities: Critter, NPC, Item, Door, PressurePad, Sign, DoorWorp), `MemoryPalace.ldtk` (older prototype) | None of Haven, Whisperwood or the Shrine are built |
| **Dialogue** | JSON node graphs + editor window, choices, conditions, commands, localization keys, `[b]`/`[color]` rich text, `ChoiceRecord` | Missing commands for quests, tutorials, cutscenes, abilities, scripted fights and relationships. No speaker portraits |
| **Flags/saves** | `FlagManager` (bool + int), `inventory.sav`, `party.sav`, `moves.sav`, `defeated.sav` | No checkpoint or "new game" reset flow, no arc-end save |
| **Party** | `Party`, `joinParty`, follow trail, stay/follow, `PartySeparation` | Kira doesn't exist yet. Forced separation (Scene 10) needs a scripted version |
| **Combat** | Deterministic interrupt/timeline combat: 1 vs up to 3, parry prompts, Arbiter, EnemyAI, archetypes, `BattleStage` arena, non-lethal duels, rest/respawn, BattleTest sandbox (F6) | No authored encounters, no boss phases, no mid-battle dialogue, no "Heal/Spare" path, no XP |
| **Inventory** | Stacking items, categories, gold, Moves tab, Party tab, placeholder Levels tab | **Can't use consumables. Can't equip gear** (bonuses are only displayed). No XP or levels |
| **Puzzles** | `PressurePad`/`PressurePlate` → `Signals` → `Door`, `ImpactEvents` camera-pan reveals | No ordered sequences, rotatable pieces, beams or reveal-on-ability |
| **Presentation** | UIKit nine-slice, input hints (`[input=]`), `ImpactState` camera pans, camera shake, Audio binding | No cutscene runner, fades, narration overlay, quest toasts or tutorial pop-ups |
| **Art** | Sprite pipeline (6 generated NPC models) plus a large 2D asset library: `Actors/Character/*` (OldMan, Hunter, Woman, Child, Master, Statue… each with `Faceset.png`), `Monster/*` (Beast, Bear, Spirit, Butterfly…), `Boss/*` (GiantSpirit, GoldStatue-like…) | Need to pick and wire looks for ~9 characters and ~6 enemy types |

---

## 1. Design decisions to make first

The script was written for a real-time action RPG. The current game is a top-down 2D RPG with
**deterministic interrupt-timeline combat**. Each of these needs an answer before content work starts:

1. **Combat translation (recommended: keep the existing system).** Map the script's controls onto it:
   - Left-click attack / Shift quick strike / hold charged attack → **Speed / Power moves** in the move book
   - Block (right-click) → **parry prompt + Brace**
   - Dodge roll → **Dodge evade** (already in Arbiter)
   - Companion commands 1–4 → **Kira is a human-controlled party combatant** (Party already feeds combat)
   - "Use pillars for cover" / "barriers must be broken" → boss **status mechanics**: a shield that blocks damage until broken, and a beam that has to be parried or Braced
   - Rewrite the tutorial prompts in the script to match.
2. **Jumping (Scene 6, fallen logs).** Top-down with no jump: replace with a hop-over interact, or cut it.
3. **Mouse puzzles (Scene 2 rope drag).** The game uses keyboard/controller glyphs, so redo it as a directional-input sequence.
4. **XP and levels.** The script awards XP everywhere, but there's no level system. Choose:
   (a) build a small XP → level → attribute-point loop, or (b) drop XP for the demo and reward moves/gear/stat blessings instead.
   (b) fits the deterministic, attribute-driven combat better and is much smaller.
5. **Scope.** The script estimates 6–8 hours. A realistic demo target is **~1.5–2.5 hours**. Mark Scene 22's side quests and Scene 18's bear as stretch goals (see §6).
6. **Script continuity fixes** (cheap to fix now, expensive after localization):
   - Scene 12 repeats "Soul fragment approaches… But know this" twice (lines 513–519). Merge them.
   - "SPITE SPRITES" vs "Corrupted Sprites": pick one name.
   - The fusion montage names **Equalitas** before anyone has said the name. Either have Guide say it, or hold the name until the Memory Viewer.
   - Fragment count: the player gets fragment 1 (Scene 12), 2 (Scene 13) and 3 (Scene 16), but Finn says "You carry one. Two more lie in the shrine." Change it to "You carry three of seven."
   - Scene 16 has two overlapping "ALTERNATIVE" blocks for the violent path. Merge them into one spec.
   - Scene 8 dialogue option 1 refers to murals that only appear *after* the door. Move one mural outside or reword the option.
   - Elena's quest needs Moonbell "near the shrine", which forces backtracking. Move it to the forest edge or give it as a Thomas reward.

---

## 2. New systems (code)

Grouped by dependency, and sized roughly: S ≈ ½–1 day, M ≈ 2–3 days, L ≈ 1 week+.

### A. Story-flow foundation
1. **Quest / journal system (M).** `Quests/QuestLog.wren`: quest id → stage int (stored in `FlagManager` ints, so it's saved for free), localized title and objective per stage. Dialogue: `startQuest`, `setQuestStage` commands and a `questStage` condition. HUD objective tracker plus a "QUEST UPDATED" toast. A Journal tab in `InventoryUI`.
2. **Tutorial prompt system (S).** `UI/Gameplay/TutorialPromptUI.wren` using `UIKit.hint`. Each tutorial shows once (bool flag). Triggered by a dialogue command `showTutorial`, a trigger-zone entity, or code (first fight, first parry). Needs an "Accessibility → hide tutorials" setting.
3. **Cutscene / scripted-sequence runner (L, the biggest enabler).** Grow `ImpactState`/`ImpactEvents` into a general `CutsceneState` that runs a list of steps:
   `fade in/out`, `wait`, `camera pan/zoom/follow`, `moveActor(name, path)`, `faceActor`, `playAnim`, `dialogue(node)`,
   `narration(key)` (letterboxed V.O. text), `spawn/destroy`, `setFlag`, `playSound`, `teleportParty(level, spot)`.
   Steps are authored as JSON next to the dialogue files, or invoked with a `playCutscene` dialogue command. Used by the opening, separation, fusion, memory flashes, Memory Viewer, Finn reveal, farewell dinner and ending.
4. **Trigger zones (S).** A new LDtk entity `Trigger(Cutscene, Dialogue, Tutorial, OnceFlag, RequiresFlag)` for walk-in events such as Kira's forest comments, the claw-mark tree and blue-light sightings.
5. **Flag-gated entities (S).** Add `RequiresFlag` / `HiddenIfFlag` fields to NPC, Sign, Item and Trigger so the same level can show its morning, evening and post-shrine versions (Scenes 19, 22, 23).
6. **New game / continue / checkpoint flow (M).** The main menu goes to New Game (wipe saves) or Continue. Add an arc-end checkpoint save. Today `Experiment` boots directly and saves persist forever.

### B. Dialogue extensions
- **Commands:** `startQuest`, `setQuestStage`, `showTutorial`, `playCutscene`, `startEncounter(id)` (scripted fight, see C), `startPuzzle(id)`, `grantAbility`, `addIntFlag` (relationship/reputation deltas), `healParty`, `equip`, `fade`, `teleport`.
- **Conditions:** `questStage`, `hasFlag` (bool; only `hasIntFlag` exists today), `hasAbility`, `choiceWas` (on top of `ChoiceRecord`).
- **Speaker portraits (S–M):** show the `Faceset.png` from the actor folders in `DialogueBoxUI`.
- **Truth Sense (S):** a `[lie]...[/lie]` markup in localization text that becomes `[color=#7fb6ff]` when the player has the ability, and stays plain text otherwise. Used in Scene 20.
- **Ancient Knowledge (S):** signs and tablets with an `ancient` variant. Without the ability they show glyph text, with it they show the translation.
- **Kira's party banter:** her lines in Scenes 6–7 and 18 are just trigger-zone dialogue with Kira as speaker.

### C. Combat content and mechanics
1. **Scripted encounters (M).** `Combat/Config/Encounters.wren`: id → enemy defs, arena backdrop, canFlee, onWin/onLose dialogue/cutscene, rewards. Started by `startEncounter` or a trigger, instead of only wandering critters. Covers Thornwolves ×2, Spite Sprites ×3, Runic Constructs ×2, Bramble Bear and the Warden.
2. **Thomas's combat tutorial (S).** Reuse the non-lethal **duel** path against `BattleCritter.trainingDummy` with Kira in the party. Add a guided mode: the tutorial prompts appear at the first choice, first parry prompt and first party-member turn.
3. **Solo fight (S).** The encounter flag `excludeParty: ["Kira"]` for the Runic Constructs during the separation.
4. **New enemy mechanics (M):**
   - *Pack tactics* (Thornwolf): a stat bonus while another ally is alive (Arbiter hook)
   - *Regenerating shield* (Runic Construct): shield HP that refills if not hit within N time units
   - *Fast/evasive* (Sprite): high agility, low HP
5. **Boss: Forgotten Warden (L).** HP-threshold **phases** (200 → 100 → 25) that swap move set and AI and play a mid-battle line or cutscene. Phase 2 adds a barrier and a charged beam. At phase 3, battle pauses into a **dialogue choice: Talk vs Keep Fighting**, which sets `warden_resolution = peaceful|violent` and changes the rewards.
6. **Heal / Spare action (M).** Once Empathic Healing is unlocked, a combat option on *corrupted* enemies (Thornwolf, Bear): ends the fight peacefully, sets a flag, and grants different rewards. This drives the "Healer's Heart" achievement and the bear-cubs payoff.
7. **Kira's battle kit (S).** A ranged archetype or moves ("Aimed Shot" weak-point, "Pinning Shot" stagger) plus her post-Scene 21 upgrades as moves or attribute bonuses.
8. **Rewards on win (S).** Encounter rewards → inventory, plus XP if decision 4(a) is chosen.

### D. Player abilities ("Soul" powers)
- `Abilities.wren` registry (owned flags, saved) plus an Abilities section on the character sheet.
- **Soul Sight (M):** a toggle key (or always-on after fusion) that reveals `Hidden` entities (doors, compartments, markers, auras) and tints corruption. Needs a `Revealable` component and a screen tint/shader pass (`assets/Shaders` exists).
- **Empathic Healing (S):** an overworld interact on corrupted creatures and a combat move (reuse `Mend` → heal ally).
- **Truth Sense / Ancient Knowledge:** dialogue-only (see B).

### E. Inventory, equipment, consumables
- **Use consumables (S–M):** Healing Herb, Bread Roll and Special Ration (HP + stamina) from the inventory, and possibly in battle as an "Item" action.
- **Equip gear (M):** weapon/armor slots per party member, applied to `Stats` via the existing `bonuses`. A character-sheet page (`C` key per the script). Items: Simple Iron Sword, Leather Armor, Hunter's Bow +1, Soul-Touched Blade, Thomas's hunting gear.
- **Forge upgrade (S–M):** Finn's hammer menu spends fragment energy to upgrade the sword or bow (choose one bonus path).
- **Item database entries** for ~25 new items: Village Key, Old Map, Emergency Horn, Thornwolf Pelt, Sprite Essence, Ancient Gear, Ancient Knowledge Codex, Guardian's Key, Communication Crystal, recipe herbs, etc.

### F. Puzzle toolkit
Build generic pieces, then author the puzzles from them in LDtk:
| Piece | Used by |
|---|---|
| **SequenceLock** (ordered inputs; a wrong input gives a gentle chime and resets) | Shrine pedestals (Scene 8), Crystal Memory (10), Memory Viewer (17), Well rope (2, as a directional sequence) |
| **Rotatable** (interact to rotate 90°, saved) | Light-bridge crystals (9), Water channels (10) |
| **Beam/Flow resolver** (trace from a source through rotatables to a sink, draw the beam or water) | Light bridge, Aqueduct |
| **Bridge/Barrier** (collision toggled by a Signal) | Light bridge walkway, separation barriers |
| **Revealable** (visible only with Soul Sight) | Hidden compartment (13), secret doors |
| **Readable tablet** (Sign plus ancient variant) | Poem tablets, murals, stone pillars |

### G. Arc wrap-up and meta
- **World map transition (S–M):** a simple full-screen illustrated map with an animated route (Scenes 6 and 24), plus the Act II destination choice saved as a flag.
- **Relationship/reputation tracking (S):** int flags `rel_kira`, `village_rep`, `village_harmony`. Optionally show them on the Party page.
- **Arc completion screen (S):** stats (fragments, peaceful resolutions, combats, play time) and achievements stored in a flags/achievements save. Play time needs a timer.
- **Audio pass (M):** village, forest, shrine and boss music, plus SFX for puzzle chimes, door chord, fusion and wolves (check what the `Audio` binding supports).
- **Lighting/time of day (S–M):** dawn, day and evening tints for Haven (a screen tint is enough).

---

## 3. Content to author

### Levels (LDtk, `SevenGods.ldtk`)
| # | Level | Scenes | Key entities |
|---|---|---|---|
| 1 | Finn's Workshop: upstairs bedroom + workshop floor | 1, 20 | Bed spawn, Finn, forge (hidden compartment), stairs warp |
| 2 | Haven Village Center | 2, 5, 19, 22, 23 | Elena, Garrett + well, child(ren), storage buildings (Village Key doors), gathering spot, evening variant |
| 3 | Training Grounds | 3 | Kira, archery targets, dummies |
| 4 | Thomas's Cabin + yard arena | 4 | Thomas, practice dummies |
| 5 | Whisperwood: Forest Path | 6 | Claw-mark tree, Thornwolf encounter trigger, banter triggers |
| 6 | Whisperwood: Deep Forest + Sprite Clearing | 7, 18 | Blue lights (optional path), sprite encounter, mural pillars, bear encounter (return), wolf-heal spot, treasure cache |
| 7 | Shrine Courtyard | 8 | 5 pedestals, 5 tablets, vine door |
| 8 | Shrine Upper Hall: Light Bridge | 9 | 3 rotatable crystals, gap, bridge collider |
| 9 | Separation Chamber + Player Wing | 10, 11 | Trap plate, barriers, aqueduct channels, crystal ring, construct arena with pillars |
| 10 | Soul Fragment Chamber | 12 | Crystal centerpiece, murals, fusion cutscene anchor |
| 11 | Memory Hall | 13 | Revealable compartment, Soul-Sight mural details |
| 12 | Guardian Chamber | 14–16 | Warden (scripted encounter), throne mechanism |
| 13 | Deep Archive | 17 | Memory Viewer sequence, lore readables |

Decide whether to rebuild over `Level_0–4`/tents or keep them as a dev playground (recommended: keep them, add the arc in new levels, and point New Game at the workshop bed).
Haven can share one tileset family. The shrine needs a ruined-stone/crystal tileset; check the asset library before committing.

### Characters (look + NPCData + battle def where needed)
Finn, Elena, Garrett, Kira (follower + battle sheet), Thomas, Village Child(ren) ×1–3, Guide (light-orb effect, not a sprite), Forgotten Warden (Boss sheet, e.g. a statue/golem from `Actors/Boss` or `Character/Statue`/`GoldStatue`).
The `NPCData.forName` table is keyed by name and currently maps 12 names onto 6 generated models. Either add new pipeline models or allow the 2D `Actors/Character/*` sheets as an NPC look (a small loader change).

### Enemies (BattleCritter defs + overworld looks)
Thornwolf (`Monster/Beast`), Spite Sprite (`Monster/Spirit`/`Butterfly`), Runic Construct (`Character/Statue`/`Robot*`), Bramble Bear (`Monster/Bear`), Forgotten Warden (Boss), Training Dummy (exists). Healed variants of the Thornwolf and Bear for the overworld afterwards.

### Dialogue and localization
- New dialogue files: `HavenDialogue.json` (Finn, Elena, Garrett, child, Thomas), `KiraDialogue.json` (recruit, banter, Scene 21), `ShrineDialogue.json` (Guide, Warden, tablets), `ArcEndDialogue.json`.
- Rough volume: **~300–400 localization keys** in `en.json`. French can wait until the script is locked.
- Every "all paths converge" block maps directly onto the existing `choices` → `next` pattern. Branches that matter later (violent Warden, healed animals, side quests done, Act II pick) set flags.

### Cutscenes (step lists for the runner)
Opening dawn pan + narration, Finn reveals the map, separation trap, soul-fragment fusion + montage, second-fragment war flash, Warden memory flash, Warden kneel/throne, Memory Viewer history, Finn's forge reveal, village dinner (3 variants by side quests done), closing narration + stats.

---

## 4. Iterations

### 4.1 Iteration 1 – Playable arc skeleton ("greybox loop")

> **Status (2026-11-14):** the code side is done. That covers `Arc1Dialogue.json` and its localization, the game
> booting into `GameWorldState` with no hard-coded spawns, story gating (`World/StoryGates.wren`), duel follow-ups,
> the `endArc` end card, and the cast's looks, items and fighters. **Left to do: build the levels in LDtk, following
> [Arc1LevelGuide.md](Arc1LevelGuide.md).**

**Goal:** a player can start a new game, play Scenes 1 → 24 in order with every story NPC from the
script, and reach an "End of Arc 1" screen. It should take about 15–25 minutes. Nothing new is built
that a later iteration would replace. Where a feature is missing, a placeholder stands in for it and
is tagged `[PH]` in its dialogue text and in this table.

**Only systems that already exist are used:** `ExperimentState` overworld, `spawnNpc`/LDtk NPCs, dialogue
JSON (choices, conditions, `giveItem`, `setIntFlag`, `hasIntFlag`, `hasItem`, `joinParty`, `startDuel`,
`openDoor`, `restParty`), Key doors (`Door` with `DoorType=Key`), `DoorWorp` level links, wandering
`Critter` fights, NPC duels (`BattleCritter.fromNpc`, non-lethal), and `GameOverState` (title/continue text)
for the end card.

**Progress is one int flag, `arc1_stage`.** Every NPC's dialogue entry node branches on it with
`hasIntFlag`. That is the whole quest system until Iteration 2.

| Stage | Scene(s) | Where | What the player does | Placeholder `[PH]` used |
|---|---|---|---|---|
| 0 | 1 | Finn's Workshop | Wake up, talk to **Finn** (3-option menus from the script). Gets Iron Sword and Healing Herb ×3. Stage → 1 | Opening narration = a Sign by the bed. No cutscene |
| 1 | 2 | Village Center | **Elena** gives Bread Rolls ×5. **Garrett** asks about the well: a choice "Left/Down/Right" sequence in dialogue (wrong choice → "Hmm, not quite"). Success gives the Village Key | Well puzzle = dialogue-choice NPC |
| 1→2 | 3 | Training Grounds | **Kira** recruit dialogue → `joinParty Kira`. Stage → 2 | — |
| 2→3 | 4 | Thomas's Cabin | **Thomas** dialogue → `startDuel` vs Thomas (non-lethal) as the combat tutorial. After the win he gives Leather Armor and the Emergency Horn. Stage → 3 | Tutorial prompts = Thomas's lines. Gear only goes in the inventory (no equipping) |
| 3→4 | 5 | Village Center | **Elena** gives Special Rations. **Village Child** asks 3-option question. **Finn** at the village gate gives the **Old Map**. Stage → 4 | — |
| 4 | 6 | Village gate → Forest Path | A Key door that needs the Old Map ("The overgrown trail") leads out of the village | World map = a `DoorWorp` |
| 4 | 6–7 | Forest Path / Deep Forest | 2–3 wandering Critters as **Thornwolves** and an optional cluster as **Sprites**. A Sign for the claw-mark tree and another for the mural pillars, with Kira's lines as sign text | Enemies reuse the Chinlin/Chicken species (names only). Kira's banter = Signs |
| 4→5 | 8 | Shrine Courtyard | 5 tablet Signs (the poem). A **"Pedestal Circle"** NPC asks the order as a 5-step choice chain. Correct → `openDoor Shrine Door`. Stage → 5 | Pedestal puzzle = dialogue-choice NPC + door |
| 5 | 9 | Shrine Upper Hall | **"Crystal Array"** NPC: a "rotate crystals" choice → `openDoor Light Bridge` | Light bridge = NPC door |
| 5→6 | 10–11 | Separation Chamber | A Sign: "The floor shifts – Kira is cut off!" (`partyStay Kira`). **"Aqueduct"** and **"Crystal Ring"** NPC puzzles open doors. **Runic Construct** = an NPC duel (solo, because Kira is staying behind) | Separation = `partyStay`. Constructs = NPC duel |
| 6→7 | 12 | Soul Fragment Chamber | **Guide** NPC (a glowing NPC look) runs the full fusion dialogue. Gives the "Soul Fragment" item. Stage → 7 | Fusion = dialogue only. Abilities = flags with no effect yet |
| 7 | 13 | Memory Hall | Rejoin Kira (`partyFollow Kira`) and have the Scene 13 talk. A "Hidden Compartment" Sign gives fragment 2 | Soul Sight reveal = a plain Sign |
| 7→8 | 14–16 | Guardian Chamber | **Forgotten Warden** NPC: dialogue → `startDuel Warden`. After the win, the Talk-vs-Fight choice sets `warden_resolution` and gives fragment 3. Stage → 8 | Boss phases = one duel + dialogue after. Both paths only change dialogue |
| 8→9 | 17 | Deep Archive | An **"Memory Viewer"** NPC tells the history in dialogue and gives the map/records. Stage → 9 | Memory Viewer = dialogue |
| 9 | 18 | Forest (return) | Same forest. A Sign beside a "Wounded Thornwolf" NPC offers Heal / Leave, setting `healed_wolf` | Empathic Healing = dialogue choice |
| 9→10 | 19–20 | Village → Workshop | **Child** greets you on return. **Finn** reveal dialogue (lie line written as `[color]` text for now), map talk, "path forward" choice. Stage → 10 | Truth Sense = always-on colored text. Forge upgrade skipped |
| 10→11 | 21 | Village well | **Kira** Scene 21 talk. Stage → 11 | Kira upgrades skipped |
| 11 | 23–24 | Village gathering spot | **Finn** farewell (variant by `warden_resolution`). Act II choice (`act2_path`). End → `GameOverState` "End of Arc 1 – thanks for playing" → main menu | Dinner, stats and achievements = text on the end card |

Scene 22 side quests are left out of Iteration 1. They are added later as their own iteration.

**Work items for Iteration 1:**
1. **Levels (LDtk greybox).** Workshop (2 rooms), Village Center, Training Grounds, Thomas's Cabin, Forest Path, Deep Forest, Shrine Courtyard, Shrine Upper, Separation Chamber, Fragment Chamber, Memory Hall, Guardian Chamber, Deep Archive, connected with `DoorWorp`. Reuse existing tilesets, with layout only and no art pass. The existing `Level_0–4` and tents stay as the dev playground.
2. **Spawning.** Place story NPCs as LDtk `NPC` entities, or via `spawnNpc` in a small `Story/Arc1Spawns.wren` that `ExperimentState.enter` calls. Gate who stands where by `arc1_stage` (Kira and Finn move between scenes), using `if` checks in that one file.
3. **Looks.** Add Finn, Elena, Garrett, Kira, Thomas, Child, Guide, Warden and the puzzle stand-in names to `NPCData.forName`, mapped onto the 6 existing generated models. Add `BattleCritter` defs for the duelists Thomas, Runic Construct and Warden with `fromNpc`.
4. **Items.** Add to `ItemDatabase`: Healing Herb, Bread Roll, Special Ration, Village Key, Leather Armor (Iron Sword exists), Hunter's Bow, Emergency Horn, Old Map, Soul Fragment, Ancient Records.
5. **Dialogue.** `assets/Dialogue/Arc1/` (one JSON file per character since 2026-11-23 - it started as one `Arc1Dialogue.json`), with the script's lines as `en.json` keys under `dialogue.arc1.*`. Apply the continuity fixes from §1.6 while transcribing.
6. **New game and start point.** Point `ExperimentState.startPosition` at the workshop bed. Add a "New Game" to the main menu that clears the save files (`flags/inventory/party/moves/defeated.sav`) and enters `Experiment`. Switch the boot state to `MainMenu`.
7. **End card.** The final Finn dialogue triggers `GameOverState` with the arc-end title. This needs a tiny dialogue command, `endArc`, or reuse of an event ExperimentState already listens for.
8. **Playthrough check.** A full run from New Game to the end card. Also check that saving and quitting mid-arc resumes at the right stage.

**Done when:** a fresh player can finish the arc without debug keys, every script scene has a
stand-in, and every `[PH]` is listed in the table above.

### 4.2 Later iterations – one feature each

Each iteration names the placeholders it removes. They are listed in the recommended order, but after
Iteration 1 any order works, because the loop stays playable.

| # | Feature (see §2) | Replaces these `[PH]` | Notes |
|---|---|---|---|
| 2 | **Quest log + HUD tracker + "Quest Updated" toast** (A1) | `arc1_stage` used as an invisible quest | Wraps `arc1_stage`. No content changes |
| 3 | **Tutorial prompts** (A2) | Thomas's "tutorial" lines | Add prompts at the script's tutorial beats |
| 4 | **Consumables + equipment + character sheet** (E) | Gear only sitting in the inventory | Herbs, rations, sword, armor, bow |
| 5 | **Cutscene runner** (A3) | Opening Sign, separation Sign, flat fusion dialogue | The biggest enabler. Do the opening first |
| 6 | **Trigger zones + flag-gated entities** (A4–A5) | Kira's banter as Signs, the same village at all times | Morning/evening village, walk-in banter |
| 7 | **Scripted encounters + real enemy defs** (C1, C4) | Chinlin/Chicken stand-ins, Construct duel | Thornwolf pack, Sprites, Construct shield |
| 8 | **Kira's battle kit** (C7) | Kira fights as a generic NPC | Ranged moves |
| 9 | **Puzzle toolkit: SequenceLock** (F) | Pedestal NPC, Well NPC, Crystal Ring NPC | One component, three puzzles |
| 10 | **Puzzle toolkit: Rotatable + beam/flow + bridge** (F) | Crystal Array NPC, Aqueduct NPC | |
| 11 | **Forced separation** (C3 + barriers) | `partyStay` Sign | Barrier doors + solo encounter flag |
| 12 | **Abilities: Soul Sight + Ancient Knowledge** (D) | Hidden-compartment Sign, plain tablets | Revealable component + tint |
| 13 | **Truth Sense** (B) | Always-on colored lie text | `[lie]` markup gated by ability |
| 14 | **Warden boss phases + mid-battle Talk/Fight** (C5) | Single duel + dialogue after | Consequence flags already exist from Iteration 1 |
| 15 | **Empathic Healing / Spare in combat** (C6, D) | Wounded-wolf dialogue choice | Adds the Bramble Bear here |
| 16 | **Memory Viewer + forge upgrade** (F, E) | Viewer dialogue, skipped upgrade | |
| 17 | **Portraits + speaker polish** (B) | Text-only dialogue | |
| 18 | **Scene 22 side quests** (one per iteration if wanted) | — (new content) | Elena → Thomas → Children |
| 19 | **Ending: dinner variants, world map, stats, achievements** (G) | End-card text | |
| 20 | **Audio, time-of-day, art pass, accessibility** (G) | Greybox levels | Can be spread across iterations |

---

## 5. Consequence and flag map (keep this table updated as you write)

| Flag | Set in | Read in |
|---|---|---|
| `quest_fragments` (stage 0–6) | Scenes 1, 3, 5, 8, 12, 17, 20 | Tracker, Finn/Kira dialogue |
| `kira_joined` | Scene 3 | Party, combat |
| `warden_resolution` (peaceful/violent) | Scene 16 | Kira 16/18, Thomas, village NPCs 19, later guardians, Codex reward |
| `rel_kira` (int) | Choices in 3, 13, 16, 18 | Scene 21 tone, ending |
| `healed_wolf`, `healed_bear` | Scene 18 | Thomas side quest, achievements |
| `told_kira_fragment` | Scene 13 | Kira lines 13–21 |
| `sidequest_elena/thomas/children` | Scene 22 | Scene 23 dinner variant |
| `act2_path` | Scene 24 | Act II opening |
| `children_lesson` (choice) | Scene 22 | Future quest hook |

---

## 6. Suggested cuts if time is short
1. Scene 22's three side quests → keep **one** (Thomas's fits best since it reuses healing), and turn the dinner variants into two instead of three.
2. Bramble Bear and cubs → cut, or fold into the Thomas side quest.
3. Spite Sprite optional clearing → keep as one quick fight, no cutscene.
4. Memory Viewer puzzle → play it as a cutscene only.
5. World map → a fade with a location title card.
6. Achievements → show them on the end stats screen only, with no persistent system.

---

## 7. Open questions for you
1. Combat translation as in §1.1. OK, or do you want to move toward real-time?
2. XP and levels: build them, or rewards-only for the demo?
3. Character looks: new 3D-pipeline models, or allow the 2D `Actors/Character` sheets?
4. Should the existing test levels and Memory Palace content stay reachable in the demo build?
5. Target demo length (I've assumed ~2 hours).
