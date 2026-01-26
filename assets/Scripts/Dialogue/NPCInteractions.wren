// NPCInteractions.wren
// Dialogues for other NPCs in the game

import "dialogue" for DialogueData

class NPCInteractions {
    static getData() {
        var data = DialogueData.new()
        
        // ===== MERCHANT =====
        
        // Merchant greeting
        data.addNode("merchant_greeting", {
            "speaker": "Merchant",
            "text": "Welcome, welcome! Looking to buy or sell? I've got the finest wares in town!",
            "choices": [
                {"text": "What are you selling?", "target": "merchant_browse"},
                {"text": "Just looking around", "target": "merchant_goodbye"}
            ]
        })
        
        data.addNode("merchant_browse", {
            "speaker": "Merchant",
            "text": "I have healing potions for 50 gold, and maps of the region for 30 gold. What interests you?",
            "choices": [
                {"text": "Buy Healing Potion [50 gold]", "target": "merchant_buy_potion", "conditions": [
                    {"type": "intFlag", "parameters": {"flag": "player_gold", "op": ">=", "value": "50"}}
                ]},
                {"text": "Buy Map [30 gold]", "target": "merchant_buy_map", "conditions": [
                    {"type": "intFlag", "parameters": {"flag": "player_gold", "op": ">=", "value": "30"}}
                ]},
                {"text": "Maybe later", "target": "merchant_goodbye"}
            ]
        })
        
        data.addNode("merchant_buy_potion", {
            "speaker": "Merchant",
            "text": "Excellent choice! Here you go.",
            "commands": [
                {"type": "addInt", "parameters": {"flag": "player_gold", "value": "-50"}},
                {"type": "giveItem", "parameters": {"item": "healing_potion"}}
            ],
            "next": "merchant_buy_potion_confirm"
        })
        
        data.addNode("merchant_buy_potion_confirm", {
            "speaker": "Merchant",
            "text": "Obtained Healing Potion",
            "next": "merchant_browse"
        })
        
        data.addNode("merchant_buy_map", {
            "speaker": "Merchant",
            "text": "Wise investment! Never get lost again.",
            "commands": [
                {"type": "addInt", "parameters": {"flag": "player_gold", "value": "-30"}},
                {"type": "giveItem", "parameters": {"item": "old_map"}}
            ],
            "next": "merchant_buy_map_confirm"
        })
        
        data.addNode("merchant_buy_map_confirm", {
            "speaker": "Merchant",
            "text": "Obtained Old Map",
            "next": "merchant_browse"
        })
        
        data.addNode("merchant_goodbye", {
            "speaker": "Merchant",
            "text": "Come back anytime! Safe travels!"
        })
        
        // ===== GUARD =====
        
        data.addNode("guard_greeting", {
            "speaker": "Guard",
            "text": "Halt! State your business.",
            "choices": [
                {"text": "Just passing through", "target": "guard_passing"},
                {"text": "Any trouble in town?", "target": "guard_trouble"}
            ]
        })
        
        data.addNode("guard_passing", {
            "speaker": "Guard",
            "text": "Alright, move along then. Stay out of trouble."
        })
        
        data.addNode("guard_trouble_1", {
            "speaker": "Guard",
            "text": "Few reports of bandits on the roads. Oh, and someone said they saw strange lights at the old monastery east of here.",
            "next": "guard_trouble_2"
        })
        
        data.addNode("guard_trouble_2", {
            "speaker": "Guard",
            "text": "Strange things happen there. I wouldn't go alone if I were you.",
            "commands": [
                {"type": "setInt", "parameters": {"flag": "monastery_hint_received", "value": "1"}}
            ]
        })
        
        // ===== LIBRARIAN =====
        
        data.addNode("librarian_greeting_1", {
            "speaker": "Librarian",
            "text": "*Whispers* Shhh! This is a library. What can I help you with?",
            "next": "librarian_greeting_2"
        })
        
        data.addNode("librarian_greeting_2", {
            "speaker": "Librarian",
            "text": "We have books on history, magic, and local legends. Greg the collector comes here often - nice fellow, always looking for rare texts.",
            "choices": [
                {"text": "[Whisper] Tell me about local legends", "target": "librarian_legends"},
                {"text": "[Whisper] I'll browse", "target": "librarian_goodbye"}
            ]
        })
        
        data.addNode("librarian_legends_1", {
            "speaker": "Librarian",
            "text": "*Whispers* There are stories of an old monastery that burned down long ago. They say monks collected rare texts there.",
            "next": "librarian_legends_2"
        })
        
        data.addNode("librarian_legends_2", {
            "speaker": "Librarian",
            "text": "*Whispers* Some of their books survived though - scattered across the region. Greg would pay handsomely for any ancient tomes you find."
        })
        
        data.addNode("librarian_goodbye", {
            "speaker": "Librarian",
            "text": "*Whispers* Happy reading!"
        })
        
        return data
    }
    
    static getEntryPoint(npcName) {
        if (npcName == "merchant") return "merchant_greeting"
        if (npcName == "guard") return "guard_greeting"
        if (npcName == "librarian") return "librarian_greeting_1"
        
        return "npc_generic_greeting"
    }
}
