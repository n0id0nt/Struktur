// DialogueLoader.wren
// Loads all dialogue definitions and passes them to the dialogue manager

import "dialogue" for DialogueManager
import "Dialogue/GregDialogue" for GregDialogue
import "Dialogue/RegisteredFunctions/Commands" for Commands
import "Dialogue/RegisteredFunctions/Conditions" for Conditions
//import "Dialogue/ItemInteractions" for ItemInteractions
//import "Dialogue/NPCInteractions" for NPCInteractions

class DialogueLoader {
    static loadAllDialogue() {
        System.print("Loading all dialogue...")
        
        // Load Greg's dialogue
        var gregData = GregDialogue.getData()
        DialogueManager.loadDialogueData(gregData)
        System.print("Loaded Greg dialogue")
        
        //// Load item interactions
        //var itemData = ItemInteractions.getData()
        //DialogueManager.loadDialogue(itemData)
        //System.print("Loaded item interactions")
        //
        //// Load NPC interactions
        //var npcData = NPCInteractions.getData()
        //DialogueManager.loadDialogue(npcData)
        //System.print("Loaded NPC interactions")
        
        System.print("All dialogue loaded successfully")

        Commands.registerFunctions()
        Conditions.registerFunctions()
    }
}