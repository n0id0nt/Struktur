# Arc 1 – LDtk Level Guide (Iteration 1 greybox)

> **Superseded (2026-11-14):** the level was built with its own entity set, and the code now follows it:
> - NPC `Versions` = the stages it's visible in.
> - `Interactable` (by `MessageKey`).
> - `RestSpot` → linked `Spawn`; the spawn point is saved in flags.sav, and a new game starts at the workshop bed.
> - `Critter` `Enemy` dropdown.
> - Door `KeyRefs` (pressure pads / key items) and `KeyIds` (story events fired by the `triggerEvent` dialogue command).
> - Arenas are picked by the level's `Terrain` (`<Terrain>BattleStage`).
>
> The dialogue lookup names match the LDtk names (`Dialogue/DialogueEntryPoints.wren`). The field tables and the
> map below describe the earlier proposal, not the level as built.

What to build in `assets/Levels/SevenGods.ldtk` so the Arc 1 dialogue (`assets/Dialogue/Arc1Dialogue.json`)
plays start to finish. The game boots straight into `GameWorldState`, which spawns only the player (and party
followers). **Everything else is placed here.**

Story progress is the int flag **`arc1_stage`**. NPCs and signs can be limited to a range of stages with
the new gate fields (§1), so the same level can hold Finn in the morning and again in the evening.

| Stage | Meaning (set by…) |
|---|---|
| 0 | New game |
| 1 | Finn gave the sword (Finn, Scene 1) |
| 2 | Kira joined (Kira, Scene 3) |
| 3 | Beat Thomas and got the gear (Thomas, Scene 4) |
| 4 | Got the Old Map at the gate (Finn, Scene 5) |
| 5 | Shrine door opened (Shrine Door, Scene 8) |
| 6 | Runic Construct beaten (Scene 11) |
| 7 | Fused with the Guide (Scene 12) |
| 8 | Warden resolved (Scenes 14–16) |
| 9 | Memory Viewer seen (Scene 17): head home |
| 10 | Finn's reveal done (Scene 20) |
| 11 | Kira's talk done (Scene 21): farewell at the gathering → End of Arc 1 |

---

## 1. Entity definitions to add or extend (Project → Entities)

| Entity | Change | Why |
|---|---|---|
| **NPC** | Add `ShowFromStage` (Int, nullable), `HideFromStage` (Int, nullable), `HideIfFlag` (String, nullable) | Story gating (`World/StoryGates.wren`). Empty = no limit |
| **Sign** | Add the same 3 gate fields, plus `Sprite` (String, nullable: an image path such as `Items/Potion/Heart.png`) | Gating, and so rest spots and the wounded wolf don't all look like a paper note |
| **Door** | Add `Name` (String) | Puzzle doors are found by name (`openDoor` command, dialogue entry). Use `DoorType = Dialogue` for every Arc 1 door |
| **Item** | Add `Name` (String) | `Item.wren` needs the item id (e.g. `Healing Herb`). It currently logs "has no Name" |
| **Chinlin** *(new)* | New entity, identifier exactly `Chinlin`, 16×16, with a `Name` (String) field whose **default value is `Chinlin`** | Entities run the script named after their identifier. The existing **`Critter` entity has no script and spawns nothing**. The Name picks the battle stats (`BattleCritter.forName`), and any other value fights as a chicken. Chinlin = the aggressive Thornwolf stand-in |
| **Chicken** *(new)* | New entity, identifier exactly `Chicken`, 16×16, `Name` field defaulting to `Chicken` | Passive stand-in for the Spite Sprites (press Interact to fight) |
| `Critter` | Delete it, or leave it unused | No `Critter` script exists |
| `DoorWorp` | No change | Interiors (the workshop) connect with a DoorWorp pair: Source/Destination point at each other, plus a direction |
| `PressurePad` | No change | Not used in Iteration 1 |

**Level fields (Project → Levels):** add `SpawnX` (Int, nullable) and `SpawnY` (Int, nullable). These are
pixels inside the level where a new game starts. **Tag the workshop level `Start`** (level → Tags). With no
`Start` tag the player spawns at (208, 208) and logs a warning. Keep the existing `Runable` field (false indoors).

---

## 2. World layout

Free layout with a 16 px grid. Levels that **touch edges are neighbours**: the player walks across the edge
and the camera slides. Leave a 2-tile opening in the walls where they meet. Interiors sit **apart from
everything** and connect by DoorWorp, so a stage change made outside always reloads them fresh.

**Scaled map:** [Arc1Map.svg](Arc1Map.svg) draws every level at its real world coordinates, with the
openings, doors, warp pair and every entity placed. The ASCII sketch below is the same layout.

