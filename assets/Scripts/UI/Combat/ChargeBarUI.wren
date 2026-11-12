// UI/Combat/ChargeBarUI.wren
// The design doc's ASCII "Enemy Move Timeline" made real - a UI-kit bar (UI/Theme/KitBar.wren) like HealthBarUI, but
// it fills toward the right as a combatant charges its committed move, brightens to a "ready" colour the moment the
// bar is full, and flashes an alarm colour while the combatant is staggered by an interrupt (Phase 3).
import "math" for Vec4
import "UI/Theme/KitBar" for KitBar

var FILL_COLOR    = Vec4.new(110, 176, 196, 255)
var READY_COLOR   = Vec4.new(170, 236, 255, 255)
var STAGGER_COLOR = Vec4.new(240, 120, 60, 255)

class ChargeBarUI {
    construct new(parent, x, y, width, height) {
        _bar = KitBar.new(parent, x, y, width, height, FILL_COLOR, 2)
        _bar.setFraction(0)
        _staggered = false
    }

    // fraction: 0..1
    setFraction(fraction) {
        _bar.setFraction(fraction)
        applyColor_()
    }

    setStaggered(staggered) {
        _staggered = staggered
        applyColor_()
    }

    applyColor_() {
        if (_staggered) {
            _bar.setColor(STAGGER_COLOR)
        } else if (_bar.fraction >= 1) {
            _bar.setColor(READY_COLOR)
        } else {
            _bar.setColor(FILL_COLOR)
        }
    }
}
