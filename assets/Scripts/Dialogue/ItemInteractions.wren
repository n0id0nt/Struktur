// ItemInteractions.wren
// Generic item interaction dialogues

foreign class DialogueData {
    construct new() {}
    foreign addNode(nodeId, nodeData)
}

class ItemInteractions {
    static getData() {
        var data = DialogueData.new()
        
        // === HEALING POTION ===
        data.addNode("item_healing_potion", {
            "speaker": "Narrator",
            "text": "A small vial filled with red liquid. It smells faintly of herbs and magic.",
            "choices": [
                {"text": "Drink it", "target": "item_healing_potion_drink"},
                {"text": "Keep it for later", "target": "item_generic_keep"}
            ]
        })
        
        data.addNode("item_healing_potion_drink", {
            "speaker": "Narrator",
            "text": "You drink the healing potion. Warmth spreads through your body as your wounds begin to close. You feel refreshed!",
            "commands": [
                {"type": "removeItem", "parameters": {"item": "healing_potion"}},
                {"type": "addInt", "parameters": {"flag": "player_health", "value": "50"}}
            ]
        })
        
        // === MYSTERIOUS KEY ===
        data.addNode("item_mysterious_key", {
            "speaker": "Narrator",
            "text": "An ornate key with strange symbols etched into its surface. It feels slightly warm to the touch.",
            "choices": [
                {"text": "Examine it closely", "target": "item_mysterious_key_examine"},
                {"text": "Put it away", "target": "item_generic_keep"}
            ]
        })
        
        data.addNode("item_mysterious_key_examine", {
            "speaker": "Narrator",
            "text": "The symbols seem to shift when you look at them directly. This key must unlock something important... but what?",
        })
        
        // === OLD MAP ===
        data.addNode("item_old_map", {
            "speaker": "Narrator",
            "text": "A weathered map showing the surrounding region. Some locations are marked with mysterious symbols.",
            "choices": [
                {"text": "Study the map", "target": "item_old_map_study"},
                {"text": "Fold it up", "target": "item_generic_keep"}
            ]
        })
        
        data.addNode("item_old_map_study", {
            "speaker": "Narrator",
            "text": "You notice several locations marked: an abandoned monastery to the east, ancient ruins to the north, and a cave system to the west. Each marked location has a symbol you don't recognize.",
            "commands": [
                {"type": "setInt", "parameters": {"flag": "map_studied", "value": "1"}},
                {"type": "setInt", "parameters": {"flag": "monastery_revealed", "value": "1"}},
                {"type": "setInt", "parameters": {"flag": "ruins_revealed", "value": "1"}},
                {"type": "setInt", "parameters": {"flag": "caves_revealed", "value": "1"}}
            ]
        })
        
        // === ANCIENT BOOK ===
        data.addNode("item_ancient_book", {
            "speaker": "Narrator",
            "text": "The ancient book Greg was searching for. Its dark leather cover is adorned with golden runes that glow faintly. The pages are yellowed but well-preserved.",
            "choices": [
                {"text": "Read it", "target": "item_ancient_book_read"},
                {"text": "Take it to Greg", "target": "item_ancient_book_keep"},
                {"text": "Close it", "target": "item_generic_keep"}
            ]
        })
        
        data.addNode("item_ancient_book_read", {
            "speaker": "Narrator",
            "text": "You flip through the pages. The text is in an ancient language, but some passages are translated. You see mentions of powerful spells, magical artifacts, and cryptic prophecies. This would take days to fully understand.",
            "commands": [
                {"type": "setInt", "parameters": {"flag": "book_partially_read", "value": "1"}}
            ],
            "choices": [
                {"text": "Keep reading", "target": "item_ancient_book_deep_read"},
                {"text": "That's enough for now", "target": "item_generic_keep"}
            ]
        })
        
        data.addNode("item_ancient_book_deep_read", {
            "speaker": "Narrator",
            "text": "You spend some time deciphering the text. You learn about an ancient order of mages who sought to preserve knowledge. This book was part of their great library before it was lost.",
            "commands": [
                {"type": "setInt", "parameters": {"flag": "book_knowledge_gained", "value": "1"}}
            ]
        })
        
        data.addNode("item_ancient_book_keep", {
            "speaker": "Narrator",
            "text": "You should find Greg. He's been looking for this book.",
        })
        
        // === RARE GEM ===
        data.addNode("item_rare_gem", {
            "speaker": "Narrator",
            "text": "A beautiful gem that sparkles with inner light. Greg said this is worth far more than gold to the right buyer.",
            "choices": [
                {"text": "Admire it", "target": "item_rare_gem_admire"},
                {"text": "Put it away carefully", "target": "item_generic_keep"}
            ]
        })
        
        data.addNode("item_rare_gem_admire", {
            "speaker": "Narrator",
            "text": "The gem seems to pulse with a faint magical energy. You've never seen anything quite like it. It must be incredibly valuable!",
        })
        
        // === GENERIC RESPONSES ===
        data.addNode("item_generic_keep", {
            "speaker": "Narrator",
            "text": "You carefully put the item away for later.",
        })
        
        data.addNode("item_generic_consume", {
            "speaker": "Narrator",
            "text": "You consume the item.",
        })
        
        data.addNode("item_generic_use", {
            "speaker": "Narrator",
            "text": "You use the item.",
        })
        
        return data
    }
    
    static getEntryPoint(itemType) {
        // Map item types to dialogue entry points
        if (itemType == "healing_potion") return "item_healing_potion"
        if (itemType == "mysterious_key") return "item_mysterious_key"
        if (itemType == "old_map") return "item_old_map"
        if (itemType == "ancient_book") return "item_ancient_book"
        if (itemType == "rare_gem") return "item_rare_gem"
        
        // Default generic interaction
        return "item_generic_examine"
    }
}
