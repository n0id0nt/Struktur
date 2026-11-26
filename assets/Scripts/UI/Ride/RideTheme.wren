// UI/Ride/RideTheme.wren
// The ride screens' look: dark translucent panels, a clean condensed font (Barlow Semi Condensed, SIL OFL - see
// assets/Fonts/BarlowSemiCondensed/OFL.txt) for large numbers readable from the bike, and flat buttons. Deliberately
// separate from the pixel-art UIKit the menus use. Fonts are smooth (not point-sampled), so the engine bakes them at the
// window's pixel density and they stay crisp from 1080p to 4K.
//
//   RideFonts   one shared Font per size for a screen - call unloadAll() in the screen's teardown
//   TextField   a label that only touches the engine when its text or colour actually changes (setters always mark
//               the UI dirty, and the HUD refreshes several times a second)
//   RideButton  a flat, focusable, clickable button
import "math" for Vec2, Vec4
import "resourceManager" for Font
import "ui" for UIPanel, UILabel, TextAlignment, NavigationDirection

var FONT_REGULAR = "Fonts/BarlowSemiCondensed/BarlowSemiCondensed-Medium.ttf"
var FONT_BOLD = "Fonts/BarlowSemiCondensed/BarlowSemiCondensed-Bold.ttf"

var PANEL = Vec4.new(12, 16, 22, 215)
var PANEL_EDGE = Vec4.new(255, 255, 255, 28)
var TEXT = Vec4.new(240, 244, 248, 255)
var MUTED = Vec4.new(150, 162, 176, 255)
var ACCENT = Vec4.new(40, 200, 235, 255)
var GOOD = Vec4.new(80, 210, 110, 255)
var WARN = Vec4.new(245, 170, 50, 255)
var BAD = Vec4.new(235, 80, 70, 255)
var BUTTON = Vec4.new(44, 54, 68, 255)
var BUTTON_FOCUS = Vec4.new(40, 160, 200, 255)
var CLEAR = Vec4.new(0, 0, 0, 0)

class RideTheme {
    static panelColor { PANEL }
    static edgeColor { PANEL_EDGE }
    static text { TEXT }
    static muted { MUTED }
    static accent { ACCENT }
    static good { GOOD }
    static warn { WARN }
    static bad { BAD }
    static button { BUTTON }
    static buttonFocus { BUTTON_FOCUS }
    static clear { CLEAR }

    // [r, g, b] from Ride.state() -> Vec4
    static color(rgb) { rgb == null ? TEXT : Vec4.new(rgb[0], rgb[1], rgb[2], 255) }

    // A translucent panel at absPos/relPos with `anchor` as its own pivot.
    static panel(parent, absPos, relPos, anchor, size, z) {
        var panel = UIPanel.new(absPos, relPos, size, Vec2.new(0, 0))
        panel.setAnchorPoint(anchor)
        panel.setBackgroundColor(PANEL)
        panel.setBorderColor(PANEL_EDGE)
        panel.setBorderWidth(1)
        panel.setZIndex(z)
        if (parent != null) parent.addChild(panel)
        return panel
    }
}

class RideFonts {
    construct new() {
        _regular = {}
        _bold = {}
    }

    regular(size) { fontFrom_(_regular, FONT_REGULAR, size) }
    bold(size) { fontFrom_(_bold, FONT_BOLD, size) }

    fontFrom_(cache, path, size) {
        if (!cache.containsKey(size)) cache[size] = Font.load(path, size)
        return cache[size]
    }

    unloadAll() {
        for (font in _regular.values) font.unload()
        for (font in _bold.values) font.unload()
        _regular = {}
        _bold = {}
    }
}

class TextField {
    // anchor: (0,0) left-top, (0.5,0) centre-top, (1,0) right-top...
    construct new(parent, font, size, absPos, relPos, anchor, color, z) {
        _text = ""
        _color = color
        _label = UILabel.new(absPos, relPos, "", size)
        _label.setFont(font)
        _label.setTextColor(color)
        _label.setAlignment(anchor.x < 0.25 ? TextAlignment.LEFT : (anchor.x > 0.75 ? TextAlignment.RIGHT : TextAlignment.CENTER))
        _label.setBoundingBoxToText()
        _label.setAnchorPoint(anchor)
        _label.setZIndex(z)
        parent.addChild(_label)
    }

