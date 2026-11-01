import "dialogue" for DialogueRegistry
import "flags" for FlagManager
import "Inventory" for Inventory
import "Party" for Party
import "debug" for Debug
import "gameObjectComponents" for Camera, Script, WorldTransform
import "Party/DuelRequest" for DuelRequest
import "Dialogue/ChoiceRecord" for ChoiceRecord
import "gameObject" for GameObject
import "events" for Event

class Commands {
    static registerFunctions() {
        DialogueRegistry.registerCommand("giveItem") { |params|
            var item = params["item"]
            Inventory.addItem(item)
            Inventory.save("inventory.sav")
            Debug.info("[Dialogue Command] Added item to inventory %(item)")
            Event.addEvent("giveItem", [["__type", "map"], ["item", item]])
        }
        DialogueRegistry.registerCommand("removeItem") { |params|
            var item = params["item"]
            Inventory.removeItem(item)
            Inventory.save("inventory.sav")
            Debug.info("[Dialogue Command] Removed item from inventory %(item)")
            Event.addEvent("removeItem", [["__type", "map"], ["item", item]])
        }
        DialogueRegistry.registerCommand("setIntFlag") { |params|
            var flag = params["flag"]
            var value = params["value"]
            FlagManager.setIntFlag(flag, value)
            FlagManager.save("flags.sav")
            Debug.info("[Dialogue Command] Set Int Flag for %(flag): %(value)")
            //Event.addEvent("setIntFlag", params)
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
        // where they stood (States/ExperimentState.syncPartyFollowers does the spawning).
        DialogueRegistry.registerCommand("joinParty") { |params|
            var npcName = params["name"]
            var spawnPosition = null
            for (npcEntity in GameObject.getAllWithIdentifier(npcName)) {
                spawnPosition = WorldTransform.getPosition(npcEntity)
                GameObject.destroy(npcEntity)
            }
            Party.join(npcName, spawnPosition)
            Debug.info("[Dialogue Command] Party joined by %(npcName)")
            Event.addEvent("joinParty", [["__type", "map"], ["name", npcName]])
        }
        // A rest spot: the whole party back to full HP / stamina, downed members included (Party.restAll).
        DialogueRegistry.registerCommand("restParty") { |params|
            Party.restAll()
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
            DuelRequest.request(params["name"])
            Debug.info("[Dialogue Command] Duel requested against %(params["name"])")
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
