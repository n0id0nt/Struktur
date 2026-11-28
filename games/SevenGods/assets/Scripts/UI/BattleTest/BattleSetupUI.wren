// UI/BattleTest/BattleSetupUI.wren
// The battle test's setup screen (States/BattleTestState.wren), in the UI kit's look: on the left the six fighter
// slots (three on your side, three opponents); on the right everything about the selected slot, as click-to-cycle
// pickers in three columns -
//   gear      the fighter, its stats tier, main hand, off-hand, armour, gear quality, move tier
//   elements  weapon element, armour element, two weapon effects and two armour effects (each an effect + a rank)
//   moves     the five loadout moves (only what the equipment allows)
// - then what that adds up to (tax per move, the weapon x armour pairing, Resonance), the scenario and Soul Sight level,
// and Start / Back with the last result.
//
// Pure view: every button is a picker the state wires and relabels. Pickers are named by id (PICKERS below).
import "math" for Vec2, Vec4
import "ui" for UIManager, UIPanel, NavigationDirection
import "app" for Application
import "UI/Theme/UIKit" for UIKit
import "UI/Theme/KitButton" for KitButton

var SLOTS_PER_SIDE = 3
var MOVE_SLOTS = 5   // the player's loadout size (Combat/MoveBook.wren)

var SLOT_WIDTH = 250
var SLOT_HEIGHT = 34
var PICKER_HEIGHT = 30
var ROW_STRIDE = 35
var COLUMN_X = [20, 322, 624]
var COLUMN_WIDTH = 290
var EFFECT_WIDTH = 214
var RANK_WIDTH = 70
var PANEL_X = 300
var PANEL_Y = 86
var PANEL_WIDTH = 956
var PANEL_HEIGHT = 420
var BACKDROP = Vec4.new(24, 28, 30, 255)

// The pickers of each column, top to bottom. An effect picker sits beside its rank picker ("wfx0" + "wfx0r").
var GEAR_PICKERS = ["fighter", "stats", "main", "off", "armour", "quality", "tier"]
var ELEMENT_PICKERS = ["welem", "aelem", "wfx0", "wfx1", "afx0", "afx1"]
var MOVE_PICKERS = ["move0", "move1", "move2", "move3", "move4", "move5"]
var GLOBAL_PICKERS = ["scenario", "soul"]

class BattleSetupUI {
    construct new() {
        UIKit.open()
        var font = UIKit.font()
        _font = font
        var gw = Application.gameWidth
        var gh = Application.gameHeight
        _onFocus = null

        _root = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(gw, gh), Vec2.new(0, 0))
        _root.setBackgroundColor(BACKDROP)
        _root.setBorderColor(BACKDROP)
        UIManager.addUIElement(_root)

        text_(_root, "Battle Test", Vec2.new(24, 16), 27.0, UIKit.accent)
        text_(_root, "[input=UIAccept] Next value    [input=UITab] Previous / next    [input=UIDir] Move",
            Vec2.new(24, 54), 18.0, UIKit.textMuted)

        // The six slots.
        _slotButtons = []
        _slotRings = []
        text_(_root, "Your side", Vec2.new(24, PANEL_Y), 20.0, UIKit.text)
        for (i in 0...SLOTS_PER_SIDE) {
            addSlot_(24, PANEL_Y + 30 + i * (SLOT_HEIGHT + 6), font)
        }
        var enemyY = PANEL_Y + 30 + SLOTS_PER_SIDE * (SLOT_HEIGHT + 6) + 16
        text_(_root, "Opponents (AI)", Vec2.new(24, enemyY), 20.0, UIKit.text)
        for (i in 0...SLOTS_PER_SIDE) {
            addSlot_(24, enemyY + 30 + i * (SLOT_HEIGHT + 6), font)
        }

