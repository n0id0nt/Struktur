// Individual Quest Item Dialogue Files
// These are ready-to-use without needing template instantiation

// ===== ANCIENT TOME =====
class AncientTomeDialogue {
    static getData() {
        return [
            { 
                "node": "ancient_tome", "data": {
                    "targets": [
                        {
                            "node": "ancient_tome_pickup",
                        },
                    ]
                },
            },
            { 
                "node": "ancient_tome_pickup", "data": {
                    "speaker": "Narrator",
                    "text": "A leather-bound book filled with scholarly research and mystical symbols.\nThe pages seem to shift when you're not looking directly at them.",
                    "next": "ancient_tome_obtain"
                },
            },
            { 
                "node": "ancient_tome_obtain", "data": {
                    "speaker": "Narrator",
                    "text": "Obtained Ancient Tome",
                    "commands": [
                        {"type": "giveItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Ancient Tome"
                                },
                            ]
                        },
                        {"type": "deleteEntity", "parameters": []},
                    ],
                },
            },
        ]
    }
}

// ===== ROSE =====
class RoseDialogue {
    static getData() {
        return [
            { 
                "node": "rose", "data": {
                    "targets": [
                        {
                            "node": "rose_pickup",
                        },
                    ]
                },
            },
            { 
                "node": "rose_pickup", "data": {
                    "speaker": "Narrator",
                    "text": "A perfect red rose that never wilts.\nIt smells of eternal spring and promises kept.",
                    "next": "rose_obtain"
                },
            },
            { 
                "node": "rose_obtain", "data": {
                    "speaker": "Narrator",
                    "text": "Obtained Rose",
                    "commands": [
                        {"type": "giveItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Rose"
                                },
                            ]
                        },
                        {"type": "deleteEntity", "parameters": []},
                    ],
                },
            },
        ]
    }
}

// ===== FRESH BREAD =====
class FreshBreadDialogue {
    static getData() {
        return [
            { 
                "node": "fresh_bread", "data": {
                    "targets": [
                        {
                            "node": "fresh_bread_pickup",
                        },
                    ]
                },
            },
            { 
                "node": "fresh_bread_pickup", "data": {
                    "speaker": "Narrator",
                    "text": "Still-warm bread that smells of home and family gatherings.\nTaking a bite fills you with comfort and nostalgia.",
                    "next": "fresh_bread_obtain"
                },
            },
            { 
                "node": "fresh_bread_obtain", "data": {
                    "speaker": "Narrator",
                    "text": "Obtained Fresh Bread",
                    "commands": [
                        {"type": "giveItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Fresh Bread"
                                },
                            ]
                        },
                        {"type": "deleteEntity", "parameters": []},
                    ],
                },
            },
        ]
    }
}

// ===== TOOL BOX =====
class ToolBoxDialogue {
    static getData() {
        return [
            { 
                "node": "tool_box", "data": {
                    "targets": [
                        {
                            "node": "tool_box_pickup",
                        },
                    ]
                },
            },
            { 
                "node": "tool_box_pickup", "data": {
                    "speaker": "Narrator",
                    "text": "A weathered box filled with various tools and crafting implements.\nEach tool tells a story of problems solved and things built.",
                    "next": "tool_box_obtain"
                },
            },
            { 
                "node": "tool_box_obtain", "data": {
                    "speaker": "Narrator",
                    "text": "Obtained Tool Box",
                    "commands": [
                        {"type": "giveItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Tool Box"
                                },
                            ]
                        },
                        {"type": "deleteEntity", "parameters": []},
                    ],
                },
            },
        ]
    }
}

// ===== PERSONAL DIARY =====
class PersonalDiaryDialogue {
    static getData() {
        return [
            { 
                "node": "personal_diary", "data": {
                    "targets": [
                        {
                            "node": "personal_diary_pickup",
                        },
                    ]
                },
            },
            { 
                "node": "personal_diary_pickup", "data": {
                    "speaker": "Narrator",
                    "text": "A private journal filled with intimate thoughts and dreams.\nThe final entry speaks of hope for escape from endless sleep.",
                    "next": "personal_diary_obtain"
                },
            },
            { 
                "node": "personal_diary_obtain", "data": {
                    "speaker": "Narrator",
                    "text": "Obtained Personal Diary",
                    "commands": [
                        {"type": "giveItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Personal Diary"
                                },
                            ]
                        },
                        {"type": "deleteEntity", "parameters": []},
                    ],
                },
            },
        ]
    }
}

// ===== TELESCOPE =====
class TelescopeDialogue {
    static getData() {
        return [
            { 
                "node": "telescope", "data": {
                    "targets": [
                        {
                            "node": "telescope_pickup",
                        },
                    ]
                },
            },
            { 
                "node": "telescope_pickup", "data": {
                    "speaker": "Narrator",
                    "text": "A brass telescope for observing celestial phenomena.\nThrough its lens, you can see the cosmic patterns that influence earthly events.",
                    "next": "telescope_obtain"
                },
            },
            { 
                "node": "telescope_obtain", "data": {
                    "speaker": "Narrator",
                    "text": "Obtained Telescope",
                    "commands": [
                        {"type": "giveItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Telescope"
                                },
                            ]
                        },
                        {"type": "deleteEntity", "parameters": []},
                    ],
                },
            },
        ]
    }
}

// ===== GOLD COINS =====
class GoldCoinsDialogue {
    static getData() {
        return [
            { 
                "node": "gold_coins", "data": {
                    "targets": [
                        {
                            "node": "gold_coins_pickup",
                        },
                    ]
                },
            },
            { 
                "node": "gold_coins_pickup", "data": {
                    "speaker": "Narrator",
                    "text": "Tarnished gold coins from another era.\nThey feel heavy with the weight of dreams that money couldn't buy.",
                    "next": "gold_coins_obtain"
                },
            },
            { 
                "node": "gold_coins_obtain", "data": {
                    "speaker": "Narrator",
                    "text": "Obtained Gold Coins",
                    "commands": [
                        {"type": "giveItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Gold Coins"
                                },
                            ]
                        },
                        {"type": "deleteEntity", "parameters": []},
                    ],
                },
            },
        ]
    }
}

// ===== ANCIENT SEAL =====
class AncientSealDialogue {
    static getData() {
        return [
            { 
                "node": "ancient_seal", "data": {
                    "targets": [
                        {
                            "node": "ancient_seal_pickup",
                        },
                    ]
                },
            },
            { 
                "node": "ancient_seal_pickup", "data": {
                    "speaker": "Narrator",
                    "text": "A mystical seal carved with protective wards and binding runes.\nIt pulses with contained power - clearly a dangerous artifact that should be properly secured.",
                    "next": "ancient_seal_obtain"
                },
            },
            { 
                "node": "ancient_seal_obtain", "data": {
                    "speaker": "Narrator",
                    "text": "Obtained Ancient Seal",
                    "commands": [
                        {"type": "giveItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Ancient Seal"
                                },
                            ]
                        },
                        {"type": "deleteEntity", "parameters": []},
                    ],
                },
            },
        ]
    }
}
