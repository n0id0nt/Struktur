// states/InteractState.wren
// Main gameplay state - handles player movement, interaction checks
// This is the default state when playing the game

import "States/BaseState" for BaseState
import "resourceManager" for Font, Texture, Sound
import "ui" for UIManager, UILabel, UIPanel, TextWrapping
import "app" for Application, Inventory
import "math" for Vec2, Vec3, Vec4
import "input" for Input
import "gameObjectComponents" for Camera, Script
import "gameObject" for GameObject

var WHITE = Vec4.new(255, 255, 255, 255)
var DARKGRAY = Vec4.new(80, 80, 80, 255)
var BLACK = Vec4.new(0, 0, 0, 255)
var BLANK = Vec4.new(0, 0, 0, 0)
var TEXT_SCROLL_SPEED = 0.02

class DialogueStep {
    construct new(dialogue, conditionContainsItemVector, conditionExcludeItemVector, itemAddVector, itemRemoveVector, applyCameraShake, deleteInteractingEntity, animateText, exitInteraction) {
        _dialogue = dialogue
        _conditionContainsItemVector = conditionContainsItemVector
        _conditionExcludeItemVector = conditionExcludeItemVector
        _itemAddVector = itemAddVector
        _itemRemoveVector = itemRemoveVector
        _applyCameraShake = applyCameraShake
        _deleteInteractingEntity = deleteInteractingEntity
        _animateText = animateText
        _exitInteraction = exitInteraction
    }

    dialogue { _dialogue }
    conditionContainsItemVector { _conditionContainsItemVector }
    conditionExcludeItemVector { _conditionExcludeItemVector }
    itemAddVector { _itemAddVector }
    itemRemoveVector { _itemRemoveVector }
    applyCameraShake { _applyCameraShake }
    deleteInteractingEntity { _deleteInteractingEntity }
    animateText { _animateText }
    exitInteraction { _exitInteraction }
}

