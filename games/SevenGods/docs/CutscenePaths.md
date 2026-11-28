# Cutscene and wander paths - LDtk setup and the arc's path list

How walking routes are laid out in `SevenGods.ldtk`, used both by dialogue scenes and by wandering NPCs, plus
every route the arc's cutscenes need.

## The entities

| Entity | Fields | What it is |
|---|---|---|
| `WayPoint` | none | One point of a route. Characters walk to its centre |
| `Path` | `Name` (String)<br>`Entity_ref` (array of WayPoint refs)<br>`PathFollowType` (enum `Once` / `Loop` / `PingPong` / `Random`) | A route: its WayPoints in the order listed |
| `NPC` | `Entity_ref` (a Path ref, optional) | The Path that NPC wanders |

Scripts: `GameObjects/WayPoint.wren`, `GameObjects/Path.wren`, the `World/Paths.wren` registry, and
`World/PathWalkers.wren` for wandering.

**How a route is walked**
- Points are visited in the order of the Path's `Entity_ref` list.
- The Path entity's own position isn't part of the route, so put it wherever is easy to see.
- Each leg is a straight line with no pathfinding. Leave a cell of clearance from walls and furniture.
- Keep a Path and its WayPoints in one level. A route into the next room is two Paths.
- **A single spot** (somewhere a character stands for a scene) is just a Path with one WayPoint.
- Several Paths can share a `Name`, for example one per room. A scene uses the one nearest the character.

## Dialogue scenes

`moveTo` with `to` set to a Path's `Name` walks the character through every WayPoint in order. They walk once,
whatever the `PathFollowType`, and end facing the way they walked. `teleport` with a Path puts them on its first
WayPoint. Both still accept an LDtk `Marker` by name, if you ever add one, but you don't need it.

```python
c('moveTo', who='Finn', to='FinnToGate')                       # walk it, the conversation waits
c('moveTo', who='Kira', to='KiraJumpsBack', speed='run')       # run it
c('moveTo', who='Finn', to='FinnHome', wait=0)                 # walk it while the conversation carries on
c('teleport', who='Finn', to='FinnAtAnvil')                    # a one-point Path: stand here
c('moveToPlayer', who='Kira')                                  # walk to the tile in front of the player
```

**Walking over to the player.** The player could be anywhere, so a Path can't end at them. Use `moveToPlayer`: it walks
to the tile one cell in front of the player, the way they're facing, and ends facing them. Moves for the same
character queue, so a Path followed by `moveToPlayer` in the same node walks the Path first, then comes over:

```python
add('finn_fetch', {'next': 'finn_give', 'commands': [
    c('moveTo', who='Finn', to='FinnToWorkBench'),   # fetch the sword...
    c('moveToPlayer', who='Finn'),                   # ...then bring it over
]})
```

A `playAnimation` stops a move straight away. To have Finn pick something up in between, put the `moveTo` in one
text-less node and the `playAnimation` (`wait=1`) plus `moveToPlayer` in the next.
In the table below, a "to you" beat needs no Path at all, or only a Path to get round furniture before `moveToPlayer`.

## Wandering NPCs

An NPC walks the Path its `Entity_ref` points at, following the Path's `PathFollowType`:

| Type | Walks |
|---|---|
| `Once` | Through once, then stands at the last point |
| `Loop` | Round and round, from the last point back to the first |
| `PingPong` | There and back: 1, 2, 3, 2, 1… |
| `Random` | From point to a random other point |

**What the wanderer does**
- It starts from whichever point is nearest and ambles a little slower than the player walks.
- It stands for 1–3 seconds at each point.
- When a conversation opens, every wanderer stops. One within 3 cells of the player turns to face them, and they
  all carry on afterwards.
- A scene can move or pose a wandering NPC. It waits while the scene does that. Afterwards it walks back to its
  route, except when a scene left it in a pose (like Finn Working): then it stays put until a scene changes it.
- Hidden story-gated NPCs don't wander. Several NPCs can share one Path.

**In the level now**

| Path | Level | Type | Walked by |
|---|---|---|---|
| `ChildPath` | HavenVillage | Random | HavenVillageChild |
| `KiraTrainingPath` | HavenVillageTrainingGrounds | PingPong | Kira |

