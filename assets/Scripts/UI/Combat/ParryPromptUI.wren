// UI/Combat/ParryPromptUI.wren
// The parry decision prompt shown while States/CombatState.wren is in its "parryPrompt" phase: an
// offensive move is about to land and the target gets one discrete choice, attempt a parry or not.
// Built from two UI/Shared/ButtonUI buttons, so the usual UIAccept/UIDir/UICancel navigation just
// works - no dedicated input binding needed. Pure view: CombatState supplies the callbacks and
// decides when to show/hide it.
import "ui" for UIManager, UILabel
import "math" for Vec2
import "Colors" for WHITE
import "UI/Shared/ButtonUI" for ButtonUI

var BUTTON_WIDTH  = 140
var BUTTON_HEIGHT = 40
var BUTTON_GAP    = 16

class ParryPromptUI {
    // x/y: the centre point the two buttons straddle (label sits just above it).
    construct new(parent, x, y, font) {
        _label = UILabel.new(Vec2.new(x, y - 26), Vec2.new(0, 0), "Parry?", 20.0)
        _label.setBoundingBoxToText()
        _label.setAnchorPoint(Vec2.new(0.5, 0))
        _label.setTextColor(WHITE)
        _label.setFont(font)
        parent.addChild(_label)

        var half = BUTTON_WIDTH + BUTTON_GAP / 2
        _yesButton = ButtonUI.new(parent, "Parry", Vec2.new(x - half, y), Vec2.new(0, 0),
                                  Vec2.new(BUTTON_WIDTH, BUTTON_HEIGHT), font)
        _noButton = ButtonUI.new(parent, "No", Vec2.new(x + BUTTON_GAP / 2, y), Vec2.new(0, 0),
                                 Vec2.new(BUTTON_WIDTH, BUTTON_HEIGHT), font)

        // setOnClick is registered exactly once here, not per show() - the underlying native callback
        // wrapper asserts on disposal if a button's click handler is ever replaced instead of set once
        // (see every other button in this codebase, all wired once in their owning State's enter()).
        // _onYes/_onNo are plain Wren fields show() reassigns per prompt instead.
        _onYes = null
        _onNo = null
        _yesButton.setOnClick { |s, m|
            if (_onYes != null) {
                _onYes.call()
            }
        }
        _noButton.setOnClick { |s, m|
            if (_onNo != null) {
                _onNo.call()
            }
        }
        setVisible(false)
    }

    // onYes/onNo: zero-arg Fn, called once when the corresponding button is clicked/confirmed.
    show(onYes, onNo) {
        _onYes = onYes
        _onNo = onNo
        setVisible(true)
        UIManager.setFocus(_yesButton.panel)
    }

    hide() { setVisible(false) }

    setVisible(visible) {
        _label.setVisible(visible)
        _yesButton.panel.setVisible(visible)
        _noButton.panel.setVisible(visible)
    }
}