        // The selected slot's pickers.
        _panel = UIKit.frame(_root, "window", Vec2.new(PANEL_X, PANEL_Y), Vec2.new(0, 0), Vec2.new(PANEL_WIDTH, PANEL_HEIGHT),
            Vec2.new(0, 0), 1)
        _panelTitle = text_(_panel, "", Vec2.new(COLUMN_X[0] + 4, 18), 20.0, UIKit.accent)
        _pickers = {}
        var top = 78
        text_(_panel, "Gear", Vec2.new(COLUMN_X[0] + 4, top - 26), 18.0, UIKit.textMuted)
        text_(_panel, "Elements and custom effects", Vec2.new(COLUMN_X[1] + 4, top - 26), 18.0, UIKit.textMuted)
        text_(_panel, "Moves (5 + Hold)", Vec2.new(COLUMN_X[2] + 4, top - 26), 18.0, UIKit.textMuted)
        for (i in 0...GEAR_PICKERS.count) {
            addPicker_(GEAR_PICKERS[i], COLUMN_X[0], top + i * ROW_STRIDE, COLUMN_WIDTH)
        }
        for (i in 0...ELEMENT_PICKERS.count) {
            var id = ELEMENT_PICKERS[i]
            var y = top + i * ROW_STRIDE
            if (i < 2) {
                addPicker_(id, COLUMN_X[1], y, COLUMN_WIDTH)
            } else {
                addPicker_(id, COLUMN_X[1], y, EFFECT_WIDTH)
                addPicker_(id + "r", COLUMN_X[1] + EFFECT_WIDTH + 6, y, RANK_WIDTH)
            }
        }
        for (i in 0...MOVE_PICKERS.count) {
            addPicker_(MOVE_PICKERS[i], COLUMN_X[2], top + i * ROW_STRIDE, COLUMN_WIDTH)
        }
        // What it adds up to.
        _info = []
        for (i in 0...3) {
            _info.add(text_(_panel, "", Vec2.new(COLUMN_X[0] + 4, top + 7 * ROW_STRIDE + 6 + i * 24), 18.0, UIKit.text))
        }

        // Scenario, Soul Sight level, Start, Back.
        var bottomY = PANEL_Y + PANEL_HEIGHT + 16
        _globals = UIPanel.new(Vec2.new(PANEL_X, bottomY), Vec2.new(0, 0), Vec2.new(PANEL_WIDTH, 120), Vec2.new(0, 0))
        _globals.setBackgroundColor(BACKDROP)
        _globals.setBorderColor(BACKDROP)
        _root.addChild(_globals)
        _pickers["scenario"] = KitButton.new(_globals, "", Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(400, 34), font, 18.0)
        _pickers["soul"] = KitButton.new(_globals, "", Vec2.new(412, 0), Vec2.new(0, 0), Vec2.new(240, 34), font, 18.0)
        watch_("scenario")
        watch_("soul")
        _startButton = KitButton.new(_globals, "Start battle", Vec2.new(0, 46), Vec2.new(0, 0), Vec2.new(240, 40), font)
        _backButton = KitButton.new(_globals, "Back to game", Vec2.new(252, 46), Vec2.new(0, 0), Vec2.new(240, 40), font)
        _result = text_(_globals, "", Vec2.new(508, 54), 18.0, UIKit.textMuted)

