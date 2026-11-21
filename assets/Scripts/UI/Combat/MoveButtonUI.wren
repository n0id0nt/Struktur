// UI/Combat/MoveButtonUI.wren
// A command-panel button in the UI kit's "button" art (like UI/Theme/KitButton.wren), with two texts: a name and a
// short readout ("1.0s  9sp"). Disabled buttons stay focusable - so the player can still read why a move can't be
// added - but draw muted. Three layouts (setLayout): "row" - name left, readout right; "stacked" - the name over the
// readout, for narrow grid buttons; "center" - just the name, centred (a menu button). inset= pushes the texts right
// (room for an icon on the left); setIcon puts one there (a move's type - UI/Combat/MoveIcons.wren).
import "ui" for UILabel, UITexture
import "math" for Vec2, Vec4
import "UI/Theme/UIKit" for UIKit

var PAD_X = 12
var ICON_X = 10              // the icon's left edge, in from the button's
var ICON_SLOT = 24           // the room an icon takes (the pack's tiles are 24 px), before the texts start
var ICON_GAP = 6
var DISABLED_ICON = Vec4.new(150, 150, 150, 170)
var ICON_TINT = Vec4.new(255, 255, 255, 255)
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
        _layout = "row"
        _inset = 0
        _icon = null       // the icon image, made the first time one is set
        _iconPath = null

        _panel.setOnFocus { |sender|
            _focused = true
            UIKit.applyStyle(_panel, "buttonHover")
            recolour_()
            if (_onFocused != null) _onFocused.call()
        }
        _panel.setOnLoseFocus { |sender|
            _focused = false
            UIKit.applyStyle(_panel, _enabled ? "button" : "buttonDisabled")
            recolour_()
        }
        recolour_()
    }

    panel { _panel }
    enabled { _enabled }

    // Moves and sizes the button (the command panel lays its pool out per screen).
    place(absPos, size) {
        _panel.setPosition(absPos, Vec2.new(0, 0))
        _panel.setSize(size, Vec2.new(0, 0))
    }

    // "row", "stacked" or "center" - see the top of the file.
    setLayout(layout) {
        if (layout == _layout) return
        _layout = layout
        arrange_()
    }

    // Px the texts start further right - room for an icon.
    inset=(px) {
        if (px == _inset) return
        _inset = px
        arrange_()
    }

    arrange_() {
        var left = PAD_X - 5 + _inset
        if (_layout == "center") {
            _name.setPosition(Vec2.new(0, 0), Vec2.new(0.5, 0.5))
            _name.setAnchorPoint(Vec2.new(0.5, 0.5))
            _info.setVisible(false)
        } else if (_layout == "stacked") {
            _name.setPosition(Vec2.new(left, -10), Vec2.new(0, 0.5))
            _name.setAnchorPoint(Vec2.new(0, 0.5))
            _info.setPosition(Vec2.new(left, 11), Vec2.new(0, 0.5))
            _info.setAnchorPoint(Vec2.new(0, 0.5))
            _info.setVisible(true)
        } else {
            _name.setPosition(Vec2.new(left, 0), Vec2.new(0, 0.5))
            _name.setAnchorPoint(Vec2.new(0, 0.5))
            _info.setPosition(Vec2.new(-PAD_X + 5, 0), Vec2.new(1, 0.5))
            _info.setAnchorPoint(Vec2.new(1, 0.5))
            _info.setVisible(true)
        }
    }

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

    // An icon at the button's left, the texts moved over for it - or none (null).
    setIcon(path) {
        if (path == _iconPath) return
        _iconPath = path
        if (path == null) {
            if (_icon != null) _icon.setVisible(false)
            inset = 0
            return
        }
        var texture = UIKit.texture(path)
        if (_icon == null) {
            _icon = UITexture.new(Vec2.new(ICON_X, 0), Vec2.new(0, 0.5), Vec2.new(texture.width, texture.height), Vec2.new(0, 0))
            _icon.setAnchorPoint(Vec2.new(0, 0.5))
            _icon.setZIndex(6)
            _panel.addChild(_icon)
        }
        _icon.setTexture(texture)
        // Centred in the slot (the Hourglass is smaller than the pack's tiles).
        _icon.setPosition(Vec2.new(ICON_X + ((ICON_SLOT - texture.width) / 2).floor, 0), Vec2.new(0, 0.5))
        _icon.setSize(Vec2.new(texture.width, texture.height), Vec2.new(0, 0))
        _icon.setVisible(true)
        inset = ICON_SLOT + ICON_GAP
        recolour_()
    }

    recolour_() {
        var color = _focused ? UIKit.textDark : (_enabled ? UIKit.text : DISABLED_TEXT)
        _name.setTextColor(color)
        _info.setTextColor(color)
        if (_icon != null) _icon.setTint(_enabled || _focused ? ICON_TINT : DISABLED_ICON)
    }

    setOnClick(fn) { _panel.setOnClick(fn) }
    // Called (no arguments) whenever the button gains focus.
    setOnFocused(fn) { _onFocused = fn }
}
