class ScholarDialogue {
    static getData() {
        return [
            { 
                "node": "scholar", "data": {
                    "targets": [
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
                                            "value": "Ancient Tome"
                                        },
                                    ]
                                },
                            ],
                            "node": "scholar_has_tome_1",
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
                            "node": "scholar_needs_tome",
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
                "node": "scholar_has_tome_1", "data": {
                    "speaker": "Scholar",
                    "text": "Perfect! My lost research tome! This contains decades of study into the house's mysteries.",
                    "next": "scholar_has_tome_2"
                },
            },
            { 
                "node": "scholar_has_tome_2", "data": {
                    "speaker": "Scholar",
                    "text": "In exchange for returning my life's work, I offer you this Crystal Key of Knowledge.\nMay it unlock the secrets you seek.",
                    "commands": [
                        {"type": "giveItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Red Crystal Key"
                                },
                            ]
                        },
                        {"type": "removeItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Ancient Tome"
                                },
                            ]
                        },
                        {"type": "setIntFlag", "parameters": [
                                {
                                    "type": "flag",
                                    "value": "scholar_quest_state"
                                },
                                {
                                    "type": "value",
                                    "value": 3
                                },
                            ]
                        },
                    ],
                    "next": "scholar_has_tome_3"
                },
            },
            { 
                "node": "scholar_has_tome_3", "data": {
                    "speaker": "Scholar",
                    "text": "Obtained Red Crystal Key"
                },
            },
            { 
                "node": "scholar_needs_tome", "data": {
                    "speaker": "Scholar",
                    "text": "My Ancient Tome is still missing - it contains all my research into this house's curse.",
                },
            },
            { 
                "node": "scholar_first_1", "data": {
                    "speaker": "Scholar",
                    "text": "Welcome to my library, fellow seeker of knowledge.\nI've been trapped here since 1943, studying the curse that binds us all.",
                    "next": "scholar_first_2"
                },
            },
            { 
                "node": "scholar_first_2", "data": {
                    "speaker": "Scholar",
                    "text": "I had nearly solved the mystery when my research was scattered.\nIf you could find my Ancient Tome, I would gladly share a Crystal Key with you.",
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
                        },
                    ],
                },
            },
        ]
    }
}
