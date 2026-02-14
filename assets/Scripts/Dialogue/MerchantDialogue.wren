class MerchantDialogue {
    static getData() {
        return [
            { 
                "node": "merchant", "data": {
                    "targets": [
                        {
                            "conditions": [
                                {"type": "hasIntFlag", "parameters": [
                                        {
                                            "type": "flag",
                                            "value": "merchant_quest_state"
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
                            "node": "merchant_complete",
                        },
                        {
                            "conditions": [
                                {"type": "hasIntFlag", "parameters": [
                                        {
                                            "type": "flag",
                                            "value": "merchant_quest_state"
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
                                            "value": "Gold Coins"
                                        },
                                    ]
                                },
                            ],
                            "node": "merchant_has_gold_1",
                        },
                        {
                            "conditions": [
                                {"type": "hasIntFlag", "parameters": [
                                        {
                                            "type": "flag",
                                            "value": "merchant_quest_state"
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
                            "node": "merchant_needs_gold",
                        },
                        {
                            "node": "merchant_first_1",
                        },
                    ]
                },
            },
            { 
                "node": "merchant_complete", "data": {
                    "speaker": "Merchant",
                    "text": "Ah, my precious gold! These coins represent years of careful investment and trade.\nThis Crystal Key is the most valuable thing I possess - a fair exchange for my life's wealth.",
                },
            },
            { 
                "node": "merchant_has_gold_1", "data": {
                    "speaker": "Merchant",
                    "text": "My gold coins! The foundation of all wealth and prosperity in this house's golden days.",
                    "next": "merchant_has_gold_2"
                },
            },
            { 
                "node": "merchant_has_gold_2", "data": {
                    "speaker": "Merchant",
                    "text": "For returning my fortune, I'll give you this Crystal Key - the most valuable treasure in my collection.",
                    "commands": [
                        {"type": "giveItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Blue Crystal Key"
                                },
                            ]
                        },
                        {"type": "removeItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Gold Coins"
                                },
                            ]
                        },
                        {"type": "setIntFlag", "parameters": [
                                {
                                    "type": "flag",
                                    "value": "merchant_quest_state"
                                },
                                {
                                    "type": "value",
                                    "value": 3
                                },
                            ]
                        },
                    ],
                    "next": "merchant_has_gold_3"
                },
            },
            { 
                "node": "merchant_has_gold_3", "data": {
                    "speaker": "Merchant",
                    "text": "Obtained Blue Crystal Key"
                },
            },
            { 
                "node": "merchant_needs_gold", "data": {
                    "speaker": "Merchant",
                    "text": "My treasury feels empty without my gold coins - the foundation of all true wealth.",
                },
            },
            { 
                "node": "merchant_first_1", "data": {
                    "speaker": "Merchant",
                    "text": "Welcome to my treasury!\nI am Augustus Goldstein, and I've spent decades learning what truly has value in this cursed place.",
                    "next": "merchant_first_2"
                },
            },
            { 
                "node": "merchant_first_2", "data": {
                    "speaker": "Merchant",
                    "text": "I've learned that some treasures can't be bought or sold, but gold still has its place in the world.\nBring me my coins, and I'll trade you something more valuable than money.",
                    "commands": [
                        {"type": "setIntFlag", "parameters": [
                                {
                                    "type": "flag",
                                    "value": "merchant_quest_state"
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
