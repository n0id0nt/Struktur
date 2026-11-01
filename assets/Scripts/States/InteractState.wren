// states/InteractState.wren
// Interact state - handles NPC and item interactions using the dialogue system. Drives the
// Dialogue/DialogueInterpreter model (advance/choice) and the glyph-reveal timing, and owns the
// UI/Dialogue/DialogueBoxUI + (while a node offers choices) UI/Dialogue/ChoiceListUI views. The world keeps running
// while a conversation is open; only the player is held - every Player is put into its interacting mode (no input,
// stands idle - GameObjects/Player.setInteracting) for the duration, so only the dialogue reads input.
import "resourceManager" for Music, Sound
import "app" for Time
import "input" for Input
import "gameObject" for GameObject
import "gameObjectComponents" for Script
import "debug" for Debug
import "localization" for Localization

import "States/BaseState" for BaseState
import "Dialogue/DialogueInterpreter" for DialogueInterpreter
import "Dialogue/DialogueEntryPoints" for DialogueEntryPoints
import "reflect" for Reflect
import "Dialogue/ChoiceRecord" for ChoiceRecord
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
        _pendingChoiceTexts       = null   // the current node's choices, held back until its line finishes scrolling

        _menuMusic       = null
        _textScrollSound = null
    }

    enter(stateManager, params) {
        super.enter(stateManager, params)
        holdPlayers_(true)

        _interactingEntity = params["interactingEntity"]

        _menuMusic = Music.load("Sounds/menuMusic.wav")
        _menuMusic.setLooping(true)
        _menuMusic.play()
        _textScrollSound = Sound.load("Sounds/scroll.wav")

        _view = DialogueBoxUI.new()

        var interactable = Script.getInstance(_interactingEntity)
        // A script can pick its own conversation (a party member asking whether to stay - GameObjects/PartyMember).
        var entryNodeId = null
        if (Reflect.hasMethod(interactable, "dialogueEntryPoint()")) {
            entryNodeId = interactable.dialogueEntryPoint()
        } else {
            entryNodeId = DialogueEntryPoints.get(interactable.name)
        }

        if (entryNodeId == null) {
            Debug.warning("No dialogue entry point for %(interactable.name)")
            stateManager.clearCurrentState()
            return
        }

        Debug.info("Starting dialogue for %(interactable.name) at node %(entryNodeId)")
        _currentResult = DialogueInterpreter.startDialogue(entryNodeId)
        processDialogueResult(_currentResult)
    }

    // Puts every player into (or back out of) its interacting mode - see the header.
    holdPlayers_(held) {
        for (entity in GameObject.getAllWithIdentifier("Player")) {
            var player = Script.getInstance(entity)
            if (player != null) {
                player.setInteracting(held)
            }
        }
    }

    update(stateManager) {
        var inputInteract = Input.isInputJustReleased("Interact")

        // Text scroll animation - glyph-count-based (not raw-string slicing) since the dialogue label is a
        // UIRichLabel: getGlyphCount()/setVisibleGlyphCount() count visible codepoints only, skipping markup
        // tags entirely, so this paces correctly regardless of how much [b]/[color]/etc. markup a line carries.
        if (_dialogueScrolling) {
            var charCount = ((Time.scaledTime - _currentDialogueStartTime) / TEXT_SCROLL_SPEED).floor
            if (charCount >= _currentGlyphCount) {
                finishScroll_()
                // The line (and any choices) only just appeared - a press this same frame mustn't also act on them.
                return
            }
            _view.revealGlyphs(charCount)
        }

        // Confirm / advance
        if (inputInteract) {
            if (_dialogueScrolling) {
                // Skip to end of scroll - which also brings up the choices, if this line has any
                finishScroll_()
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
        holdPlayers_(false)

        DialogueInterpreter.endDialogue()
        _pendingChoiceTexts = null

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

    // The current line is fully shown (it finished scrolling, or the player skipped it): show the continue prompt,
    // or - for a line that asks a question - its choices, which are held back until now (processDialogueResult).
    finishScroll_() {
        _dialogueScrolling = false
        _view.revealGlyphs(_currentGlyphCount)
        if (!_waitingForChoice) {
            _view.showContinuePrompt(true)
        } else if (_pendingChoiceTexts != null) {
            showChoices_(_pendingChoiceTexts)
            _pendingChoiceTexts = null
        }
    }

    // Replaces whatever choice list is up (e.g. the collapsed record of the last pick) with these choices.
    showChoices_(choiceTexts) {
        if (_choiceList != null) {
            _choiceList.teardown()
            _choiceList = null
        }
        _choiceList = ChoiceListUI.new(_view.screenPanel, choiceTexts, _view.font, Fn.new { |index|
            _textScrollSound.play()
        })
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

        // After a pick the list stays up collapsed to that one option (confirmChoice) as a record of it - until the
        // next set of choices replaces it (once their question has finished scrolling - finishScroll_), a node clears
        // it ("closeChoice" command - Dialogue/ChoiceRecord.wren), or the conversation ends (exit).
        var choices = result.choices
        var hasChoices = choices && choices.count > 0
        var closeRequested = ChoiceRecord.takeCloseRequest()
        if (_choiceList != null && (closeRequested || !_choiceList.collapsed)) {
            _choiceList.teardown()
            _choiceList = null
        }
        _pendingChoiceTexts = null

        if (hasChoices) {
            _waitingForChoice = true

            var choiceTexts = []
            var i = 0
            for (choice in choices) {
                choiceTexts.add("%(i + 1).  %(Localization.get(choice))")
                i = i + 1
            }
            // Shown once the question has finished scrolling (finishScroll_), not while it's still being "spoken".
            _pendingChoiceTexts = choiceTexts
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
        if (!_waitingForChoice || _choiceList == null || _choiceList.collapsed) return

        var index = _choiceList.focusedIndex
        Debug.info("Confirming choice %(index)")
        _choiceList.collapseTo(index)
        _currentResult = DialogueInterpreter.makeChoice(index)
        processDialogueResult(_currentResult)
    }
}
