class DreamerDialogue {
    static getData() {
        return [
            { 
                "node": "dreamer", "data": {
                    "targets": [
                        {
                            "conditions": [
                                {"type": "hasIntFlag", "parameters": [
                                        {
                                            "type": "flag",
                                            "value": "dreamer_quest_state"
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
                            "node": "dreamer_complete",
                        },
                        {
                            "conditions": [
                                {"type": "hasIntFlag", "parameters": [
                                        {
                                            "type": "flag",
                                            "value": "dreamer_quest_state"
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
                                            "value": "Personal Diary"
                                        },
                                    ]
                                },
                            ],
                            "node": "dreamer_has_diary_1",
                        },
                        {
                            "conditions": [
                                {"type": "hasIntFlag", "parameters": [
                                        {
                                            "type": "flag",
                                            "value": "dreamer_quest_state"
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
                            "node": "dreamer_needs_diary",
                        },
                        {
                            "node": "dreamer_first_1",
                        },
                    ]
                },
            },
            { 
                "node": "dreamer_complete", "data": {
                    "speaker": "Dreamer",
                    "text": "My diary... full of dreams and visions of freedom.\nThis Crystal Key has been glowing in my dreams - it belongs with someone who seeks truth.",
                },
            },
            { 
                "node": "dreamer_has_diary_1", "data": {
                    "speaker": "Dreamer",
                    "text": "My personal diary! I've been dreaming of its return...\nIt contains all my visions and prophetic dreams about this house.",
                    "next": "dreamer_has_diary_2"
                },
            },
            { 
                "node": "dreamer_has_diary_2", "data": {
                    "speaker": "Dreamer",
                    "text": "For reuniting me with my dreams and memories, I offer you this Crystal Key of Visions.",
                    "commands": [
                        {"type": "giveItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Yellow Crystal Key"
                                },
                            ]
                        },
                        {"type": "removeItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Personal Diary"
                                },
                            ]
                        },
                        {"type": "setIntFlag", "parameters": [
                                {
                                    "type": "flag",
                                    "value": "dreamer_quest_state"
                                },
                                {
                                    "type": "value",
                                    "value": 3
                                },
                            ]
                        },
                    ],
                    "next": "dreamer_has_diary_3"
                },
            },
            { 
                "node": "dreamer_has_diary_3", "data": {
                    "speaker": "Dreamer",
                    "text": "Obtained Yellow Crystal Key"
                },
            },
            { 
                "node": "dreamer_needs_diary", "data": {
                    "speaker": "Dreamer",
                    "text": "My diary holds the key to understanding the visions that haunt my sleep.",
                },
            },
            { 
                "node": "dreamer_first_1", "data": {
                    "speaker": "Dreamer",
                    "text": "Welcome to my sanctuary of dreams and visions.\nI've seen both futures and pasts in my slumber.",
                    "next": "dreamer_first_2"
                },
            },
            { 
                "node": "dreamer_first_2", "data": {
                    "speaker": "Dreamer",
                    "text": "My diary contains prophetic visions that might guide us all to freedom.\nFind it for me, and I'll share something precious from my dreams.",
                    "commands": [
                        {"type": "setIntFlag", "parameters": [
                                {
                                    "type": "flag",
                                    "value": "dreamer_quest_state"
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
