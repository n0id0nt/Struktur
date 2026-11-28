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
        
        // Arc 1 ("The Awakening") - docs/FirstArcPlan.md, one file per character (places and things in World). All of
        // them load into one graph, so a node can lead into another file's nodes. docs/Arc1Dialogue.md is the script.
        var dialogueFiles = [
            "Dialogue/Arc1/Finn.json",
            "Dialogue/Arc1/Thomas.json",
            "Dialogue/Arc1/Kira.json",
            "Dialogue/Arc1/Elena.json",
            "Dialogue/Arc1/Garrett.json",
            "Dialogue/Arc1/Child.json",
            "Dialogue/Arc1/Guide.json",
            "Dialogue/Arc1/Warden.json",
            "Dialogue/Arc1/World.json"
        ]

        for (filePath in dialogueFiles) {
            var result = FileSystem.readString(filePath)
            if (result.success) {
                var value = result.value
                var data = Json.parse(value)
                // A dialogue file is {"root": the node the editor opens on, "nodes": [...], "editor": {"positions"}} -
                // or, older, just the list of nodes.
                if (data is Map) data = data["nodes"]
                if (data is List) {
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