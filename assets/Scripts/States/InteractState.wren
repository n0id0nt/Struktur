// states/InteractState.wren
// Interact state - handles NPC and item interactions using the dialogue system. Drives the
// Dialogue/DialogueInterpreter model (advance/choice) and the glyph-reveal timing, and owns the
// UI/Dialogue/DialogueBoxUI + (while a node offers choices) UI/Dialogue/ChoiceListUI views.
import "resourceManager" for Music, Sound
import "app" for Time
import "input" for Input
import "gameObjectComponents" for Script
import "debug" for Debug
import "localization" for Localization

import "States/BaseState" for BaseState
import "Dialogue/DialogueInterpreter" for DialogueInterpreter
import "Dialogue/DialogueEntryPoints" for DialogueEntryPoints
import "Localization/DisplayNames" for DisplayNames
import "UI/Dialogue/DialogueBoxUI" for DialogueBoxUI
import "UI/Dialogue/ChoiceListUI" for ChoiceListUI

var TEXT_SCROLL_SPEED = 0.02

class InteractState is BaseState {
    construct new() {
        super()
        name = "InteractState"
        _view = null
        _choiceList = null   // only while the current node offers choices

        _interactingEntity        = null
        _dialogueScrolling        = false
        _currentGlyphCount        = 0    // reveal cap for the current line - see processDialogueResult/update
        _currentDialogueStartTime = 0
        _currentResult            = null
        _waitingForChoice         = false

        _menuMusic       = null
        _textScrollSound = null
    }

    enter(stateManager, params) {
        super.enter(stateManager, params)

        _interactingEntity = params["interactingEntity"]

        _menuMusic = Music.load("Sounds/menuMusic.wav")
        _menuMusic.setLooping(true)
        _menuMusic.play()
        _textScrollSound = Sound.load("Sounds/scroll.wav")

        _view = DialogueBoxUI.new()

        var interactable = Script.getInstance(_interactingEntity)
        var entryNodeId = DialogueEntryPoints.get(interactable.name)

        if (entryNodeId == null) {
            Debug.warning("No dialogue entry point for %(interactable.name)")
            stateManager.clearCurrentState()
            return
        }

        Debug.info("Starting dialogue for %(interactable.name) at node %(entryNodeId)")
        _currentResult = DialogueInterpreter.startDialogue(entryNodeId)
        processDialogueResult(_currentResult)
    }

    update(stateManager) {
        var inputInteract = Input.isInputJustReleased("Interact")

        // Text scroll animation - glyph-count-based (not raw-string slicing) since the dialogue label is a
        // UIRichLabel: getGlyphCount()/setVisibleGlyphCount() count visible codepoints only, skipping markup
        // tags entirely, so this paces correctly regardless of how much [b]/[color]/etc. markup a line carries.
        if (_dialogueScrolling) {
            var charCount = ((Time.scaledTime - _currentDialogueStartTime) / TEXT_SCROLL_SPEED).floor
            if (charCount >= _currentGlyphCount) {
                charCount = _currentGlyphCount
                _dialogueScrolling = false
                if (!_waitingForChoice) {
                    _view.showContinuePrompt(true)
                }
            }
            _view.revealGlyphs(charCount)
        }

        // Confirm / advance
        if (inputInteract) {
            if (_dialogueScrolling) {
                // Skip to end of scroll
                _dialogueScrolling = false
                _view.revealGlyphs(_currentGlyphCount)
                if (!_waitingForChoice) {
                    _view.showContinuePrompt(true)
                }
            } else if (_waitingForChoice) {
                confirmChoice(stateManager)
            } else {
                continueDialogue(stateManager)
            }
        }
    }

    exit() {
        super.exit()
        Debug.info("Unloading InteractState...")

        DialogueInterpreter.endDialogue()

        if (_choiceList != null) {
            _choiceList.teardown()
            _choiceList = null
        }
        if (_view != null) {
            _view.teardown()
            _view = null
        }

        _menuMusic.stop()
        _menuMusic.unload()
        _menuMusic = null

        _textScrollSound.unload()
        _textScrollSound = null

        Debug.info("InteractState unloaded")
    }

    // --- dialogue flow -----------------------------------------------------

    processDialogueResult(result) {
        var speaker = result.speaker
        var text = result.text

        if (text) {
            // result.text/result.speaker are localization keys/raw npc ids respectively - resolve both to
            // display strings, then run the composed string through the existing {var} substitution pipeline
            // once (not per-frame - rich-text markup shouldn't be re-parsed every frame during reveal).
            var localizedText = Localization.get(text)
            var composed
            if (speaker && speaker != "") {
                var speakerDisplay = DisplayNames.npcDisplay(speaker)
                composed = "%(speakerDisplay):\n%(localizedText)"
            } else {
                composed = localizedText
            }
            var processed = DialogueInterpreter.processString(composed)
            _view.setMarkupText(processed)
            _currentGlyphCount = _view.glyphCount
        }

        _dialogueScrolling        = true
        _currentDialogueStartTime = Time.scaledTime
        _view.showContinuePrompt(false)
        _waitingForChoice = false

        if (_choiceList != null) {
            _choiceList.teardown()
            _choiceList = null
        }

        var choices = result.choices
        if (choices && choices.count > 0) {
            _waitingForChoice = true

            var choiceTexts = []
            var i = 0
            for (choice in choices) {
                choiceTexts.add("%(i + 1).  %(Localization.get(choice))")
                i = i + 1
            }
            _choiceList = ChoiceListUI.new(_view.screenPanel, choiceTexts, _view.font, Fn.new { |index|
                _textScrollSound.play()
            })
        }
    }

    continueDialogue(stateManager) {
        if (_waitingForChoice) return

        if (_currentResult.hasEnded) {
            stateManager.clearCurrentState()
            return
        }

        _currentResult = DialogueInterpreter.continueDialogue()
        processDialogueResult(_currentResult)
    }

    confirmChoice(stateManager) {
        if (!_waitingForChoice) return

        Debug.info("Confirming choice %(_choiceList.focusedIndex)")
        _currentResult = DialogueInterpreter.makeChoice(_choiceList.focusedIndex)
        processDialogueResult(_currentResult)
    }
}