class InteractState is BaseState {
    construct new() {
        super()
        _name = "InteractState"
        _screenPanel = null
        _dialogueLabel = null
        _continueDialogueLabel = null

        _interactingEntity = null

        _interaction = null
        
        _dialogueSrolling = false
        _breakAfterStep = false
        _interactionStep = 0
        _currentString = ""

        _currentDialogueStartTime = 0

        _menuMusic = null

        _interactionMap = { 
            // ===== NPC DIALOGUES =====
            "Scholar": [ // Source: Library
                // After receiving Ancient Tome and giving Red Crystal Key
                DialogueStep.new("Ah, my precious Ancient Tome! Years of research finally returned to me.\nThis knowledge belongs in my library, where it can illuminate minds for eternity.", ["Red Crystal Key"], [], [], [], false, false, true, true),
                // Has Ancient Tome to trade
                DialogueStep.new("Perfect! My lost research tome! This contains decades of study into the house's mysteries.", ["Scholar's Memory Note", "Ancient Tome"], ["Red Crystal Key"], [], [], false, false, true, false),
                DialogueStep.new("In exchange for returning my life's work, I offer you this Crystal Key of Knowledge.\nMay it unlock the secrets you seek.", ["Scholar's Memory Note", "Ancient Tome"], ["Red Crystal Key"], ["Red Crystal Key"], ["Ancient Tome"], false, false, true, false),
                DialogueStep.new("Obtained Red Crystal Key", ["Scholar's Memory Note", "Red Crystal Key"], [], [], [], false, false, false, true),
                // Subsequent meetings without Ancient Tome
                DialogueStep.new("My Ancient Tome is still missing - it contains all my research into this house's curse.", ["Scholar's Memory Note"], ["Ancient Tome"], [], [], false, false, true, true),
                // First Meeting (Default)
                DialogueStep.new("Welcome to my library, fellow seeker of knowledge.\nI've been trapped here since 1943, studying the curse that binds us all.", [], ["Scholar's Memory Note"], [], [], false, false, true, false),
                DialogueStep.new("I had nearly solved the mystery when my research was scattered.\nIf you could find my Ancient Tome, I would gladly share a Crystal Key with you.", [], ["Scholar's Memory Note"], ["Scholar's Memory Note"], [], false, false, true, true),
            ],
            "Gardener": [ // Source: Garden (when Love Letter held)
                // After receiving Ancient Tome and giving Red Crystal Key
                DialogueStep.new("Oh, my beautiful rose! It belongs here among the other flowers, where love once bloomed eternal.\nThis garden is complete again.", ["Ancient Tome"], [], [], [], false, false, true, true),
                // Has Rose to trade for Ancient Tome
                DialogueStep.new("A lovely rose! Yes, I do have the Scholar's tome, but this garden feels incomplete without its most beautiful flower.", ["Gardener's Memory Note", "Rose"], ["Ancient Tome"], [], [], false, false, true, false),
                DialogueStep.new("I'll gladly trade this dusty old book for something that belongs in a garden of eternal spring.", ["Gardener's Memory Note", "Rose"], ["Ancient Tome"], ["Ancient Tome"], ["Rose"], false, false, true, false),
                DialogueStep.new("Obtained Ancient Tome", ["Gardener's Memory Note", "Ancient Tome"], [], [], [], false, false, false, true),
                // Subsequent meetings without Rose
                DialogueStep.new("Welcome to my garden where love once bloomed.\nI found the Scholar's tome among my flowers, but I won't trade it for just anything.", ["Gardener's Memory Note"], ["Rose"], [], [], false, false, true, false),
                DialogueStep.new("Bring me a perfect rose", ["Gardener's Memory Note"], ["Rose"], [], [], false, false, true, true),
                // First Meeting (Default)
                DialogueStep.new("Welcome, dear visitor, to my sanctuary of eternal spring.\nI tend to memories as if they were flowers, nurturing what should grow and bloom.", [], ["Gardener's Memory Note"], [], [], false, false, true, false),
                DialogueStep.new("I had nearly solved the mystery when my research was scattered.\nIf you could find my Ancient Tome, I would gladly share a Crystal Key with you.", [], ["Gardener's Memory Note"], ["Gardener's Memory Note"], [], false, false, true, true),
            ],
            "Cook": [ // Source: Kitchen
                // After receiving Fresh Bread and giving Green Crystal Key
                DialogueStep.new("My warm, fresh bread! The kitchen feels like home again with the scent of baking.\nThis Crystal Key has been keeping my recipes company - please, take it.", ["Green Crystal Key"], [], [], [], false, false, true, true),
                // Has Rose to trade for Ancient Tome
                DialogueStep.new("A lovely rose! Yes, I do have the Scholar's tome, but this garden feels incomplete without its most beautiful flower.", ["Gardener's Memory Note", "Rose"], ["Ancient Tome"], [], [], false, false, true, false),
                DialogueStep.new("I'll gladly trade this dusty old book for something that belongs in a garden of eternal spring.", ["Gardener's Memory Note", "Rose"], ["Ancient Tome"], ["Ancient Tome"], ["Rose"], false, false, true, false),
                DialogueStep.new("Obtained Ancient Tome", ["Gardener's Memory Note", "Ancient Tome"], [], [], [], false, false, false, true),
                // Subsequent meetings without Rose
                DialogueStep.new("Welcome to my garden where love once bloomed.\nI found the Scholar's tome among my flowers, but I won't trade it for just anything.", ["Gardener's Memory Note"], ["Rose"], [], [], false, false, true, false),
                DialogueStep.new("Bring me a perfect rose", ["Gardener's Memory Note"], ["Rose"], [], [], false, false, true, true),
                // First Meeting (Default)
                DialogueStep.new("Welcome, dear visitor, to my sanctuary of eternal spring.\nI tend to memories as if they were flowers, nurturing what should grow and bloom.", [], ["Gardener's Memory Note"], [], [], false, false, true, false),
                DialogueStep.new("I had nearly solved the mystery when my research was scattered.\nIf you could find my Ancient Tome, I would gladly share a Crystal Key with you.", [], ["Gardener's Memory Note"], ["Gardener's Memory Note"], [], false, false, true, true),
            ],
        }
    }
    
