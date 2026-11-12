// UI/Combat/ParryPromptUI.wren
// The parry decision prompt shown while States/Combat/ParryPromptState.wren is active: an
// offensive move is about to land and the target gets one discrete choice, attempt a parry or not.
// It takes the move menu's place in the combat screen's command bar (UI/Combat/CombatUI.wren - the two are never up
// at once) and shows, top to bottom, who is attacking and with what ("Brute's Cleave - Parry?"), a timing bar that
// drains as the attack closes in (how long is left to choose - the same game time the attack animation runs on, so
// it crawls while time is slowed), and the Parry / No buttons. (The attacker's card is also ringed - see
// CombatSession.parryAttacker.)
// Built from two UI/Theme/KitButton buttons, so the usual UIAccept/UIDir/UICancel navigation just
// works - no dedicated input binding needed. Pure view: ParryPromptState supplies the callbacks and the
// remaining fraction, and decides when to show/hide it.
import "ui" for UIManager, UIPanel
import "math" for Vec2, Vec4
import "Util/Colors" for BLANK
import "UI/Theme/UIKit" for UIKit
import "UI/Theme/KitBar" for KitBar
import "UI/Theme/KitButton" for KitButton

var BUTTON_GAP    = 12
var BUTTON_HEIGHT = 44
var BAR_HEIGHT    = 15        // the kit track's 3px rim plus a 9px fill
var LABEL_Y       = 0
var BAR_Y         = 36

var BAR_FILL_COLOR = Vec4.new(255, 200, 87, 255)
var BAR_LOW_COLOR  = Vec4.new(224, 57, 76, 255)
var BAR_LOW_BELOW  = 0.3   // the bar turns red once less than this fraction of the window is left

class ParryPromptUI {
    // x/y/width/height: the area it fills, in parent's pixels - the command bar's command area. The attacker line
    // sits at the top, the timing bar under it, the buttons along the bottom.
    construct new(parent, x, y, width, height, font) {
        _area = UIPanel.new(Vec2.new(x, y), Vec2.new(0, 0), Vec2.new(width, height), Vec2.new(0, 0))
        _area.setBackgroundColor(BLANK)
        _area.setBorderColor(BLANK)
        parent.addChild(_area)

        // Rich text sized to its content, so it centres exactly (-2: the rich label's own top padding).
        _label = UIKit.richText(_area, "Parry?", Vec2.new(0, LABEL_Y - 2), Vec2.new(0.5, 0), Vec2.new(0.5, 0), 20.0,
            UIKit.text, 6)
        _bar = KitBar.new(_area, 0, BAR_Y, width, BAR_HEIGHT, BAR_FILL_COLOR, 5)

        var buttonWidth = (width - BUTTON_GAP) / 2
        var buttonY = height - BUTTON_HEIGHT
        _yesButton = KitButton.new(_area, "Parry", Vec2.new(0, buttonY), Vec2.new(0, 0),
                                   Vec2.new(buttonWidth, BUTTON_HEIGHT), font)
        _noButton = KitButton.new(_area, "No", Vec2.new(buttonWidth + BUTTON_GAP, buttonY), Vec2.new(0, 0),
                                  Vec2.new(buttonWidth, BUTTON_HEIGHT), font)

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
        _label.setMarkupText("%(attackerName)'s %(moveName)  -  Parry?")
        setRemaining(1)
        setVisible(true)
        UIManager.setFocus(_yesButton.panel)
    }

    // fraction: 0..1 of the choosing window still left - the bar drains toward 0 and goes red when it's
    // nearly out.
    setRemaining(fraction) {
        _bar.setFraction(fraction)
        _bar.setColor(_bar.fraction < BAR_LOW_BELOW ? BAR_LOW_COLOR : BAR_FILL_COLOR)
    }

    hide() { setVisible(false) }

    setVisible(visible) { _area.setVisible(visible) }
}
