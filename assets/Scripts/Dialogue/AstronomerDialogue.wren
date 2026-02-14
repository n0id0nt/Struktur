class AstronomerDialogue {
    static getData() {
        return [
            { 
                "node": "astronomer", "data": {
                    "targets": [
                        {
                            "conditions": [
                                {"type": "hasIntFlag", "parameters": [
                                        {
                                            "type": "flag",
                                            "value": "astronomer_quest_state"
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
                            "node": "astronomer_complete",
                        },
                        {
                            "conditions": [
                                {"type": "hasIntFlag", "parameters": [
                                        {
                                            "type": "flag",
                                            "value": "astronomer_quest_state"
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
                                            "value": "Telescope"
                                        },
                                    ]
                                },
                            ],
                            "node": "astronomer_has_telescope_1",
                        },
                        {
                            "conditions": [
                                {"type": "hasIntFlag", "parameters": [
                                        {
                                            "type": "flag",
                                            "value": "astronomer_quest_state"
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
                            "node": "astronomer_needs_telescope",
                        },
                        {
                            "node": "astronomer_first_1",
                        },
                    ]
                },
            },
            { 
                "node": "astronomer_complete", "data": {
                    "speaker": "Astronomer",
                    "text": "My precious telescope! Now I can properly observe the celestial patterns that govern this house.\nThis diary contains earthly dreams, but the stars hold greater truths.",
                },
            },
            { 
                "node": "astronomer_has_telescope_1", "data": {
                    "speaker": "Astronomer",
                    "text": "Magnificent! My telescope for studying the stars and their influence on temporal anomalies!",
                    "next": "astronomer_has_telescope_2"
                },
            },
            { 
                "node": "astronomer_has_telescope_2", "data": {
                    "speaker": "Astronomer",
                    "text": "I've been analyzing the Dreamer's diary for cosmic patterns, but I need my telescope more.\nA fair exchange - earthly dreams for celestial observations.",
                    "commands": [
                        {"type": "giveItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Personal Diary"
                                },
                            ]
                        },
                        {"type": "removeItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Telescope"
                                },
                            ]
                        },
                        {"type": "setIntFlag", "parameters": [
                                {
                                    "type": "flag",
                                    "value": "astronomer_quest_state"
                                },
                                {
                                    "type": "value",
                                    "value": 3
                                },
                            ]
                        },
                    ],
                    "next": "astronomer_has_telescope_3"
                },
            },
            { 
                "node": "astronomer_has_telescope_3", "data": {
                    "speaker": "Astronomer",
                    "text": "Obtained Personal Diary"
                },
            },
            { 
                "node": "astronomer_needs_telescope", "data": {
                    "speaker": "Astronomer",
                    "text": "I've been calculating the house's temporal distortions, but I need my telescope for proper astronomical observations.",
                },
            },
            { 
                "node": "astronomer_first_1", "data": {
                    "speaker": "Astronomer",
                    "text": "Greetings, earthbound traveler!\nI am Professor Orion Starweaver, astronomer and calculator of celestial influences.",
                    "next": "astronomer_first_2"
                },
            },
            { 
                "node": "astronomer_first_2", "data": {
                    "speaker": "Astronomer",
                    "text": "I've been studying how the stars affect this house's temporal anomalies.\nBring me my telescope, and I'll trade you this diary I've been analyzing for cosmic patterns.",
                    "commands": [
                        {"type": "setIntFlag", "parameters": [
                                {
                                    "type": "flag",
                                    "value": "astronomer_quest_state"
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
