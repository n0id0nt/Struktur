class Test {
    static getData() {
        return [
            { 
                "node": "Choice 1", "data": {
                    "text": "New node text",
                    "next": "End"
                }
            },
            { 
                "node": "Choice 2", "data": {
                    "text": "New node text",
                    "next": "Choice 2. 2"
                }
            },
            { 
                "node": "Choice 2. 2", "data": {
                    "text": "New node text",
                    "next": "End"
                }
            },
            { 
                "node": "End", "data": {
                    "text": "New node text"
                }
            },
            { 
                "node": "Test 2", "data": {
                    "text": "New node text",
                    "choices": [
                        {"text": "Choice 1", "target": "Choice 1"},
                        {"text": "Choice 2", "target": "Choice 2"}
                    ]
                }
            },
            { 
                "node": "test", "data": {
                    "speaker": "Speaker",
                    "text": "Welcome to the dialogue!",
                    "next": "Test 2"
                }
            }
        ]
    }
}
