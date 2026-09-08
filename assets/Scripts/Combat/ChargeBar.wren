// Combat/ChargeBar.wren
// The design doc's ASCII "Enemy Move Timeline" made real. Same two-rect build as HealthBar, but it
// fills toward the right as a combatant charges its committed move, brightens to a "ready" colour
// the moment the bar is full, and flashes an alarm colour while the combatant is staggered by an
// interrupt (Phase 3).
import "ui" for UIColor
import "math" for Vec2, Vec4

var TRACK_COLOR    = Vec4.new(14, 18, 24, 220)
var FILL_COLOR     = Vec4.new(85, 165, 205, 235)
var READY_COLOR    = Vec4.new(150, 235, 255, 255)
var STAGGER_COLOR  = Vec4.new(230, 120, 60, 255)

class ChargeBar {
    construct new(parent, x, y, width, height) {
        _width = width
        _height = height
        _fraction = 0
        _staggered = false

        _track = UIColor.new(Vec2.new(x, y), Vec2.new(0, 0), Vec2.new(width, height), Vec2.new(0, 0))
        _track.setAnchorPoint(Vec2.new(0, 0))
        _track.setColor(TRACK_COLOR)
        _track.setZIndex(0)
        parent.addChild(_track)

        _fill = UIColor.new(Vec2.new(x, y), Vec2.new(0, 0), Vec2.new(0, height), Vec2.new(0, 0))
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
        _fraction = f
        _fill.setSize(Vec2.new(_width * f, _height), Vec2.new(0, 0))
        applyColor_()
    }

    setStaggered(staggered) {
        _staggered = staggered
        applyColor_()
    }

    applyColor_() {
        if (_staggered) {
            _fill.setColor(STAGGER_COLOR)
        } else if (_fraction >= 1) {
            _fill.setColor(READY_COLOR)
        } else {
            _fill.setColor(FILL_COLOR)
        }
    }
}
