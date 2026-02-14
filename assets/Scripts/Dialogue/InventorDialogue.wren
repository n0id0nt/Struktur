class InventorDialogue {
    static getData() {
        return [
            { 
                "node": "inventor", "data": {
                    "targets": [
                        {
                            "conditions": [
                                {"type": "hasIntFlag", "parameters": [
                                        {
                                            "type": "flag",
                                            "value": "inventor_quest_state"
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
                            "node": "inventor_complete",
                        },
                        {
                            "conditions": [
                                {"type": "hasIntFlag", "parameters": [
                                        {
                                            "type": "flag",
                                            "value": "inventor_quest_state"
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
                                            "value": "Tool Box"
                                        },
                                    ]
                                },
                            ],
                            "node": "inventor_has_toolbox_1",
                        },
                        {
                            "conditions": [
                                {"type": "hasIntFlag", "parameters": [
                                        {
                                            "type": "flag",
                                            "value": "inventor_quest_state"
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
                            "node": "inventor_needs_toolbox",
                        },
                        {
                            "node": "inventor_first_1",
                        },
                    ]
                },
            },
            { 
                "node": "inventor_complete", "data": {
                    "speaker": "Inventor",
                    "text": "My tool box! Every craftsman needs his complete set of implements.\nThis bread was keeping me fed during long work sessions, but you've earned it.",
                },
            },
            { 
                "node": "inventor_has_toolbox_1", "data": {
                    "speaker": "Inventor",
                    "text": "Ah, my missing tool box!\nI've been making do with individual tools, but a craftsman needs his complete kit.",
                    "next": "inventor_has_toolbox_2"
                },
            },
            { 
                "node": "inventor_has_toolbox_2", "data": {
                    "speaker": "Inventor",
                    "text": "I've been saving this fresh bread for sustenance during long projects.\nA fair trade - tools for nourishment!",
                    "commands": [
                        {"type": "giveItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Fresh Bread"
                                },
                            ]
                        },
                        {"type": "removeItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Tool Box"
                                },
                            ]
                        },
                        {"type": "setIntFlag", "parameters": [
                                {
                                    "type": "flag",
                                    "value": "inventor_quest_state"
                                },
                                {
                                    "type": "value",
                                    "value": 3
                                },
                            ]
                        },
                    ],
                    "next": "inventor_has_toolbox_3"
                },
            },
            { 
                "node": "inventor_has_toolbox_3", "data": {
                    "speaker": "Inventor",
                    "text": "Obtained Fresh Bread"
                },
            },
            { 
                "node": "inventor_needs_toolbox", "data": {
                    "speaker": "Inventor",
                    "text": "My workshop isn't complete without my full set of tools.",
                },
            },
            { 
                "node": "inventor_first_1", "data": {
                    "speaker": "Inventor",
                    "text": "Welcome to my workshop! I've been trying to build our way out of this cursed house for decades.",
                    "next": "inventor_first_2"
                },
            },
            { 
                "node": "inventor_first_2", "data": {
                    "speaker": "Inventor",
                    "text": "Every problem has a practical solution if you have the right tools.\nBring me my complete tool box, and I'll trade you something nourishing in return.",
                    "commands": [
                        {"type": "setIntFlag", "parameters": [
                                {
                                    "type": "flag",
                                    "value": "inventor_quest_state"
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
