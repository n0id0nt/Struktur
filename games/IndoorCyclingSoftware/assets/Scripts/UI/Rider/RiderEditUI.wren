// UI/Rider/RiderEditUI.wren
// The rider editor (States/RiderEditState.wren owns it): rows for name, units, weight, height, FTP and threshold
// heart rate, Save / Cancel /
// Delete, and a name keyboard that opens over the screen. Every row but the name changes with Left/Right on
// row (the state reads the direction; this view only reports which row has focus). The keyboard is for pads -
// a physical keyboard types straight in while it is open (the state runs Input text entry). Pure view: clicks go
// back through the callbacks the state sets. Built from the UI kit, so the owner brackets it with UIKit.open/close.
import "app" for Application
import "math" for Vec2
import "ui" for UIManager, UIPanel, NavigationDirection
import "localization" for Localization
import "UI/Theme/UIKit" for UIKit
import "UI/Theme/KitButton" for KitButton
import "UI/Rider/RiderTheme" for RiderTheme

var WINDOW_SIZE = Vec2.new(760, 690)
var ROW_SIZE = Vec2.new(560, 48)
var ROW_TOP = 104
var ROW_STEP = 54
var BUTTON_SIZE = Vec2.new(180, 52)
var BUTTON_TOP = 528
var BUTTON_GAP = 20

var KEYBOARD_SIZE = Vec2.new(680, 470)
var KEY_SIZE = Vec2.new(52, 46)
var KEY_GAP = 6
var KEY_ROWS = ["1234567890", "QWERTYUIOP", "ASDFGHJKL'", "ZXCVBNM-."]
var KEY_TOP = 150
var WIDE_KEY_SIZE = Vec2.new(142, 46)

// Field rows, in order; every one but "name" responds to Left/Right.
var FIELDS = ["name", "units", "weight", "height", "ftp", "maxHeartRate", "thresholdHeartRate"]

class RiderEditUI {
    // isNew picks the title; canDelete adds the Delete button.
    construct new(isNew, canDelete) {
        _font = UIKit.font()
        _focusedField = null
        _keyboard = null

        _root = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(Application.gameWidth, Application.gameHeight),
            Vec2.new(0, 0))
        _root.setBackgroundColor(RiderTheme.background)
        _root.setBorderColor(RiderTheme.clear)
        _root.setZIndex(RiderTheme.screenZ)
        UIManager.addUIElement(_root)

        _window = UIKit.frame(_root, "window", Vec2.new(0, 0), Vec2.new(0.5, 0.5), WINDOW_SIZE, Vec2.new(0, 0),
            RiderTheme.frameZ)
        _window.setAnchorPoint(Vec2.new(0.5, 0.5))
        RiderTheme.label(_window, _font, Localization.get(isNew ? "menu.rider.new_title" : "menu.rider.edit_title"),
            26, 36.0, UIKit.accent)

        _fieldButtons = {}
        for (i in 0...FIELDS.count) {
            var field = FIELDS[i]
            var button = KitButton.new(_window, "", Vec2.new(-ROW_SIZE.x / 2, ROW_TOP + i * ROW_STEP), Vec2.new(0.5, 0),
                ROW_SIZE, _font)
            button.setOnFocused(Fn.new { _focusedField = field })
            button.setOnClick { |sender, mousePos|
                if (_onFieldChosen != null) _onFieldChosen.call(field)
            }
            _fieldButtons[field] = button
        }

        _requirementLabel = RiderTheme.label(_window, _font, "", ROW_TOP + FIELDS.count * ROW_STEP + 6, 18.0,
            RiderTheme.warning)

        var buttonDefs = [["save", "menu.rider.save"], ["cancel", "menu.rider.cancel"]]
        if (canDelete) buttonDefs.add(["delete", "menu.rider.delete"])
        var buttonsWidth = BUTTON_SIZE.x * buttonDefs.count + BUTTON_GAP * (buttonDefs.count - 1)
        _buttons = []
        for (i in 0...buttonDefs.count) {
            var action = buttonDefs[i][0]
            var button = KitButton.new(_window, Localization.get(buttonDefs[i][1]),
                Vec2.new(-buttonsWidth / 2 + i * (BUTTON_SIZE.x + BUTTON_GAP), BUTTON_TOP), Vec2.new(0.5, 0),
                BUTTON_SIZE, _font)
            button.setOnFocused(Fn.new { _focusedField = null })
            button.setOnClick { |sender, mousePos|
                if (button.isEnabled && _onButton != null) _onButton.call(action)
            }
            _buttons.add(button)
            if (action == "save") _saveButton = button
        }