        link_()
        UIManager.setFocus(_startButton.panel)
    }

    text_(parent, markup, pos, size, color) {
        return UIKit.richText(parent, markup, Vec2.new(pos.x - 5, pos.y - 2), Vec2.new(0, 0), Vec2.new(0, 0), size, color, 6)
    }

    addSlot_(x, y, font) {
        var index = _slotButtons.count
        var button = KitButton.new(_root, "", Vec2.new(x, y), Vec2.new(0, 0), Vec2.new(SLOT_WIDTH, SLOT_HEIGHT), font, 18.0)
        button.setOnFocused(Fn.new {
            if (_onFocus != null) _onFocus.call("slot", index)
        })
        _slotButtons.add(button)
        _slotRings.add(UIKit.focusRing(button.panel, 8))
    }

    addPicker_(id, x, y, width) {
        _pickers[id] = KitButton.new(_panel, "", Vec2.new(x, y), Vec2.new(0, 0), Vec2.new(width, PICKER_HEIGHT), _font, 17.0)
        watch_(id)
    }

    watch_(id) {
        _pickers[id].setOnFocused(Fn.new {
            if (_onFocus != null) _onFocus.call("picker", id)
        })
    }

    // Explicit neighbours everywhere (the engine's spatial search compares top-left corners): the slots form
    // column 0, the three picker columns 1-3, with the globals row and Start / Back underneath.
    link_() {
        var columns = [
            _slotButtons.map { |button| button.panel }.toList,
            GEAR_PICKERS.map { |id| _pickers[id].panel }.toList,
            ELEMENT_PICKERS.map { |id| _pickers[id].panel }.toList,
            MOVE_PICKERS.map { |id| _pickers[id].panel }.toList
        ]
        var scenario = _pickers["scenario"].panel
        var soul = _pickers["soul"].panel
        var start = _startButton.panel
        var back = _backButton.panel
        for (columnIndex in 0...columns.count) {
            var column = columns[columnIndex]
            for (rowIndex in 0...column.count) {
                var panel = column[rowIndex]
                BattleSetupUI.set_(panel, NavigationDirection.UP, rowIndex > 0 ? column[rowIndex - 1] : null)
                BattleSetupUI.set_(panel, NavigationDirection.DOWN, rowIndex + 1 < column.count ? column[rowIndex + 1] : (columnIndex == 0 ? start : scenario))
                var left = columnIndex > 0 ? columns[columnIndex - 1] : null
                var right = columnIndex + 1 < columns.count ? columns[columnIndex + 1] : null
                BattleSetupUI.set_(panel, NavigationDirection.LEFT, left == null ? null : left[rowIndex.min(left.count - 1)])
                BattleSetupUI.set_(panel, NavigationDirection.RIGHT, right == null ? null : right[rowIndex.min(right.count - 1)])
            }
        }
        // The rank pickers hang off their effect picker.
        for (i in 2...ELEMENT_PICKERS.count) {
            var effect = _pickers[ELEMENT_PICKERS[i]].panel
            var rank = _pickers[ELEMENT_PICKERS[i] + "r"].panel
            var moves = columns[3][i.min(columns[3].count - 1)]
            BattleSetupUI.set_(effect, NavigationDirection.RIGHT, rank)
            BattleSetupUI.set_(rank, NavigationDirection.LEFT, effect)
            BattleSetupUI.set_(rank, NavigationDirection.RIGHT, moves)
            BattleSetupUI.set_(moves, NavigationDirection.LEFT, rank)
            BattleSetupUI.set_(rank, NavigationDirection.UP, i > 2 ? _pickers[ELEMENT_PICKERS[i - 1] + "r"].panel : _pickers[ELEMENT_PICKERS[1]].panel)
            BattleSetupUI.set_(rank, NavigationDirection.DOWN, i + 1 < ELEMENT_PICKERS.count ? _pickers[ELEMENT_PICKERS[i + 1] + "r"].panel : scenario)
        }
        var lastGear = columns[1][columns[1].count - 1]
        BattleSetupUI.set_(scenario, NavigationDirection.UP, lastGear)
        BattleSetupUI.set_(scenario, NavigationDirection.DOWN, start)
        BattleSetupUI.set_(scenario, NavigationDirection.LEFT, columns[0][columns[0].count - 1])
        BattleSetupUI.set_(scenario, NavigationDirection.RIGHT, soul)
        BattleSetupUI.set_(soul, NavigationDirection.UP, columns[2][columns[2].count - 1])
        BattleSetupUI.set_(soul, NavigationDirection.DOWN, back)
        BattleSetupUI.set_(soul, NavigationDirection.LEFT, scenario)
        BattleSetupUI.set_(soul, NavigationDirection.RIGHT, null)
        BattleSetupUI.set_(start, NavigationDirection.UP, scenario)
        BattleSetupUI.set_(start, NavigationDirection.DOWN, null)
        BattleSetupUI.set_(start, NavigationDirection.LEFT, columns[0][columns[0].count - 1])
        BattleSetupUI.set_(start, NavigationDirection.RIGHT, back)
        BattleSetupUI.set_(back, NavigationDirection.UP, soul)
        BattleSetupUI.set_(back, NavigationDirection.DOWN, null)
        BattleSetupUI.set_(back, NavigationDirection.LEFT, start)
        BattleSetupUI.set_(back, NavigationDirection.RIGHT, null)
    }

    static set_(element, direction, target) {
        element.setNavigationNeighbor(direction, target != null ? target : element)
    }

    // fn(kind, id): kind "slot" (id = its index) or "picker" (id = its name) - whatever just got focus.
    onFocus=(fn) { _onFocus = fn }

    slotButtons { _slotButtons }
    picker(id) { _pickers[id] }
    pickerIds { GEAR_PICKERS + ELEMENT_PICKERS + ELEMENT_PICKERS[2..-1].map { |id| id + "r" }.toList + MOVE_PICKERS + GLOBAL_PICKERS }
    startButton { _startButton }
    backButton { _backButton }

    setSlotText(index, text) { _slotButtons[index].setText(text) }
    setPickerText(id, text) { _pickers[id].setText(text) }
    setTitle(text) { _panelTitle.setMarkupText(text) }
    setInfo(lines) {
        for (i in 0..._info.count) {
            _info[i].setMarkupText(i < lines.count ? lines[i] : "")
        }
    }
    setResult(text) { _result.setMarkupText(text) }

    // Rings the slot the panel is showing (it keeps the ring while focus is on the pickers).
    setSelected(index) {
        for (i in 0..._slotRings.count) {
            _slotRings[i].setVisible(i == index)
        }
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
            _font.unload()
            _font = null
            UIKit.close()
        }
    }
}
