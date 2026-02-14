// Transformation Items - Items that can be picked up and returned
// These items trigger camera shake and room transformations

class ItemDialogue {
    static getData() {
        return [
            { 
                "node": "love_letter", "data": {
                    "targets": [
                        {
                            "node": "love_letter_pickup",
                        },
                    ]
                },
            },
            { 
                "node": "love_letter_pickup", "data": {
                    "speaker": "Narrator",
                    "text": "A romantic letter filled with promises of eternal love.\nCarrying it awakens thoughts of gardens and blooming romance.",
                    "next": "love_letter_transform"
                },
            },
            { 
                "node": "love_letter_transform", "data": {
                    "speaker": "Narrator",
                    "text": "The North Room stirs with romantic longing.",
                    "commands": [
                        {"type": "cameraShake", "parameters": []},
                    ],
                    "next": "love_letter_obtain"
                },
            },
            { 
                "node": "love_letter_obtain", "data": {
                    "speaker": "Narrator",
                    "text": "Obtained Love Letter",
                    "commands": [
                        {"type": "giveItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Love Letter"
                                },
                            ]
                        },
                        {"type": "deleteEntity", "parameters": []},
                    ],
                },
            },
            { 
                "node": "love_letter_return", "data": {
                    "targets": [
                        {
                            "conditions": [
                                {"type": "hasItem", "parameters": [
                                        {
                                            "type": "item",
                                            "value": "Love Letter"
                                        },
                                    ]
                                },
                            ],
                            "node": "love_letter_return_place",
                        },
                    ]
                },
            },
            { 
                "node": "love_letter_return_place", "data": {
                    "speaker": "Narrator",
                    "text": "You place the letter back on the nightstand where dreams of love belong.",
                    "next": "love_letter_return_transform"
                },
            },
            { 
                "node": "love_letter_return_transform", "data": {
                    "speaker": "Narrator",
                    "text": "The North Room's romantic yearning settles back to scholarly calm.",
                    "commands": [
                        {"type": "cameraShake", "parameters": []},
                    ],
                    "next": "love_letter_return_putback"
                },
            },
            { 
                "node": "love_letter_return_putback", "data": {
                    "speaker": "Narrator",
                    "text": "Put Back Love Letter",
                    "commands": [
                        {"type": "removeItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Love Letter"
                                },
                            ]
                        },
                        {"type": "deleteEntity", "parameters": []},
                    ],
                },
            },
            { 
                "node": "hammer", "data": {
                    "targets": [
                        {
                            "node": "hammer_pickup",
                        },
                    ]
                },
            },
            { 
                "node": "hammer_pickup", "data": {
                    "speaker": "Narrator",
                    "text": "A well-used hammer that speaks of creation and honest work.\nCarrying it awakens the urge to build and transform.",
                    "next": "hammer_transform"
                },
            },
            { 
                "node": "hammer_transform", "data": {
                    "speaker": "Narrator",
                    "text": "The East Room resonates with creative potential.",
                    "commands": [
                        {"type": "cameraShake", "parameters": []},
                    ],
                    "next": "hammer_obtain"
                },
            },
            { 
                "node": "hammer_obtain", "data": {
                    "speaker": "Narrator",
                    "text": "Obtained Hammer",
                    "commands": [
                        {"type": "giveItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Hammer"
                                },
                            ]
                        },
                        {"type": "deleteEntity", "parameters": []},
                    ],
                },
            },
            { 
                "node": "hammer_return", "data": {
                    "targets": [
                        {
                            "conditions": [
                                {"type": "hasItem", "parameters": [
                                        {
                                            "type": "item",
                                            "value": "Hammer"
                                        },
                                    ]
                                },
                            ],
                            "node": "hammer_return_place",
                        },
                    ]
                },
            },
            { 
                "node": "hammer_return_place", "data": {
                    "speaker": "Narrator",
                    "text": "You return the hammer to its place among the valuables.",
                    "next": "hammer_return_transform"
                },
            },
            { 
                "node": "hammer_return_transform", "data": {
                    "speaker": "Narrator",
                    "text": "The East Room's creative energy settles back to domestic comfort.",
                    "commands": [
                        {"type": "cameraShake", "parameters": []},
                    ],
                    "next": "hammer_return_putback"
                },
            },
            { 
                "node": "hammer_return_putback", "data": {
                    "speaker": "Narrator",
                    "text": "Put Back Hammer",
                    "commands": [
                        {"type": "removeItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Hammer"
                                },
                            ]
                        },
                        {"type": "deleteEntity", "parameters": []},
                    ],
                },
            },
            { 
                "node": "star_chart", "data": {
                    "targets": [
                        {
                            "node": "star_chart_pickup",
                        },
                    ]
                },
            },
            { 
                "node": "star_chart_pickup", "data": {
                    "speaker": "Narrator",
                    "text": "Ancient astronomical maps covered in mystical calculations.\nCarrying it awakens thoughts of celestial observations and cosmic mysteries.",
                    "next": "star_chart_transform"
                },
            },
            { 
                "node": "star_chart_transform", "data": {
                    "speaker": "Narrator",
                    "text": "The South Room reaches toward the stars and cosmic understanding.",
                    "commands": [
                        {"type": "cameraShake", "parameters": []},
                    ],
                    "next": "star_chart_obtain"
                },
            },
            { 
                "node": "star_chart_obtain", "data": {
                    "speaker": "Narrator",
                    "text": "Obtained Star Chart",
                    "commands": [
                        {"type": "giveItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Star Chart"
                                },
                            ]
                        },
                        {"type": "deleteEntity", "parameters": []},
                    ],
                },
            },
            { 
                "node": "star_chart_return", "data": {
                    "targets": [
                        {
                            "conditions": [
                                {"type": "hasItem", "parameters": [
                                        {
                                            "type": "item",
                                            "value": "Star Chart"
                                        },
                                    ]
                                },
                            ],
                            "node": "star_chart_return_place",
                        },
                    ]
                },
            },
            { 
                "node": "star_chart_return_place", "data": {
                    "speaker": "Narrator",
                    "text": "You return the star chart to its secure place among the other valuable documents.",
                    "next": "star_chart_return_transform"
                },
            },
            { 
                "node": "star_chart_return_transform", "data": {
                    "speaker": "Narrator",
                    "text": "The South Room's cosmic aspirations settle back to earthly rest.",
                    "commands": [
                        {"type": "cameraShake", "parameters": []},
                    ],
                    "next": "star_chart_return_putback"
                },
            },
            { 
                "node": "star_chart_return_putback", "data": {
                    "speaker": "Narrator",
                    "text": "Put Back Star Chart",
                    "commands": [
                        {"type": "removeItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Star Chart"
                                },
                            ]
                        },
                        {"type": "deleteEntity", "parameters": []},
                    ],
                },
            },
            { 
                "node": "ornate_key", "data": {
                    "targets": [
                        {
                            "node": "ornate_key_pickup",
                        },
                    ]
                },
            },
            { 
                "node": "ornate_key_pickup", "data": {
                    "speaker": "Narrator",
                    "text": "An ornate key that promises to unlock hidden secrets.\nCarrying it awakens thoughts of security and protected treasures.",
                    "next": "ornate_key_transform"
                },
            },
            { 
                "node": "ornate_key_transform", "data": {
                    "speaker": "Narrator",
                    "text": "The West Room shifts toward impenetrable security.",
                    "commands": [
                        {"type": "cameraShake", "parameters": []},
                    ],
                    "next": "ornate_key_obtain"
                },
            },
            { 
                "node": "ornate_key_obtain", "data": {
                    "speaker": "Narrator",
                    "text": "Obtained Key",
                    "commands": [
                        {"type": "giveItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Ornate Key"
                                },
                            ]
                        },
                        {"type": "deleteEntity", "parameters": []},
                    ],
                },
            },
            { 
                "node": "ornate_key_return", "data": {
                    "targets": [
                        {
                            "conditions": [
                                {"type": "hasItem", "parameters": [
                                        {
                                            "type": "item",
                                            "value": "Ornate Key"
                                        },
                                    ]
                                },
                            ],
                            "node": "ornate_key_return_place",
                        },
                    ]
                },
            },
            { 
                "node": "ornate_key_return_place", "data": {
                    "speaker": "Narrator",
                    "text": "You return the key to its place among the tools and inventions.",
                    "next": "ornate_key_return_transform"
                },
            },
            { 
                "node": "ornate_key_return_transform", "data": {
                    "speaker": "Narrator",
                    "text": "The West Room's protective aura settles back to simple commerce.",
                    "commands": [
                        {"type": "cameraShake", "parameters": []},
                    ],
                    "next": "ornate_key_return_putback"
                },
            },
            { 
                "node": "ornate_key_return_putback", "data": {
                    "speaker": "Narrator",
                    "text": "Put Back Key",
                    "commands": [
                        {"type": "removeItem", "parameters": [
                                {
                                    "type": "item",
                                    "value": "Ornate Key"
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
