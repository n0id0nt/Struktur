// AUTO-GENERATED FILE - DO NOT EDIT
// Generated from C++ bindings
// Module: dialogue

// Enum for the status codes for dialogue operations
class DialogueStatus {
    static SUCCESS { 0 }
    static NO_ACTIVE_NODE { 1 }
    static INVALID_CHOICE { 2 }
    static NODE_NOT_FOUND { 3 }
    static ERROR { 4 }
}

class DialogueManager {
    // Start dialogue at a specific node
    foreign static startDialogue(arg0)
    // Make a choice in dialogue
    foreign static makeChoice(arg0)
    // Continue to next node
    foreign static continueDialogue()
    // Check if dialogue is active
    foreign static isDialogueActive()
    // Get current node ID
    foreign static getCurrentNodeId()
    // Get total number of loaded nodes
    foreign static getNodeCount()
    // Clear all loaded dialogue nodes
    foreign static clearAllNodes()
    // Loads in and interprets a wren map as dialogue
    foreign static loadDialogueData(arg0)
    // End the current dialogue interaction
    foreign static endDialogue()
}

// Container for dialogue data
foreign class DialogueResult {
    // Get the status of the dialogue result
    foreign status
    // Get the status of the node id
    foreign nodeId
    // Get the status speaker of the node
    foreign speaker
    // Get the status text of the node
    foreign text
    // Get the text choices
    foreign choices
    // Get the text has ended
    foreign hasEnded
    // Get the text should auto advance
    foreign shouldAutoAdvance
}

class DialogueRegistry {
    // Register a condition type with callback
    foreign static registerCondition(arg0,arg1)
    // Register a command type with callback
    foreign static registerCommand(arg0,arg1)
    // Register an operator with callback
    foreign static registerOperator(arg0,arg1)
}