**Other good wanderers:**
- Garrett: between his door and the well.
- Elena: between the oven and the counter.
- Thomas: around the practice dummies.
- Leave Finn without one, so he stays at the anvil Working.

The Wardens and the shrine's Interactables have no sprite, so they can't wander or be animated. The constructs and
the Guide are NPCs, so they can.

## Cutscenes with walking

**Wired into the dialogue so far:** every Path placed in the level is now used by these scenes.

| Scene | What plays |
|---|---|
| 1. Opening | Finn is put at `FinnAtAnvil`, Working and facing down. After the narration you walk `PlayerOutOfBed`, then the "you climb out of bed" line shows |
| 2. Finn's task | After "you shouldn't go alone", Finn walks `FinnToWorkBench`, plays PickingUp, then walks `FinnBackToPlayer` and comes up to you (`moveToPlayer`). After "Take this with you" he walks `FinnToAnvil` and goes back to Working. His reminder line turns him to you, then he goes back to work |
| 3. Kira | When you talk to her, she walks to `KiraAtTargets`, then turns to you. Before "we should talk to father first" she walks up to you |
| 4. Thomas | After "about equipment...", Thomas walks `ThomasToRack`, pauses, walks `ThomasBackToPair`, then turns to you and hands over the gear |
| 6. The gate | You walk `PartyToGate`, then Finn walks `FinnToParty`, and the two of you turn to each other. Kira walks `KiraToMap` and looks at Finn. Finn walks `FinToDoors`, Works the doors open and walks `FinnStepAside`. He says goodbye and walks `FinnHome`. Only then does the story move on to stage 4, which removes him |

The generator, `gen_arc1.py`, puts each walk in a node with no text, ahead of the line it leads into, because a
`face` or `playAnimation` in the same node would stop the walk.

Every scene in the arc's dialogue where someone should walk, in story order. Each beat is taken from the script's
stage directions (the *(italic)* notes in the old `Arc1Dialogue.json`, now `assets/Dialogue/Arc1/`). **Path** is the name to give the LDtk Path, placed
in that scene's level. A "1 point" Path is just a spot to stand on.

**Party followers:** Kira follows the player by herself, so the player's walks bring her along. Give her a Path only
where she does something of her own.

**Where scenes happen:** the level is wherever the character you talk to stands for that story stage, from the NPC's
`Versions` field. For example, `finn_map` plays at the Whisperwood gate, because the stage-3 Finn is in
HavenVillageBorderNorth. There's no table in that scene. The scene that does put a map on a table is Finn's reveal
(cutscene 15), so that's where I've put the "walk to the table, the player follows" beat.

### 1. The opening - FinnsWorkshop (new game)

| Node | Beat | Who | Path |
|---|---|---|---|
| `bed_sign` | Finn is at his anvil, Working | Finn | `FinnAtAnvil` (1 point, teleport) |
| `bed_wake` | "You climb out of bed": walk out onto the workshop floor, stopping a couple of cells from Finn | Player | `PlayerOutOfBed` |

### 2. Finn's task - FinnsWorkshop (stage 0, talk to Finn)

| Node | Beat | Who | Path |
|---|---|---|---|
| `finn_s1_end` | "Take this with you": Finn fetches the sword and herbs from his bench… | Finn | `FinnToWorkbench` |
| `finn_s1_end` | …and brings them back to you | Finn | `FinnBackToPlayer`, then `moveToPlayer` |
| `finn_reminder` | Finn returns to the anvil (`wait` 0, then Working) | Finn | `FinnToAnvil` |

### 3. Kira joins - HavenVillageTrainingGrounds (stage 1, talk to Kira)

| Node | Beat | Who | Path |
|---|---|---|---|
| `kira_early` / `kira_s3` | "Loosing an arrow": she stands at her shooting spot facing the targets, then turns ("noticing you") | Kira | `KiraAtTargets` (1 point) |
| `kira_s3_end_2` | She walks over to you before joining the party | Kira | `moveToPlayer` (no Path needed) |

### 4. Thomas - ThomasCabin (stage 2, talk to Thomas, after the spar)