The old test levels (`Level_0–4`, `PlayerTent`, `Tent1–4`) overlap where the forest goes. Move them well
out of the way (e.g. x ≥ 3000) or delete them. **Keep `BattleStage`**: every fight is staged there.

```
                                   ┌──────────────┐
                                   │ DEEP ARCHIVE │ (512,-1984) 256x192
                                   │  [MemViewer] │
                                   └──────┬───────┘
                                 ┌────────┴─────────┐
                                 │ GUARDIAN CHAMBER │ (448,-1792) 384x256
                                 │ (W)arden [Archive│Door]
                                 └────────┬─────────┘
                ┌──────────────┐ ┌────────┴─────┐
                │ FRAGMENT     │ │ MEMORY HALL  │ (384,-1536) 256x256
                │ CHAMBER  (G) ├─┤ [Murals]     │
                │  Guide       │ │ [CentralMural│
                └──────┬───────┘ └──────────────┘
       (128,-1536) 256x256
            ┌──────────┴───────────┐
            │ SEPARATION CHAMBER   │ (64,-1280) 384x256
            │ [ShiftFloor] ‖Aqueduct‖ ‖CrystalRing‖ (R)Construct
            └──────────┬───────────┘
            ┌──────────┴───────────┐
            │ SHRINE UPPER HALL    │ (64,-1024) 384x256
            │   gap  ‖Light Bridge‖│
            └──────────┬───────────┘
            ┌──────────┴───────────┐
            │ SHRINE COURTYARD     │ (64,-768) 384x256
            │ [T1]..[T5] [Rest] ‖Shrine Door‖
            └──────────┬───────────┘
          ┌────────────┴─────────────┐
          │ WHISPERWOOD DEEP         │ (0,-512) 512x256
          │ [BlueLights] c c c  [Pillars]  w
          └────────────┬─────────────┘
          ┌────────────┴─────────────┐
          │ WHISPERWOOD PATH         │ (0,-256) 512x256
          │ [ClawMarks]  w  w  [WoundedWolf]
          └────────────┬─────────────┘
                 ‖Overgrown Trail‖
┌───────────┐ ┌────────┴─────────────────┐ ┌──────────────┐
│ THOMAS'S  │ │ HAVEN VILLAGE            │ │ TRAINING     │
│ CABIN     ├─┤ (E)lena (Ga)rrett [Well] ├─┤ GROUNDS      │
│ (T)homas  │ │ (C)hild [Rest Spot]      │ │ (K)ira       │
│           │ │ (F)inn@gate (F)inn@supper│ │ [targets]    │
└───────────┘ │ ⇄ workshop door          │ └──────────────┘
(-256,64)     └──────────────────────────┘ (512,64) 256x256
256x256        (0,0) 512x384

   Detached interior (no shared edges), e.g. (-768, 96):
   ┌─────────────────────┐
   │ FINN'S WORKSHOP     │ 256x192, tag "Start", SpawnX/SpawnY = beside the bed
   │ [Bed] (F)inn (F)inn │ DoorWorp ⇄ village door
   └─────────────────────┘

Key: (X) NPC   [X] Sign   ‖X‖ Door (DoorType=Dialogue; one 32 px door fills a
     2-tile gap)   w Chinlin   c Chicken   ⇄ DoorWorp pair
```

---

## 3. What goes in each level

`Name` is the NPC's `Name` field, the Sign's `MessageKey`, or the Door's `Name`. **It must match exactly**:
the dialogue entry point is looked up by it (`Dialogue/DialogueEntryPoints.wren`).

### Finn's Workshop (interior, tag `Start`)
| Entity | Name / fields | Placement |
|---|---|---|
| Sign | `Bed`, Sprite optional | On the bed. The opening narration |
| NPC | `Finn`, HideFromStage **3** | At the anvil (morning, Scenes 1–2) |
| NPC | `Finn`, ShowFromStage **9**, HideFromStage **11** | At the forge (evening reveal, Scene 20) |
| DoorWorp | ↔ the village's workshop door | Workshop exit |
| Level | SpawnX/SpawnY next to the bed, `Runable` = false | |

### Haven Village
| Entity | Name / fields | Placement |
|---|---|---|
| DoorWorp | ↔ the workshop | The workshop front door |
| NPC | `Elena` | By the bakery |
| NPC | `Garrett` | Sitting next to the well |
| Sign | `Village Well`, Sprite e.g. `Items/Resource/Water.png` | The well (the rope puzzle) |
| NPC | `Child` | Village square |
| Sign | `Rest Spot`, Sprite `Items/Potion/Heart.png` | The **save-point house**. Resting heals the party. Saves are automatic, so this is the heal point |
| NPC | `Finn`, ShowFromStage **3**, HideFromStage **4** | At the north gate (gives the Old Map) |
| NPC | `Finn`, ShowFromStage **11** | The gathering spot (farewell → end card) |
| Door | `Overgrown Trail`, DoorType Dialogue | Fills the 2-tile (32 px) gap in the north wall into Whisperwood Path |
| *(optional)* Item | `Healing Herb` | Somewhere to find |

