class CookDialogue {
    static getData() {
        return [
            { 
                "node": "cook", "data": {
                    "targets": [
                        {
                            "conditions": [
                                {"type": "hasIntFlag", "parameters": [
                                        {
                                            "type": "flag",
                                            "value": "cook_quest_state"
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
                            "node": "cook_complete",
                        },
                        {
                            "conditions": [
                                {"type": "hasIntFlag", "parameters": [
                                        {
                                            "type": "flag",
                                            "value": "cook_quest_state"
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
                                            "value": "Fresh Bread"
                                        },
                                    ]
                                },
                            ],
                            "node": "cook_has_bread_1",
                        },
                        {
                            "conditions": [
                                {"type": "hasIntFlag", "parameters": [
                                        {
                                            "type": "flag",
                                            "value": "cook_quest_state"
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
                            "node": "cook_needs_bread",
                        },
                        {
                            "node": "cook_first_1",
                        },
                    ]
                },
            },
            { 
                "node": "cook_complete", "data": {
                    "speaker": "Cook",
                    "text": "My warm, fresh bread! The kitchen feels like home again with the scent of baking.\nThis Crystal Key has been keeping my recipes company - please, take it.",
                },
            },
            { 
                "node": "cook_has_bread_1", "data": {
                    "speaker": "Cook",
                    "text": "Oh, wonderful! Fresh bread, still warm from the oven!\nThe heart of any kitchen is the bread that feeds the family.",
                    "next": "cook_has_bread_2"
                },
            },
            { 
                "node": "cook_has_bread_2", "data": {
                    "speaker": "Cook",
                    "text": "For bringing warmth back to my kitchen, I'll give you this Crystal Key I've been safeguarding.",
                    "commands": [
                        {"type": "giveItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Green Crystal Key"
                                },
                            ]
                        },
                        {"type": "removeItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Fresh Bread"
                                },
                            ]
                        },
                        {"type": "setIntFlag", "parameters": [
                                {
                                    "type": "flag",
                                    "value": "cook_quest_state"
                                },
                                {
                                    "type": "value",
                                    "value": 3
                                },
                            ]
                        },
                    ],
                    "next": "cook_has_bread_3"
                },
            },
            { 
                "node": "cook_has_bread_3", "data": {
                    "speaker": "Cook",
                    "text": "Obtained Green Crystal Key"
                },
            },
            { 
                "node": "cook_needs_bread", "data": {
                    "speaker": "Cook",
                    "text": "My kitchen feels so empty without the warmth of fresh bread.",
                },
            },
            { 
                "node": "cook_first_1", "data": {
                    "speaker": "Cook",
                    "text": "Welcome to my kitchen, dear!\nI've been preparing the same Christmas feast for decades,\nwaiting for family to gather.",
                    "next": "cook_first_2"
                },
            },
            { 
                "node": "cook_first_2", "data": {
                    "speaker": "Cook",
                    "text": "A kitchen without fresh bread is like a hearth without fire.\nBring me the bread that belongs here, and I'll share something precious in return.",
                    "commands": [
                        {"type": "setIntFlag", "parameters": [
                                {
                                    "type": "flag",
                                    "value": "cook_quest_state"
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
