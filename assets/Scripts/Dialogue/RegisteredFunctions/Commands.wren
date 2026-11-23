import "dialogue" for DialogueRegistry
import "flags" for FlagManager
import "Inventory/Inventory" for Inventory
import "Combat/MoveBook" for MoveBook
import "Party/Party" for Party
import "debug" for Debug
import "gameObjectComponents" for Camera, Script, WorldTransform
import "Party/DuelRequest" for DuelRequest
import "Dialogue/ChoiceRecord" for ChoiceRecord
import "gameObject" for GameObject
import "events" for Event
import "World/Npcs" for Npcs
import "Story/ArcEnd" for ArcEnd
import "World/StoryEvents" for StoryEvents
import "World/StageDirector" for StageDirector
import "Story/QuestLog" for QuestLog
import "Events/GameEvents" for GameEvents
import "GameObjects/Door" for Door
import "World/SpawnPoint" for SpawnPoint
import "World/Rooms" for Rooms
import "math" for Vec2
import "Dialogue/Portraits" for Portraits
import "Dialogue/CloseUp" for CloseUp
import "GameObjects/Critter" for Critter
import "reflect" for Reflect

// px above the crystal's centre that the soul fragment comes loose from ("soulFragment").
var FRAGMENT_LIFT = 14
// "shootArrow": where the arrow leaves the bow, from the shooter's origin - its collider's centre (px).
var BOW_OFFSET = Vec2.new(6, -11)

class Commands {
    // A staging command's "wait" parameter: on unless it's 0 / false / "no" (JSON bools are avoided in dialogue
    // files, so 0 / 1 is the usual spelling).
    static waitParam_(params) {
        var value = params["wait"]
        if (value == null) return true
        if (value is Num) return value != 0
        if (value is Bool) return value
        return value != "no" && value != "false" && value != "0"
    }

    // The scripts of every loaded door called `doorName`. Script-spawned doors use their name as the entity identifier;
    // doors placed in the level are all "Door".
    static doorsNamed_(doorName) {
        var doors = []
        for (doorEntity in GameObject.getAllWithIdentifier(doorName) + GameObject.getAllWithIdentifier("Door")) {
            var door = Script.getInstance(doorEntity)
            if (door && door.name == doorName) doors.add(door)
        }
        return doors
    }

