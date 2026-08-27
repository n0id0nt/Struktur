// states/InteractState.wren
// Interact state - handles NPC and item interactions using the dialogue system

import "States/BaseState" for BaseState
import "resourceManager" for Font, Texture, Music, Sound
import "ui" for UIManager, UILabel, UIRichLabel, UIPanel, TextWrapping
import "app" for Application, Time
import "math" for Vec2, Vec3, Vec4
import "input" for Input
import "gameObjectComponents" for Camera, Script
import "gameObject" for GameObject
import "debug" for Debug
import "localization" for Localization

import "Colors" for BLANK, BLACK, DARKGRAY, WHITE, LIGHTGRAY
import "Inventory" for Inventory
import "dialogue" for DialogueRegistry, DialogueManager, DialogueResult, VariableSubstitution
import "Localization/DisplayNames" for DisplayNames

var TEXT_SCROLL_SPEED = 0.02

// Colour constants for choice panel theming
var CHOICE_BG_DEFAULT    = Vec4.new(20,  16,  12,  200)
var CHOICE_BG_FOCUSED    = Vec4.new(180, 140, 60,  230)
var CHOICE_BORDER_DEFAULT = Vec4.new(80,  65,  45,  200)
var CHOICE_BORDER_FOCUSED = Vec4.new(220, 180, 80,  255)

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

    static processString(text) {
        return processString(text, null)
    }

    static processString(text, overrides) {
        if (text == null || text == "") {
            return text
        }
        var result = text
        var offset = 0

        while (offset < result.count) {
            var startIdx = result.indexOf("{", offset)
            if (startIdx == -1) break

            var endIdx = result.indexOf("}", startIdx)
            if (endIdx == -1) break

            var expression = result[startIdx + 1...endIdx]

            if (expression == "") {
                offset = endIdx + 1
                continue
            }

            var pipeParts = expression.split("|")
            var nameAndParams = pipeParts[0]
            var modifiers = pipeParts.count > 1 ? pipeParts[1..-1].join("|") : ""

            var colonIdx = nameAndParams.indexOf(":")
            var varName
            var params = {}
            if (colonIdx != -1) {
                varName = nameAndParams[0...colonIdx].trim()
                var paramStr = nameAndParams[colonIdx + 1..-1]
                var paramPairs = paramStr.split(",")
                for (pair in paramPairs) {
                    var eqIdx = pair.indexOf("=")
                    if (eqIdx != -1) {
                        var key = pair[0...eqIdx].trim()
                        var val = pair[eqIdx + 1..-1].trim()
                        params[key] = val
                    }
                }
            } else {
                varName = nameAndParams.trim()
            }

            var value
            if (overrides != null && overrides.containsKey(varName)) {
                value = overrides[varName]
            } else {
                value = DialogueRegistry.getRegisteredVariable(varName).call(params)
            }
            if (value == null) {
                value = varName
            }

            var formatted = VariableSubstitution.applyModifiers(value, modifiers)

            result = result[0...startIdx] + formatted + result[endIdx + 1..-1]
            offset = startIdx + formatted.count
        }

        return result
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
            Debug.warning("makeChoice called but no active dialogue node")
            return DialogueResult.noActiveNode()
        }
        var choices = currentNode.choices
        if (!choices) {
            Debug.error("makeChoice called but no choices on node %(currentNode.id)")
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

// 

class InteractState is BaseState {
    construct new() {
        super()
        name = "InteractState"

        _screenPanel        = null
        _textBackgroundPanel = null
        _dialogueLabel      = null
        _continueLabel      = null

        // Choice UI
        _choiceContainerPanel = null   // the outer panel sitting above the text box
        _choicePanels         = []     // one UIPanel per choice
        _choiceLabels         = []     // one UILabel per choice (child of its panel)
        _focusedChoiceIndex   = 0

        _interactingEntity       = null
        _dialogueScrolling       = false
        _currentGlyphCount       = 0    // reveal cap for the current line - see processDialogueResult/update
        _currentDialogueStartTime = 0
        _currentResult           = null
        _waitingForChoice        = false

        _font            = null
        _regularFont     = null
        _boldFont        = null
        _italicFont      = null
        _boldItalicFont  = null
        _menuMusic       = null
        _textScrollSound = null
    }

    //  enter 

    enter(stateManager, params) {
        super.enter(stateManager, params)

        _interactingEntity = params["interactingEntity"]

        _menuMusic = Music.load("Sounds/menuMusic.wav")
        _menuMusic.setLooping(true)
        _menuMusic.play()
        _textScrollSound = Sound.load("Sounds/scroll.wav")

        _font = Font.load("Fonts/medieval_sharp/MedievalSharp-Bold.ttf", 30)
        // Rich-text font variants for the dialogue label itself - same medieval_sharp family/sizing convention
        // MainMenuState's showcaseRichText() uses, just at the dialogue box's existing text size (20).
        _regularFont    = Font.load("Fonts/medieval_sharp/MedievalSharp-Book.ttf", 20)
        _boldFont       = Font.load("Fonts/medieval_sharp/MedievalSharp-Bold.ttf", 20)
        _italicFont     = Font.load("Fonts/medieval_sharp/MedievalSharp-BookOblique.ttf", 20)
        _boldItalicFont = Font.load("Fonts/medieval_sharp/MedievalSharp-BoldOblique.ttf", 20)
        var dialoguePanelTexture = Texture.load("Tiles/DialoguePanel.png")

        //  Full-screen transparent root 
        _screenPanel = UIPanel.new(
            Vec2.new(0, 0), Vec2.new(0, 0),
            Vec2.new(Application.gameWidth, Application.gameHeight),
            Vec2.new(0, 0)
        )
        _screenPanel.setBackgroundColor(BLANK)
        _screenPanel.setBorderColor(BLANK)
        UIManager.addUIElement(_screenPanel)

        //  Dialogue text box – bottom-centre 
        // Anchored at (0.5, 1) so the bottom edge sits at 95 % of screen height.
        _textBackgroundPanel = UIPanel.new(
            Vec2.new(0, -30),     // 30 px gap from screen bottom
            Vec2.new(0.5, 1),     // anchor: horizontal centre, vertical bottom
            Vec2.new(800, 200),
            Vec2.new(0, 0)
        )
        _textBackgroundPanel.setAnchorPoint(Vec2.new(0.5, 1))
        _textBackgroundPanel.setBorderColor(BLANK)
        _textBackgroundPanel.setBackgroundColor(DARKGRAY)
        _textBackgroundPanel.setBackgroundTexture(dialoguePanelTexture)
        dialoguePanelTexture.unload()
        _screenPanel.addChild(_textBackgroundPanel)

        //  Dialogue text label (inside text box)
        _dialogueLabel = UIRichLabel.new(Vec2.new(40, 25), Vec2.new(0, 0), "", 20.0)
        _dialogueLabel.setTextColor(BLACK)
        _dialogueLabel.setAnchorPoint(Vec2.new(0, 0))
        _dialogueLabel.setFont(_regularFont)
        _dialogueLabel.setBoldFont(_boldFont)
        _dialogueLabel.setItalicFont(_italicFont)
        _dialogueLabel.setBoldItalicFont(_boldItalicFont)
        _dialogueLabel.setWordWrap(TextWrapping.WORD_WRAP)
        // Fill the panel minus 40 px left/right margin and 60 px from bottom
        // so the continue prompt has room.
        _dialogueLabel.setSize(Vec2.new(-80, -60), Vec2.new(1, 1))
        _textBackgroundPanel.addChild(_dialogueLabel)

        // UIRichLabel holds its own reference once set via setFont/setBoldFont/etc - safe to release ours
        // immediately, matching MainMenuState.showcaseRichText()'s identical pattern.
        _regularFont.unload()
        _boldFont.unload()
        _italicFont.unload()
        _boldItalicFont.unload()

        //  "Continue ▶" prompt – bottom-right of text box
        // Position: 20 px from right edge, 18 px from bottom edge.
        // setAnchorPoint(1,1) means the label's own bottom-right corner sits
        // at the specified position.
        _continueLabel = UILabel.new(Vec2.new(-20, -18), Vec2.new(1, 1), Localization.get("menu.interact.continue_prompt"), 16.0)
        _continueLabel.setTextColor(BLACK)
        _continueLabel.setAnchorPoint(Vec2.new(1, 1))
        _continueLabel.setFont(_font)
        _continueLabel.setVisible(false)
        _textBackgroundPanel.addChild(_continueLabel)

        //  Start dialogue 
        var interactable = Script.getInstance(_interactingEntity)
        var entryNodeId  = getEntryPoint(interactable.name)

        if (entryNodeId == null) {
            Debug.warning("No dialogue entry point for %(interactable.name)")
            stateManager.clearCurrentState()
            return
        }

        Debug.info("Starting dialogue for %(interactable.name) at node %(entryNodeId)")
        _currentResult = DialogueManagerHelper.startDialogue(entryNodeId)
        processDialogueResult(_currentResult)
    }

    // update 

    update(stateManager) {
        var inputInteract   = Input.isInputJustReleased("Interact")
        //var inputNavUp      = Input.isInputJustReleased("NavigateUp")
        //var inputNavDown    = Input.isInputJustReleased("NavigateDown")

        // Text scroll animation - glyph-count-based (not raw-string slicing) since the dialogue label is a
        // UIRichLabel now: getGlyphCount()/setVisibleGlyphCount() count visible codepoints only, skipping markup
        // tags entirely, so this paces correctly regardless of how much [b]/[color]/etc. markup a line carries.
        if (_dialogueScrolling) {
            var charCount = ((Time.scaledTime - _currentDialogueStartTime) / TEXT_SCROLL_SPEED).floor
            if (charCount >= _currentGlyphCount) {
                charCount = _currentGlyphCount
                _dialogueScrolling = false
                if (!_waitingForChoice) {
                    _continueLabel.setVisible(true)
                }
            }
            _dialogueLabel.setVisibleGlyphCount(charCount)
        }

        // Choice navigation (up / down) 
        //if (_waitingForChoice && _choicePanels.count > 0) {
        //    if (inputNavUp) {
        //        _focusedChoiceIndex = (_focusedChoiceIndex - 1 + _choicePanels.count) % _choicePanels.count
        //        applyChoiceFocus(_focusedChoiceIndex)
        //    }
        //    if (inputNavDown) {
        //        _focusedChoiceIndex = (_focusedChoiceIndex + 1) % _choicePanels.count
        //        applyChoiceFocus(_focusedChoiceIndex)
        //    }
        //}

        // Confirm / advance 
        if (inputInteract) {
            if (_dialogueScrolling) {
                // Skip to end of scroll
                _dialogueScrolling = false
                _dialogueLabel.setVisibleGlyphCount(_currentGlyphCount)
                if (!_waitingForChoice) {
                    _continueLabel.setVisible(true)
                }
            } else if (_waitingForChoice) {
                // Confirm the currently highlighted choice
                confirmChoice(stateManager)
            } else {
                continueDialogue(stateManager)
            }
        }
    }

    //  exit 

    exit() {
        super.exit()
        Debug.info("Unloading InteractState...")

        DialogueManagerHelper.endDialogue()

        UIManager.removeUIElement(_screenPanel)
        _screenPanel = null

        _menuMusic.stop()
        _menuMusic.unload()
        _menuMusic = null

        _textScrollSound.unload()
        _textScrollSound = null

        _font.unload()
        _font = null

        Debug.info("InteractState unloaded")
    }

    // dialogue flow 

    processDialogueResult(result) {
        var speaker = result.speaker
        var text    = result.text

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
            var processed = DialogueManagerHelper.processString(composed)
            _dialogueLabel.setMarkupText(processed)
            _currentGlyphCount = _dialogueLabel.getGlyphCount()
        }

        _dialogueScrolling        = true
        _currentDialogueStartTime = Time.scaledTime
        _continueLabel.setVisible(false)
        _waitingForChoice = false

        clearChoiceUI()

        var choices = result.choices
        if (choices && choices.count > 0) {
            _waitingForChoice = true
            buildChoiceUI(choices)
        }
    }

    continueDialogue(stateManager) {
        if (_waitingForChoice) return

        if (_currentResult.hasEnded) {
            stateManager.clearCurrentState()
            return
        }

        _currentResult = DialogueManagerHelper.continueDialogue()
        processDialogueResult(_currentResult)
    }

    confirmChoice(stateManager) {
        if (!_waitingForChoice) return

        Debug.info("Confirming choice %(  _focusedChoiceIndex)")
        _currentResult = DialogueManagerHelper.makeChoice(_focusedChoiceIndex)
        processDialogueResult(_currentResult)
    }

    // choice UI 

    // Build a vertical list of choice panels in the top-right corner of the
    // screen, positioned directly above the dialogue text box.
    //
    // Layout (all child of _screenPanel):
    //
    //   ┌┐  ← _choiceContainerPanel
    //   │  1.  First choice text     │    anchored (1, 1) = top-right relative
    //   │  2.  Second choice text    │    to _screenPanel, sits above text box
    //   │  3.  Third choice text     │
    //   └┘
    //   ┌┐
    //   │  Speaker:  dialogue text …                          Continue ▶     │
    //   └┘

    buildChoiceUI(choices) {
        _focusedChoiceIndex = 0

        var panelW    = 380
        var rowH      = 44
        var padding   = 12
        var gap       = 6
        var totalH    = choices.count * rowH + (choices.count - 1) * gap + padding * 2

        // Container sits above the text box: anchor bottom-right of screen,
        // offset up by (textbox height + margin) and in from the right edge.
        var textBoxH  = 200
        var marginR   = 40
        var marginB   = textBoxH + 20   // 20 px gap between container and text box

        _choiceContainerPanel = UIPanel.new(
            Vec2.new(-marginR, -marginB),   // offset from anchor
            Vec2.new(1, 1),                  // anchor: bottom-right of parent
            Vec2.new(panelW, totalH),
            Vec2.new(0, 0)
        )
        _choiceContainerPanel.setAnchorPoint(Vec2.new(1, 1))
        _choiceContainerPanel.setBackgroundColor(Vec4.new(10, 8, 6, 210))
        _choiceContainerPanel.setBorderColor(Vec4.new(80, 65, 45, 180))
        _screenPanel.addChild(_choiceContainerPanel)

        var curY = padding
        var i    = 0
        for (choice in choices) {
            var choicePanel = UIPanel.new(
                Vec2.new(padding, curY),
                Vec2.new(0, 0),
                Vec2.new(panelW - padding * 2, rowH),
                Vec2.new(0, 0)
            )
            choicePanel.setAnchorPoint(Vec2.new(0, 0))
            choicePanel.setBackgroundColor(CHOICE_BG_DEFAULT)
            choicePanel.setBorderColor(CHOICE_BORDER_DEFAULT)
            choicePanel.setFocusable(true)

            // Capture index for the closure
            var capturedIndex = i
            choicePanel.setOnFocus { |sender|
                _focusedChoiceIndex = capturedIndex
                applyChoiceFocus(capturedIndex)
                _textScrollSound.play()
            }

            _choiceContainerPanel.addChild(choicePanel)
            _choicePanels.add(choicePanel)

            // Choice text label inside the row panel - choice is a localization key (see the dialogue JSON's
            // choice "text" fields), not literal text.
            var choiceLabel = UILabel.new(
                Vec2.new(12, 0),
                Vec2.new(0, 0.5),
                "%(i + 1).  %(Localization.get(choice))",
                16.0
            )
            choiceLabel.setTextColor(WHITE)
            choiceLabel.setAnchorPoint(Vec2.new(0, 0.5))
            choiceLabel.setFont(_font)
            choicePanel.addChild(choiceLabel)
            _choiceLabels.add(choiceLabel)

            curY = curY + rowH + gap
            i    = i + 1
        }

        // Give focus to first row so controller/keyboard navigation starts there
        if (_choicePanels.count > 0) {
            UIManager.setFocus(_choicePanels[0])
            applyChoiceFocus(0)
        }
    }

    // Update visual state of all choice rows to reflect which is focused
    applyChoiceFocus(focusedIndex) {
        var i = 0
        for (panel in _choicePanels) {
            if (i == focusedIndex) {
                panel.setBackgroundColor(CHOICE_BG_FOCUSED)
                panel.setBorderColor(CHOICE_BORDER_FOCUSED)
                _choiceLabels[i].setTextColor(BLACK)
            } else {
                panel.setBackgroundColor(CHOICE_BG_DEFAULT)
                panel.setBorderColor(CHOICE_BORDER_DEFAULT)
                _choiceLabels[i].setTextColor(WHITE)
            }
            i = i + 1
        }
    }

    clearChoiceUI() {
        if (_choiceContainerPanel) {
            _screenPanel.removeChild(_choiceContainerPanel)
            _choiceContainerPanel = null
        }
        _choicePanels.clear()
        _choiceLabels.clear()
        _focusedChoiceIndex = 0
    }

    //  entry point lookup 

    getEntryPoint(interactableName) {
        // NPCs
        if (interactableName == "Scholar")    return "scholar"
        if (interactableName == "Gardener")   return "gardener"
        if (interactableName == "Cook")       return "cook"
        if (interactableName == "Merchant")   return "merchant"
        if (interactableName == "Guard")      return "guard"
        if (interactableName == "Librarian")  return "librarian"
        if (interactableName == "Astronomer") return "astronomer"
        if (interactableName == "Cordelia")   return "cordelia"
        if (interactableName == "Dreamer")    return "dreamer"
        if (interactableName == "Guardian")   return "guardian"
        if (interactableName == "Inventor")   return "inventor"

        // Immovable interactables
        if (interactableName == "Safe")                    return "safe"
        if (interactableName == "Red Pedestal Inactive")   return "red_pedestal_inactive"
        if (interactableName == "Red Pedestal Active")     return "red_pedestal_active"
        if (interactableName == "Blue Pedestal Inactive")  return "blue_pedestal_inactive"
        if (interactableName == "Blue Pedestal Active")    return "blue_pedestal_active"
        if (interactableName == "Green Pedestal Inactive") return "green_pedestal_inactive"
        if (interactableName == "Green Pedestal Active")   return "green_pedestal_active"
        if (interactableName == "Yellow Pedestal Inactive") return "yellow_pedestal_inactive"
        if (interactableName == "Yellow Pedestal Active")  return "yellow_pedestal_active"
        if (interactableName == "Entrance Door")           return "entrance_door"

        // World items (always pickuppable — no suffix logic)
        if (interactableName == "Rose")         return "rose"
        if (interactableName == "Tool Box")     return "tool_box"
        if (interactableName == "Telescope")    return "telescope"
        if (interactableName == "Ancient Seal") return "ancient_seal"

        // Returnable items — use "_return" entry if already in inventory
        var suffix = Inventory.contains(interactableName) ? "_return" : ""
        if (interactableName == "Ancient Tome") return "ancient_book" + suffix
        if (interactableName == "Love Letter")  return "love_letter" + suffix
        if (interactableName == "Hammer")       return "hammer" + suffix
        if (interactableName == "Star Chart")   return "star_chart" + suffix
        if (interactableName == "Ornate Key")   return "ornate_key" + suffix

        Debug.warning("No dialogue entry point found for '%(interactableName)'")
        return null
    }
}
