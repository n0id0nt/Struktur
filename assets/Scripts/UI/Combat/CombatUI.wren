// UI/Combat/CombatUI.wren
// The combat screen's view, built from the UI kit (UI/Theme/UIKit.wren) as a frame around the arena so nothing sits
// on the fighters (Combat/BattleStage.wren places them in the strip between):
//   - enemy CombatantUI cards across the top, centred;
//   - the battle-log message on a dark banner just under them;
//   - a full-width command bar along the bottom: the party's cards on the left, and on the right a command area that
//     holds either the move menu (UI/Theme/KitButton buttons, two columns, with a button hint) or the ParryPromptUI -
//     the two are never up at once.
// Pure construction + refresh - it never reads a Combatant or
// Timeline itself and never calls back into game logic (unlike the old inline buildUI(), whose
// button click handlers called straight into CombatState's turn-flow methods). States/
// CombatState.wren wires every button's setOnClick and feeds every refresh() call primitives it
// already resolved from the model.
import "ui" for UIManager, UIPanel
import "math" for Vec2
import "app" for Application
import "Util/Colors" for BLANK
import "UI/Combat/CombatantUI" for CombatantUI
import "UI/Combat/ParryPromptUI" for ParryPromptUI
import "UI/Theme/KitButton" for KitButton
import "UI/Theme/UIKit" for UIKit

var TOP_EDGE = 14             // the enemy cards, down from the screen's top
var ENEMY_CARD_WIDTH = 260
var ENEMY_GAP = 20
var MESSAGE_Y = 162           // the message banner's centre, down from the screen's top
var MESSAGE_SIZE = 30.0       // the font draws it at 27
var MESSAGE_PAD_X = 26        // around the message, inside its banner (the well's rim is 2px art at 4x = 8px)
var MESSAGE_PAD_Y = 14
var BAR_MIN_HEIGHT = 200      // the command bar along the bottom (taller only if the move grid needs it)
var BAR_PAD = 28              // inside the bar's window (its rim is 5px art at 4x = 20px)
var PARTY_CARD_WIDTH = 224
var PARTY_GAP = 12
var COMMAND_WIDTH = 464       // the right-hand command area: two 226px button columns and their gap
var BUTTON_WIDTH = 226
var BUTTON_HEIGHT = 40
var BUTTON_GAP_X = 12
var BUTTON_GAP_Y = 8
var HINT_GAP = 6              // between the move grid and its button hint
var HINT_HEIGHT = 32          // one rich-text line at size 18 (27px) plus its padding

class CombatUI {
    // enemyNames/partyNames: fixed for the fight, used once to label each CombatantUI card. moveButtonCount: how many
    // move buttons the menu needs - the most moves any party member has; setMoveLabels() relabels (and hides the
    // unused ones) for whoever is choosing. A Flee button is always added.
    construct new(enemyNames, partyNames, moveButtonCount) {
        UIKit.open()
        var gw = Application.gameWidth
        var gh = Application.gameHeight
        var font = UIKit.font()

        // Just a full-screen layout container for the views below - the arena level (see Combat/BattleStage.wren)
        // reads as its own space, no dimming overlay needed on top of it.
        _root = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(gw, gh), Vec2.new(0, 0))
        _root.setBackgroundColor(BLANK)
        _root.setBorderColor(BLANK)
        _root.setZIndex(-1)
        UIManager.addUIElement(_root)

        // Enemy cards across the top, centred as a group (narrower if there are ever too many to fit).
        _enemyViews = []
        var n = enemyNames.count
        var enemyWidth = ENEMY_CARD_WIDTH.min(((gw - 2 * TOP_EDGE - (n - 1) * ENEMY_GAP) / n.max(1)).floor)
        var startX = (gw - (n * enemyWidth + (n - 1) * ENEMY_GAP)) / 2
        for (i in 0...n) {
            var x = startX + i * (enemyWidth + ENEMY_GAP)
            _enemyViews.add(CombatantUI.new(_root, enemyNames[i], x, TOP_EDGE, enemyWidth, true, font))
        }

        // Battle log / prompt, under the enemy cards, on a dark banner. The message is rich text sized to its content,
        // and the banner is its child sized relative to it - so the banner always wraps the text exactly, however long
        // it is (and messages can carry [input=Action] button prompts).
        _messageLabel = UIKit.richText(_root, "", Vec2.new(0, MESSAGE_Y), Vec2.new(0.5, 0), Vec2.new(0.5, 0.5),
            MESSAGE_SIZE, UIKit.text, 3)
        // (5 / 2: the rich label's own padding, already inside its bounds.)
        UIKit.frame(_messageLabel, "well", Vec2.new(5 - MESSAGE_PAD_X, 2 - MESSAGE_PAD_Y), Vec2.new(0, 0),
            Vec2.new(2 * (MESSAGE_PAD_X - 5), 2 * (MESSAGE_PAD_Y - 2)), Vec2.new(1, 1), 2)
        setMessage("")

