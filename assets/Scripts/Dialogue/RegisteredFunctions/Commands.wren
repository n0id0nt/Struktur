import "dialogue" for DialogueRegistry
import "flags" for FlagManager
import "Inventory" for Inventory
import "debug" for Debug

class Commands {
    static registerFunctions() {
        DialogueRegistry.registerCommand("giveItem") { |params|
            var item = params["item"]
            Inventory.addItem(item)
            Debug.info("[Dialogue Command] Added item to inventory %(item)")
        }
        DialogueRegistry.registerCommand("removeItem") { |params|
            var item = params["item"]
            Inventory.removeItem(item)
            Debug.info("[Dialogue Command] Removed item from inventory %(item)")
        }
        DialogueRegistry.registerCommand("setIntFlag") { |params|
            var flag = params["flag"]
            var value = params["value"]
            var intFlag = FlagManager.setIntFlag(flag, value)
            Debug.info("[Dialogue Command] Set Int Flag for %(flag): %(value)")
        }
        DialogueRegistry.registerCommand("camerShake") { |params|
            var flag = params["flag"]
            var value = params["value"]
            var intFlag = FlagManager.setIntFlag(flag, value)
            Debug.info("[Dialogue Command] Set Int Flag for %(flag): %(value)")
        }
    }
}
