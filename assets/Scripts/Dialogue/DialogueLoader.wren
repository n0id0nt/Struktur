// DialogueLoader.wren
// Loads all dialogue definitions and passes them to the dialogue manager

import "dialogue" for DialogueManager
import "Dialogue/RegisteredFunctions/Commands" for Commands
import "Dialogue/RegisteredFunctions/Conditions" for Conditions
import "Dialogue/RegisteredFunctions/Variables" for Variables
import "fileSystem" for FileSystem
import "serialisation" for Json
import "debug" for Debug

class DialogueLoader {
    static loadAllDialogue() {
        System.print("Loading all dialogue...")
        
        var dialogueFiles = [
            "Dialogue/GardenerDialogue.json",
            "Dialogue/ScholarDialogue.json",
            "Dialogue/AstronomerDialogue.json",
            "Dialogue/CookDialogue.json",
            "Dialogue/CordeliaDialogue.json",
            "Dialogue/DreamerDialogue.json",
            "Dialogue/GuardianDialogue.json",
            "Dialogue/InventorDialogue.json",
            "Dialogue/MerchantDialogue.json",
            "Dialogue/ItemDialogue.json"
        ]

        for (filePath in dialogueFiles) {
            var result = FileSystem.readString(filePath)
            if (result.success) {
                var value = result.value
                var data = Json.parse(value)
                if (data) {
                    DialogueManager.loadDialogueData(data)
                    Debug.info("[Dialogue Loading] succesfuly loaded %(filePath)")
                } else {
                    Debug.error("[Dialogue Loading] failed to parse %(filePath), error message: %(value)")
                }
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