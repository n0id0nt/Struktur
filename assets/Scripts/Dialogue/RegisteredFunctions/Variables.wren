import "dialogue" for DialogueRegistry
import "flags" for FlagManager
import "Dialogue/RegisteredFunctions/Operators" for OperatorFunctions
import "Inventory" for Inventory
import "debug" for Debug

class Variables {
    static registerFunctions() {
        DialogueRegistry.registerVariable("item_count") { |params|
            var item = params["item"]
            var result = Inventory.contains(item)
            if (result) {
                return 1
            }
            return 0
        }
        DialogueRegistry.registerVariable("contains_item") { |params|
            var item = params["item"]
            var result = Inventory.contains(item)
            return result
        }
        DialogueRegistry.registerVariable("keys_collected") { |params|
            return 0
        }
    }
}