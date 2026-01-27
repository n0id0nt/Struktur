class GregDialogue {
    static getData() {
        return [
            { 
                "node": "greg", "data": {
                    "targets": [
                        {
                            "conditions": [
                                {"type": "intFlag", "parameters": {"flag": "greg_quest_state", "op": "==", "value": "3"}},
                            ],
                            "node": "greg_complete".
                        },
                        {
                            "conditions": [
                                {"type": "intFlag", "parameters": {"flag": "greg_quest_state", "op": "==", "value": "1"}},
                                {"type": "hasItem", "parameters": {"item": "ancient_book"}},
                            ],
                            "node": "greg_has_book_1".
                        },
                        {
                            "conditions": [
                                {"type": "intFlag", "parameters": {"flag": "greg_quest_state", "op": "==", "value": "1"}},
                            ],
                            "node": "greg_needs_book".
                        },
                        {
                            "node": "greg_first_1".
                        },
                    ]
                },
            },
            { 
                "node": "greg_complete", "data": {
                    "speaker": "Greg",
                    "text": "Ah, my precious Ancient Tome! Years of research finally returned to me.\nThis knowledge belongs in my library, where it can illuminate minds for eternity.",
                },
            },
            { 
                "node": "greg_has_book_1", "data": {
                    "speaker": "Greg",
                    "text": "Perfect! My lost research tome! This contains decades of study into ancient mysteries.",
                    "next": "greg_has_book_2"
                },
            },
            { 
                "node": "greg_has_book_2", "data": {
                    "speaker": "Greg",
                    "text": "In exchange for returning my life's work, I offer you this rare gem.\nMay it serve you well in your travels.",
                    "commands": [
                        {"type": "giveItem", "parameters": {"item": "rare_gem"}},
                        {"type": "removeItem", "parameters": {"item": "ancient_book"}},
                        {"type": "setInt", "parameters": {"flag": "greg_quest_state", "value": "3"}}
                    ],
                    "next": "greg_has_book_3"
                },
            },
            { 
                "node": "greg_has_book_3", "data": {
                    "speaker": "Greg",
                    "text": "Obtained Rare Gem"
                },
            },
            { 
                "node": "greg_needs_book", "data": {
                    "speaker": "Greg",
                    "text": "My Ancient Tome is still missing - it contains all my research into forgotten lore.",
                },
            },
            { 
                "node": "greg_first_1", "data": {
                    "speaker": "Greg",
                    "text": "Welcome, fellow seeker of knowledge.\nI'm a collector of rare books and ancient texts.",
                    "next": "greg_first_2"
                },
            },
            { 
                "node": "greg_first_2", "data": {
                    "speaker": "Greg",
                    "text": "Welcome, fellow seeker of knowledge.\nI'm a collector of rare books and ancient texts.",
                    "choices": [
                        {"text": "Tell me about local legends", "target": "greg_legends_1"},
                        {"text": "You look agitated, what is the issue?", "target": "greg_quest"},
                        {"text": "I'll browse", "target": "greg_goodbye"},
                    ],
                },
            },
            { 
                "node": "greg_legends_1", "data": {
                    "speaker": "Greg",
                    "text": "There are stories of an old monastery that burned down long ago. They say monks collected rare texts there.",
                    "next": "librarian_legends_2",
                },
            },
            { 
                "node": "greg_legends_2", "data": {
                    "speaker": "Greg",
                    "text": "There are stories of an old monastery that burned down long ago. They say monks collected rare texts there.",
                },
            },
            { 
                "node": "greg_quest", "data": {
                    "speaker": "Greg",
                    "text": "I had an Ancient Tome - a very special book - but I seem to have misplaced it.\nIf you could find it, I would gladly trade you something valuable.",
                    "commands": [
                        {"type": "setInt", "parameters": {"flag": "greg_quest_state", "value": "1"}}
                    ]
                },
            },
            { 
                "node": "greg_goodbye", "data": {
                    "speaker": "Greg",
                    "text": "Happy reading!",
                },
            },
        ]
    }
}
