class CordeliaDialogue {
    static getData() {
        return [
            { 
                "node": "cordelia", "data": {
                    "targets": [
                        {
                            "conditions": [
                                {"type": "hasItem", "parameters": [
                                        {
                                            "type": "item",
                                            "value": "Red Pedestal Active"
                                        },
                                    ]
                                },
                                {"type": "hasItem", "parameters": [
                                        {
                                            "type": "item",
                                            "value": "Green Pedestal Active"
                                        },
                                    ]
                                },
                                {"type": "hasItem", "parameters": [
                                        {
                                            "type": "item",
                                            "value": "Yellow Pedestal Active"
                                        },
                                    ]
                                },
                                {"type": "hasItem", "parameters": [
                                        {
                                            "type": "item",
                                            "value": "Blue Pedestal Active"
                                        },
                                    ]
                                },
                            ],
                            "node": "cordelia_complete",
                        },
                        {
                            "node": "cordelia_has_all_keys_1",
                        },
                    ]
                },
            },
            { 
                "node": "cordelia_complete", "data": {
                    "speaker": "Cordelia",
                    "text": "The curse is finally broken, dear Elena. All the trapped souls can rest now.\nThank you for succeeding where I failed.\nThe exit portal awaits - step through and be free.",
                },
            },
            { 
                "node": "cordelia_has_all_keys_1", "data": {
                    "speaker": "Cordelia",
                    "text": "Elena, my dear great-niece. You've done what I spent my whole life trying to achieve.\nYou have all four Crystal Keys! Now you must activate each pedestal in turn to break Theodore's curse forever.",
                    "next": "cordelia_has_all_keys_2"
                },
            },
            { 
                "node": "cordelia_has_all_keys_2", "data": {
                    "speaker": "Cordelia",
                    "text": "Place each key in its corresponding pedestal.\nThe red pedestal calls to knowledge, green to creation, yellow to dreams, and blue to protection.\nOnly when all four are activated will the portal to freedom open.",
                },
            },
        ]
    }
}
