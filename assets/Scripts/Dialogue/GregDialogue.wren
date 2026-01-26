// GregDialogue.wren
// Greg the Collector - Book Quest

import "dialogue" for DialogueData
import "flags" for FlagManager
import "Inventory" for Inventory

class GregDialogue {
    static getData() {
        var data = DialogueData.new()
        
        // After receiving Ancient Tome and giving Red Crystal Key (Quest Complete)
        data.addNode("greg_complete", {
            "speaker": "Greg",
            "text": "Ah, my precious Ancient Tome! Years of research finally returned to me.\nThis knowledge belongs in my library, where it can illuminate minds for eternity.",
            "conditions": [
                {"type": "intFlag", "parameters": {"flag": "greg_quest_state", "op": "==", "value": "3"}}
            ]
        })
        
        // Has Ancient Tome to trade (Step 1)
        data.addNode("greg_has_book_1", {
            "speaker": "Greg",
            "text": "Perfect! My lost research tome! This contains decades of study into ancient mysteries.",
            "conditions": [
                {"type": "intFlag", "parameters": {"flag": "greg_quest_state", "op": "==", "value": "1"}},
                {"type": "hasItem", "parameters": {"item": "ancient_book"}}
            ],
            "next": "greg_has_book_2"
        })
        
        // Has Ancient Tome to trade (Step 2 - offer)
        data.addNode("greg_has_book_2", {
            "speaker": "Greg",
            "text": "In exchange for returning my life's work, I offer you this rare gem.\nMay it serve you well in your travels.",
            "commands": [
                {"type": "giveItem", "parameters": {"item": "rare_gem"}},
                {"type": "removeItem", "parameters": {"item": "ancient_book"}},
                {"type": "setInt", "parameters": {"flag": "greg_quest_state", "value": "3"}}
            ],
            "next": "greg_has_book_3"
        })
        
        // Obtained rare gem confirmation
        data.addNode("greg_has_book_3", {
            "speaker": "Greg",
            "text": "Obtained Rare Gem"
        })
        
        // Still needs book
        data.addNode("greg_needs_book", {
            "speaker": "Greg",
            "text": "My Ancient Tome is still missing - it contains all my research into forgotten lore.",
            "conditions": [
                {"type": "intFlag", "parameters": {"flag": "greg_quest_state", "op": "==", "value": "1"}}
            ]
        })
        
        // First Meeting (Step 1)
        data.addNode("greg_first_1", {
            "speaker": "Greg",
            "text": "Welcome, fellow seeker of knowledge.\nI'm a collector of rare books and ancient texts.",
            "next": "greg_first_2"
        })
        
        // First Meeting (Step 2)
        data.addNode("greg_first_2", {
            "speaker": "Greg",
            "text": "I had an Ancient Tome - a very special book - but I seem to have misplaced it.\nIf you could find it, I would gladly trade you something valuable.",
            "commands": [
                {"type": "setInt", "parameters": {"flag": "greg_quest_state", "value": "1"}}
            ]
        })
        
        return data
    }
    
    static getEntryPoint() {
        var questState = FlagManager.getIntFlag("greg_quest_state")
        var hasBook = Inventory.hasItem("ancient_book")
        
        // Check in order - most specific first
        if (questState == 3) return "greg_complete"
        if (questState == 1 && hasBook) return "greg_has_book_1"
        if (questState == 1) return "greg_needs_book"
        return "greg_first_1"
    }
}
