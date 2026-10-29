// UI/BattleTest/BattleSetupUI.wren
// The battle test's setup screen (States/BattleTestState.wren): two sides of SLOTS_PER_SIDE rows each - a fighter
// picker, MOVES_PER_FIGHTER move pickers and a stat readout per row - then Start / Back and a line for the last
// result. Pure view: every button is a click-to-cycle picker the state wires and relabels.
import "math" for Vec2, Vec4
import "resourceManager" for Font
import "ui" for UIManager, UILabel, UIPanel
import "app" for Application
import "Colors" for BLANK, WHITE, LIGHTGRAY
import "UI/Shared/ButtonUI" for ButtonUI

var SLOTS_PER_SIDE    = 3
var MOVES_PER_FIGHTER = 3

var FIGHTER_WIDTH = 230
var MOVE_WIDTH    = 160
var BUTTON_HEIGHT = 34
var GAP           = 10
var ROW_HEIGHT    = 62    // button + stat line
var ROW_WIDTH     = FIGHTER_WIDTH + MOVES_PER_FIGHTER * (MOVE_WIDTH + GAP)

// One row: the fighter picker, its move pickers and its stat line.
class SetupRow {
    construct new(fighterButton, moveButtons, statLabel) {
        _fighterButton = fighterButton
        _moveButtons = moveButtons
        _statLabel = statLabel
    }

    fighterButton { _fighterButton }
    moveButtons { _moveButtons }

    setStats(text) {
        _statLabel.setText(text)
        _statLabel.setBoundingBoxToText()
    }
}

class BattleSetupUI {
    construct new() {
        var font = Font.load("Fonts/medieval_sharp/MedievalSharp-Bold.ttf", 60)
        _font = font

        _root = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0),
            Vec2.new(Application.gameWidth, Application.gameHeight), Vec2.new(0, 0))
        _root.setBackgroundColor(Vec4.new(20, 22, 30, 255))
        _root.setBorderColor(BLANK)
        UIManager.addUIElement(_root)

        label_("Battle Test", 20, 34.0, WHITE)
        label_("Click a box to cycle it.  Fighters on the left side are yours to command; the right side is AI.", 62, 16.0, LIGHTGRAY)

        var y = 92
        label_("Your side", y, 22.0, WHITE)
        y = y + 30
        _partyRows = []
        for (i in 0...SLOTS_PER_SIDE) {
            _partyRows.add(row_(y))
            y = y + ROW_HEIGHT
        }

        y = y + 8
        label_("Opponents", y, 22.0, WHITE)
        y = y + 30
        _enemyRows = []
        for (i in 0...SLOTS_PER_SIDE) {
            _enemyRows.add(row_(y))
            y = y + ROW_HEIGHT
        }

        _resultLabel = label_("", y + 4, 18.0, LIGHTGRAY)

        var by = y + 34
        _startButton = ButtonUI.new(_root, "Start battle", Vec2.new(-(200 + GAP / 2), by), Vec2.new(0.5, 0),
                                    Vec2.new(200, 40), font)
        _backButton = ButtonUI.new(_root, "Back to game", Vec2.new(GAP / 2, by), Vec2.new(0.5, 0),
                                   Vec2.new(200, 40), font)
        UIManager.setFocus(_startButton.panel)

        // The labels keep their own reference (same pattern as UI/MainMenu/MainMenuUI.wren).
        font.unload()
        _font = null
    }

    partyRows { _partyRows }
    enemyRows { _enemyRows }
    startButton { _startButton }
    backButton { _backButton }

    setResult(text) {
        _resultLabel.setText(text)
        _resultLabel.setBoundingBoxToText()
    }

    // The whole screen on/off - hidden while a test battle is running.
    show(visible) {
        _root.setVisible(visible)
        if (visible) {
            UIManager.setFocus(_startButton.panel)
        }
    }

    teardown() {
        if (_root != null) {
            UIManager.removeUIElement(_root)
            _root = null
        }
    }

    // A centred line of text `y` px from the top.
    label_(text, y, size, color) {
        var label = UILabel.new(Vec2.new(0, y), Vec2.new(0.5, 0), text, size)
        label.setAnchorPoint(Vec2.new(0.5, 0))
        label.setTextColor(color)
        label.setFont(_font)
        label.setBoundingBoxToText()
        _root.addChild(label)
        return label
    }

    // One fighter row, centred, `y` px from the top.
    row_(y) {
        var x = -ROW_WIDTH / 2
        var fighter = ButtonUI.new(_root, "", Vec2.new(x, y), Vec2.new(0.5, 0), Vec2.new(FIGHTER_WIDTH, BUTTON_HEIGHT), _font)
        x = x + FIGHTER_WIDTH + GAP
        var moves = []
        for (i in 0...MOVES_PER_FIGHTER) {
            moves.add(ButtonUI.new(_root, "", Vec2.new(x, y), Vec2.new(0.5, 0), Vec2.new(MOVE_WIDTH, BUTTON_HEIGHT), _font))
            x = x + MOVE_WIDTH + GAP
        }
        var stats = UILabel.new(Vec2.new(-ROW_WIDTH / 2, y + BUTTON_HEIGHT + 3), Vec2.new(0.5, 0), "", 14.0)
        stats.setTextColor(LIGHTGRAY)
        stats.setFont(_font)
        stats.setBoundingBoxToText()
        _root.addChild(stats)
        return SetupRow.new(fighter, moves, stats)
    }
}
