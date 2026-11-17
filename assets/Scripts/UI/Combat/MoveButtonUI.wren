// UI/Combat/MoveButtonUI.wren
// A command-panel button in the UI kit's "button" art (like UI/Theme/KitButton.wren), with two texts: a name on the
// left and a short readout on the right ("1.0s  9sp"). Disabled buttons stay focusable - so the player can still
// read why a move can't be added - but draw muted.
import "ui" for UILabel
import "math" for Vec2, Vec4
import "UI/Theme/UIKit" for UIKit

var PAD_X = 12
var DISABLED_TEXT = Vec4.new(150, 140, 125, 255)

class MoveButtonUI {
    construct new(parent, absPos, size, font) {
        _panel = UIKit.frame(parent, "button", absPos, Vec2.new(0, 0), size, Vec2.new(0, 0), 5)
        _panel.setFocusable(true)
        _enabled = true
        _focused = false

        _name = UILabel.new(Vec2.new(PAD_X - 5, 0), Vec2.new(0, 0.5), "", 19.0)
        _name.setFont(font)
        _name.setBoundingBoxToText()
        _name.setAnchorPoint(Vec2.new(0, 0.5))
        _name.setZIndex(6)
        _panel.addChild(_name)

        _info = UILabel.new(Vec2.new(-PAD_X + 5, 0), Vec2.new(1, 0.5), "", 19.0)
        _info.setFont(font)
        _info.setBoundingBoxToText()
        _info.setAnchorPoint(Vec2.new(1, 0.5))
        _info.setZIndex(6)
        _panel.addChild(_info)
        _nameText = ""
        _infoText = ""

        _panel.setOnFocus { |s|
            _focused = true
            UIKit.applyStyle(_panel, "buttonHover")
            recolour_()
            if (_onFocused != null) _onFocused.call()
        }
        _panel.setOnLoseFocus { |s|
            _focused = false
            UIKit.applyStyle(_panel, _enabled ? "button" : "buttonDisabled")
            recolour_()
        }
        recolour_()
    }

    panel { _panel }
    enabled { _enabled }

    setTexts(name, info) {
        if (name != _nameText) {
            _nameText = name
            _name.setText(name)
            _name.setBoundingBoxToText()
        }
        if (info != _infoText) {
            _infoText = info
            _info.setText(info)
            _info.setBoundingBoxToText()
        }
    }

    setEnabled(enabled) {
        if (enabled == _enabled) return
        _enabled = enabled
        if (!_focused) UIKit.applyStyle(_panel, enabled ? "button" : "buttonDisabled")
        recolour_()
    }

    recolour_() {
        var color = _focused ? UIKit.textDark : (_enabled ? UIKit.text : DISABLED_TEXT)
        _name.setTextColor(color)
        _info.setTextColor(color)
    }

    setOnClick(fn) { _panel.setOnClick(fn) }
    // Called (no arguments) whenever the button gains focus.
    setOnFocused(fn) { _onFocused = fn }
}
