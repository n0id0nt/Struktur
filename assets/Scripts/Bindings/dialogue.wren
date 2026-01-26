// AUTO-GENERATED FILE - DO NOT EDIT
// Generated from C++ bindings
// Module: dialogue

// Container for dialogue data
foreign class DialogueData {
    // Create new DialogueData container
    foreign construct new()
    // Add a dialogue node
    foreign addNode(arg0,arg1)
}

class DialogueManager {
    // Load dialogue data into the manager
    foreign static loadDialogue(arg0)
    // Start dialogue at a specific node
    foreign static startDialogue(arg0)
    // Make a dialogue choice
    foreign static makeChoice(arg0)
    // Continue to next dialogue node
    foreign static continueDialogue()
    // End the current dialogue
    foreign static endDialogue()
    // Check if dialogue is currently active
    foreign static isActive()
    // Get current dialogue node ID
    foreign static getCurrentNodeId()
}