        // The command bar - tall enough for the move grid (two columns: the moves, then Flee).
        var rows = ((moveButtonCount + 2) / 2).floor
        var gridHeight = rows * BUTTON_HEIGHT + (rows - 1) * BUTTON_GAP_Y
        var commandHeight = gridHeight + HINT_GAP + HINT_HEIGHT
        var barHeight = BAR_MIN_HEIGHT.max(commandHeight + 2 * BAR_PAD)
        _bar = UIKit.frame(_root, "window", Vec2.new(0, 0), Vec2.new(0, 1), Vec2.new(gw, barHeight), Vec2.new(0, 0), 0)
        _bar.setAnchorPoint(Vec2.new(0, 1))

        // Party cards, left of the command area, vertically centred in the bar (narrower if there are ever too many).
        _partyViews = []
        var commandX = gw - BAR_PAD - COMMAND_WIDTH
        var partyCount = partyNames.count
        var partyRoom = commandX - BAR_PAD - 16
        var partyWidth = PARTY_CARD_WIDTH.min(((partyRoom - (partyCount - 1) * PARTY_GAP) / partyCount.max(1)).floor)
        var cardY = ((barHeight - CombatantUI.height(true)) / 2).floor
        for (k in 0...partyCount) {
            _partyViews.add(CombatantUI.new(_bar, partyNames[k], BAR_PAD + k * (partyWidth + PARTY_GAP), cardY,
                partyWidth, true, font))
        }

        // Move menu - the command area's usual content: one button per move in the chooser's kit, then Flee, in two
        // columns, the button hint under them. setMoveLabels lays it out for however many moves the chooser has.
        _moveMenu = UIPanel.new(Vec2.new(commandX, BAR_PAD), Vec2.new(0, 0), Vec2.new(COMMAND_WIDTH, commandHeight),
            Vec2.new(0, 0))
        _moveMenu.setBackgroundColor(BLANK)
        _moveMenu.setBorderColor(BLANK)
        _bar.addChild(_moveMenu)
        _moveButtons = []
        for (i in 0...moveButtonCount) {
            _moveButtons.add(KitButton.new(_moveMenu, "", Vec2.new(0, 0), Vec2.new(0, 0),
                                           Vec2.new(BUTTON_WIDTH, BUTTON_HEIGHT), font))
        }
        _fleeButton = KitButton.new(_moveMenu, "Flee", Vec2.new(0, 0), Vec2.new(0, 0),
                                    Vec2.new(BUTTON_WIDTH, BUTTON_HEIGHT), font)
        _menuHint = UIKit.richText(_moveMenu, "[input=UIAccept] Select", Vec2.new(0, gridHeight + HINT_GAP),
            Vec2.new(0.5, 0), Vec2.new(0.5, 0), 18.0, UIKit.textMuted, 6)
        layoutMenu_(moveButtonCount)
        _moveMenu.setVisible(false)

        // Parry decision prompt - the command area's other content, hidden until States/Combat/ParryPromptState.wren
        // shows it (the move menu is never up at the same time).
        _parryPrompt = ParryPromptUI.new(_bar, commandX, BAR_PAD, COMMAND_WIDTH, barHeight - 2 * BAR_PAD, font)

        font.unload()
    }

    moveButtons { _moveButtons }
    fleeButton { _fleeButton }

    setVisible(visible) { _root.setVisible(visible) }
    showMoveMenu(visible) { _moveMenu.setVisible(visible) }
    focusFirstMoveButton() { UIManager.setFocus(_moveButtons[0].panel) }

    // Labels the move buttons for whoever is choosing - one label per move; buttons beyond that are hidden, and Flee
    // takes the next spot after the last one used.
    setMoveLabels(labels) {
        for (i in 0..._moveButtons.count) {
            var used = i < labels.count
            _moveButtons[i].panel.setVisible(used)
            if (used) {
                _moveButtons[i].setText(labels[i])
            }
        }
        layoutMenu_(labels.count.min(_moveButtons.count))
    }

    setMessage(text) {
        _messageLabel.setMarkupText(text)
        _messageLabel.setVisible(text != "")
    }

    // Refreshes party member `index`'s card; selected rings it (whose move is being picked).
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

    // The prompt takes the move menu's spot - the flow never has both up, and this makes sure it can't.
    showParryPrompt(attackerName, moveName, onYes, onNo) {
        _moveMenu.setVisible(false)
        _parryPrompt.show(attackerName, moveName, onYes, onNo)
    }
    setParryRemaining(fraction) { _parryPrompt.setRemaining(fraction) }
    hideParryPrompt() { _parryPrompt.hide() }

    teardown() {
        if (_root != null) {
            UIManager.removeUIElement(_root)
            _root = null
            UIKit.close()
        }
    }

    // `moves` move buttons in reading order across two columns, Flee in the next spot.
    layoutMenu_(moves) {
        for (i in 0..moves) {
            var button = i < moves ? _moveButtons[i] : _fleeButton
            var column = i % 2
            var row = (i / 2).floor
            button.panel.setPosition(Vec2.new(column * (BUTTON_WIDTH + BUTTON_GAP_X), row * (BUTTON_HEIGHT + BUTTON_GAP_Y)),
                Vec2.new(0, 0))
        }
    }
}
