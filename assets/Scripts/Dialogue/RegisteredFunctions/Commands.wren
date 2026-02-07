import "dialogue" for DialogueRegistry
import "flags" for FlagManager
import "Dialogue/RegisteredFunctions/Operators" for OperatorFunctions

class Commands {
    static registerFunctions() {
        DialogueRegistry.registerCommand("giveItem") { |params|
            var intFlag = FlagManager.getIntFlag(params[flag])
            return OperatorFunctions.evaluate(params[op], intFlag, params[value])
        }
        DialogueRegistry.registerCommand("removeItem") { |params|
            var intFlag = FlagManager.getIntFlag(params[flag])
            return OperatorFunctions.evaluate(params[op], intFlag, params[value])
        }
        DialogueRegistry.registerCommand("setIntFlag") { |params|
            var intFlag = FlagManager.getIntFlag(params[flag])
            return OperatorFunctions.evaluate(params[op], intFlag, params[value])
        }
    }
}