        UIKit.richText(_window, Localization.get("menu.rider.edit_hints"), Vec2.new(0, -24), Vec2.new(0.5, 1),
            Vec2.new(0.5, 1), 18.0, UIKit.text, RiderTheme.textZ)

        linkNavigation_()
        UIManager.setFocus(_fieldButtons["name"].panel)
    }

    // --- callbacks the state sets ------------------------------------------------------------------------------
    onFieldChosen=(fn) { _onFieldChosen = fn }   // fn.call(field) - Accept on a field row
    onButton=(fn) { _onButton = fn }             // fn.call("save" | "cancel" | "delete")
    onKey=(fn) { _onKey = fn }                   // fn.call(keyName) - "char:X", "shift", "space", "delete", "done"

    // The field row with focus (one of FIELDS) or null.
    focusedField { _focusedField }
    isKeyboardOpen { _keyboard != null }
    // Where focus goes back to after a popup closes.
    deleteButtonPanel { _buttons[-1].panel }

    // --- editor ------------------------------------------------------------------------------------------------

    setField(field, text) { _fieldButtons[field].setText(text) }

    setCanSave(canSave) {
        _saveButton.setEnabled(canSave)
        RiderTheme.setText(_requirementLabel, canSave ? "" : Localization.get("menu.rider.need_name"))
    }

    // --- name keyboard -----------------------------------------------------------------------------------------

    openKeyboard() {
        if (_keyboard != null) return
        _keyboard = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(Application.gameWidth, Application.gameHeight),
            Vec2.new(0, 0))
        _keyboard.setBackgroundColor(RiderTheme.overlay)
        _keyboard.setBorderColor(RiderTheme.clear)
        _keyboard.setZIndex(RiderTheme.overlayZ)
        UIManager.addUIElement(_keyboard)

        var box = UIKit.frame(_keyboard, "window", Vec2.new(0, 0), Vec2.new(0.5, 0.5), KEYBOARD_SIZE, Vec2.new(0, 0),
            RiderTheme.frameZ)
        box.setAnchorPoint(Vec2.new(0.5, 0.5))
        RiderTheme.label(box, _font, Localization.get("menu.rider.keyboard_title"), 22, 30.0, UIKit.accent)

        var well = UIKit.frame(box, "well", Vec2.new(-260, 70), Vec2.new(0.5, 0), Vec2.new(520, 56), Vec2.new(0, 0),
            RiderTheme.wellZ)
        _typedLabel = RiderTheme.label(well, _font, "_", 12, 27.0, UIKit.text)

        _letterKeys = []
        _keyRows = []
        for (rowIndex in 0...KEY_ROWS.count) {
            var characters = KEY_ROWS[rowIndex]
            var rowWidth = characters.count * KEY_SIZE.x + (characters.count - 1) * KEY_GAP
            var keys = []
            for (column in 0...characters.count) {
                var character = characters[column]
                var position = Vec2.new(-rowWidth / 2 + column * (KEY_SIZE.x + KEY_GAP),
                    KEY_TOP + rowIndex * (KEY_SIZE.y + KEY_GAP))
                var key = keyButton_(box, character, position, KEY_SIZE, "char:" + character)
                keys.add(key)
                if (character.bytes[0] >= 65 && character.bytes[0] <= 90) {
                    _letterKeys.add([key, character])
                }
            }
            _keyRows.add(keys)
        }

        var bottomNames = [["shift", "menu.rider.key_shift"], ["space", "menu.rider.key_space"],
            ["delete", "menu.rider.key_delete"], ["done", "menu.rider.key_done"]]
        var bottomWidth = bottomNames.count * WIDE_KEY_SIZE.x + (bottomNames.count - 1) * KEY_GAP
        var bottomTop = KEY_TOP + KEY_ROWS.count * (KEY_SIZE.y + KEY_GAP) + 6
        var bottomKeys = []
        for (i in 0...bottomNames.count) {
            var position = Vec2.new(-bottomWidth / 2 + i * (WIDE_KEY_SIZE.x + KEY_GAP), bottomTop)
            bottomKeys.add(keyButton_(box, Localization.get(bottomNames[i][1]), position, WIDE_KEY_SIZE,
                bottomNames[i][0]))
        }
        _keyRows.add(bottomKeys)

        UIKit.richText(box, Localization.get("menu.rider.keyboard_hint"), Vec2.new(0, -22), Vec2.new(0.5, 1),
            Vec2.new(0.5, 1), 18.0, UIKit.textMuted, RiderTheme.textZ)

        linkKeyboardNavigation_()
        UIManager.setFocus(_keyRows[1][0].panel)
    }

    // text: the name so far; shifted: letter keys show capitals.
    setKeyboardText(text, shifted) {
        if (_keyboard == null) return
        RiderTheme.setText(_typedLabel, text + "_")
        for (entry in _letterKeys) {
            entry[0].setText(shifted ? entry[1] : lowerCase_(entry[1]))
        }
    }

    closeKeyboard() {
        if (_keyboard == null) return
        UIManager.removeUIElement(_keyboard)
        _keyboard = null
        _typedLabel = null
        _letterKeys = []
        _keyRows = []
        UIManager.setFocus(_fieldButtons["name"].panel)
    }

    teardown() {
        closeKeyboard()
        if (_root != null) {
            UIManager.removeUIElement(_root)
            _root = null
        }
        _window = null
        _fieldButtons = {}
        _buttons = []
        _saveButton = null
        _requirementLabel = null
        if (_font != null) {
            _font.unload()
            _font = null
        }
    }

    // --- building ----------------------------------------------------------------------------------------------

    keyButton_(parent, text, position, size, keyName) {
        var key = KitButton.new(parent, text, position, Vec2.new(0.5, 0), size, _font)
        key.setOnClick { |sender, mousePos|
            if (_onKey != null) _onKey.call(keyName)
        }
        return key
    }

    lowerCase_(character) {
        var code = character.bytes[0]
        return code >= 65 && code <= 90 ? String.fromByte(code + 32) : character
    }

    linkNavigation_() {
        var firstButton = _buttons[0].panel
        for (i in 0...FIELDS.count) {
            var panel = _fieldButtons[FIELDS[i]].panel
            var above = i > 0 ? _fieldButtons[FIELDS[i - 1]].panel : panel
            var below = i < FIELDS.count - 1 ? _fieldButtons[FIELDS[i + 1]].panel : firstButton
            panel.setNavigationNeighbor(NavigationDirection.UP, above)
            panel.setNavigationNeighbor(NavigationDirection.DOWN, below)
            // Left/Right change the value - never move focus off a field row.
            panel.setNavigationNeighbor(NavigationDirection.LEFT, panel)
            panel.setNavigationNeighbor(NavigationDirection.RIGHT, panel)
        }
        var lastField = _fieldButtons[FIELDS[-1]].panel
        for (i in 0..._buttons.count) {
            var panel = _buttons[i].panel
            panel.setNavigationNeighbor(NavigationDirection.UP, lastField)
            panel.setNavigationNeighbor(NavigationDirection.DOWN, panel)
            panel.setNavigationNeighbor(NavigationDirection.LEFT, i > 0 ? _buttons[i - 1].panel : panel)
            panel.setNavigationNeighbor(NavigationDirection.RIGHT, i < _buttons.count - 1 ? _buttons[i + 1].panel : panel)
        }
    }

    // A grid: columns line up between letter rows; moving onto/off the wide bottom row maps by horizontal position.
    linkKeyboardNavigation_() {
        for (rowIndex in 0..._keyRows.count) {
            var keys = _keyRows[rowIndex]
            for (column in 0...keys.count) {
                var panel = keys[column].panel
                panel.setNavigationNeighbor(NavigationDirection.LEFT, column > 0 ? keys[column - 1].panel : panel)
                panel.setNavigationNeighbor(NavigationDirection.RIGHT,
                    column < keys.count - 1 ? keys[column + 1].panel : panel)
                var above = rowIndex > 0 ? keyInRow_(rowIndex - 1, column, keys.count) : panel
                var below = rowIndex < _keyRows.count - 1 ? keyInRow_(rowIndex + 1, column, keys.count) : panel
                panel.setNavigationNeighbor(NavigationDirection.UP, above)
                panel.setNavigationNeighbor(NavigationDirection.DOWN, below)
            }
        }
    }

    // The key in `rowIndex` under column `column` of a row that has `fromCount` keys.
    keyInRow_(rowIndex, column, fromCount) {
        var keys = _keyRows[rowIndex]
        var target = ((column + 0.5) * keys.count / fromCount).floor
        if (target >= keys.count) target = keys.count - 1
        return keys[target].panel
    }
}
