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
import "dialogue" for DialogueManager

//import "Dialogue/DialogueLoader" for DialogueLoader
import "Dialogue/GregDialogue" for GregDialogue
//import "Dialogue/ItemInteractions" for ItemInteractions
//import "Dialogue/NPCInteractions" for NPCInteractions

var TEXT_SCROLL_SPEED = 0.02

class DialogueManagerHelper {
    static evaluateTargets(targets) {

    }
    
    static processNode(nodeId) {
        var node = DialogueManager.setActiveNode(nodeId)

        var commands = node.commands

        for (command in commands)
        {
            command.call()
        }

        var result = DialogueResult.Success(nodeId)
        result.speaker = node.speaker
        result.text = node.text

        if (node.hasTargets()) {
            var targetNode = evaluateTargets(node.targets)
            if (targetNode.hasValue()) {
                return ProcessNode(targetNode.value);
            } else {
                Debug.warning("No target conditions matched in node '%(nodeId)', ending dialogue")
                DialogueManager.setActiveNode(null)
                result.hasEnded = true
            }
        } else if (node.hasChoices()) {
            var choices = node.choices
        }
    }

    static startDialogue(startNodeId) {
        return DialogueManager.startDialogue(startNodeId)
    }

    static endDialogue() {
        return DialogueManager.endDialogue()
    }

    static continueDialogue() {
        return DialogueManager.continueDialogue()
    }

    static makeChoice(choiceIndex) {
        return DialogueManager.makeChoice(choiceIndex)
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
        _currentResult = DialogueManager.startDialogue(entryNodeId)
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
                if (Input.isKeyJustPressed(49 + i)) { // KEY_ONE = 49
                    makeChoice(stateManager, i)
                    return
                }
            }
        }
    }
    
    exit() {
        super.exit()
        
        System.print("Unloading InteractState...")

        DialogueManager.endDialogue()
        
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
        _currentResult = DialogueManager.continueDialogue()
        

        processDialogueResult(_currentResult)
    }

    makeChoice(stateManager, choiceIndex) {
        System.print("Making choice %(choiceIndex)")
        
        _currentResult = DialogueManager.makeChoice(choiceIndex)
        
        // Check if dialogue ended
        if (_currentResult.hasEnded || _currentResult.status != 0) {
            stateManager.clearCurrentState()
            return
        }

        processDialogueResult(_currentResult)
    }

    displayChoices(choices) {
        // Create choice labels
        var yOffset = -80
        var font = Font.load("assets/Fonts/medieval_sharp/MedievalSharp-Bold.ttf", 18)
        
        for (i in 0...choices.count) {
            var choice = choices[i]
            var choiceText = "%(i + 1). %(choice["text"])"
            
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
        if (interactableName == "Scholar") return "greg_has_book_1"
        //if (interactableName == "Gardener") return NPCInteractions.getEntryPoint("gardener")
        //if (interactableName == "Cook") return NPCInteractions.getEntryPoint("cook")
        //if (interactableName == "Merchant") return NPCInteractions.getEntryPoint("merchant")
        //if (interactableName == "Guard") return NPCInteractions.getEntryPoint("guard")
        //if (interactableName == "Librarian") return NPCInteractions.getEntryPoint("librarian")
        //
        //// Items
        //if (interactableName == "Ancient Tome") return ItemInteractions.getEntryPoint("ancient_book")
        //if (interactableName == "Rose") return ItemInteractions.getEntryPoint("rose")
        //if (interactableName == "Healing Potion") return ItemInteractions.getEntryPoint("healing_potion")
        //if (interactableName == "Mysterious Key") return ItemInteractions.getEntryPoint("mysterious_key")
        //if (interactableName == "Old Map") return ItemInteractions.getEntryPoint("old_map")
        
        // Default - no dialogue found
        Debug.Warning("Warning: No dialogue entry point found for %(interactableName)")
        return null
    }
}