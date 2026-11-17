// UI/Theme/AlertPopup.wren
// A modal popup in the UI kit (UI/Theme/UIKit.wren), for any screen: the screen dims, and a "window" panel in the
// middle shows a title, a few lines of text, an optional highlighted list, a closing line and one or more buttons.
// It keeps focus until a button is pressed - the arrows only move between its buttons - and gives focus back after.
//
//   var popup = AlertPopup.new({
//       "title": "Moves removed",
//       "body": "These moves don't work with the Hunter's Bow:",   // optional; "\n" starts a new line
//       "lines": ["Slash"],                                        // optional, listed in "linesColor" (red)
//       "footer": "Pick new ones in the Moves tab.",               // optional
//       "buttons": [["OK", null]],                                 // optional - [label, Fn or null] each; one OK
//       "cancel": 0,                                               // optional - the button Esc presses (default: the
//                                                                  //   last; -1 = Esc does nothing)
//       "returnFocus": element,                                    // optional - focused again once it closes
//       "onClose": Fn.new { |index| ... }                          // optional - after it closes: the button pressed
//   })                                                             //   (null if close() shut it)
//
// The owner calls update() every frame while the popup is open and should ignore its own input until isOpen turns
// false - the popup only owns focus, not the keys the owner reads itself. Pressing a button (mouse, Accept, or Esc for
// the cancel button) hides the popup at once and hands focus back, but the teardown and that button's Fn and onClose
// run in the next update() - a button's click arrives from inside the UI manager's own input pass, and removing the
// element being clicked from there isn't safe.
import "app" for Application
import "math" for Vec2, Vec4
import "input" for Input
import "ui" for UIManager, UIPanel, UILabel, NavigationDirection
import "UI/Theme/UIKit" for UIKit

var OVERLAY = Vec4.new(0, 0, 0, 170)
var LINES_COLOR = Vec4.new(240, 110, 95, 255)
var WIDTH = 560
var PAD = 28
var TITLE_SIZE = 30.0
var TITLE_HEIGHT = 44
var TEXT_SIZE = 20.0
var TEXT_LINE = 30
var LIST_SIZE = 24.0
var LIST_LINE = 30
var BUTTON_SIZE = Vec2.new(160, 44)
var BUTTON_GAP = 16
var Z = 20                    // above any screen's own UI

class AlertPopup {
    construct new(config) {
        UIKit.open()
        _font = UIKit.font()
        _returnFocus = config["returnFocus"]
        _onClose = config["onClose"]
        var buttons = config["buttons"] != null ? config["buttons"] : [["OK", null]]
        _actions = buttons.map { |b| b[1] }.toList
        _cancel = config["cancel"] != null ? config["cancel"] : buttons.count - 1
        var body = config["body"] != null ? config["body"].split("\n") : []
        var lines = config["lines"] != null ? config["lines"] : []
        var footer = config["footer"] != null ? config["footer"].split("\n") : []
        var linesColor = config["linesColor"] != null ? config["linesColor"] : LINES_COLOR

        var height = PAD + TITLE_HEIGHT + body.count * TEXT_LINE + lines.count * LIST_LINE +
            (lines.count > 0 ? 14 : 0) + footer.count * TEXT_LINE + 20 + BUTTON_SIZE.y + PAD

        _root = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(Application.gameWidth, Application.gameHeight),
            Vec2.new(0, 0))
        _root.setBackgroundColor(OVERLAY)
        _root.setBorderColor(Vec4.new(0, 0, 0, 0))
        _root.setZIndex(Z)
        UIManager.addUIElement(_root)
        var box = UIKit.frame(_root, "window", Vec2.new(0, 0), Vec2.new(0.5, 0.5), Vec2.new(WIDTH, height), Vec2.new(0, 0),
            Z + 1)
        box.setAnchorPoint(Vec2.new(0.5, 0.5))

