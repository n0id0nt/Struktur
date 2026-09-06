// Combat/HealthBar.wren
// The generic filled-bar widget Phase 0 of the roadmap calls for - a dark track with a coloured
// fill on top, both plain UIColor rects. Parented into a caller-supplied UIElement and positioned in
// absolute pixels by the caller. The fill is left-anchored so it drains rightward, and shifts
// green -> amber -> red as it empties.
import "ui" for UIColor
import "math" for Vec2, Vec4

var TRACK_COLOR   = Vec4.new(18, 14, 12, 220)
var FILL_HEALTHY  = Vec4.new(90, 190, 90, 255)
var FILL_HURT     = Vec4.new(220, 185, 60, 255)
var FILL_CRITICAL = Vec4.new(210, 70, 55, 255)

class HealthBar {
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
        _fill.setColor(FILL_HEALTHY)
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
        if (f > 0.5) {
            _fill.setColor(FILL_HEALTHY)
        } else if (f > 0.25) {
            _fill.setColor(FILL_HURT)
        } else {
            _fill.setColor(FILL_CRITICAL)
        }
    }
}
