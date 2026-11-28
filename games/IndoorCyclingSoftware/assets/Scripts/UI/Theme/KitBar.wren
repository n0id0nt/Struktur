// UI/Theme/KitBar.wren
// A filled bar on the UI kit's "barTrack" art (UI/Theme/UIKit.wren): the track's pixel-art rim, and a flat colour fill
// inside it that is set by fraction and drains toward the left. The colour is the caller's - a health bar going amber
// then red, a charge bar flashing when staggered - so it carries meaning, not the art. Needs UIKit.open() around the
// screen that builds it.
import "ui" for UIColor
import "math" for Math, Vec2
import "UI/Theme/UIKit" for UIKit

class KitBar {
    // x/y: top-left in parent's pixels; width/height: the whole bar including the rim (3px at the kit's 3x).
    construct new(parent, x, y, width, height, color, z) {
        _track = UIKit.frame(parent, "barTrack", Vec2.new(x, y), Vec2.new(0, 0), Vec2.new(width, height), Vec2.new(0, 0),
            z)
        var inset = UIKit.padding("barTrack")
        _innerWidth = width - inset[0] - inset[1]
        _innerHeight = height - inset[2] - inset[3]
        _fill = UIColor.new(Vec2.new(inset[0], inset[2]), Vec2.new(0, 0), Vec2.new(_innerWidth, _innerHeight),
            Vec2.new(0, 0))
        _fill.setColor(color)
        _fill.setZIndex(z + 1)
        _track.addChild(_fill)
        _fraction = 1
    }

    fraction { _fraction }

    // fraction: 0..1 (clamped). Whole pixels, so the edge of the fill never blurs.
    setFraction(fraction) {
        _fraction = Math.clamp01(fraction)
        _fill.setSize(Vec2.new((_innerWidth * _fraction).round, _innerHeight), Vec2.new(0, 0))
    }

    setColor(color) { _fill.setColor(color) }

    setVisible(visible) { _track.setVisible(visible) }
}
