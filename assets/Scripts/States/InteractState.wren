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
import "World/StageDirector" for StageDirector

var TEXT_SCROLL_SPEED = 0.02

// A single line shown without a dialogue node - what an interactable's interact() returns (see enter()). Shaped
// like the DialogueResult fields this state reads, already localized, and always the last line.
class MessageLine {
    construct new(text) {
        _text = text
    }
    text { _text }
    speaker { null }
    choices { null }
    hasEnded { true }
}

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
        _heldResult               = null   // a node waiting for its staging to finish before it's shown (show_)
        _choicePressStarted       = false  // Interact went down while the choices were up - see update

        _menuMusic       = null
        _textScrollSound = null
    }

    enter(stateManager, params) {
        super.enter(stateManager, params)
        _stateManager = stateManager   // show_ ends the conversation through it
        holdPlayers_(true)

        _interactingEntity = params["interactingEntity"]

        _menuMusic = Music.load("Audio/Musics/5 - Peaceful.ogg")
        _menuMusic.setLooping(true)
        _menuMusic.play()
        _textScrollSound = Sound.load("Audio/Sounds/Menu/Move1.wav")

        _view = DialogueBoxUI.new()
        _heldResult = null

        // A conversation started straight from a node, with nobody to talk to (the new-game opening -
        // States/GameWorldState.wren): params["entryNode"].
        var directNode = params["entryNode"]
        if (directNode != null) {
            Debug.info("Starting dialogue at node %(directNode)")
            show_(DialogueInterpreter.startDialogue(directNode))
            return
        }

        var interactable = Script.getInstance(_interactingEntity)
        // A script can react to being interacted with before its conversation starts (a rest spot becoming the
        // spawn point - GameObjects/RestSpot.wren).
        if (Reflect.hasMethod(interactable, "onInteractStarted()")) {
            interactable.onInteractStarted()
        }
        // A script can pick its own conversation (a party member asking whether to stay - GameObjects/PartyMember).
        var entryNodeId = null
        if (Reflect.hasMethod(interactable, "dialogueEntryPoint()")) {
            entryNodeId = interactable.dialogueEntryPoint()
        } else {
            entryNodeId = DialogueEntryPoints.get(interactable.name)
        }

        // No conversation for it: an interactable can handle being interacted with itself (a door from the level
        // opening, an item being picked up...) and hand back a line to show, already localized - or null for none.
        if (entryNodeId == null && Reflect.hasMethod(interactable, "interact()")) {
            var message = interactable.interact()
            if (message == null) {
                stateManager.clearCurrentState()
                return
            }
            _currentResult = MessageLine.new(message)
            processDialogueResult(_currentResult)
            return
        }

        if (entryNodeId == null) {
            Debug.warning("No dialogue entry point for %(interactable.name)")
            stateManager.clearCurrentState()
            return
        }

        Debug.info("Starting dialogue for %(interactable.name) at node %(entryNodeId)")
        show_(DialogueInterpreter.startDialogue(entryNodeId))
    }

    // True while a node is held back waiting for its staging to finish (show_).
    holding { _heldResult != null }

    // Shows the node the dialogue has just reached - or, while staging it started (moveTo / wait -
    // World/StageDirector.wren) is still playing, holds it with the box hidden so the scene can be seen, and shows it
    // once that's done (update). A node with no line of its own just carries on to the next.
    show_(result) {
        _currentResult = result
        if (StageDirector.blocking) {
            _heldResult = result
            _view.screenPanel.setVisible(false)
            return
        }
        _heldResult = null
        _view.screenPanel.setVisible(true)
        var text = result.text
        var hasChoices = result.choices != null && result.choices.count > 0
        if ((text == null || text == "") && !hasChoices) {
            if (result.hasEnded) {
                _stateManager.clearCurrentState()
            } else {
                show_(DialogueInterpreter.continueDialogue())
            }
            return
        }
        processDialogueResult(result)
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
        // The prompt's bob and the choice cursor's nudge - real time, so they keep moving whatever the time scale.
        if (_view != null) {
            _view.tick(Time.unscaledTime)
        }
        if (_choiceList != null) {
            _choiceList.tick(Time.unscaledTime)
        }

        // A node held back for its staging (show_): shown once the scene has played out. Input waits meanwhile.
        if (_heldResult != null) {
            if (!StageDirector.blocking) {
                show_(_heldResult)
            }
            return
        }

        var inputInteract = Input.isInputJustReleased("Interact")
        // A choice is only picked by a press that started after the choices appeared - otherwise holding the button to
        // skip a line that finishes on its own mid-press would pick the first choice before it was ever seen.
        if (_choiceList != null && !_choiceList.collapsed && Input.isInputJustPressed("Interact")) {
            _choicePressStarted = true
        }

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
                if (_choicePressStarted) {
                    confirmChoice(stateManager)
                }
            } else {
                continueDialogue(stateManager)
            }
        }
    }

    exit() {
        super.exit()
        Debug.info("Unloading InteractState...")
        holdPlayers_(false)
        // Players / party members held in a pose by the scene go back to their own control (NPCs keep theirs).
        StageDirector.releasePoses()

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
        _choiceList = ChoiceListUI.new(_view.textBox, choiceTexts, _view.font, Fn.new { |index|
            _textScrollSound.play()
        })
        _view.showChoosePrompt(true)
        _choicePressStarted = false
    }

    // --- dialogue flow -----------------------------------------------------

    processDialogueResult(result) {
        var speaker = result.speaker
        var text = result.text

        if (text) {
            // result.text/result.speaker are localization keys/raw npc ids respectively - resolve both to
            // display strings, then run the composed string through the existing {var} substitution pipeline
            // once (not per-frame - rich-text markup shouldn't be re-parsed every frame during reveal).
            var localizedText = (result is MessageLine) ? text : Localization.get(text)
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

            // Unnumbered - the list marks the highlighted choice with a cursor, and there are no number-key shortcuts.
            var choiceTexts = []
            for (choice in choices) {
                choiceTexts.add(Localization.get(choice))
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

        show_(DialogueInterpreter.continueDialogue())
    }

    confirmChoice(stateManager) {
        if (!_waitingForChoice || _choiceList == null || _choiceList.collapsed) return

        var index = _choiceList.focusedIndex
        Debug.info("Confirming choice %(index)")
        _choiceList.collapseTo(index)
        show_(DialogueInterpreter.makeChoice(index))
    }
}
