import "dialogue" for DialogueRegistry
import "flags" for FlagManager
import "Dialogue/RegisteredFunctions/Operators" for OperatorFunctions
import "Inventory" for Inventory
import "debug" for Debug

class Conditions {
    static registerFunctions() {
        DialogueRegistry.registerCondition("hasIntFlag") { |params|
            var flag = params["flag"]
            var op = params["op"]
            var value = params["value"]

            var intFlag = FlagManager.getIntFlag(flag)
            var result = OperatorFunctions.evaluate(op, intFlag, value)
            Debug.info("[Dialogue Condition] Checking condition for flag where %(flag) %(op) %(value) = %(result)")
            return result
        }
        DialogueRegistry.registerCondition("hasItem") { |params|
            var item = params["item"]

            var result = Inventory.contains(item)
            Debug.info("[Dialogue Condition] Checking condition inventory has %(item) = %(result)")
            return result
        }
    }
}