Openings: north (gate) → Whisperwood Path, west → Thomas's Cabin, east → Training Grounds.

### Training Grounds
| NPC | `Kira`, HideFromStage **2** | By the archery targets. She joins the party and follows from then on |

### Thomas's Cabin (outdoor yard)
| NPC | `Thomas` | Outside the cabin. Talking at stage 2 starts the tutorial **duel**. Winning reopens his dialogue for the gear |

### Whisperwood Path
| Entity | Name / fields | Placement |
|---|---|---|
| Sign | `Claw Marks` | On a tree near the entrance |
| Chinlin ×2 | `Name` = `Chinlin` (the default) | Mid-path. These are aggressive and force a fight |
| Sign | `Wounded Thornwolf`, ShowFromStage **9**, HideIfFlag `healed_wolf`, Sprite `Actors/Monster/Beast/Faceset.png` | Off the path (return trip, Scene 18) |

### Whisperwood Deep
| Entity | Name / fields | Placement |
|---|---|---|
| Sign | `Blue Lights` | Side branch |
| Chicken ×3 | | Clearing at the end of the side branch (the Sprite fight, optional) |
| Sign | `Stone Pillars` | By the main path, near the courtyard exit |
| Chinlin ×1 | | Main path |

### Shrine Courtyard
| Entity | Name / fields | Placement |
|---|---|---|
| Sign ×5 | `Shrine Tablet 1` … `Shrine Tablet 5` | Scattered (entrance, left wall, right wall, water, half-buried) |
| Sign | `Shrine Entry` | By the doors (Kira's awe) |
| Sign | `Rest Spot`, Sprite `Items/Potion/Heart.png` | Checkpoint before the dungeon |
| Door | `Shrine Door` | 2-tile gap in the north wall → Upper Hall |

### Shrine Upper Hall
| Door | `Light Bridge` | Blocks a 2-tile walkway across a pit (Collision) splitting the room |

### Separation Chamber
| Entity | Name / fields | Placement |
|---|---|---|
| Sign | `Shifting Floor` | On the trap plate at the entrance |
| Door | `Aqueduct Gate` | First gate |
| Door | `Crystal Ring Gate` | Second gate |
| NPC | `Runic Construct`, HideFromStage **6** | Standing in a **1-tile corridor** to the Fragment Chamber. It blocks the way until beaten, then disappears |

### Fragment Chamber
| NPC | `Guide`, ShowFromStage **6**, HideFromStage **7** | At the crystal in the centre |

### Memory Hall (exit only from the Fragment Chamber)
| Sign | `Memory Murals` | Main wall (Kira's Scene 13 talk) |
| Sign | `Central Mural` | Centre (the second fragment) |

### Guardian Chamber (entrance only from the Memory Hall)
| NPC | `Warden`, ShowFromStage **7**, HideFromStage **8** | In front of the throne. Duel, then the talk-or-strike choice |
| Door | `Archive Door` | Behind the throne → Deep Archive |

### Deep Archive
| Sign | `Memory Viewer` | Centre. Sets stage 9: go home |

---

## 4. Playthrough checklist after placing

1. A new game spawns beside the bed. Use **Bed** → **Finn** (sword + herbs).
2. **Elena** (bread), **Well** puzzle (Left, Down, Right) → **Garrett** (Village Key).
3. **Kira** joins → **Thomas** duel → win → his gear.
4. **Elena** (rations), **Child**, **Finn at the gate** (Old Map) → **Overgrown Trail** opens.
5. Fight the forest critters, read the signs → courtyard tablets → **Shrine Door** (Circle, Triangle, Square, Diamond, Star).
6. **Light Bridge** (centre, combine, across) → **Aqueduct** (turn, leave, turn) → **Crystal Ring** (Harmony, Growth, Knowledge, Protection, Unity, Hope) → **Runic Construct** duel.
7. **Guide** → **Memory Murals**, **Central Mural** → **Warden** duel + choice → **Archive Door** → **Memory Viewer**.
8. Back through the forest (**Wounded Thornwolf**) → **Child** → **Finn** in the workshop (reveal).
9. Talk to **Kira** (your follower) → **Finn** at the gathering → Act II choice → **End of Arc 1** card → main menu.

**Testing again from scratch:** delete `flags.sav`, `inventory.sav`, `party.sav`, `moves.sav` and `defeated.sav` in
`%APPDATA%/StrukturGames/Seven Gods/`. There's no New Game button yet. Old saves may still have the
test Villager or Duelist in your party.
