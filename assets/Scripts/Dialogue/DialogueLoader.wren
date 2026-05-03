// DialogueLoader.wren
// Loads all dialogue definitions and passes them to the dialogue manager

import "dialogue" for DialogueManager
import "Dialogue/RegisteredFunctions/Commands" for Commands
import "Dialogue/RegisteredFunctions/Conditions" for Conditions
import "Dialogue/RegisteredFunctions/Variables" for Variables
import "fileSystem" for FileSystem
import "serialisation" for Json
import "debug" for Debug

class DebugPrintMap {
    static toString(value) {
        return DebugPrintMap.toStringIndented(value, 0)
    }

    static toStringIndented(value, depth) {
        var indent = "  " * depth
        var innerIndent = "  " * (depth + 1)
        var result = ""

        if (value == null) {
            result = "%(indent)null"
        } else if (value is Map) {
            result = "%(indent){\n"
            for (entry in value) {
                if (entry.value is Map || entry.value is List) {
                    result = result + "%(innerIndent)%(entry.key):\n"
                    result = result + DebugPrintMap.toStringIndented(entry.value, depth + 1) + "\n"
                } else {
                    result = result + "%(innerIndent)%(entry.key): %(entry.value)\n"
                }
            }
            result = result + "%(indent)}"
        } else if (value is List) {
            result = "%(indent)[\n"
            for (item in value) {
                if (item is Map || item is List) {
                    result = result + DebugPrintMap.toStringIndented(item, depth + 1) + "\n"
                } else {
                    result = result + "%(innerIndent)%(item)\n"
                }
            }
            result = result + "%(indent)]"
        } else {
            result = "%(indent)%(value)"
        }

        return result
    }
}

class DialogueLoader {
    static loadAllDialogue() {
        System.print("Loading all dialogue...")
        
        var dialogueFiles = [
            "Dialogue/ScholarDialogue.json",
            "Dialogue/AstronomerDialogue.json",
            "Dialogue/CookDialogue.json",
            "Dialogue/CordeliaDialogue.json",
            "Dialogue/DreamerDialogue.json",
            "Dialogue/GardenerDialogue.json",
            "Dialogue/GuardianDialogue.json",
            "Dialogue/InventorDialogue.json",
            "Dialogue/MerchantDialogue.json",
            "Dialogue/ItemDialogue.json"
            ]

        for (filePath in dialogueFiles) {
            var result = FileSystem.readString(filePath)
            if (result.success) {
                var data = Json.parse(result.value)
                if (!(data is List)) {
                    var string = DebugPrintMap.toString(data)
                    Debug.breakpointMsg("type check: List %(data is List), Map %(data is Map), Num %(data is Num), String %(data is String), Bool %(data is Bool) file:%(filePath) JSON:%(result.value) WREN:%(string)")
                }
                DialogueManager.loadDialogueData(data)
                Debug.info("[Dialogue Loading] succesfuly loaded %(filePath)")
            } else {
                Debug.error("[Dialogue Loading] failed to load %(filePath), error message: %(result.errorMessage)")
            }
        }        
        Debug.info("All dialogue loaded")

        Commands.registerFunctions()
        Conditions.registerFunctions()
        Variables.registerFunctions()
    }
}