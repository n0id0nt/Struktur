// UI/Rider/RiderTheme.wren
// Shared bits for the rider profile screens (UI/Rider/RiderSelectUI, UI/Rider/RiderEditUI): colours, z order and a
// centred-label helper. Same layering rule as the pairing screen: a screen's root sits at z 1 and an overlay's at
// z 2, so the overlay's batch draws on top; inside a batch z is absolute, so every child sits above both roots and
// below KitButton's fixed 5 (pill) / 6 (text).
import "math" for Vec2, Vec4
import "ui" for UILabel, TextAlignment

var BACKGROUND = Vec4.new(24, 28, 34, 255)
var OVERLAY = Vec4.new(0, 0, 0, 170)
var CLEAR = Vec4.new(0, 0, 0, 0)
var WARNING = Vec4.new(240, 110, 95, 255)

class RiderTheme {
    static background { BACKGROUND }
    static overlay { OVERLAY }
    static clear { CLEAR }
    static warning { WARNING }

    static screenZ { 1 }
    static overlayZ { 2 }
    static frameZ { 3 }
    static wellZ { 4 }
    static textZ { 7 }

    // A label centred horizontally on its parent, `top` pixels from the top.
    static label(parent, font, text, top, size, color) {
        var label = UILabel.new(Vec2.new(0, top), Vec2.new(0.5, 0), text, size)
        label.setFont(font)
        label.setTextColor(color)
        label.setAlignment(TextAlignment.CENTER)
        label.setBoundingBoxToText()
        label.setAnchorPoint(Vec2.new(0.5, 0))
        label.setZIndex(RiderTheme.textZ)
        parent.addChild(label)
        return label
    }

    static setText(label, text) {
        label.setText(text)
        label.setBoundingBoxToText()
    }
}