    enter(stateManager, params) {
        super.enter(stateManager, params)

        _interactingEntity = params["interactingEntity"]
        
        _menuMusic = Sound.load("assets/Sounds/menuMusic.wav")
        _menuMusic.play()
        var font = Font.load("assets/Fonts/medieval_sharp/MedievalSharp-Bold.ttf", 30)
        var dialogueBackgroundPanelTexture = Texture.load("assets/Tiles/DialoguePanel.png")

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
        _dialogueLabel.setSize(Vec2.new(-40, -30), Vec2.new(1, 1))
        textBackgroundPanel.addChild(_dialogueLabel)

        _continueDialogueLabel = UILabel.new(Vec2.new(-40, -30), Vec2.new(0, 0), "Continue", 20.0)
        _continueDialogueLabel.setTextColor(BLACK)
        _continueDialogueLabel.setAnchorPoint(Vec2.new(1, 1))
        _continueDialogueLabel.setFont(font)
        _continueDialogueLabel.setVisible(false)
        textBackgroundPanel.addChild(_continueDialogueLabel)

        // Set State variables
        var interactable = Script.getInstance(_interactingEntity)
        
        _interaction = _interactionMap[interactable.name]
                
        var interactionStep = 0
        while (!trySetInteractionStep(interactionStep)) {
            interactionStep = interactionStep + 1
            if (interactionStep == _interaction.count) {
                System.print("No valid interaction steps for this interaction")
                stateManager.clearCurrentState()
                return
            }
        }
        System.print("Set interaction step to %(interactionStep) for interactable %(interactable.name)")
    }
    
    update(stateManager) {
        var inputInteract = Input.isInputJustReleased("Interact")

        if (inputInteract) {
            if (_dialogueSrolling) {
                _dialogueSrolling = false
                _continueDialogueLabel.setVisible(false)
            } else {
                if (_breakAfterStep) {
                    stateManager.clearCurrentState()
                    return // we are finished with the interaction.
                }
                var interactionStep = _interactionStep + 1
                while (!trySetInteractionStep(interactionStep)) {
                    interactionStep = interactionStep + 1
                    if (_interaction && interactionStep == _interaction.count) {
                        stateManager.clearCurrentState()
                        return
                    }
                }
                _continueDialogueLabel.setVisible(false)
            }
        }

        var numberOfCharactersToDraw = _dialogueSrolling ? ((Application.gameTime - _currentDialogueStartTime) / TEXT_SCROLL_SPEED).floor : _currentString.count
        if (numberOfCharactersToDraw >= _currentString.count) {
            numberOfCharactersToDraw = _currentString.count
            _dialogueSrolling = false
            _continueDialogueLabel.setVisible(false)
        }
        var subString = _currentString[0...numberOfCharactersToDraw]
        _dialogueLabel.setText(subString)
    }
    
    exit() {
        super.exit()
        
        System.print("Unloading InteractState...")

        UIManager.removeUIElement(_screenPanel)
        _menuMusic.stop()
        _menuMusic.unload()
        _menuMusic = null

        System.print("InteractState unloaded")
    }

    trySetInteractionStep(stepIndex) {
        var step = _interaction[stepIndex]
        for (item in step.conditionContainsItemVector) {
            if (!Inventory.contains(item)) {
                return false
            }
        }

        for (item in step.conditionExcludeItemVector) {
            if (Inventory.contains(item)) {
                return false
            }
        }

        _currentString = step.dialogue
        _dialogueSrolling = step.animateText
        _breakAfterStep = step.exitInteraction
        _interactionStep = stepIndex
        _currentDialogueStartTime = Application.gameTime

        for (item in step.itemAddVector) {
            Inventory.addItem(item)
        }

        for (item in step.itemRemoveVector) {
            Inventory.removeItem(item)
        }

        //if (step.deleteInteractingEntity) {
        //    
        //}

        if (step.applyCameraShake) {
            var cameraEntities = GameObject.getAllWithComponent("Camera")
            for (cameraEntity in cameraEntities) {
                Camera.addCameraTrauma(entity, 0.4)
            }
        }

        return true
    }
}
