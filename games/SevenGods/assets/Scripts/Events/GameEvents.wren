// Events/GameEvents.wren
// The game events that other systems react to, sent through the engine's event system (the "events" module's
// Event.addEvent). An event is queued when it's sent and delivered after the frame's update, to Game.onEvent ->
// the active state's onEvent (and every script component's onEvent) - so whatever caused it never calls the
// listeners itself. UI/Notifications/Notifications.wren listens for all of these and shows a banner for each.
//
// Whatever causes one of these calls the matching method at the moment it happens - nothing watches for changes:
//   itemReceived    an item lands in the inventory (the giveItem dialogue command, GameObjects/Item.wren pickups)
//   goldReceived    gold is given (the giveGold dialogue command)
//   partyJoined     an NPC joins the party (Party/Party.wren join - not loading a save)
//   questStarted    a quest stops being hidden    } Story/QuestLog.wren setFlag - every quest flag write goes through
//   questUpdated    one of its objectives is done } it, and it compares the quests before and after the write
//   questCompleted  a quest is complete           }
//   movesRemoved    equipping a weapon took moves it can't use out of the loadout (Inventory/Equipment.wren) - the
//                   inventory (States/InventoryState.wren) shows an alert and flags its Moves tab
//   partyRested     the party rested back to full (the restParty dialogue command - a rest spot, a bed)
//   soulSight       Soul Sight was switched on or off (World/SoulSight.wren) - hidden doors and plates show or hide,
//                   and the screen tint and the player's soul light follow (out in the world - a fight keeps its own)
//
// Each method names the event's type (the constants below) and builds its data as a map.
import "events" for Event

var ITEM_RECEIVED = "itemReceived"
var GOLD_RECEIVED = "goldReceived"
var PARTY_JOINED = "partyJoined"
var QUEST_STARTED = "questStarted"
var QUEST_UPDATED = "questUpdated"
var QUEST_COMPLETED = "questCompleted"
var MOVES_REMOVED = "movesRemoved"
var SOUL_SIGHT = "soulSight"
var PARTY_RESTED = "partyRested"

class GameEvents {
    static itemReceivedType { ITEM_RECEIVED }
    static goldReceivedType { GOLD_RECEIVED }
    static partyJoinedType { PARTY_JOINED }
    static questStartedType { QUEST_STARTED }
    static questUpdatedType { QUEST_UPDATED }
    static questCompletedType { QUEST_COMPLETED }
    static movesRemovedType { MOVES_REMOVED }
    static soulSightType { SOUL_SIGHT }
    static partyRestedType { PARTY_RESTED }

    // data: {}
    static partyRested() {
        Event.addEvent(PARTY_RESTED, [["__type", "map"]])
    }

    // data: {"active": 1 or 0}
    static soulSightChanged(active) {
        Event.addEvent(SOUL_SIGHT, [["__type", "map"], ["active", active ? 1 : 0]])
    }

    // data: {"item": id, "count": n}
    static itemReceived(id, count) {
        Event.addEvent(ITEM_RECEIVED, [["__type", "map"], ["item", id], ["count", count == null ? 1 : count]])
    }

    // data: {"amount": n}
    static goldReceived(amount) {
        Event.addEvent(GOLD_RECEIVED, [["__type", "map"], ["amount", amount]])
    }

    // data: {"name": npc id}
    static partyJoined(npcId) {
        Event.addEvent(PARTY_JOINED, [["__type", "map"], ["name", npcId]])
    }

    // data: {"quest": quest id}
    static questStarted(questId) {
        Event.addEvent(QUEST_STARTED, [["__type", "map"], ["quest", questId]])
    }

    // data: {"quest": quest id, "objective": objective id}
    static questUpdated(questId, objectiveId) {
        Event.addEvent(QUEST_UPDATED, [["__type", "map"], ["quest", questId], ["objective", objectiveId]])
    }

    // data: {"quest": quest id}
    static questCompleted(questId) {
        Event.addEvent(QUEST_COMPLETED, [["__type", "map"], ["quest", questId]])
    }

    // data: {"weapon": the weapon item equipped, "moves": [the move names taken out of the loadout]}
    static movesRemoved(weaponId, moveNames) {
        Event.addEvent(MOVES_REMOVED, [["__type", "map"], ["weapon", weaponId], ["moves", moveNames]])
    }
}
