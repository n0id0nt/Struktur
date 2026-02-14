// states/InteractState.wren
// Interact state - handles NPC and item interactions using the dialogue system

import "States/BaseState" for BaseState
import "resourceManager" for Font, Texture, Music, Sound
import "ui" for UIManager, UILabel, UIPanel, TextWrapping
import "app" for Application
import "math" for Vec2, Vec3, Vec4
import "input" for Input
import "gameObjectComponents" for Camera, Script
import "gameObject" for GameObject
import "debug" for Debug

import "Colors" for BLANK, BLACK, DARKGRAY, WHITE
import "Inventory" for Inventory
import "dialogue" for DialogueManager, DialogueResult

var TEXT_SCROLL_SPEED = 0.02

class DialogueManagerHelper {
    static executeCommands(commands) {
        for (command in commands) {
            var params = command.params
            command.callback.call(params)
        }
    }

    static evaluateConditions(conditions) {
        for (condition in conditions) {
            var params = condition.params
            if (!condition.callback.call(params)) {
                return false
            }
        }
        return true
    } 

    static evaluateTargets(targets) {
        for (target in targets) {
            if (!target.hasConditions()) {
                return target.targetNode
            }

            if (target.conditions.count == 0) {
                return target.targetNode
            }

            if (evaluateConditions(target.conditions)) {
                return target.targetNode
            }
        }

        return null
    }
    
    static processNode(nodeId) {
        Debug.info("Processing dialogue at node: %(nodeId)")
        var node = DialogueManager.setActiveNode(nodeId)
        if (!node) {
            Debug.breakpointMsg("Node not found '%(nodeId)'")
            return DialogueResult.nodeNotFound(node)
        }
        var commands = node.commands
        if (commands) {
            executeCommands(commands)
        }
        if (node.hasTargets()) {
            var targetNodeId = evaluateTargets(node.targets)
            if (targetNodeId) {
                return processNode(targetNodeId)
            }
            Debug.warning("No target conditions matched in node '%(nodeId)', ending dialogue")
            DialogueManager.clearDialogue()
            return DialogueResult.endDialogue(node)
        }
        if (node.hasChoices()) {
            var choices = node.choices
            return DialogueResult.choices(node)
        }
        if (node.hasNext()) {
            return DialogueResult.advance(node)
        }
        return DialogueResult.endDialogue(node)
    }

    static startDialogue(startNodeId) {
        DialogueManager.clearDialogue()
        return processNode(startNodeId)
    }

    static endDialogue() {
        return DialogueManager.clearDialogue()
    }

    static continueDialogue() {
        var currentNode = DialogueManager.currentNode
        if (!currentNode) {
            Debug.warning("Continue called but no active dialogue node")
            return DialogueResult.noActiveNode()
        }
        if (!currentNode.hasNext()) {
            Debug.warning("Continue called but current node has no 'next'")
            return DialogueResult.noActiveNode()
        }
        var nextNodeId = currentNode.next
        Debug.info("Continuing to node: %(nextNodeId)")
        return processNode(nextNodeId)
    }

    static makeChoice(choiceIndex) {
        var currentNode = DialogueManager.currentNode
        if (!currentNode) {
            Debug.warning("Continue called but no active dialogue node")
            return DialogueResult.noActiveNode()
        }
        var choices = currentNode.choices
        if (!choices) {
            Debug.error("Choice index %(choiceIndex) for node %(currentNode.id) when there is no choices associated with node")
            return DialogueResult.invalidChoice()
        }
        if (choiceIndex < 0 || choiceIndex >= choices.count) {
            Debug.error("Invalid choice index %(choiceIndex) (available: %(choices.count)) for node %(currentNode.id)")
            return DialogueResult.invalidChoice()
        }
        var targetNodeId = choices[choiceIndex].targetNodeId
        Debug.info("Player chose option %(choiceIndex), jumping to node: %(targetNodeId)")
        return processNode(targetNodeId)
    }
}

class InteractState is BaseState {
    construct new() {
        super()
        name = "InteractState"
        _screenPanel = null
        _dialogueLabel = null
        _continueDialogueLabel = null
        _choiceLabels = []

        _interactingEntity = null
        
        _dialogueSrolling = false
        _currentString = ""
        _currentDialogueStartTime = 0
        _currentResult = null
        _waitingForChoice = false

        _menuMusic = null
        _textScrollSound = null
    }
    
