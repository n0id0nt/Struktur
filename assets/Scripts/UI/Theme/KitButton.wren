// UI/Theme/KitButton.wren
// A focusable button in the UI kit's "button" art (UI/Theme/UIKit.wren): an orange pill with light text that turns to
// the lighter "buttonHover" pill with dark text while it has focus. Same interface as UI/Shared/ButtonUI (panel,
// setText, setOnClick), so a screen can switch over by swapping the class. Needs UIKit.open() around the screen
// that builds it, like any kit piece.
import "ui" for UILabel, TextAlignment
import "math" for Vec2
import "UI/Theme/UIKit" for UIKit

class KitButton {
    // absPos/relPos/size: like UIPanel's own. labelSize defaults to 19 (the font draws it at 18).
    construct new(parent, text, absPos, relPos, size, font) {
        init_(parent, text, absPos, relPos, size, font, 19.0)
    }

    construct new(parent, text, absPos, relPos, size, font, labelSize) {
        init_(parent, text, absPos, relPos, size, font, labelSize)
    }

    init_(parent, text, absPos, relPos, size, font, labelSize) {
        _enabled = true
        _focused = false
        _panel = UIKit.frame(parent, "button", absPos, relPos, size, Vec2.new(0, 0), 5)
        _panel.setFocusable(true)

        _label = UILabel.new(Vec2.new(0, 0), Vec2.new(0.5, 0.5), text, labelSize)
        _label.setFont(font)
        _label.setTextColor(UIKit.text)
        _label.setAlignment(TextAlignment.CENTER)
        _label.setBoundingBoxToText()
        _label.setAnchorPoint(Vec2.new(0.5, 0.5))
        _label.setZIndex(6)
        _panel.addChild(_label)

        _panel.setOnFocus { |sender|
            _focused = true
            applyLook_()
            if (_onFocused != null) _onFocused.call()
        }
        _panel.setOnLoseFocus { |sender|
            _focused = false
            applyLook_()
        }
    }

    // Normal / focused / disabled art. A disabled button keeps its "buttonDisabled" pill even with focus (its text
    // brightens so the player can still see where focus is); clicks still arrive, so the owner checks isEnabled.
    applyLook_() {
        if (!_enabled) {
            UIKit.applyStyle(_panel, "buttonDisabled")
            _label.setTextColor(_focused ? UIKit.text : UIKit.textMuted)
        } else if (_focused) {
            UIKit.applyStyle(_panel, "buttonHover")
            _label.setTextColor(UIKit.textDark)
        } else {
            UIKit.applyStyle(_panel, "button")
            _label.setTextColor(UIKit.text)
        }
    }

    // The button's own element - for UIManager.setFocus(button.panel), visibility, navigation links.
    panel { _panel }

    setText(text) {
        _label.setText(text)
        _label.setBoundingBoxToText()
    }

    setOnClick(fn) { _panel.setOnClick(fn) }

    isEnabled { _enabled }

    setEnabled(enabled) {
        if (_enabled == enabled) return
        _enabled = enabled
        applyLook_()
    }

    // Called (no arguments) whenever the button gains focus, after its hover look is applied - the panel's own
    // setOnFocus is taken by that, so use this instead.
    setOnFocused(fn) { _onFocused = fn }
}
