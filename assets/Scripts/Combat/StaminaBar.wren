// Combat/StaminaBar.wren
// Phase 4's resource readout. Same two-rect build as HealthBar/ChargeBar; a gold fill that drops to
// an alarm red below the exhaustion threshold (20%), where every move costs an extra time unit.
import "ui" for UIColor
import "math" for Vec2, Vec4

var TRACK_COLOR     = Vec4.new(24, 20, 10, 220)
var FILL_COLOR      = Vec4.new(215, 175, 70, 235)
var EXHAUSTED_COLOR = Vec4.new(210, 80, 55, 245)
var EXHAUSTED_BELOW = 0.2

class StaminaBar {
    construct new(parent, x, y, width, height) {
        _width = width
        _height = height

        _track = UIColor.new(Vec2.new(x, y), Vec2.new(0, 0), Vec2.new(width, height), Vec2.new(0, 0))
        _track.setAnchorPoint(Vec2.new(0, 0))
        _track.setColor(TRACK_COLOR)
        _track.setZIndex(0)
        parent.addChild(_track)

        _fill = UIColor.new(Vec2.new(x, y), Vec2.new(0, 0), Vec2.new(width, height), Vec2.new(0, 0))
        _fill.setAnchorPoint(Vec2.new(0, 0))
        _fill.setColor(FILL_COLOR)
        _fill.setZIndex(1)
        parent.addChild(_fill)
    }

    // fraction: 0..1
    setFraction(fraction) {
        var f = fraction
        if (f < 0) {
            f = 0
        }
        if (f > 1) {
            f = 1
        }
        _fill.setSize(Vec2.new(_width * f, _height), Vec2.new(0, 0))
        _fill.setColor(f < EXHAUSTED_BELOW ? EXHAUSTED_COLOR : FILL_COLOR)
    }
}
