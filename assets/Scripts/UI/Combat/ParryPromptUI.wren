// UI/Combat/ParryPromptUI.wren
// The parry decision prompt shown while States/Combat/ParryPromptState.wren is active: an
// offensive move is about to land and the target gets one discrete choice, attempt a parry or not.
// From top to bottom it shows who is attacking and with what ("Chinlin's Cleave - Parry?"), a timing
// bar that drains as the attack closes in (how long is left to choose - the same game time the attack
// animation runs on, so it crawls while time is slowed), and the Parry / No buttons. (The attacker's
// stack in UI/Combat/CombatUI is also flagged with its ">>" marker - see CombatSession.parryAttacker.)
// Built from two UI/Shared/ButtonUI buttons, so the usual UIAccept/UIDir/UICancel navigation just
// works - no dedicated input binding needed. Pure view: ParryPromptState supplies the callbacks and the
// remaining fraction, and decides when to show/hide it.
import "ui" for UIManager, UILabel, UIColor
import "math" for Vec2, Vec4
import "Colors" for WHITE
import "UI/Shared/ButtonUI" for ButtonUI

var BUTTON_WIDTH  = 140
var BUTTON_HEIGHT = 40
var BUTTON_GAP    = 16
var BAR_HEIGHT    = 10

var BAR_TRACK_COLOR = Vec4.new(14, 18, 24, 220)
var BAR_FILL_COLOR  = Vec4.new(230, 190, 90, 255)
var BAR_LOW_COLOR   = Vec4.new(225, 70, 60, 255)
var BAR_LOW_BELOW   = 0.3   // the bar turns red once less than this fraction of the window is left

class ParryPromptUI {
    // x/y: the centre point the two buttons straddle; the timing bar and the attacker line stack above it.
    construct new(parent, x, y, font) {
        var half = BUTTON_WIDTH + BUTTON_GAP / 2
        _barWidth = BUTTON_WIDTH * 2 + BUTTON_GAP
        _barX = x - half
        _barY = y - BAR_HEIGHT - 12

        _label = UILabel.new(Vec2.new(x, _barY - 30), Vec2.new(0, 0), "Parry?", 20.0)
        _label.setBoundingBoxToText()
        _label.setAnchorPoint(Vec2.new(0.5, 0))
        _label.setTextColor(WHITE)
        _label.setFont(font)
        parent.addChild(_label)

        _barTrack = UIColor.new(Vec2.new(_barX, _barY), Vec2.new(0, 0), Vec2.new(_barWidth, BAR_HEIGHT), Vec2.new(0, 0))
        _barTrack.setAnchorPoint(Vec2.new(0, 0))
        _barTrack.setColor(BAR_TRACK_COLOR)
        _barTrack.setZIndex(0)
        parent.addChild(_barTrack)

        _barFill = UIColor.new(Vec2.new(_barX, _barY), Vec2.new(0, 0), Vec2.new(_barWidth, BAR_HEIGHT), Vec2.new(0, 0))
        _barFill.setAnchorPoint(Vec2.new(0, 0))
        _barFill.setColor(BAR_FILL_COLOR)
        _barFill.setZIndex(1)
        parent.addChild(_barFill)

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

    // attackerName / moveName: whose attack this is and what it is. onYes/onNo: zero-arg Fn, called once when
    // the corresponding button is clicked/confirmed.
    show(attackerName, moveName, onYes, onNo) {
        _onYes = onYes
        _onNo = onNo
        _label.setText("%(attackerName)'s %(moveName)  -  Parry?")
        _label.setBoundingBoxToText()
        setRemaining(1)
        setVisible(true)
        UIManager.setFocus(_yesButton.panel)
    }

    // fraction: 0..1 of the choosing window still left - the bar drains toward 0 and goes red when it's
    // nearly out.
    setRemaining(fraction) {
        var f = fraction
        if (f < 0) {
            f = 0
        }
        if (f > 1) {
            f = 1
        }
        _barFill.setSize(Vec2.new(_barWidth * f, BAR_HEIGHT), Vec2.new(0, 0))
        _barFill.setColor(f < BAR_LOW_BELOW ? BAR_LOW_COLOR : BAR_FILL_COLOR)
    }

    hide() { setVisible(false) }

    setVisible(visible) {
        _label.setVisible(visible)
        _barTrack.setVisible(visible)
        _barFill.setVisible(visible)
        _yesButton.panel.setVisible(visible)
        _noButton.panel.setVisible(visible)
    }
}
