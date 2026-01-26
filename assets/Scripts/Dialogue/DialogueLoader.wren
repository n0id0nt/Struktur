// DialogueLoader.wren
// Loads all dialogue definitions and passes them to the dialogue manager

import "./GregDialogue" for GregDialogue
import "./ItemInteractions" for ItemInteractions
import "./NPCInteractions" for NPCInteractions

class DialogueLoader {
    static loadAllDialogue() {
        System.print("Loading all dialogue...")
        
        // Load Greg's dialogue
        var gregData = GregDialogue.getData()
        DialogueManager.loadDialogue(gregData)
        System.print("Loaded Greg dialogue")
        
        // Load item interactions
        var itemData = ItemInteractions.getData()
        DialogueManager.loadDialogue(itemData)
        System.print("Loaded item interactions")
        
        // Load NPC interactions
        var npcData = NPCInteractions.getData()
        DialogueManager.loadDialogue(npcData)
        System.print("Loaded NPC interactions")
        
        System.print("All dialogue loaded successfully")
    }
    
    // Get entry point for a specific interaction
    static getGregEntryPoint() {
        return GregDialogue.getEntryPoint()
    }
    
    static getItemInteractionEntry(itemType) {
        return ItemInteractions.getEntryPoint(itemType)
    }
    
    static getNPCInteractionEntry(npcName) {
        return NPCInteractions.getEntryPoint(npcName)
    }
}
