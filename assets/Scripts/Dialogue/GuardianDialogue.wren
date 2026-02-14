class GuardianDialogue {
    static getData() {
        return [
            { 
                "node": "guardian", "data": {
                    "targets": [
                        {
                            "conditions": [
                                {"type": "hasIntFlag", "parameters": [
                                        {
                                            "type": "flag",
                                            "value": "guardian_quest_state"
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
                            "node": "guardian_complete",
                        },
                        {
                            "conditions": [
                                {"type": "hasIntFlag", "parameters": [
                                        {
                                            "type": "flag",
                                            "value": "guardian_quest_state"
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
                                            "value": "Ancient Seal"
                                        },
                                    ]
                                },
                            ],
                            "node": "guardian_has_seal_1",
                        },
                        {
                            "conditions": [
                                {"type": "hasIntFlag", "parameters": [
                                        {
                                            "type": "flag",
                                            "value": "guardian_quest_state"
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
                            "node": "guardian_needs_seal",
                        },
                        {
                            "node": "guardian_first_1",
                        },
                    ]
                },
            },
            { 
                "node": "guardian_complete", "data": {
                    "speaker": "Guardian",
                    "text": "The Ancient Seal! This mystical artifact belongs in my vault where its power can be properly contained.\nThese mundane coins are of no concern to me - take them for your mortal pursuits.",
                },
            },
            { 
                "node": "guardian_has_seal_1", "data": {
                    "speaker": "Guardian",
                    "text": "Excellent! The Ancient Seal of Binding - a powerful artifact that should never be left unprotected.",
                    "next": "guardian_has_seal_2"
                },
            },
            { 
                "node": "guardian_has_seal_2", "data": {
                    "speaker": "Guardian",
                    "text": "I'll gladly trade mere gold for this relic that could unleash untold dangers if misused.\nIt belongs here, warded and watched.",
                    "commands": [
                        {"type": "giveItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Gold Coins"
                                },
                            ]
                        },
                        {"type": "removeItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Ancient Seal"
                                },
                            ]
                        },
                        {"type": "setIntFlag", "parameters": [
                                {
                                    "type": "flag",
                                    "value": "guardian_quest_state"
                                },
                                {
                                    "type": "value",
                                    "value": 3
                                },
                            ]
                        },
                    ],
                    "next": "guardian_has_seal_3"
                },
            },
            { 
                "node": "guardian_has_seal_3", "data": {
                    "speaker": "Guardian",
                    "text": "Obtained Gold Coins"
                },
            },
            { 
                "node": "guardian_needs_seal", "data": {
                    "speaker": "Guardian",
                    "text": "I am the keeper of this house's most dangerous artifacts and forbidden knowledge.\nThere is an Ancient Seal of Binding that must be secured - I sense it lies unprotected.",
                },
            },
            { 
                "node": "guardian_first_1", "data": {
                    "speaker": "Guardian",
                    "text": "I am Master Cedric Voss, Guardian of this vault and protector of the house's most perilous secrets.",
                    "next": "guardian_first_2"
                },
            },
            { 
                "node": "guardian_first_2", "data": {
                    "speaker": "Guardian",
                    "text": "Some artifacts are too dangerous to leave unguarded. Bring me the Ancient Seal, and I'll give you gold that holds no mystical corruption.",
                    "commands": [
                        {"type": "setIntFlag", "parameters": [
                                {
                                    "type": "flag",
                                    "value": "guardian_quest_state"
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
