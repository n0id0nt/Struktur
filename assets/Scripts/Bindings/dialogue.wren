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
    // Get the current node in of the dialogue
    foreign static currentNode
    // Set the active node in the dialogue
    foreign static setActiveNode(arg0)
}

// Container for dialogue Result data
foreign class Command {
    // Get the callback handle
    foreign callback
    // Get the conditions params
    foreign params
}

// Container for dialogue Result data
foreign class DialogueResult {
    // Node not found result
    foreign construct nodeNotFound(arg0)
    // End dialogue result
    foreign construct endDialogue(arg0)
    // Choices result
    foreign construct choices(arg0)
    // Advance result
    foreign construct advance(arg0)
    // Invalid choice result
    foreign construct invalidChoice()
    // No active node result
    foreign construct noActivaNode()
    // Error result
    foreign construct error()
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
foreign class Condition {
    // Get the callback handle
    foreign callback
    // Get the conditions params
    foreign params
}

// Container for dialogue Result data
foreign class ConditionalTarget {
    // Does the Target have conditions?
    foreign hasConditions()
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

