// DialogueLoader.wren
// Loads all dialogue definitions and passes them to the dialogue manager

import "dialogue" for DialogueManager
import "Dialogue/RegisteredFunctions/Commands" for Commands
import "Dialogue/RegisteredFunctions/Conditions" for Conditions

import "Dialogue/ScholarDialogue" for ScholarDialogue
import "Dialogue/AstronomerDialogue" for AstronomerDialogue
import "Dialogue/CookDialogue" for CookDialogue
import "Dialogue/CordeliaDialogue" for CordeliaDialogue
import "Dialogue/DreamerDialogue" for DreamerDialogue
import "Dialogue/GardenerDialogue" for GardenerDialogue
import "Dialogue/GuardianDialogue" for GuardianDialogue
import "Dialogue/InventorDialogue" for InventorDialogue
import "Dialogue/MerchantDialogue" for MerchantDialogue
import "Dialogue/ItemsDialogue" for ItemDialogue

class DialogueLoader {
    static loadAllDialogue() {
        System.print("Loading all dialogue...")
        
        // Load Scholar's dialogue
        var scholarData = ScholarDialogue.getData()
        DialogueManager.loadDialogueData(scholarData)
        System.print("Loaded Scholar dialogue")

        // Load Astronomer's dialogue
        var astronomerDialogue = AstronomerDialogue.getData()
        DialogueManager.loadDialogueData(astronomerDialogue)
        System.print("Loaded Astronomer dialogue")

        // Load Cook's dialogue
        var cookDialogue = CookDialogue.getData()
        DialogueManager.loadDialogueData(cookDialogue)
        System.print("Loaded Cook dialogue")

        // Load Cordelia's dialogue
        var cordeliaDialogue = CordeliaDialogue.getData()
        DialogueManager.loadDialogueData(cordeliaDialogue)
        System.print("Loaded Cordelia dialogue")

        // Load Dreamer's dialogue
        var dreamerDialogue = DreamerDialogue.getData()
        DialogueManager.loadDialogueData(dreamerDialogue)
        System.print("Loaded Dreamer dialogue")

        // Load Gardener's dialogue
        var gardenerDialogue = GardenerDialogue.getData()
        DialogueManager.loadDialogueData(gardenerDialogue)
        System.print("Loaded Gardener dialogue")

        // Load Guardian's dialogue
        var guardianDialogue = GuardianDialogue.getData()
        DialogueManager.loadDialogueData(guardianDialogue)
        System.print("Loaded Guardian dialogue")

        // Load Inventor's dialogue
        var inventorDialogue = InventorDialogue.getData()
        DialogueManager.loadDialogueData(inventorDialogue)
        System.print("Loaded Inventor dialogue")

        // Load Merchant's dialogue
        var merchantDialogue = MerchantDialogue.getData()
        DialogueManager.loadDialogueData(merchantDialogue)
        System.print("Loaded Merchant dialogue")

        // Load item's dialogue
        var itemDialogue = ItemDialogue.getData()
        DialogueManager.loadDialogueData(itemDialogue)
        System.print("Loaded Item dialogue")
        
        System.print("All dialogue loaded successfully")

        Commands.registerFunctions()
        Conditions.registerFunctions()
    }
}