| Node | Beat | Who | Path |
|---|---|---|---|
| `thomas_after_2` | "Hands Kira a fine bow, and you a set of leather armour": fetches them from his rack… | Thomas | `ThomasToRack` |
| `thomas_after_2` | …and brings them back | Thomas | `ThomasBackToPair` |

### 5. Haven errands - HavenVillage / ElenaBakery (stage 3+)

| Node | Beat | Who | Path |
|---|---|---|---|
| `elena_rations` | "Running up" from behind the counter | Elena | `ElenaRunsUp` (ElenaBakery) |
| `garrett_ask` | "Sitting by the well": he's at the well when you arrive | Garrett | `GarrettAtWell` (1 point) |
| `garrett_reward` | Once the well is fixed, he gets up and walks over to you | Garrett | `moveToPlayer` (no Path needed) |
| `child_s5` | "Tugging on your cloak": runs up to you | HavenVillageChild | `ChildRunsUp` |
| `child_s5_end` | "Runs off excitedly", then back to wandering `ChildPath` by itself | HavenVillageChild | `ChildRunsOff` |
| `child_return` (stage 9) | Runs up again | HavenVillageChild | `ChildRunsUp` (reused) |

### 6. Finn sees you off - HavenVillageBorderNorth (stage 3, talk to Finn)

| Node | Beat | Who | Path |
|---|---|---|---|
| `finn_map` | The party gathers on the trail in front of the two doors | Player | `PartyToGate` |
| `finn_map` | "Approaching with a final gift": Finn comes over with the map | Finn | `FinnToParty` |
| `finn_map_2` | "Examining the map": Kira steps up beside Finn to look | Kira | `KiraToMap` |
| `finn_map_3` | Finn goes to the doors and Works on them while they open | Finn | `FinnToDoors` |
| `finn_map_3` | He steps off the trail to let you through | Finn | `FinnStepAside` |
| `finn_away` | "Follow the map. And come home safe": Finn heads back towards the village (`wait` 0) | Finn | `FinnHome` |

### 7. The shrine courtyard - ShrineCourtyard (stage 4-5)

| Node | Beat | Who | Path |
|---|---|---|---|
| `shrine_entry_kira` | "Awed": Kira walks ahead to look up at the shrine | Kira | `KiraToShrine` |
| `shrine_door_open` | The door opens and the party walks up to it | Player | `PartyToShrineDoor` |

`shrine_entry_kira` needs a trigger in the level: an Interactable with MessageKey `Shrine Entry`.

### 8. The shifting floor - ShrineUpperHall (stage 5)

| Node | Beat | Who | Path |
|---|---|---|---|
| `shifting_floor` | "RUMBLE": the floor splits and Kira runs back across to you | Kira | `KiraJumpsBack` (run) |

Needs a trigger: an Interactable with MessageKey `Shifting Floor`.

### 9. The Runic Constructs - ShrineChamber (stage 5)

| Node | Beat | Who | Path |
|---|---|---|---|
| `construct_1_wake` | It lurches off its plinth towards you before the fight | RunicConstuct1 | `RunicConstuct1Steps` |
| `construct_2_wake` | The same | RunicConstruct2 | `RunicConstruct2Steps` |
| `construct_3_wake` | The same | RunicConstruct3 | `RunicConstruct3Steps` |

### 10. The Guide and the fusion - FragmentChamber (stage 6)

| Node | Beat | Who | Path |
|---|---|---|---|
| `guide_s12` | You step up to the crystal… | Player | `PlayerToCrystal` |
| `guide_s12` | …while Kira hangs back to watch | Kira | `KiraWatchesCrystal` (1 point) |
| `guide_fuse` | "The blue light drifts into you": the Guide floats onto you | Guide | `GuideIntoPlayer` |

### 11. The Memory Hall - MemoryHall (stage 7)

| Node | Beat | Who | Path |
|---|---|---|---|
| `murals_told` | "Amazed": Kira comes up beside you at the murals | Kira | `KiraToMurals` |
| `central_mural_find` | You step up to the central mural | Player | `PlayerToCentralMural` |

### 12. The Wardens - GuardianChamber (stage 7)