        var y = PAD
        label_(box, config["title"], y, TITLE_SIZE, UIKit.accent)
        y = y + TITLE_HEIGHT
        for (line in body) {
            label_(box, line, y, TEXT_SIZE, UIKit.text)
            y = y + TEXT_LINE
        }
        for (line in lines) {
            label_(box, line, y, LIST_SIZE, linesColor)
            y = y + LIST_LINE
        }
        if (lines.count > 0) y = y + 14
        for (line in footer) {
            label_(box, line, y, TEXT_SIZE, UIKit.textMuted)
            y = y + TEXT_LINE
        }
        y = y + 20

        // The buttons in a centred row; left / right walk it, everything else stays put.
        _buttons = []
        var rowWidth = buttons.count * BUTTON_SIZE.x + (buttons.count - 1) * BUTTON_GAP
        for (i in 0...buttons.count) {
            var x = -rowWidth / 2 + i * (BUTTON_SIZE.x + BUTTON_GAP)
            var button = UIKit.frame(box, "tile", Vec2.new(x, y), Vec2.new(0.5, 0), BUTTON_SIZE, Vec2.new(0, 0), Z + 1)
            var text = UILabel.new(Vec2.new(0, 0), Vec2.new(0.5, 0.5), buttons[i][0], 22.0)
            text.setFont(_font)
            text.setTextColor(UIKit.text)
            text.setBoundingBoxToText()
            text.setAnchorPoint(Vec2.new(0.5, 0.5))
            text.setZIndex(Z + 2)
            button.addChild(text)
            button.setFocusable(true)
            var ring = UIKit.focusRing(button, Z + 3)
            button.setOnFocus { |sender| ring.setVisible(true) }
            button.setOnLoseFocus { |sender| ring.setVisible(false) }
            var index = i
            button.setOnClick { |sender, mousePos| press(index) }
            _buttons.add(button)
        }
        for (i in 0..._buttons.count) {
            var button = _buttons[i]
            button.setNavigationNeighbor(NavigationDirection.LEFT, i > 0 ? _buttons[i - 1] : button)
            button.setNavigationNeighbor(NavigationDirection.RIGHT, i + 1 < _buttons.count ? _buttons[i + 1] : button)
            button.setNavigationNeighbor(NavigationDirection.UP, button)
            button.setNavigationNeighbor(NavigationDirection.DOWN, button)
        }
        UIManager.setFocus(_buttons[0])
    }

    isOpen { _root != null }

    // Call every frame while open: finishes a press from the frame before (teardown, the button's Fn, onClose), and
    // Esc (UICancel) presses the cancel button. Returns whether it's still open.
    update() {
        if (_root == null) return false
        if (_pressed != null) {
            var index = _pressed
            _pressed = null
            close_()
            var action = _actions[index]
            if (action != null) action.call()
            if (_onClose != null) _onClose.call(index)
            return false
        }
        if (_cancel >= 0 && _cancel < _buttons.count && Input.isInputJustReleased("UICancel")) press(_cancel)
        return true
    }

    // Presses button `index`: the popup hides and focus goes back now; the rest happens in the next update().
    press(index) {
        if (_root == null || _pressed != null) return
        _pressed = index
        _root.setVisible(false)
        if (_returnFocus != null) UIManager.setFocus(_returnFocus)
    }

    // Shuts the popup without pressing anything (onClose gets null).
    close() {
        if (_root == null) return
        close_()
        if (_onClose != null) _onClose.call(null)
    }

    close_() {
        UIManager.removeUIElement(_root)
        _root = null
        _buttons = []
        _font.unload()
        UIKit.close()
        if (_returnFocus != null) UIManager.setFocus(_returnFocus)
        _returnFocus = null
    }

    label_(parent, text, y, size, color) {
        var label = UILabel.new(Vec2.new(0, y), Vec2.new(0.5, 0), text, size)
        label.setFont(_font)
        label.setTextColor(color)
        label.setBoundingBoxToText()
        label.setAnchorPoint(Vec2.new(0.5, 0))
        label.setZIndex(Z + 2)
        parent.addChild(label)
        return label
    }
}
