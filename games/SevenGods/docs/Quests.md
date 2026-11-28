# Quests

The quest log lives on the inventory's **Quests** tab. On the left are the arc's overall progress, the active quests
under their category headings with a completion percentage and bar, and the finished quests under **Completed**. On
the right are the focused quest's summary and objectives.

| File | What it is |
|---|---|
| `assets/Quests/Arc1Quests.json` | The quest definitions |
| `assets/Scripts/Story/QuestLog.wren` | Works out each quest's state and progress |
| `assets/Scripts/UI/Inventory/QuestsPage.wren` | The tab |
| `assets/Scripts/States/InventoryState.wren` | `questsModel_()` builds the tab's model |
| `assets/Localization/en.json` | Quest text (`quest.*`, `inventory.quests.*`) |

Quest changes during play also pop up as banners in the top-right corner. See [Notifications.md](Notifications.md).

## How progress works

Progress is worked out from the game's saved int flags each time the tab opens, so there's nothing extra to save.
Most quests just watch flags the dialogue already sets, like `arc1_stage`, `well_fixed` and `garrett_key`, and an old
save shows the right progress straight away.

A quest is:
- **hidden** (not listed) until its `start` conditions hold, any objective is done, or a dialogue starts it;
- **active** after that;
- **complete** once its `complete` conditions hold. Without those, it's complete once every objective is done.

The percentage is objectives done ÷ objectives. **Arc progress** is objectives done across every quest, including
ones not started yet.

An active quest lists the objectives done so far plus the current one. Later objectives stay hidden until reached,
so they don't give away the story. A finished quest lists them all.

## Defining a quest

```json
{
  "id": "stubborn_well",
  "category": "side",
  "start": [{"flag": "garrett_asked", "op": ">=", "value": 1}],
  "objectives": [
    {"id": "fix_rope", "done": [{"flag": "well_fixed", "op": ">=", "value": 1}]},
    {"id": "tell_garrett", "done": [{"flag": "garrett_key", "op": ">=", "value": 1}]}
  ]
}
```

- A condition is `{flag, op, value}` on an int flag. `op` is one of `==` `!=` `>=` `>` `<=` `<` (`>=` if left out).
  All the conditions in a list must hold.
- `complete` (optional) works the same way.
- `categories` at the top of the file sets the order of the headings. Quests are listed in file order.
- Text goes in `en.json`:
  - `quest.category.<category>`
  - `quest.<id>.title`
  - `quest.<id>.summary`
  - `quest.<id>.<objective id>`
- To add another file (for Arc 2), add it to `QUEST_FILES` in `QuestLog.wren`.

## Dialogue commands

These are for quests that no story flag tracks. They set `quest.<id>` (1 started, 2 complete) and
`quest.<id>.<objective>` (1 done), which count alongside the JSON conditions:

| Command | Parameters |
|---|---|
| `startQuest` | `quest` |
| `completeObjective` | `quest`, `objective` (also starts the quest) |
| `completeQuest` | `quest` |

```python
c('startQuest', quest='lost_locket')
c('completeObjective', quest='lost_locket', objective='find_it')
```

Dialogue conditions can check a quest with `hasIntFlag` on `quest.<id>` (for example `>= 2` for complete).

## Arc 1 quests

| Quest | Category | Starts | Objectives (done when) |
|---|---|---|---|
| Finn's Commission | Main | stage 1 | Find Kira (stage 2), Prove yourselves to Thomas (3), Meet Finn at the gate (4) |
| The Old Shrine | Main | stage 4 | Open the entrance (5), Get past the Constructs (6), Reach the heart (7) |
| Memories in Stone | Main | stage 7 | Study the murals (`told_kira`), Search the central mural (`fragment_2`), Face the Warden (8), Use the Memory Viewer (9) |
| Homecoming | Main | stage 9 | Show Finn (10), Find Kira (11), Join the farewell (`act2_path`) |
| The Stubborn Well | Side | Garrett asks for help (`garrett_asked`, set on `garrett_ask`) | Untangle the rope (`well_fixed`), Tell Garrett (`garrett_key`) |