| Node | Beat | Who | Path |
|---|---|---|---|
| `warden_s14` | The party walks into the middle of the chamber, facing the three Wardens | Player | `PartyToWardens` |
| `warden_peace_2` | The throne mechanism opens the way, and the party walks to the door | Player | `PartyToWardenDoor` |

The Wardens can't walk or kneel themselves (`warden_p3` "kneeling"), because they're Interactables without sprites.

### 13. The Memory Viewer - DeepArchive (stage 8)

| Node | Beat | Who | Path |
|---|---|---|---|
| `viewer_s17` | You walk up and lay a hand on the device… | Player | `PlayerToViewer` |
| `viewer_s17` | …with Kira beside you | Kira | `KiraBesideViewer` (1 point) |

### 14. The wounded Thornwolf - Whisperwood (stage 8, on the way home)

| Node | Beat | Who | Path |
|---|---|---|---|
| `wolf_heal` | It "pads away into the trees" | The wolf | `WolfLeaves` |

Needs a trigger (an Interactable with MessageKey `Wounded Thornwolf`) and a wolf the dialogue can address. Critters
can't be addressed by name yet.

### 15. Finn's reveal - FinnsWorkshop (stage 9, talk to Finn)

| Node | Beat | Who | Path |
|---|---|---|---|
| `finn_reveal` | "Looking up from the forge": he's at the forge, Working | Finn | `FinnAtForge` (1 point) |
| `finn_rev_c_3` | "Opening a hidden compartment in the forge": walks to it and PickingUp… | Finn | `FinnToCompartment` |
| `finn_rev_d` | …then carries the map to the table and spreads it out | Finn | `FinnToTable` |
| `finn_rev_d` | The party follows him to the table | Player | `PartyToTable` |
| `finn_rev_d_2` | "Studying the map": Kira leans in on the other side | Kira | `KiraAtTable` (1 point) |

### 16. Kira at the well - HavenVillage (stage 10, talk to Kira)

| Node | Beat | Who | Path |
|---|---|---|---|
| `kira_s21` | "Staring into the well": she's at the well when you find her | Kira | `KiraAtWell` (1 point) |
| `kira_s21` | You walk up beside her | Player | `PlayerToWell` |

### 17. The farewell gathering - HavenVillage (stage 11, talk to Finn)

| Node | Beat | Who | Path |
|---|---|---|---|
| `farewell_violent` | "Before the others arrive": Elena walks in to the gathering (`wait` 0, all at once with the next three) | Elena | `ElenaToGathering` |
| `farewell_violent` | The same | Garrett | `GarrettToGathering` |
| `farewell_violent` | The same | Thomas | `ThomasToGathering` |
| `farewell_violent` | The same | HavenVillageChild | `ChildToGathering` |
| `farewell_1` | "Standing to address the gathering": Finn moves to the head of the group | Finn | `FinnToHead` |
| `farewell_2` | "Placing a hand on your shoulder": Finn walks over to you | Finn | `moveToPlayer` (no Path needed) |

### 18. The road - HavenVillage edge (stage 11)

| Node | Beat | Who | Path |
|---|---|---|---|
| `road_1` | "Adjusting her pack at the village edge": the party walks to the edge of the village | Player | `PartyToVillageEdge` |
| `road_end_*` | The two of them walk out along the road while the closing narration plays (`wait` 0) | Player | `PartyLeavesVillage` |

## Things these scenes need that don't exist yet

- **Characters leaving or arriving:**
  - Finn walking home (`FinnHome`), the Guide merging into you, and the wolf leaving all need a hide command.
  - The gathering needs Elena, Garrett and Thomas to appear in HavenVillage.
  - Today an NPC only appears or disappears when the story stage changes (its `Versions`). The stage-3 Finn would
    vanish the moment `finn_map` sets stage 4, even mid-walk.
- **Facing at the end of a Path.** A one-point Path doesn't set a facing yet: `FinnAtAnvil`, `KiraAtTargets` and
  `KiraAtWell` need it. A `Facing` field on Path (your `Directions` enum) would do it.
- **Triggers** for the Shrine Entry, Shifting Floor and Wounded Thornwolf scenes, as noted above.
- **The screen fade** for the road ending.
