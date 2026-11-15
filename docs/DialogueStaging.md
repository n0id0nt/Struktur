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
| `moveTo` | `who`, `to` (a Path's Name - walked through its WayPoints - or a Marker name), `speed` (`walk` - default, `run`, or px/s), `wait` (1 default / 0), `animation` (optional - a clip to move with instead, e.g. Finn's `StandardWalk`) | Walks there with the walk or run animation, then idles facing the Marker's `Facing` (or the way it walked) |
| `teleport` | `who`, `to` | Puts them on the Path's first WayPoint (or the Marker) at once |
| `face` | `who`, `towards`: `up` `down` `left` `right` `upLeft` … or `North`/`South`/`East`/`West`, or a character / Marker name | Turns them |
| `playAnimation` | `who`, `animation` (see below), `wait` (0 default / 1 - hold the conversation until it has played once), `then` (optional - the clip to switch to after, e.g. `GettingUp` then `Idle`) | Plays that clip of their sheet in the way they face, and holds it as a pose |
| `wait` | `seconds` | Pauses the conversation |

## Animations

| Sheet | Clips |
|---|---|
| Everyone (player, NPCs, party) | `Idle` `Walk` `Run` `Hurt` `Roll` `Death` |
| Finn (`Sprites/FinnSprite.wren`) | also `Working` (looping - at the anvil), `LayingIdle` (looping), `GettingUp`, `PickingUp`, `StandardWalk` (a calmer walk) |

A clip a character's sheet doesn't have is skipped with a warning in the log. One-shot clips (`GettingUp`,
`PickingUp`, `Hurt`, `Roll`, `Death`) stop on their last frame unless `then` names the next clip.

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
