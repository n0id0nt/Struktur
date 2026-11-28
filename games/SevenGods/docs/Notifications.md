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

Banners are driven by the engine's event system. Whatever causes a notification sends a game event
(`Event.addEvent`, through the helpers in `GameEvents.wren`) at the moment it happens. `Notifications` only listens: the
events are delivered after the frame's update, and it queues a banner for each one it knows. Nothing polls for changes.

| File | What it is |
|---|---|
| `assets/Scripts/Events/GameEvents.wren` | The game events and the helpers that send them |
| `assets/Scripts/UI/Notifications/Notifications.wren` | The banner stack; `onEvent` turns events into banners |
| `assets/Scripts/States/GameWorldState.wren` | Forwards every event to `Notifications.onEvent`, calls `Notifications.update(showing)` every frame, and `clear()` on exit |

## What raises one

| Game event | Sent by | Header | Message | Icon (category) |
|---|---|---|---|---|
| `questStarted` | `QuestLog.setFlag` | New quest | its title | scroll (`quest`) |
| `questUpdated` | `QuestLog.setFlag` | Quest updated | the objective | scroll (`quest`) |
| `questCompleted` | `QuestLog.setFlag` | Quest complete | its title | scroll (`quest`) |
| `itemReceived` | `giveItem` dialogue command, picking an Item up | Item received | name, plus `xN` for more than one | bag (`item`) |
| `goldReceived` | `giveGold` dialogue command | Gold received | `+amount` | coin (`gold`) |
| `partyJoined` | `Party.join` (`joinParty` dialogue command, a won recruiting duel) | Joined the party | their display name | helmet (`party`) |

Loading a save doesn't announce party members already in it (`Party.load` adds them without going through `join`).

**Quest banners**
- Quest progress comes from int flags (see [Quests.md](Quests.md)). Every int-flag write that can move a quest goes
  through `QuestLog.setFlag(name, value)`: the `setIntFlag` dialogue command, `startQuest` / `completeObjective` /
  `completeQuest`, and a won duel. It works out the quest log just before and just after the write, and sends an
  event for each quest that started, each objective done and each quest completed.
- Loading a save writes no flags through `setFlag`, so it announces nothing.
- Write quest flags with `QuestLog.setFlag`, not `FlagManager.setIntFlag`, or the change won't be announced.

## Adding a new notification

1. Add the event to `GameEvents.wren`: a type constant, a getter for it, and a helper that sends it with its data.
2. Call the helper wherever the thing happens.
3. Handle the type in `Notifications.onEvent`: build the header and message, and pick a category.

```wren
import "Events/GameEvents" for GameEvents

GameEvents.itemReceived("Healing Herb", 3)
GameEvents.goldReceived(50)
```

- Built-in categories and their icons are in `CATEGORIES` at the top of `Notifications.wren`: `quest`, `item`,
  `gold`, `party` and `info`. `Notifications.setCategoryIcon` adds or replaces one.
- Headers come from `en.json`: `notify.quest.started`, `notify.quest.updated`, `notify.quest.complete`, `notify.item`,
  `notify.gold` and `notify.party`.
- Timing and layout are constants at the top of `Notifications.wren`: `HOLD_SECONDS`, `SLIDE_SECONDS`,
  `SPAWN_INTERVAL`, `MAX_VISIBLE`, `WIDTH` / `HEIGHT` / `GAP` / `MARGIN`, and `LEAVE_AT`, which sets how far out a
  leaving banner is before the ones below move up.
