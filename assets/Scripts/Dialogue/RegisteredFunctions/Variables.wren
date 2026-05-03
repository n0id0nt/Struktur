import "dialogue" for DialogueRegistry
import "flags" for FlagManager
import "Dialogue/RegisteredFunctions/Operators" for OperatorFunctions
import "Inventory" for Inventory
import "debug" for Debug

class Variables {
    static registerFunctions() {
        DialogueRegistry.registerVariable("testVar") { |params|
            return 2
        }
    }
}