    enter(stateManager, params) {
        super.enter(stateManager, params)

        _interactingEntity = params["interactingEntity"]
        
        _menuMusic = Music.load("assets/Sounds/menuMusic.wav")
        _menuMusic.setLooping(true)
        _menuMusic.play()
        _textScrollSound = Sound.load("assets/Sounds/scroll.wav")
        var font = Font.load("assets/Fonts/medieval_sharp/MedievalSharp-Bold.ttf", 30)
        var dialogueBackgroundPanelTexture = Texture.load("assets/Tiles/DialoguePanel.png")

        // Create UI panel
        _screenPanel = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(Application.gameWidth, Application.gameHeight), Vec2.new(0, 0))
        _screenPanel.setBackgroundColor(BLANK)
        _screenPanel.setBorderColor(BLANK)
        UIManager.addUIElement(_screenPanel)

        var textBackgroundPanel = UIPanel.new(Vec2.new(0, 0), Vec2.new(0.5, 0.95), Vec2.new(800, 200), Vec2.new(0, 0))
        textBackgroundPanel.setAnchorPoint(Vec2.new(0.5, 1))
        textBackgroundPanel.setBorderColor(BLANK)
        textBackgroundPanel.setBackgroundColor(DARKGRAY)
        textBackgroundPanel.setBackgroundTexture(dialogueBackgroundPanelTexture)
        dialogueBackgroundPanelTexture.unload()
        _screenPanel.addChild(textBackgroundPanel)

        _dialogueLabel = UILabel.new(Vec2.new(40, 30), Vec2.new(0, 0), "", 20.0)
        _dialogueLabel.setTextColor(BLACK)
        _dialogueLabel.setAnchorPoint(Vec2.new(0, 0))
        _dialogueLabel.setFont(font)
        _dialogueLabel.setWordWrap(TextWrapping.WORD_WRAP)
        _dialogueLabel.setSize(Vec2.new(-40, -80), Vec2.new(1, 1))
        textBackgroundPanel.addChild(_dialogueLabel)

        _continueDialogueLabel = UILabel.new(Vec2.new(-40, -30), Vec2.new(1, 1), "Continue", 20.0)
        _continueDialogueLabel.setTextColor(BLACK)
        _continueDialogueLabel.setAnchorPoint(Vec2.new(1, 1))
        _continueDialogueLabel.setFont(font)
        _continueDialogueLabel.setVisible(false)
        textBackgroundPanel.addChild(_continueDialogueLabel)

        // Get the interactable entity and determine entry point
        var interactable = Script.getInstance(_interactingEntity)
        var entryNodeId = getEntryPoint(interactable.name)
        
        if (entryNodeId == null) {
            System.print("No dialogue entry point for %(interactable.name)")
            stateManager.clearCurrentState()
            return
        }

        System.print("Starting dialogue for %(interactable.name) at node %(entryNodeId)")
        
