# Notification banners

Banners in the top-right corner announce game events: quests starting, moving on or finishing, and items or gold
received.

**How they move**
- Each banner slides in from the right edge, holds for 3 seconds, then slides back out.
- Several stack downwards, with the newest at the bottom. When one has slid out, the ones below slide up into its
  place.
- A burst of events, like a dialogue giving three items at once, comes in one after another, 0.25 s apart.
- At most 5 show at once. The rest wait their turn.
- Banners show over the world and over dialogue. While the inventory or a fight covers the screen, they freeze out of
  sight and carry on afterwards.

| File | What it is |
|---|---|
| `assets/Scripts/UI/Notifications/Notifications.wren` | The banner stack, and the `push` API |
| `assets/Scripts/Story/QuestTracker.wren` | Turns quest-log changes into banners |
| `assets/Scripts/States/GameWorldState.wren` | Calls `QuestTracker.poll()` and `Notifications.update(showing)` every frame, and `clear()` / `reset()` on exit |

## What raises one

| Event | Header | Message | Icon (category) |
|---|---|---|---|
| A quest starts | New quest | its title | scroll (`quest`) |
| An objective is done | Quest updated | the objective | scroll (`quest`) |
| A quest finishes | Quest complete | its title | scroll (`quest`) |
| `giveItem` dialogue command, or picking an Item up | Item received | name, plus `xN` for more than one | bag (`item`) |
| `giveGold` dialogue command | Gold received | `+amount` | coin (`gold`) |
| Someone joins the party (`joinParty` dialogue command, or a won recruiting duel - anything calling `Party.join`) | Joined the party | their display name | helmet (`party`) |

Loading a save doesn't announce party members already in it (`Party.load` adds them without going through `join`).

**Quest banners**
- Quest progress comes from flags (see [Quests.md](Quests.md)), so `QuestTracker` compares the quest log against its
  last look, 4 times a second, instead of every flag-setter reporting in.
- The first look after gameplay opens only takes note of where things stand, so loading a save doesn't announce every
  quest already in it.

## Raising your own

```wren
import "UI/Notifications/Notifications" for Notifications

Notifications.push("info", "Door unlocked", "The shrine gate grinds open")
Notifications.push("quest", "Secret found", "A hidden passage", "Items/Object/Book.png")   // a one-off icon
Notifications.item("Healing Herb", 3)
Notifications.gold(50)
Notifications.setCategoryIcon("ability", "Items/Scroll/ScrollFire.png")   // a new category
```

- Built-in categories and their icons are in `CATEGORIES` at the top of `Notifications.wren`: `quest`, `item`,
  `gold`, `party` and `info`.
- Headers come from `en.json`: `notify.quest.started`, `notify.quest.updated`, `notify.quest.complete`, `notify.item`,
  `notify.gold` and `notify.party`.
- Timing and layout are constants at the top of `Notifications.wren`: `HOLD_SECONDS`, `SLIDE_SECONDS`,
  `SPAWN_INTERVAL`, `MAX_VISIBLE`, `WIDTH` / `HEIGHT` / `GAP` / `MARGIN`, and `LEAVE_AT`, which sets how far out a
  leaving banner is before the ones below move up.
