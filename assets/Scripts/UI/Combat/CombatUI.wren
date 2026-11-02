// UI/Combat/CombatUI.wren
// The combat screen's view: root panel, enemy/party CombatantUI stacks, battle-log message, the
// move menu (via UI/Shared/ButtonUI), and a ParryPromptUI shown only during the "parryPrompt" phase.
// Pure construction + refresh - it never reads a Combatant or
// Timeline itself and never calls back into game logic (unlike the old inline buildUI(), whose
// button click handlers called straight into CombatState's turn-flow methods). States/
// CombatState.wren wires every button's setOnClick and feeds every refresh() call primitives it
// already resolved from the model.
import "ui" for UIManager, UILabel, UIPanel, TextAlignment
import "math" for Vec2
import "app" for Application
import "resourceManager" for Font
import "Util/Colors" for WHITE, BLANK
import "UI/Combat/CombatantUI" for CombatantUI
import "UI/Combat/ParryPromptUI" for ParryPromptUI
import "UI/Shared/ButtonUI" for ButtonUI

class CombatUI {
    // enemyNames/partyNames: fixed for the fight, used once to label each CombatantUI stack (the party's stacks
    // run up the bottom-left, leader at the bottom). moveButtonCount: how many move buttons the menu needs - the
    // most moves any party member has; setMoveLabels() relabels (and hides the unused ones) for whoever is
    // choosing. A Flee button is always added.
    construct new(enemyNames, partyNames, moveButtonCount) {
        var gw = Application.gameWidth
        var gh = Application.gameHeight
        var font = Font.load("Fonts/medieval_sharp/MedievalSharp-Bold.ttf", 60)

        // Just a full-screen layout container for the views/menu below - Battle_Arena (see
        // Combat/BattleStage.wren) reads as its own space, no dimming overlay needed on top of it.
        _root = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(gw, gh), Vec2.new(0, 0))
        _root.setBackgroundColor(BLANK)
        _root.setBorderColor(BLANK)
        _root.setZIndex(-1)
        UIManager.addUIElement(_root)

        // Enemy stacks across the top.
        _enemyViews = []
        var n = enemyNames.count
        var slotW = 300
        var gap = 44
        var startX = (gw - (n * slotW + (n - 1) * gap)) / 2
        for (i in 0...n) {
            var x = startX + i * (slotW + gap)
            _enemyViews.add(CombatantUI.new(_root, enemyNames[i], x, 40, slotW, true, font))
        }

        // Battle log / prompt, centre.
        _messageLabel = UILabel.new(Vec2.new(0, 0), Vec2.new(0.5, 0.4), "", 30.0)
        _messageLabel.setFont(font)
        _messageLabel.setTextColor(WHITE)
        _messageLabel.setBoundingBoxToText()
        _messageLabel.setAlignment(TextAlignment.CENTER)
        _messageLabel.setAnchorPoint(Vec2.new(0.5, 0.5))
        _root.addChild(_messageLabel)

        // Party stacks, bottom-left: the leader at the bottom, each further member 80px above.
        _partyViews = []
        for (k in 0...partyNames.count) {
            _partyViews.add(CombatantUI.new(_root, partyNames[k], 56, gh - 150 - k * 80, 320, true, font))
        }

        // Parry decision prompt - hidden until States/Combat/ParryPromptState.wren shows it, centred
        // just below the battle-log message.
        _parryPrompt = ParryPromptUI.new(_root, gw / 2, gh * 0.4 + 110, font)

        // Move menu, bottom-right - one button per move in the player's kit, plus Flee.
        _moveMenu = UIPanel.new(Vec2.new(-30, -30), Vec2.new(1, 1), Vec2.new(320, 300), Vec2.new(0, 0))
        _moveMenu.setAnchorPoint(Vec2.new(1, 1))
        _moveMenu.setBackgroundColor(BLANK)
        _moveMenu.setBorderColor(BLANK)
        _root.addChild(_moveMenu)

        _moveButtons = []
        var h = 44
        var i = 0
        while (i < moveButtonCount) {
            _moveButtons.add(ButtonUI.new(_moveMenu, "", Vec2.new(0, i * (h + 8)), Vec2.new(0, 0),
                                          Vec2.new(304, h), font))
            i = i + 1
        }
        _fleeButton = ButtonUI.new(_moveMenu, "Flee", Vec2.new(0, i * (h + 8)), Vec2.new(0, 0),
                                   Vec2.new(304, h), font)
        _moveMenu.setVisible(false)

        font.unload()
    }

    moveButtons { _moveButtons }
    fleeButton { _fleeButton }

    setVisible(visible) { _root.setVisible(visible) }
    showMoveMenu(visible) { _moveMenu.setVisible(visible) }
    focusFirstMoveButton() { UIManager.setFocus(_moveButtons[0].panel) }

    // Labels the move buttons for whoever is choosing - one label per move; buttons beyond that are hidden.
    setMoveLabels(labels) {
        for (i in 0..._moveButtons.count) {
            var used = i < labels.count
            _moveButtons[i].panel.setVisible(used)
            if (used) {
                _moveButtons[i].setText(labels[i])
            }
        }
    }

    setMessage(text) {
        _messageLabel.setText(text)
        _messageLabel.setBoundingBoxToText()
    }

    // Refreshes party member `index`'s stack; selected flags it with the ">>" marker (whose move is being picked).
    refreshParty(index, alive, hpFraction, chargeFraction, staminaFraction, moveText, staggered, selected) {
        var v = _partyViews[index]
        v.refresh(alive, hpFraction, chargeFraction, staminaFraction, moveText, staggered)
        v.selected = selected
    }

    refreshEnemy(index, alive, hpFraction, chargeFraction, staminaFraction, moveText, staggered, selected) {
        var v = _enemyViews[index]
        v.refresh(alive, hpFraction, chargeFraction, staminaFraction, moveText, staggered)
        v.selected = selected
    }

    showParryPrompt(attackerName, moveName, onYes, onNo) { _parryPrompt.show(attackerName, moveName, onYes, onNo) }
    setParryRemaining(fraction) { _parryPrompt.setRemaining(fraction) }
    hideParryPrompt() { _parryPrompt.hide() }

    teardown() {
        if (_root != null) {
            UIManager.removeUIElement(_root)
            _root = null
        }
    }
}