        // Start the dialogue
        _currentResult = DialogueManagerHelper.startDialogue(entryNodeId)
        processDialogueResult(_currentResult)
    }
    
    update(stateManager) {
        var inputInteract = Input.isInputJustReleased("Interact")

        // Handle text scrolling
        if (_dialogueSrolling) {
            var numberOfCharactersToDraw = ((Application.gameTime - _currentDialogueStartTime) / TEXT_SCROLL_SPEED).floor
            if (numberOfCharactersToDraw >= _currentString.count) {
                numberOfCharactersToDraw = _currentString.count
                _dialogueSrolling = false
                _continueDialogueLabel.setVisible(true)
            }
            var subString = _currentString[0...numberOfCharactersToDraw]
            _dialogueLabel.setText(subString)
        }

        // Handle input
        if (inputInteract) {
            if (_dialogueSrolling) {
                _dialogueSrolling = false
                _continueDialogueLabel.setVisible(true)
                _dialogueLabel.setText(_currentString)
            } else if (_waitingForChoice) {
                // TODO: Handle choice selection via keyboard/gamepad
                // For now, just continue if there's only one choice or no choices
                if (!_currentResult.choices && _currentResult.choices.count == 0) {
                    continueDialogue(stateManager)
                }
            } else {
                continueDialogue(stateManager)
            }
        }

        // Handle number key input for choices
        if (_waitingForChoice && _currentResult.choices && _currentResult.choices.count > 0) {
            for (i in 0..._currentResult.choices.count) {
                // Check for number keys 1-9
                if (Input.isKeyJustReleased((i + 1).toString)) {
                    makeChoice(stateManager, i)
                    return
                }
            }
        }
    }
    
    exit() {
        super.exit()
        
        System.print("Unloading InteractState...")

        DialogueManagerHelper.endDialogue()
        
        UIManager.removeUIElement(_screenPanel)
        _menuMusic.stop()
        _menuMusic.unload()
        _menuMusic = null
        _textScrollSound.unload()
        _textScrollSound = null

        System.print("InteractState unloaded")
    }

    processDialogueResult(result) {
        // Get the text and speaker
        var speaker = result.speaker
        var text = result.text
        
        // Format with speaker name if present
        if (text) {
            if (speaker && speaker != "") {
                _currentString = "%(speaker):\n%(text)"
            } else {
                _currentString = text
            }
        }


        // Start text scrolling animation
        _dialogueSrolling = true
        _currentDialogueStartTime = Application.gameTime
        _continueDialogueLabel.setVisible(false)
        _waitingForChoice = false

        // Clear old choice labels
        clearChoiceLabels()

        // Check if there are choices
        var choices = result.choices
        if (choices && choices.count > 0) {
            _waitingForChoice = true
            displayChoices(choices)
        }
    }

    continueDialogue(stateManager) {
        // If there are choices, we shouldn't continue automatically
        if (_waitingForChoice && _currentResult.choices && _currentResult.choices.count > 0) {
            return
        }

        // Check if dialogue ended
        if (_currentResult.hasEnded) {
            stateManager.clearCurrentState()
            return
        }

        // Continue to next node
        _currentResult = DialogueManagerHelper.continueDialogue()
        

        processDialogueResult(_currentResult)
    }

    makeChoice(stateManager, choiceIndex) {
        System.print("Making choice %(choiceIndex)")
        
        _currentResult = DialogueManagerHelper.makeChoice(choiceIndex)
        processDialogueResult(_currentResult)
    }

    displayChoices(choices) {
        // Create choice labels
        var yOffset = -80
        var font = Font.load("assets/Fonts/medieval_sharp/MedievalSharp-Bold.ttf", 18)
        
        for (i in 0...choices.count) {
            var choice = choices[i]
            var choiceText = "%(i + 1). %(choice)"
            
            var choiceLabel = UILabel.new(Vec2.new(40, yOffset), Vec2.new(0, 1), choiceText, 18.0)
            choiceLabel.setTextColor(BLACK)
            choiceLabel.setAnchorPoint(Vec2.new(0, 1))
            choiceLabel.setFont(font)
            _screenPanel.addChild(choiceLabel)
            _choiceLabels.add(choiceLabel)
            
            yOffset = yOffset - 25
        }
    }

    clearChoiceLabels() {
        for (label in _choiceLabels) {
            _screenPanel.removeChild(label)
        }
        _choiceLabels.clear()
    }

    getEntryPoint(interactableName) {
        // Determine which dialogue entry point to use based on the interactable
        // This maps your old interaction names to the new dialogue system
        
        // NPCs
        if (interactableName == "Scholar") return "scholar"
        if (interactableName == "Gardener") return "gardener"
        if (interactableName == "Cook") return "cook"
        if (interactableName == "Merchant") return "merchant"
        if (interactableName == "Guard") return "guard"
        if (interactableName == "Librarian") return "librarian"
        if (interactableName == "Astronomer") return "astronomer"
        if (interactableName == "Cordelia") return "cordelia"
        if (interactableName == "Dreamer") return "dreamer"
        if (interactableName == "Guardian") return "guardian"
        if (interactableName == "Inventor") return "inventor"
        
        // Items
        if (interactableName == "Ancient Tome") return "ancient_book"
        if (interactableName == "Rose") return "rose"
        if (interactableName == "Healing Potion") return "healing_potion"
        if (interactableName == "Mysterious Key") return "mysterious_key"
        if (interactableName == "Old Map") return "old_map"
        
        // Default - no dialogue found
        Debug.Warning("Warning: No dialogue entry point found for %(interactableName)")
        return null
    }
}