// GregDialogue.wren
// Greg the Collector - Book Quest

foreign class DialogueData {
    construct new() {}
    
    foreign addNode(nodeId, nodeData)
}

class GregDialogue {
    static getData() {
        var data = DialogueData.new()
        
        // === FIRST MEETING ===
        data.addNode("greg_first_meeting", {
            "speaker": "Greg",
            "text": "Ah, hello there! I've been searching everywhere for a very special book. It's an ancient tome about forgotten magic. If you happen to find it, would you bring it to me? I'd make it worth your while, I promise!",
            "conditions": [
                {"type": "intFlag", "parameters": {"flag": "greg_quest_state", "op": "==", "value": "0"}}
            ],
            "commands": [
                {"type": "setInt", "parameters": {"flag": "greg_quest_state", "value": "1"}}
            ],
            "choices": [
                {"text": "I'll keep an eye out for it", "target": "greg_accept_quest"},
                {"text": "What's so special about this book?", "target": "greg_book_info"},
                {"text": "Maybe later, I'm busy right now", "target": "greg_decline_initial"}
            ]
        })
        
        data.addNode("greg_book_info", {
            "speaker": "Greg",
            "text": "It contains knowledge that's been lost for centuries. Very valuable to a collector like myself. The book has a dark leather cover with golden runes - you'll know it when you see it.",
            "choices": [
                {"text": "Sounds interesting, I'll look for it", "target": "greg_accept_quest"},
                {"text": "Why do you want it so badly?", "target": "greg_motivation"},
                {"text": "I don't have time for this", "target": "greg_decline_initial"}
            ]
        })
        
        data.addNode("greg_motivation", {
            "speaker": "Greg",
            "text": "I'm a collector, you see. Knowledge shouldn't be lost to time. This book could teach us so much about the old ways of magic. Plus, it would complete my collection!",
            "choices": [
                {"text": "Alright, I'll help you find it", "target": "greg_accept_quest"},
                {"text": "Not interested, sorry", "target": "greg_decline_initial"}
            ]
        })
        
        data.addNode("greg_accept_quest", {
            "speaker": "Greg",
            "text": "Wonderful! Thank you so much! Check old ruins and libraries - that's where ancient texts usually end up. Good luck!",
            "commands": [
                {"type": "setInt", "parameters": {"flag": "greg_quest_accepted", "value": "1"}}
            ]
        })
        
        data.addNode("greg_decline_initial", {
            "speaker": "Greg",
            "text": "Oh... well, if you change your mind, I'll be here. The offer stands!",
            "commands": [
                {"type": "setInt", "parameters": {"flag": "greg_declined_once", "value": "1"}}
            ]
        })
        
        // === QUEST IN PROGRESS - NO BOOK YET ===
        data.addNode("greg_still_needs_book", {
            "speaker": "Greg",
            "text": "Ah, have you found that ancient book yet? I'm still very eager to get my hands on it. Please let me know if you find it!",
            "conditions": [
                {"type": "intFlag", "parameters": {"flag": "greg_quest_state", "op": "==", "value": "1"}},
                {"type": "lacksItem", "parameters": {"item": "ancient_book"}}
            ],
            "choices": [
                {"text": "I'm still looking", "target": "greg_still_looking"},
                {"text": "Can you describe it again?", "target": "greg_book_description"},
                {"text": "Where should I search?", "target": "greg_search_hints"}
            ]
        })
        
        data.addNode("greg_still_looking", {
            "speaker": "Greg",
            "text": "I appreciate your help. These things take time, I understand. Safe travels!"
        })
        
        data.addNode("greg_book_description", {
            "speaker": "Greg",
            "text": "It has a dark leather cover with golden runes etched into it. The runes glow faintly in darkness. The pages are yellowed with age. You really can't miss it!",
            "next": "greg_still_needs_book"
        })
        
        data.addNode("greg_search_hints", {
            "speaker": "Greg",
            "text": "Try the old library in the eastern district, or perhaps the abandoned monastery outside town. Ancient books tend to end up in forgotten places.",
            "choices": [
                {"text": "Thanks for the tips", "target": "greg_still_looking"},
                {"text": "Tell me more about the monastery", "target": "greg_monastery_info"}
            ]
        })
        
        data.addNode("greg_monastery_info", {
            "speaker": "Greg",
            "text": "The monks there used to study ancient texts before they... well, before they disappeared. No one knows what happened to them. Be careful if you go there!",
            "next": "greg_still_looking"
        })
        
        // === PLAYER HAS THE BOOK ===
        data.addNode("greg_has_book", {
            "speaker": "Greg",
            "text": "Wait... is that... do you have the ancient book?! That's it! That's exactly what I've been looking for! *His eyes light up with excitement*",
            "conditions": [
                {"type": "intFlag", "parameters": {"flag": "greg_quest_state", "op": "==", "value": "1"}},
                {"type": "hasItem", "parameters": {"item": "ancient_book"}}
            ],
            "choices": [
                {"text": "Yes, here you go [Give book]", "target": "greg_give_book"},
                {"text": "How much is it worth to you?", "target": "greg_negotiate"},
                {"text": "Not yet, I'm still looking through it", "target": "greg_keep_book"}
            ]
        })
        
        data.addNode("greg_negotiate", {
            "speaker": "Greg",
            "text": "Ah, a negotiator! I respect that. I was going to offer you this rare gem - it's worth far more than gold to the right buyer. But if you'd prefer coin instead, I can offer 500 gold pieces. What do you say?",
            "choices": [
                {"text": "I'll take the rare gem", "target": "greg_give_book_gem"},
                {"text": "I'll take the gold", "target": "greg_give_book_gold"},
                {"text": "Let me think about it", "target": "greg_keep_book"}
            ]
        })
        
        data.addNode("greg_give_book_gem", {
            "speaker": "Narrator",
            "text": "You hand the ancient book to Greg.",
            "commands": [
                {"type": "removeItem", "parameters": {"item": "ancient_book"}},
                {"type": "setInt", "parameters": {"flag": "greg_quest_state", "value": "2"}}
            ],
            "next": "greg_receive_book_gem"
        })
        
        data.addNode("greg_give_book_gold", {
            "speaker": "Narrator",
            "text": "You hand the ancient book to Greg.",
            "commands": [
                {"type": "removeItem", "parameters": {"item": "ancient_book"}},
                {"type": "setInt", "parameters": {"flag": "greg_quest_state", "value": "2"}},
                {"type": "setInt", "parameters": {"flag": "greg_took_gold", "value": "1"}}
            ],
            "next": "greg_receive_book_gold"
        })
        
        data.addNode("greg_give_book", {
            "speaker": "Narrator",
            "text": "You hand the ancient book to Greg without negotiating.",
            "commands": [
                {"type": "removeItem", "parameters": {"item": "ancient_book"}},
                {"type": "setInt", "parameters": {"flag": "greg_quest_state", "value": "2"}}
            ],
            "next": "greg_receive_book_gem"
        })
        
        data.addNode("greg_receive_book_gem", {
            "speaker": "Greg",
            "text": "Finally! You have no idea how long I've been searching for this! Here, take this rare gem as payment. It's worth far more than gold!",
            "commands": [
                {"type": "giveItem", "parameters": {"item": "rare_gem"}},
                {"type": "setInt", "parameters": {"flag": "greg_quest_state", "value": "3"}}
            ],
            "choices": [
                {"text": "You're welcome, happy to help", "target": "greg_thanks_response"},
                {"text": "What will you do with the book?", "target": "greg_book_plans"}
            ]
        })
        
        data.addNode("greg_receive_book_gold", {
            "speaker": "Greg",
            "text": "Finally! You have no idea how long I've been searching for this! Here's your gold - 500 pieces as promised!",
            "commands": [
                {"type": "addInt", "parameters": {"flag": "player_gold", "value": "500"}},
                {"type": "setInt", "parameters": {"flag": "greg_quest_state", "value": "3"}}
            ],
            "choices": [
                {"text": "Pleasure doing business", "target": "greg_thanks_response"},
                {"text": "What will you do with the book?", "target": "greg_book_plans"}
            ]
        })
        
        data.addNode("greg_book_plans", {
            "speaker": "Greg",
            "text": "I'll study it, of course! And preserve it properly. Knowledge like this deserves to be protected and understood, not locked away gathering dust.",
            "next": "greg_thanks_response"
        })
        
        data.addNode("greg_keep_book", {
            "speaker": "Greg",
            "text": "Oh... well, please don't keep me waiting too long! The book belongs in proper hands.",
        })
        
        data.addNode("greg_thanks_response", {
            "speaker": "Greg",
            "text": "You're a true friend. If you ever need anything, just ask! I owe you one.",
        })
        
        // === QUEST COMPLETE ===
        data.addNode("greg_quest_finished", {
            "speaker": "Greg",
            "text": "Ah, my friend! Thanks again for finding that book for me. It has a place of honor in my library now. I've learned so much from it already!",
            "conditions": [
                {"type": "intFlag", "parameters": {"flag": "greg_quest_state", "op": "==", "value": "3"}}
            ],
            "choices": [
                {"text": "Glad I could help", "target": "greg_glad_response"},
                {"text": "Any other books you need?", "target": "greg_other_books"},
                {"text": "What have you learned?", "target": "greg_learned_info"}
            ]
        })
        
        data.addNode("greg_glad_response", {
            "speaker": "Greg",
            "text": "Anytime you need something, just ask! I'm always happy to help a friend.",
        })
        
        data.addNode("greg_other_books", {
            "speaker": "Greg",
            "text": "Not at the moment, but I'll let you know if I hear about any interesting finds! You've proven yourself quite the treasure hunter.",
        })
        
        data.addNode("greg_learned_info", {
            "speaker": "Greg",
            "text": "Fascinating things! Ancient spells, historical accounts of magical events, even some prophecies. Though I'm not sure I believe all of it - some of it sounds rather far-fetched!",
            "choices": [
                {"text": "Tell me about the prophecies", "target": "greg_prophecies"},
                {"text": "Maybe another time", "target": "greg_glad_response"}
            ]
        })
        
        data.addNode("greg_prophecies", {
            "speaker": "Greg",
            "text": "Well, there's one about a great darkness returning, a chosen hero, the usual stuff. I'm a collector, not a fortune teller! But it makes for good reading.",
            "next": "greg_glad_response"
        })
        
        return data
    }
    
    static getEntryPoint() {
        // Determine which node to start at based on quest state
        var questState = FlagManager.getIntFlag("greg_quest_state")
        var hasBook = Inventory.hasItem("ancient_book")
        
        if (questState == 0) return "greg_first_meeting"
        if (questState == 1 && !hasBook) return "greg_still_needs_book"
        if (questState == 1 && hasBook) return "greg_has_book"
        if (questState >= 3) return "greg_quest_finished"
        
        return "greg_first_meeting"
    }
}
