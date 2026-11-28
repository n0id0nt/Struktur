// UI/Inventory/InventoryTheme.wren
// The inventory's look, on top of the shared UI kit (UI/Theme/UIKit.wren - the nine-sliced frames, focus ring and
// prompt icons): the text colours that read on the kit's olive and dark panels, fonts, and the small builders every
// inventory page (UI/Inventory/*) shares, so the tabs look like one screen. Colours are RGBA 0-255 Vec4s like
// Util/Colors.wren - shared objects, so don't change them in place. Most come from the kit art's own palette.
import "math" for Math, Vec2, Vec4
import "ui" for UILabel, UIPanel, UIColor
import "Util/Colors" for BLANK
import "UI/Theme/UIKit" for UIKit

var OVERLAY = Vec4.new(0, 0, 0, 170)
var TEXT = Vec4.new(245, 240, 228, 255)
var TEXT_DARK = Vec4.new(19, 27, 27, 255)       // on light surfaces - item cells, the open tab
var TEXT_MUTED = Vec4.new(171, 181, 156, 255)
var ACCENT = Vec4.new(255, 173, 85, 255)
var GOOD = Vec4.new(160, 214, 110, 255)
var BAD = Vec4.new(240, 110, 95, 255)
var LINE = Vec4.new(19, 27, 27, 255)
var ROW_BAND = Vec4.new(19, 27, 27, 70)        // a non-interactive table row's backing
var HP_FILL = Vec4.new(224, 57, 76, 255)
var STAMINA_FILL = Vec4.new(255, 200, 87, 255)
var XP_FILL = Vec4.new(110, 176, 196, 255)
var EMPTY_TINT = Vec4.new(150, 150, 150, 255)   // an empty item cell, darkened
var ARMED_TINT = Vec4.new(255, 196, 128, 255)   // a tile picked as the first half of a swap, warmed
var NO_TINT = Vec4.new(255, 255, 255, 255)

class InventoryTheme {
    static overlay { OVERLAY }
    static text { TEXT }
    static textDark { TEXT_DARK }
    static textMuted { TEXT_MUTED }
    static accent { ACCENT }
    static good { GOOD }
    static bad { BAD }
    static rowBand { ROW_BAND }
    static hpFill { HP_FILL }
    static staminaFill { STAMINA_FILL }
    static xpFill { XP_FILL }
    static emptyTint { EMPTY_TINT }
    static armedTint { ARMED_TINT }
    static noTint { NO_TINT }

    // The game font (UIKit.font - a single-weight pixel font, so "bold" and "book" are the same face; the names
    // just say how each label is meant to read). Unload both in teardown (InventoryUI does).
    static loadBoldFont() { UIKit.font() }
    static loadBookFont() { UIKit.font() }

    // A label sized to its text. anchor picks which point of the label sits at absPos/relPos - (1, 0) right-aligns
    // it on that point, (0.5, 0) centres it.
    static label(parent, text, absPos, relPos, anchor, size, font, color, z) {
        var label = UILabel.new(absPos, relPos, text, size)
        label.setFont(font)
        label.setTextColor(color)
        label.setBoundingBoxToText()
        label.setAnchorPoint(anchor)
        label.setZIndex(z)
        parent.addChild(label)
        return label
    }

    // A flat, borderless panel (null parent = a root to add to UIManager yourself) - the dimmed backdrop.
    static fill(parent, absPos, relPos, size, relSize, color, z) {
        var panel = UIPanel.new(absPos, relPos, size, relSize)
        panel.setBackgroundColor(color)
        panel.setBorderColor(BLANK)
        panel.setZIndex(z)
        if (parent != null) {
            parent.addChild(panel)
        }
        return panel
    }

    // An invisible container - just groups and positions its children.
    static group(parent, absPos, size, z) {
        return InventoryTheme.fill(parent, absPos, Vec2.new(0, 0), size, Vec2.new(0, 0), BLANK, z)
    }

    // A UI kit frame - "window", "well", "tile", "cell"... (UI/Theme/UIKit.wren).
    static frame(parent, style, absPos, relPos, size, relSize, z) {
        return UIKit.frame(parent, style, absPos, relPos, size, relSize, z)
    }

    // Makes `element` focusable with the kit's focus ring over it; onFocus / onLoseFocus (either may be null) run
    // after the ring is shown / hidden. Returns the ring.
    static focusable(element, onFocus, onLoseFocus) {
        element.setFocusable(true)
        var ring = UIKit.focusRing(element, 20)
        element.setOnFocus { |sender|
            ring.setVisible(true)
            if (onFocus != null) {
                onFocus.call()
            }
        }
        element.setOnLoseFocus { |sender|
            ring.setVisible(false)
            if (onLoseFocus != null) {
                onLoseFocus.call()
            }
        }
        return ring
    }

    // A horizontal bar: the kit's bar track with a fill `fraction` (0..1) of the way along its inside.
    static bar(parent, absPos, width, height, fraction, fill, z) {
        var track = UIKit.frame(parent, "barTrack", absPos, Vec2.new(0, 0), Vec2.new(width, height), Vec2.new(0, 0), z)
        var inset = UIKit.padding("barTrack")
        var inner = width - inset[0] - inset[1]
        var bar = UIColor.new(Vec2.new(inset[0], inset[2]), Vec2.new(0, 0),
            Vec2.new(inner * Math.clamp01(fraction), height - inset[2] - inset[3]), Vec2.new(0, 0))
        bar.setColor(fill)
        bar.setZIndex(z + 1)
        track.addChild(bar)
        return bar
    }

    // A 2px line, stretching across its parent's width minus `inset` either side.
    static divider(parent, y, inset, z) {
        var line = UIColor.new(Vec2.new(inset, y), Vec2.new(0, 0), Vec2.new(-2 * inset, 2), Vec2.new(1, 0))
        line.setColor(LINE)
        line.setZIndex(z)
        parent.addChild(line)
        return line
    }

    // Sets where `direction` (a NavigationDirection) moves focus from `element`. A null target blocks that
    // direction - focus stays put - rather than leaving it to the engine's spatial search, which compares top-left
    // corners and so happily jumps from the grid up into the tab bar (the tabs start right of the first column).
    static link(element, direction, target) {
        element.setNavigationNeighbor(direction, target != null ? target : element)
    }
}
