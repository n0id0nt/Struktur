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
        Debug.info("Loading all dialogue...")
        
        // Arc 1 ("The Awakening") - docs/FirstArcPlan.md. Generated content; one file until it's worth splitting.
        var dialogueFiles = [
            "Dialogue/Arc1Dialogue.json"
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