# Dialogue staging - moving and animating characters from dialogue

Dialogue nodes can move, turn and animate characters while a conversation runs. The commands hand the work to
`World/StageDirector.wren`, which plays it out frame by frame.

## Positions: the LDtk `Marker` entity

Add an entity to `SevenGods.ldtk` (Project → Entities):

| Identifier | Size | Fields |
|---|---|---|
| `Marker` | 16×16 (any) | `Name` (String) - what the dialogue calls it, e.g. `FinnAtForge`<br>`Facing` (Enum `Directions`, optional) - the way a character arriving here ends up facing |

Markers are invisible in game. Several can share a `Name`, for example one per room. A character goes to the one
nearest them. Only rooms that are currently loaded are searched (the player's room and its neighbours).

## Commands

`who` is `Player`, a party member's name (`Kira`), an NPC's `Name` or an Interactable's `MessageKey`.

| Command | Parameters | What it does |
|---|---|---|
| `moveTo` | `who`, `to` (a Path's Name - walked through its WayPoints - or a Marker name), `speed` (`walk` - default, `run`, or px/s), `wait` (1 default / 0), `animation` (optional - a clip to move with instead of Walk / Run) | Walks there with the walk or run animation, then idles facing the Marker's `Facing` (or the way it walked) |
| `moveToPlayer` | `who`, `speed`, `wait`, `animation` (as `moveTo`) | Walks to the tile in front of the player (one tile the way the player faces) and turns to face them. `moveTo` with `to='Player'` does the same |
| `teleport` | `who`, `to` | Puts them on the Path's first WayPoint (or the Marker) at once |
| `face` | `who`, `towards`: `up` `down` `left` `right` `upLeft` … or `North`/`South`/`East`/`West`, or a character / Marker name | Turns them |
| `playAnimation` | `who`, `animation` (see below), `wait` (0 default / 1 - hold the conversation until it has played once), `then` (optional - the clip to switch to after, e.g. `GettingUp` then `Idle`) | Plays that clip of their sheet in the way they face, and holds it as a pose |
| `wait` | `seconds` | Pauses the conversation |
| `expression` | `mood` (a word or two - `chuckling`, `uneasy`) | How the node's line is said, shown on the speaker's portrait. Portraits only have their default face so far, so the mood is written under the portrait as a placeholder (`Dialogue/Portraits.wren`) |
| `closeUp` | `image` (e.g. `CloseUps/kira_book_wolf_fact.png`) | Shows a picture large above the dialogue box - a book page, a carving, a memory - until `closeUpEnd` or the end of the conversation. It stays up while a staged node hides the box, so a memory can play with no text (`closeUp` + `wait`). Pictures are 320x180, shown at 2x (`Dialogue/CloseUp.wren`) |
| `closeUpEnd` | - | Puts the close-up away |
| `shootArrow` | `who`, `distance` (px, default 64), `delay` (seconds, default 0.8) | An arrow particle flies east from `who`'s bow and ends in a puff of dust (`GameObjects/ArrowShot.wren`). Play it alongside `playAnimation` `Shoot` - the delay matches the draw. The arrow art points east, so shoot east |
| `chargePlayer` | - | Every creature here that can charge (the perimeter wolf - `GameObjects/PerimeterWolf.wren`) runs at the player; catching them starts the fight once the conversation closes. The perimeter wolf's fight is the player's alone |

## Animations

| Sheet | Clips |
|---|---|
| Everyone (player, NPCs, party) | `Idle` `Walk` `Run` `Hurt` `Roll` `Death` `SittingIdle` `LyingDown` `GettingUp` |
| The player (`ExplorerSprite`) | also `Drinking` `Inspecting` (looping) `Lifting` - Lifting plays when an item is picked up |
| Kira (`YellowPantsGirlSprite`) | also `Drinking` `Inspecting` (looping - reading her book) `Lifting` `Shoot` (with her bow) |
| Finn (`FinnSprite`) | also `Working` (looping - at the anvil), `LayingIdle` (looping), `PickingUp`, `Inspecting`, `Lifting` |
| Thomas (`GoblinVikingSprite`) | also `Working` (looping) |

A clip a character's sheet doesn't have is skipped with a warning in the log. One-shot clips (`GettingUp`,
`PickingUp`, `Lifting`, `Drinking`, `Shoot`, `LyingDown`, `Hurt`, `Roll`, `Death`) stop on their last frame unless
`then` names the next clip.

Finn starts in `Working` when he's placed at his anvil or forge (`GameObjects/NPC.wren` `STARTING_POSES`, by his
`Versions`). An NPC entity with a String field `Animation` would start in that clip instead (not added to the LDtk
NPC entity yet).

## Portraits and close-ups

- A speaker's portrait shows at the left of the dialogue box (`Dialogue/Portraits.wren` maps speaker ids to
  `assets/Portraits/*.png`). They're rendered by 3dModelsToSprites: each character's `job.portrait.json` frames the
  head and shoulders (`"framing": "head"`) and `build_sprites.bat <character> --sheet portrait` installs the image.
- The whole Arc 1 script, with staging and what's missing, is in `docs/Arc1Dialogue.md`
  (`python tools/dialogue_to_markup.py` regenerates it).

**Poses**
- Any clip but `Idle` is held as a pose until a `moveTo`, `face` or another `playAnimation` replaces it.
- A player or party member's pose ends with the conversation, and they go back to idling.
- An NPC keeps its pose after the conversation, so Finn carries on Working at his anvil, until a scene changes it.

## How it plays out

- A node's commands run as soon as the node is reached.
- If any of them hold the conversation (`moveTo`, unless `wait` is 0, and `wait`), the node's line is held with the
  dialogue box hidden until they finish. Then the line shows.
- A node with no `text` just runs its commands and moves on to its `next` once they've finished. Chain text-less
  nodes to run moves one after another.
- `moveTo` with `wait` 0 moves the character while the conversation carries on.
- **Moves queue up.** A `moveTo` / `moveToPlayer` for a character who is already moving starts once the current move
  ends, even when both are in the same node. So `moveTo FinnToTable` then `moveToPlayer` walks the Path and then over
  to the player. The spot in front of the player is worked out when that walk starts. `teleport`, `face` and
  `playAnimation` still stop a move straight away.
- `moveToPlayer` doesn't check what's on the tile in front of the player. If the player faces a wall or furniture,
  the character stands on it, so turn the player with `face` first if needed.
- While moved, the player and party members are in scripted mode, so their own movement stands aside. Characters
  slide straight to the Marker and don't path around walls, so put Markers in clear lines.

Example (in `gen_arc1.py` terms):

```python
# Finn stops working, walks over to the player, then speaks.
add('finn_walks_over', {'next': 'finn_greets', 'commands': [
    c('moveTo', who='Finn', to='FinnBesideBed', animation='StandardWalk'),
    c('face', who='Player', towards='Finn')]})
say('finn_greets', 'Finn', "Ah, you're awake!")
```

Positions are usually LDtk Paths (a one-WayPoint Path for a single spot). Path setup, wandering NPCs and the arc's
route list are in `docs/CutscenePaths.md`.

## New game

A fresh save (no `opening_played` flag) opens straight into the `opening` node once the player has landed on the
spawn point (`States/GameWorldState.wren` `OPENING_NODE`). The node is defined in the dialogue. To see it again,
delete the saves in `%APPDATA%/StrukturGames/Seven Gods/`.
