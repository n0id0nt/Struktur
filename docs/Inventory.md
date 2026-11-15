# Inventory

The inventory has the tabs **Items**, **Moves**, **Party**, **Levels** and **Quests**, plus unlockable tabs that only
show once earned. So far there's one unlockable tab: **Map**.

| File | What it is |
|---|---|
| `assets/Scripts/States/InventoryState.wren` | `TABS` (the order and unlock rules), and builds each page's model |
| `assets/Scripts/UI/Inventory/InventoryUI.wren` | The window, the tab bar, and `openTab(id)` |
| `assets/Scripts/UI/Inventory/ItemGridPage.wren` | The Items tab |
| `assets/Scripts/UI/Inventory/MapPage.wren` | The Map tab |
| `assets/Scripts/Inventory/ItemDatabase.wren` | Each item's category, bonuses, icon, and `opensTab` |
| `assets/Scripts/World/MapLog.wren` | Which rooms have been visited (`map.sav`) |

## Items tab

One scrolling grid split into four sections, each with a heading. Every section shows at least two rows of boxes
(`MIN_ROWS` in `ItemGridPage.wren`), with unused boxes drawn darker, and grows a row at a time as it fills. A scroll
bar on the right shows where you are.

| Section | Category | Stacking |
|---|---|---|
| Key Items | `key` | One of a kind: holding one already, getting another adds nothing |
| Consumables | `consumable` | One stack with a count |
| Weapons | `weapon` | Each one is its own cell |
| Armor | `armor` | Each one is its own cell |

- **An item not listed in `ITEMS` is a key item.** That covers the quest items handed out by dialogue: the map, the
  horn, keys, records and so on.
- **`"collect": true`** lets a key item gather a count. Soul Fragments use this, since there are several.
- **Old `material` items** from earlier saves show under Key Items.
- **Navigation:** empty boxes can be focused too, so the arrows reach every row and scroll the grid. Up and down step
  one row at a time, crossing into the next section. Up from the top row goes back to the tab bar. An empty box
  clears the detail panel.

**Tab bar:** the tabs stretch to fill the bar between the Q and E prompts, so a locked tab makes the others wider
instead of leaving a gap.

## Unlockable tabs

`TABS` in `InventoryState.wren` lists every tab with an unlock rule, or `null` for always shown. A locked tab isn't in
the tab bar at all.

```wren
var TABS = [
    ["items", null],
    ...
    ["map", Fn.new { InventoryState.carrying_("map") }]
]
```

- **`carrying_(id)`** is true while the player carries an item whose `opensTab` is that tab.
- **A rule can check anything else too**, for example a flag: `Fn.new { FlagManager.getIntFlag("x") >= 1 }`.
- **To add a new tab**, add its row to `TABS`, its page to `InventoryUI.buildPage_`, its model in `buildModel_`, and
  `inventory.tab.<id>` to `en.json`.
- **Items that open a tab:** an item with `"opensTab": "<id>"` in `ITEMS` opens that tab when pressed. Its detail panel
  shows `inventory.open.<id>` (for the map: "Select to view the map").
- **Opening on a given tab:** `changeState("InventoryState", {"tab": "map"})` opens the inventory straight onto that
  tab, for example from a map hotkey.

## Map tab

- **Unlocked by** carrying the Old Map.
- **Rooms:** every room the player has walked into is drawn as a rectangle where it sits in the LDtk world, scaled to
  fit the page.
- **Colours:** each room is filled with the colour of its **Terrain** level field. The colour is the `BattleArena`
  enum value's colour set in LDtk, so changing a colour there changes it on the map. Rooms with no Terrain are grey.
- **Where you are:** the room you're in has an orange outline and a marker. Its name is shown top right, made from the
  level identifier (`FinnsWorkshop` becomes "Finns Workshop").
- **Legend:** along the bottom, naming each terrain colour on the map. The names come from `terrain.<value>` in
  `en.json`.
- **What counts as visited:** a room is logged when the player enters it (`GameWorldState.updateLevelStreaming_` calls
  `MapLog.visit`). Battle arenas (`*BattleStage`) never count. The log is saved in `map.sav`.
- **Older saves:** the log starts empty, so an older save's map fills in from the next room you enter.

**Engine bindings.** The map uses two new `World` functions in `src/WrenBindings/WrenGameObjectComponents.cpp`:
- `World.getLevelIdentifier(world, index)`: a level's LDtk name.
- `World.getLevelFieldColor(world, index, field)`: the LDtk colour of the enum value that a level's enum field holds.

The LDtk parser now keeps every enum value's colour (`World::enumColors`) and each enum field's enum type.