    static registerFunctions() {
        // "count" is optional (default 1) - stackable items add to their stack, gear adds that many separate pieces.
        DialogueRegistry.registerCommand("giveItem") { |params|
            var item = params["item"]
            var count = params["count"] != null ? params["count"] : 1
            Inventory.addItem(item, count)
            Inventory.save("inventory.sav")
            GameEvents.itemReceived(item, count)
            Debug.info("[Dialogue Command] Added item to inventory %(item)")
            Event.addEvent("giveItem", [["__type", "map"], ["item", item]])
        }
        DialogueRegistry.registerCommand("removeItem") { |params|
            var item = params["item"]
            var count = params["count"] != null ? params["count"] : 1
            Inventory.removeItem(item, count)
            Inventory.save("inventory.sav")
            Debug.info("[Dialogue Command] Removed item from inventory %(item)")
            Event.addEvent("removeItem", [["__type", "map"], ["item", item]])
        }
        DialogueRegistry.registerCommand("giveGold") { |params|
            Inventory.addGold(params["amount"])
            Inventory.save("inventory.sav")
            GameEvents.goldReceived(params["amount"])
            Debug.info("[Dialogue Command] Gave %(params["amount"]) gold")
        }
        // Takes the gold only if there is enough - pair it with the "hasGold" condition to branch on that.
        DialogueRegistry.registerCommand("takeGold") { |params|
            if (Inventory.spendGold(params["amount"])) {
                Inventory.save("inventory.sav")
                Debug.info("[Dialogue Command] Took %(params["amount"]) gold")
            }
        }
        // Teaches the player a move (a Combat/Config/Moves.wren name, e.g. "Cleave") - it then shows in the
        // inventory's Moves tab, ready to equip.
        DialogueRegistry.registerCommand("unlockMove") { |params|
            if (MoveBook.unlock(params["name"])) {
                MoveBook.save("moves.sav")
                Debug.info("[Dialogue Command] Unlocked move %(params["name"])")
            }
        }
        DialogueRegistry.registerCommand("setIntFlag") { |params|
            var flag = params["flag"]
            var value = params["value"]
            // Through QuestLog so any quest this moves on is announced (Events/GameEvents.wren).
            QuestLog.setFlag(flag, value)
            Debug.info("[Dialogue Command] Set Int Flag for %(flag): %(value)")
        }
        DialogueRegistry.registerCommand("setFlag") { |params|
            var flag = params["flag"]
            var value = params["value"]
            FlagManager.setFlag(flag, value)
            FlagManager.save("flags.sav")
            Debug.info("[Dialogue Command] Set Bool Flag for %(flag): %(value)")
            //Event.addEvent("setFlag", params)
        }
        DialogueRegistry.registerCommand("cameraShake") { |params|
            var cameras = GameObject.getAllWithComponent("Camera")
            for (cameraId in cameras) {
                Camera.addCameraTrauma(cameraId, 0.4)
            }
            //Event.addEvent("cameraShake", params)
        }
        DialogueRegistry.registerCommand("pickupEntity") { |params|
            var entityName = params["name"]
            var itemEntities = GameObject.getAllWithIdentifier("Item")

            for (itemEntity in itemEntities) {
                var item = Script.getInstance(itemEntity)
                if (item && item.name == entityName) {
                    if (item.returnable) {
                        item.setIsShadow(true)
                    } else {
                        FlagManager.setFlag(entityName, true)
                        FlagManager.save("flags.sav")
                        GameObject.destroy(itemEntity)
                    }
                    Debug.info("[Dialogue Command] Picked up Item %(name): %(itemEntity)")
                }
            }
            Event.addEvent("pickupEntity", [["__type", "map"], ["name", entityName]])
        }
        // The named NPC joins the party: the standing NPC steps out of the overworld, and their follower appears
        // where they stood (States/GameWorldState.syncPartyFollowers does the spawning).
        // "at" (optional): a Path or Marker name - the follower appears there instead (someone arriving through a door
        // rather than standing in the room). The follower appears straight away, even mid-conversation, so a later
        // node can walk them over (moveTo / moveToPlayer).
        DialogueRegistry.registerCommand("joinParty") { |params|
            var npcName = params["name"]
            var spawnPosition = null
            for (npcEntity in Npcs.find(npcName)) {
                spawnPosition = WorldTransform.getPosition(npcEntity)
                GameObject.destroy(npcEntity)
            }
            if (params["at"] != null) {
                var spot = StageDirector.spotNamed(params["at"])
                if (spot != null) spawnPosition = spot
            }
            Party.join(npcName, spawnPosition)
            Debug.info("[Dialogue Command] Party joined by %(npcName)")
            Event.addEvent("joinParty", [["__type", "map"], ["name", npcName]])
        }
        // The named NPC leaves the party - their follower vanishes (best done off screen). Saved, so they stay gone
        // after a reload until a "joinParty" brings them back.
        DialogueRegistry.registerCommand("leaveParty") { |params|
            Party.leave(params["name"])
        }
        // Where the player starts a session / wakes after a wipe from now on: the LDtk Spawn with this "iid", in the
        // level "level" (World/SpawnPoint.wren) - so a scene that shuts the way back doesn't strand a reload behind it.
        DialogueRegistry.registerCommand("setSpawnPoint") { |params|
            SpawnPoint.set(params["iid"], params["level"])
        }
        // The arc is over: once this dialogue closes, the gameplay state shows the end-of-arc card (Story/ArcEnd.wren).
        DialogueRegistry.registerCommand("endArc") { |params|
            ArcEnd.request()
            Debug.info("[Dialogue Command] End of arc requested")
        }
        // A rest spot: the whole party back to full HP / stamina, downed members included (Party.restAll).
        DialogueRegistry.registerCommand("restParty") { |params|
            Party.restAll()
            GameEvents.partyRested()
            Debug.info("[Dialogue Command] Party rested")
        }
        // The named party member waits where they stand (holding a pressure plate down, say) / follows again.
        DialogueRegistry.registerCommand("partyStay") { |params|
            Party.setStaying(params["name"], true)
        }
        DialogueRegistry.registerCommand("partyFollow") { |params|
            Party.setStaying(params["name"], false)
        }
        // Clears the collapsed record of the last choice made (States/InteractState.wren keeps it up after a pick
        // until the next set of choices, this, or the end of the conversation).
        DialogueRegistry.registerCommand("closeChoice") { |params|
            ChoiceRecord.requestClose()
        }
        // Challenges the named NPC to a duel: the fight starts once this dialogue has ended (the world picks the
        // request up - see Party/DuelRequest.wren).
        DialogueRegistry.registerCommand("startDuel") { |params|
            DuelRequest.request(params["name"], false)
            Debug.info("[Dialogue Command] Duel requested against %(params["name"])")
        }
        // A story battle against everyone here with this name (the three Wardens): unlike a duel it's lethal - losing
        // is a game over. Starts once this dialogue has ended, like a duel.
        DialogueRegistry.registerCommand("startBattle") { |params|
            var foeName = params["name"]
            DuelRequest.request(foeName, true)
            Debug.info("[Dialogue Command] Battle requested against %(foeName)")
        }
        // Opens every door with this name - its entity identifier (both halves of a double door - GameObjects/Door.wren)
        // for good; the door saves that it's open. A Barrier lifts, even with its room not loaded (its flag clears).
        DialogueRegistry.registerCommand("openDoor") { |params|
            var doorName = params["name"]
            if (FlagManager.getFlag(Door.closedFlag(doorName))) {
                FlagManager.setFlag(Door.closedFlag(doorName), false)
                FlagManager.save("flags.sav")
            }
            for (door in Commands.doorsNamed_(doorName)) {
                if (!door.open) door.openNow()
            }
            Debug.info("[Dialogue Command] Opened door %(doorName)")
        }
        // Drops every Barrier door with this name (GameObjects/Door.wren), and saves that it's down.
        DialogueRegistry.registerCommand("closeDoor") { |params|
            var doorName = params["name"]
            FlagManager.setFlag(Door.closedFlag(doorName), true)
            FlagManager.save("flags.sav")
            for (door in Commands.doorsNamed_(doorName)) {
                door.closeNow()
            }
            Debug.info("[Dialogue Command] Closed door %(doorName)")
        }
        // --- Staging (World/StageDirector.wren) ---------------------------------------------------------------
        // "who" is "Player", a party member or an NPC / Interactable name; "to" is an LDtk Path's (or Marker's) Name, or "Player". moveTo and
        // wait hold the conversation until they finish (the next line shows after) unless "wait" is 0; a node with no
        // text just plays its commands and carries on.
        DialogueRegistry.registerCommand("moveTo") { |params|
            // "animation" (optional): move with this clip instead of Walk / Run (any clip the sheet has).
            StageDirector.moveTo(params["who"], params["to"], params["speed"], Commands.waitParam_(params), params["animation"])
        }
        // Walks "who" to the tile in front of the player and turns them to face the player. After a moveTo of the same
        // character it waits its turn, so the two chain: walk a Path, then come over to the player.
        DialogueRegistry.registerCommand("moveToPlayer") { |params|
            StageDirector.moveToPlayer(params["who"], params["speed"], Commands.waitParam_(params), params["animation"])
        }
        // Scene 9: the soul fragment drifts from "from" (a character / Interactable name - the shrine's crystal) into
        // the player's chest (GameObjects/SoulFragment.wren), holding the conversation until it's in.
        // "seconds" (optional, default 2.5): how long the drift takes.
        DialogueRegistry.registerCommand("soulFragment") { |params|
            var seconds = params["seconds"] is Num ? params["seconds"] : 2.5
            var source = StageDirector.actor(params["from"])
            if (source == null || Rooms.world == null) return
            var from = WorldTransform.getPosition(source)
            var entity = GameObject.create("SoulFragment", Rooms.world)
            Script.createArg(entity, "SoulFragment", {"FromX": from.x, "FromY": from.y - FRAGMENT_LIFT, "Seconds": seconds})
            StageDirector.wait(seconds + 0.3)
        }
        // An arrow from "who"'s bow (GameObjects/ArrowShot.wren) - a particle flying east, "distance" px (default 64),
        // loosed "delay" seconds after the node is reached (default 0.8 - the draw of the Shoot clip that plays
        // alongside it). The arrow art points east, so only eastward shots look right.
        DialogueRegistry.registerCommand("shootArrow") { |params|
            var shooter = StageDirector.actor(params["who"])
            if (shooter == null || Rooms.world == null) return
            var feet = WorldTransform.getPosition(shooter)
            var distance = params["distance"] is Num ? params["distance"] : 64
            var delay = params["delay"] is Num ? params["delay"] : 0.8
            var fromX = feet.x + BOW_OFFSET.x
            var fromY = feet.y + BOW_OFFSET.y
            var entity = GameObject.create("ArrowShot", Rooms.world)
            Script.createArg(entity, "ArrowShot", {"FromX": fromX, "FromY": fromY, "ToX": fromX + distance, "ToY": fromY, "Delay": delay})
        }
        // Every creature here that can charge (the perimeter wolf - GameObjects/PerimeterWolf.wren) runs at the player;
        // catching them starts the fight once the conversation is over.
        DialogueRegistry.registerCommand("chargePlayer") { |params|
            for (critter in Critter.all) {
                if (Reflect.hasMethod(critter, "charge()")) critter.charge()
            }
        }
        DialogueRegistry.registerCommand("teleport") { |params|
            StageDirector.teleport(params["who"], params["to"])
        }
        // "towards": a direction (up / down / left / right / upLeft... or North / South / East / West), or the name of
        // a character or Marker to turn to.
        DialogueRegistry.registerCommand("face") { |params|
            StageDirector.face(params["who"], params["towards"])
        }
        // "animation": a clip of the character's sheet - Idle, Walk, Run, Hurt, Roll, Death, SittingIdle, LyingDown,
        // GettingUp (everyone); the player and Kira also Drinking, Inspecting, Lifting, and Kira Shoot; Finn Working,
        // LayingIdle, PickingUp, Inspecting, Lifting; Thomas Working (docs/DialogueStaging.md). Held as a pose until changed. "wait" (default 0
        // here): hold the conversation until it has played once. "then" (optional): the clip to switch to after.
        DialogueRegistry.registerCommand("playAnimation") { |params|
            var wait = params["wait"] == null ? false : Commands.waitParam_(params)
            StageDirector.playAnimation(params["who"], params["animation"], wait, params["then"])
        }
        // How the node's line is said - shown on the speaker's portrait (Dialogue/Portraits.wren): "mood" is a short
        // description ("chuckling", "quietly"). Without art for it yet, the portrait shows its default face with the
        // mood written under it.
        DialogueRegistry.registerCommand("expression") { |params|
            Portraits.expression = params["mood"]
        }
        // The close-up view (Dialogue/CloseUp.wren): "image" (e.g. "CloseUps/kira_book_wolf_fact.png") shown large
        // above the dialogue box until "closeUpEnd" or the end of the conversation.
        DialogueRegistry.registerCommand("closeUp") { |params|
            CloseUp.show(params["image"])
        }
        DialogueRegistry.registerCommand("closeUpEnd") { |params|
            CloseUp.hide()
        }
        DialogueRegistry.registerCommand("wait") { |params|
            var seconds = params["seconds"]
            StageDirector.wait(seconds is Num ? seconds : 1)
        }
        // --- Quests (Story/QuestLog.wren) ----------------------------------------------------------------------
        // For quests not already tracked by story flags in their JSON definition.
        DialogueRegistry.registerCommand("startQuest") { |params|
            QuestLog.start(params["quest"])
        }
        DialogueRegistry.registerCommand("completeObjective") { |params|
            QuestLog.completeObjective(params["quest"], params["objective"])
        }
        DialogueRegistry.registerCommand("completeQuest") { |params|
            QuestLog.finish(params["quest"])
        }
        // A story event happened (World/StoryEvents.wren) - every level door listing it in KeyIds opens once all of its
        // events have.
        DialogueRegistry.registerCommand("triggerEvent") { |params|
            var eventId = params["event"]
            StoryEvents.fire(eventId)
            Debug.info("[Dialogue Command] Story event %(eventId)")
        }
        DialogueRegistry.registerCommand("putdownEntity") { |params|
            var entityName = params["name"]
            var itemEntities = GameObject.getAllWithIdentifier("Item")

            for (itemEntity in itemEntities) {
                var item = Script.getInstance(itemEntity)
                if (item && item.name == entityName) {
                    item.setIsShadow(false)
                    Debug.info("[Dialogue Command] Put down Item %(name): %(itemEntity)")
                }
            }
            Event.addEvent("putdownEntity", [["__type", "map"], ["name", entityName]])
        }
    }
}
