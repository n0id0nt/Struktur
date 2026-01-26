// ItemInteractions.wren
// Generic item interaction dialogues

import "dialogue" for DialogueData

class ItemInteractions {
    static getData() {
        var data = DialogueData.new()
        
        // ===== ANCIENT BOOK =====
        data.addNode("ancient_book_1", {
            "speaker": "Narrator",
            "text": "A leather-bound book filled with scholarly research and mystical symbols.\nThe pages seem to shift when you're not looking directly at them.",
            "next": "ancient_book_2"
        })
        
        data.addNode("ancient_book_2", {
            "speaker": "Narrator",
            "text": "Obtained Ancient Book",
            "commands": [
                {"type": "giveItem", "parameters": {"item": "ancient_book"}}
            ]
        })
        
        // ===== RARE GEM (already obtained, just examine) =====
        data.addNode("rare_gem_examine", {
            "speaker": "Narrator",
            "text": "A beautiful gem that sparkles with inner light. Greg said this is worth far more than gold to the right buyer.",
            "conditions": [
                {"type": "hasItem", "parameters": {"item": "rare_gem"}}
            ]
        })
        
        // ===== HEALING POTION =====
        data.addNode("healing_potion_1", {
            "speaker": "Narrator",
            "text": "A small vial filled with red liquid. It smells faintly of herbs and magic.",
            "choices": [
                {"text": "Drink it", "target": "healing_potion_drink"},
                {"text": "Keep it for later", "target": "healing_potion_keep"}
            ]
        })
        
        data.addNode("healing_potion_drink", {
            "speaker": "Narrator",
            "text": "You drink the healing potion. Warmth spreads through your body as your wounds begin to close.",
            "commands": [
                {"type": "removeItem", "parameters": {"item": "healing_potion"}},
                {"type": "addInt", "parameters": {"flag": "player_health", "value": "50"}}
            ]
        })
        
        data.addNode("healing_potion_keep", {
            "speaker": "Narrator",
            "text": "You carefully tuck the potion away for later."
        })
        
        // ===== MYSTERIOUS KEY =====
        data.addNode("mysterious_key_1", {
            "speaker": "Narrator",
            "text": "An ornate key with strange symbols etched into its surface. It feels slightly warm to the touch.",
            "next": "mysterious_key_2"
        })
        
        data.addNode("mysterious_key_2", {
            "speaker": "Narrator",
            "text": "Obtained Mysterious Key",
            "commands": [
                {"type": "giveItem", "parameters": {"item": "mysterious_key"}}
            ]
        })
        
        // ===== OLD MAP =====
        data.addNode("old_map_1", {
            "speaker": "Narrator",
            "text": "A weathered map showing the surrounding region. Some locations are marked with mysterious symbols.",
            "next": "old_map_2"
        })
        
        data.addNode("old_map_2", {
            "speaker": "Narrator",
            "text": "The map reveals several locations: an abandoned monastery to the east, ancient ruins to the north, and a cave system to the west.",
            "commands": [
                {"type": "giveItem", "parameters": {"item": "old_map"}},
                {"type": "setInt", "parameters": {"flag": "monastery_revealed", "value": "1"}},
                {"type": "setInt", "parameters": {"flag": "ruins_revealed", "value": "1"}},
                {"type": "setInt", "parameters": {"flag": "caves_revealed", "value": "1"}}
            ],
            "next": "old_map_3"
        })
        
        data.addNode("old_map_3", {
            "speaker": "Narrator",
            "text": "Obtained Old Map"
        })
        
        return data
    }
    
    static getEntryPoint(itemType) {
        if (itemType == "ancient_book") return "ancient_book_1"
        if (itemType == "rare_gem") return "rare_gem_examine"
        if (itemType == "healing_potion") return "healing_potion_1"
        if (itemType == "mysterious_key") return "mysterious_key_1"
        if (itemType == "old_map") return "old_map_1"
        
        // Default
        return "generic_examine"
    }
}