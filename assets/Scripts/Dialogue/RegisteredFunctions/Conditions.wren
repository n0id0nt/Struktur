import "dialogue" for DialogueRegistry
import "flags" for FlagManager
import "Dialogue/RegisteredFunctions/Operators" for OperatorFunctions
import "Inventory" for Inventory

class Conditions {
    static registerFunctions() {
        DialogueRegistry.registerCondition("hasIntFlag") { |params|
            var intFlag = FlagManager.getIntFlag(params[flag])
            return OperatorFunctions.evaluate(params[op], intFlag, params[value])
        }
        DialogueRegistry.registerCondition("hasItem") { |params|
            return Inventory.contains(params[item])
        }
    }
}