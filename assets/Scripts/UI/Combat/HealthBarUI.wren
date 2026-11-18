// UI/Combat/HealthBarUI.wren
// A combatant's HP - a UI-kit bar (UI/Theme/KitBar.wren) parented into a caller-supplied UIElement and positioned in
// absolute pixels by the caller. The fill drains toward the left and shifts green -> amber -> red as it empties.
import "math" for Vec4
import "UI/Theme/KitBar" for KitBar

var FILL_HEALTHY  = Vec4.new(110, 196, 84, 255)
var FILL_HURT     = Vec4.new(255, 173, 85, 255)    // the kit's accent orange
var FILL_CRITICAL = Vec4.new(224, 57, 76, 255)     // the kit's HP red

class HealthBarUI {
    construct new(parent, x, y, width, height) {
        _bar = KitBar.new(parent, x, y, width, height, FILL_HEALTHY, 2)
    }

    // fraction: 0..1
    setFraction(fraction) {
        _bar.setFraction(fraction)
        var shown = _bar.fraction
        _bar.setColor(shown > 0.5 ? FILL_HEALTHY : (shown > 0.25 ? FILL_HURT : FILL_CRITICAL))
    }
}
