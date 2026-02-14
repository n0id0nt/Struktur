class ScholarDialogue {
    static getData() {
        return [
            { 
                "node": "scholar", "data": {
                    "targets": [
                        {
                            "conditions": [
                                {
                                    "type": "hasIntFlag", "parameters": [
                                        {
                                            "type": "flag",
                                            "value": "scholar_quest_state"
                                        },
                                        {
                                            "type": "op",
                                            "value": "=="
                                        },
                                        {
                                            "type": "value",
                                            "value": 3
                                        },
                                    ]
                                },
                            ],
                            "node": "scholar_complete",
                        },
                        {
                            "conditions": [
                                {"type": "hasIntFlag", "parameters": [
                                        {
                                            "type": "flag",
                                            "value": "scholar_quest_state"
                                        },
                                        {
                                            "type": "op",
                                            "value": "=="
                                        },
                                        {
                                            "type": "value",
                                            "value": 1
                                        },
                                    ]
                                },
                                {"type": "hasItem", "parameters": [
                                        {
                                            "type": "item",
                                            "value": "ancient_book"
                                        },
                                    ]
                                },
                            ],
                            "node": "scholar_has_book_1",
                        },
                        {
                            "conditions": [
                                {"type": "hasIntFlag", "parameters": [
                                        {
                                            "type": "flag",
                                            "value": "scholar_quest_state"
                                        },
                                        {
                                            "type": "op",
                                            "value": "=="
                                        },
                                        {
                                            "type": "value",
                                            "value": 1
                                        },
                                    ]
                                },
                            ],
                            "node": "scholar_needs_book",
                        },
                        {
                            "node": "scholar_first_1",
                        },
                    ]
                },
            },
            { 
                "node": "scholar_complete", "data": {
                    "speaker": "Scholar",
                    "text": "Ah, my precious Ancient Tome! Years of research finally returned to me.\nThis knowledge belongs in my library, where it can illuminate minds for eternity.",
                },
            },
            { 
                "node": "scholar_has_book_1", "data": {
                    "speaker": "Scholar",
                    "text": "Perfect! My lost research tome! This contains decades of study into ancient mysteries.",
                    "next": "scholar_has_book_2"
                },
            },
            { 
                "node": "scholar_has_book_2", "data": {
                    "speaker": "Scholar",
                    "text": "In exchange for returning my life's work, I offer you this rare gem.\nMay it serve you well in your travels.",
                    "commands": [
                        {"type": "giveItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "rare_gem"
                                },
                            ]
                        },
                        {"type": "removeItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "ancient_book"
                                },
                            ]
                        },
                        {"type": "sethasIntFlag", "parameters": [
                                {
                                    "type": "flag",
                                    "value": "scholar_quest_state"
                                },
                                {
                                    "type": "value",
                                    "value": 3
                                },
                            ]
                        }
                    ],
                    "next": "scholar_has_book_3"
                },
            },
            { 
                "node": "scholar_has_book_3", "data": {
                    "speaker": "Scholar",
                    "text": "Obtained Rare Gem"
                },
            },
            { 
                "node": "scholar_needs_book", "data": {
                    "speaker": "Scholar",
                    "text": "My Ancient Tome is still missing - it contains all my research into forgotten lore.",
                },
            },
            { 
                "node": "scholar_first_1", "data": {
                    "speaker": "Scholar",
                    "text": "Welcome, fellow seeker of knowledge.\nI'm a collector of rare books and ancient texts.",
                    "next": "scholar_first_2"
                },
            },
            { 
                "node": "scholar_first_2", "data": {
                    "speaker": "Scholar",
                    "text": "Welcome, fellow seeker of knowledge.\nI'm a collector of rare books and ancient texts.",
                    "choices": [
                        {"text": "Tell me about local legends", "target": "scholar_legends_1"},
                        {"text": "You look agitated, what is the issue?", "target": "scholar_quest"},
                        {"text": "I'll browse", "target": "scholar_goodbye"},
                    ],
                },
            },
            { 
                "node": "scholar_legends_1", "data": {
                    "speaker": "Scholar",
                    "text": "There are stories of an old monastery that burned down long ago. They say monks collected rare texts there.",
                    "next": "scholar_legends_2",
                },
            },
            { 
                "node": "scholar_legends_2", "data": {
                    "speaker": "Scholar",
                    "text": "There are stories of an old monastery that burned down long ago. They say monks collected rare texts there.",
                },
            },
            { 
                "node": "scholar_quest", "data": {
                    "speaker": "Scholar",
                    "text": "I had an Ancient Tome - a very special book - but I seem to have misplaced it.\nIf you could find it, I would gladly trade you something valuable.",
                    "commands": [
                        {"type": "setIntFlag", "parameters": [
                                {
                                    "type": "flag",
                                    "value": "scholar_quest_state"
                                },
                                {
                                    "type": "value",
                                    "value": 1
                                },
                            ]
                        }
                    ]
                },
            },
            { 
                "node": "scholar_goodbye", "data": {
                    "speaker": "Scholar",
                    "text": "Happy reading!",
                },
            },
        ]
    }
}
