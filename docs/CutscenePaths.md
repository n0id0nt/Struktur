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
```

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

## Every path the arc's cutscenes need

These names are suggestions; whatever you name them in LDtk is what the dialogue will use. "Node" is where the scene
plays in `Arc1Dialogue.json`.

**Party followers:** Kira follows the player on her own while the player walks a path. Give her a path
only where she does something different from the player.

### Scene 1 - the opening (FinnsWorkshop)

| Kind | Name | Who | Route / spot | Ends facing | Node |
|---|---|---|---|---|---|
| Path, 1 point | `FinnAtAnvil` | Finn | In front of his anvil | Towards the anvil | `bed_sign` (teleport, then Working) |
| Path | `PlayerOutOfBed` | Player | From the bed's edge, out onto the workshop floor towards Finn | Towards Finn | `bed_wake` |

### Haven Village

| Kind | Name | Who | Route / spot | Ends facing | Node |
|---|---|---|---|---|---|
| Path | `KiraToPlayer` | Kira | From the targets to beside the player | The player | `kira_s3_end_2` (HavenVillageTrainingGrounds) |
| Path | `ChildRunsUp` | HavenVillageChild | Running up to the player | The player | `child_s5` (HavenVillage) |
| Path | `ChildRunsOff` | HavenVillageChild | Away again, back towards its wander route | - | `child_s5_end` |
| Path | `GarrettToWell` | Garrett | From his door to the well | The well | `garrett_reward` |

### Whisperwood gate (HavenVillageBorderNorth)

| Kind | Name | Who | Route / spot | Ends facing | Node |
|---|---|---|---|---|---|
| Path | `PartyToGate` | Player | Up the trail to stand before the two doors | Up | `wisperwood_door` |
| Path | `FinnToGate` | Finn | To the middle of WisperWoodsDoor / WisperWoodsDoor2 | Up (he Works on them while they open) | `wisperwood_door` |
| Path | `FinnToParty` | Finn | From the doors back to beside the player | The player | `finn_map` ("approaching with a final gift") |
| Path | `FinnHome` | Finn | Back down the trail towards the village, out of sight | - | `finn_away` |

### Whisperwood

| Kind | Name | Who | Route / spot | Ends facing | Node |
|---|---|---|---|---|---|
| Path | `WolfLeaves` | The healed wolf | Off into the trees | - | `wolf_heal` (needs critters to be addressable by name, later) |

### Shrine

| Kind | Name | Who | Route / spot | Ends facing | Node |
|---|---|---|---|---|---|
| Path | `KiraToShrine` | Kira | A few steps ahead of the player, towards the shrine | Up | `shrine_entry_kira` (ShrineCourtyard) |
| Path | `KiraJumpsBack` | Kira | Running back across the split in the floor to the player | The player | `shifting_floor` |
| Path | `RunicConstuct1Steps` | RunicConstuct1 | One or two cells forward off its plinth | The player | `construct_1_wake` (ShrineChamber) |
| Path | `RunicConstruct2Steps` | RunicConstruct2 | The same | The player | `construct_2_wake` |
| Path | `RunicConstruct3Steps` | RunicConstruct3 | The same | The player | `construct_3_wake` |
| Path | `PlayerToCrystal` | Player | Up to the ShrineCrystal | Up | `guide_s12` (FragmentChamber) |
| Path, 1 point | `KiraWatchesCrystal` | Kira | Off to one side, out of the way | The crystal | `guide_s12` |
| Path | `GuideIntoPlayer` | Guide | Drifting from the crystal onto the player's spot (then hidden) | - | `guide_fuse` |
| Path | `PartyToWardens` | Player | Into the middle of GuardianChamber, facing the three Wardens | Up | `warden_s14` |
| Path | `PartyToWardenDoor` | Player | From the middle to the ShrineWardenDoor once it's open | Up | after `warden_peace` / `warden_violent` |
| Path | `PlayerToViewer` | Player | Up to the MemoryViewer | Up | `viewer_s17` (DeepArchive) |

### Home again

| Kind | Name | Who | Route / spot | Ends facing | Node |
|---|---|---|---|---|---|
| Path, 1 point | `FinnAtForge` | Finn | At the forge | The forge (Working) | `finn_reveal` (FinnsWorkshop) |
| Path | `FinnToCompartment` | Finn | From the forge to the hidden compartment (PickingUp there) | The compartment | `finn_reveal_*` |
| Path | `FinnBackToPlayer` | Finn | Back to the player with what he found | The player | `finn_reveal_*` |
| Paths, 1 point each | `FinnAtGathering`, `KiraAtGathering`, `PlayerAtGathering`, `ElenaAtGathering`, `GarrettAtGathering`, `ThomasAtGathering` | Everyone | Their places at the evening gathering (teleport) | The centre | `finn_farewell` |
| Path | `PartyLeavesVillage` | Player (Kira follows) | From the village edge out along the road, as the screen fades | Along the road | `road_1` -> `road_end_*` |

**Commands still to come:** `GuideIntoPlayer`, `FinnHome` and the wolf need a hide command so characters can leave the
scene. `PartyLeavesVillage` needs the screen fade.
