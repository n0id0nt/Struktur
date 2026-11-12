// UI/Combat/StaminaBarUI.wren
// Phase 4's resource readout - a UI-kit bar (UI/Theme/KitBar.wren) like HealthBarUI/ChargeBarUI; a gold fill that
// drops to an alarm red below the exhaustion threshold (20%), where every move costs an extra time unit.
import "math" for Vec4
import "UI/Theme/KitBar" for KitBar

var FILL_COLOR      = Vec4.new(255, 200, 87, 255)
var EXHAUSTED_COLOR = Vec4.new(224, 57, 76, 255)
var EXHAUSTED_BELOW = 0.2

class StaminaBarUI {
    construct new(parent, x, y, width, height) {
        _bar = KitBar.new(parent, x, y, width, height, FILL_COLOR, 2)
    }

    // fraction: 0..1
    setFraction(fraction) {
        _bar.setFraction(fraction)
        _bar.setColor(_bar.fraction < EXHAUSTED_BELOW ? EXHAUSTED_COLOR : FILL_COLOR)
    }
}