    label { _label }

    text=(value) {
        if (value == _text) return
        _text = value
        _label.setText(value)
        _label.setBoundingBoxToText()
    }

    color=(value) {
        if (_color != null && value.x == _color.x && value.y == _color.y && value.z == _color.z && value.w == _color.w) return
        _color = value
        _label.setTextColor(value)
    }

    visible=(value) { _label.setVisible(value) }
}

class RideButton {
    construct new(parent, font, text, absPos, relPos, anchor, size, z) {
        _enabled = true
        _active = false
        _panel = UIPanel.new(absPos, relPos, size, Vec2.new(0, 0))
        _panel.setAnchorPoint(anchor)
        _panel.setBackgroundColor(BUTTON)
        _panel.setBorderColor(PANEL_EDGE)
        _panel.setBorderWidth(1)
        _panel.setZIndex(z)
        _panel.setFocusable(true)
        parent.addChild(_panel)
        _label = TextField.new(_panel, font, size.y * 0.5, Vec2.new(0, 0), Vec2.new(0.5, 0.5), Vec2.new(0.5, 0.5), TEXT, z + 1)
        _label.text = text
        _panel.setOnFocus { |sender| applyLook_(true) }
        _panel.setOnLoseFocus { |sender| applyLook_(false) }
        // The HUD reads its own keys: arrows must never walk focus around the screen.
        for (direction in [NavigationDirection.UP, NavigationDirection.DOWN, NavigationDirection.LEFT, NavigationDirection.RIGHT]) {
            _panel.setNavigationNeighbor(direction, _panel)
        }
    }

    panel { _panel }
    text=(value) { _label.text = value }
    setOnClick(fn) { _panel.setOnClick(fn) }

    // "Active" buttons (ERG ON) keep an accent colour regardless of focus.
    active=(value) {
        if (_active == value) return
        _active = value
        applyLook_(false)
    }

    activeColor=(value) { _activeColor = value }

    applyLook_(focused) {
        if (_active) {
            _panel.setBackgroundColor(_activeColor != null ? _activeColor : GOOD)
        } else {
            _panel.setBackgroundColor(focused ? BUTTON_FOCUS : BUTTON)
        }
    }
}

// A focusable list row with a label on the left and a value on the right (workout list, settings rows). Focus is
// shown by colour; setOnFocused gets a callback when the row gains focus (keyboard or mouse).
class RideListRow {
    construct new(parent, fonts, label, value, absPos, size, z) {
        _panel = UIPanel.new(absPos, Vec2.new(0, 0), size, Vec2.new(0, 0))
        _panel.setBackgroundColor(BUTTON)
        _panel.setBorderColor(PANEL_EDGE)
        _panel.setBorderWidth(1)
        _panel.setZIndex(z)
        _panel.setFocusable(true)
        parent.addChild(_panel)
        var textSize = (size.y * 0.42).round
        _label = TextField.new(_panel, fonts.regular(textSize), textSize, Vec2.new(14, (size.y - textSize * 1.25) / 2),
            Vec2.new(0, 0), Vec2.new(0, 0), TEXT, z + 1)
        _label.text = label
        _value = TextField.new(_panel, fonts.bold(textSize), textSize, Vec2.new(size.x - 14, (size.y - textSize * 1.25) / 2),
            Vec2.new(0, 0), Vec2.new(1, 0), MUTED, z + 1)
        _value.text = value
        _panel.setOnFocus { |sender|
            _panel.setBackgroundColor(BUTTON_FOCUS)
            _value.color = TEXT
            if (_onFocused != null) _onFocused.call()
        }
        _panel.setOnLoseFocus { |sender|
            _panel.setBackgroundColor(BUTTON)
            _value.color = MUTED
        }
    }

    panel { _panel }
    label=(text) { _label.text = text }
    value=(text) { _value.text = text }
    labelColor=(color) { _label.color = color }
    setOnClick(fn) { _panel.setOnClick(fn) }
    setOnFocused(fn) { _onFocused = fn }
}
