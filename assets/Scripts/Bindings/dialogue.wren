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
    // Clear the current dialogue interaction
    foreign static clearDialogue()
}

// Container for dialogue Result data
foreign class DialogueResult {
    // Get the status of the dialogue result
    foreign status
    // Get of the node id
    foreign nodeId
    // Get the speaker of the node
    foreign speaker
    // Get the text of the node
    foreign text
    // Get the text choices
    foreign choices
    // Get the text has ended
    foreign hasEnded
    // Get the text should auto advance
    foreign shouldAutoAdvance
}

// Container for dialogue Result data
foreign class ConditionalTarget {
    // Get the conditions of the Conditional Target
    foreign conditions
    // Get the target node of the Conditional Target
    foreign targetNode
}

// Container for dialogue Node data
foreign class DialogueNode {
    // Get the node id
    foreign id
    // Get the speaker of the node
    foreign speaker
    // Get the text of the node
    foreign text
    // Get the commands of the node
    foreign commands
    // Get the choices of the node
    foreign choices
    // Get the next node
    foreign next
    // Get the conditional targets of the node
    foreign targets
    // Check if node has choices
    foreign hasChoices()
    // Check if node has next target
    foreign hasNext()
    // Check if node has conditional targets
    foreign hasTargets()
}

class DialogueRegistry {
    // Register a condition type with callback
    foreign static registerCondition(arg0,arg1)
    // Register a command type with callback
    foreign static registerCommand(arg0,arg1)
}

