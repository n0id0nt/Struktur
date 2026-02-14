class GardenerDialogue {
    static getData() {
        return [
            { 
                "node": "gardener", "data": {
                    "targets": [
                        {
                            "conditions": [
                                {"type": "hasIntFlag", "parameters": [
                                        {
                                            "type": "flag",
                                            "value": "gardener_quest_state"
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
                            "node": "gardener_complete",
                        },
                        {
                            "conditions": [
                                {"type": "hasIntFlag", "parameters": [
                                        {
                                            "type": "flag",
                                            "value": "gardener_quest_state"
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
                                            "value": "Rose"
                                        },
                                    ]
                                },
                            ],
                            "node": "gardener_has_rose_1",
                        },
                        {
                            "conditions": [
                                {"type": "hasIntFlag", "parameters": [
                                        {
                                            "type": "flag",
                                            "value": "gardener_quest_state"
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
                            "node": "gardener_needs_rose_1",
                        },
                        {
                            "node": "gardener_first_1",
                        },
                    ]
                },
            },
            { 
                "node": "gardener_complete", "data": {
                    "speaker": "Gardener",
                    "text": "Oh, my beautiful rose! It belongs here among the other flowers, where love once bloomed eternal.\nThis garden is complete again.",
                },
            },
            { 
                "node": "gardener_has_rose_1", "data": {
                    "speaker": "Gardener",
                    "text": "A lovely rose! Yes, I do have the Scholar's tome, but this garden feels incomplete without its most beautiful flower.",
                    "next": "gardener_has_rose_2"
                },
            },
            { 
                "node": "gardener_has_rose_2", "data": {
                    "speaker": "Gardener",
                    "text": "I'll gladly trade this dusty old book for something that belongs in a garden of eternal spring.",
                    "commands": [
                        {"type": "giveItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Ancient Tome"
                                },
                            ]
                        },
                        {"type": "removeItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Rose"
                                },
                            ]
                        },
                        {"type": "setIntFlag", "parameters": [
                                {
                                    "type": "flag",
                                    "value": "gardener_quest_state"
                                },
                                {
                                    "type": "value",
                                    "value": 3
                                },
                            ]
                        },
                    ],
                    "next": "gardener_has_rose_3"
                },
            },
            { 
                "node": "gardener_has_rose_3", "data": {
                    "speaker": "Gardener",
                    "text": "Obtained Ancient Tome"
                },
            },
            { 
                "node": "gardener_needs_rose_1", "data": {
                    "speaker": "Gardener",
                    "text": "Welcome to my garden where love once bloomed.\nI found the Scholar's tome among my flowers, but I won't trade it for just anything.",
                    "next": "gardener_needs_rose_2"
                },
            },
            { 
                "node": "gardener_needs_rose_2", "data": {
                    "speaker": "Gardener",
                    "text": "Bring me a perfect rose",
                },
            },
            { 
                "node": "gardener_first_1", "data": {
                    "speaker": "Gardener",
                    "text": "Welcome, dear visitor, to my sanctuary of eternal spring.\nI tend to memories as if they were flowers, nurturing what should grow and bloom.",
                    "next": "gardener_first_2"
                },
            },
            { 
                "node": "gardener_first_2", "data": {
                    "speaker": "Gardener",
                    "text": "Love requires patience and care, just like any garden.\nSome things belong together - a rose belongs among flowers, not gathering dust in workshops.",
                    "commands": [
                        {"type": "setIntFlag", "parameters": [
                                {
                                    "type": "flag",
                                    "value": "gardener_quest_state"
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
