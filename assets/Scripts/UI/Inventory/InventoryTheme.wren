// UI/Inventory/InventoryTheme.wren
// The colours, fonts and small builders every inventory page (UI/Inventory/*) shares, so the tabs look like one
// screen. Colours are RGBA 0-255 Vec4s like Util/Colors.wren - shared objects, so don't change them in place.
import "math" for Math, Vec2, Vec4
import "resourceManager" for Font
import "ui" for UILabel, UIPanel, UIColor
import "Util/Colors" for BLANK

var OVERLAY = Vec4.new(0, 0, 0, 170)
var WINDOW_BG = Vec4.new(28, 25, 36, 250)
var PANEL_BG = Vec4.new(40, 36, 52, 255)
var PANEL_BORDER = Vec4.new(74, 66, 92, 255)
var SLOT_BG = Vec4.new(52, 47, 67, 255)
var SLOT_EMPTY_BG = Vec4.new(34, 31, 44, 255)
var SLOT_FOCUS_BG = Vec4.new(74, 66, 95, 255)
var ACCENT = Vec4.new(230, 192, 98, 255)
var TEXT = Vec4.new(236, 230, 218, 255)
var TEXT_DARK = Vec4.new(28, 25, 36, 255)
var TEXT_MUTED = Vec4.new(150, 142, 162, 255)
var GOOD = Vec4.new(132, 206, 124, 255)
var BAD = Vec4.new(224, 112, 102, 255)
var BAR_BG = Vec4.new(20, 18, 26, 255)
var HP_FILL = Vec4.new(204, 72, 72, 255)
var STAMINA_FILL = Vec4.new(224, 184, 72, 255)
var XP_FILL = Vec4.new(112, 152, 232, 255)

class InventoryTheme {
    static overlay { OVERLAY }
    static windowBg { WINDOW_BG }
    static panelBg { PANEL_BG }
    static panelBorder { PANEL_BORDER }
    static slotBg { SLOT_BG }
    static slotEmptyBg { SLOT_EMPTY_BG }
    static slotFocusBg { SLOT_FOCUS_BG }
    static accent { ACCENT }
    static text { TEXT }
    static textDark { TEXT_DARK }
    static textMuted { TEXT_MUTED }
    static good { GOOD }
    static bad { BAD }
    static barBg { BAR_BG }
    static hpFill { HP_FILL }
    static staminaFill { STAMINA_FILL }
    static xpFill { XP_FILL }

    // Loaded big and drawn smaller, so every label size stays sharp. Unload both in teardown (InventoryUI does).
    static loadBoldFont() { Font.load("Fonts/medieval_sharp/MedievalSharp-Bold.ttf", 64) }
    static loadBookFont() { Font.load("Fonts/medieval_sharp/MedievalSharp-Book.ttf", 48) }

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

    // A solid panel with a border (width 0 = none). parent may be null for a root (add it to UIManager yourself).
    static panel(parent, absPos, relPos, size, relSize, bg, border, borderWidth, z) {
        var panel = UIPanel.new(absPos, relPos, size, relSize)
        panel.setBackgroundColor(bg)
        panel.setBorderColor(borderWidth > 0 ? border : BLANK)
        if (borderWidth > 0) {
            panel.setBorderWidth(borderWidth)
        }
        panel.setZIndex(z)
        if (parent != null) {
            parent.addChild(panel)
        }
        return panel
    }

    // An invisible container - just groups and positions its children.
    static group(parent, absPos, size, z) {
        return InventoryTheme.panel(parent, absPos, Vec2.new(0, 0), size, Vec2.new(0, 0), BLANK, BLANK, 0, z)
    }

    // A horizontal bar: dark track plus a fill `fraction` (0..1) of its width. Returns the fill so it can be resized.
    static bar(parent, absPos, width, height, fraction, fill, z) {
        var track = UIColor.new(absPos, Vec2.new(0, 0), Vec2.new(width, height), Vec2.new(0, 0))
        track.setColor(BAR_BG)
        track.setZIndex(z)
        parent.addChild(track)
        var bar = UIColor.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(width * Math.clamp01(fraction), height), Vec2.new(0, 0))
        bar.setColor(fill)
        bar.setZIndex(z + 1)
        track.addChild(bar)
        return bar
    }

    // Sets where `direction` (a NavigationDirection) moves focus from `element`. A null target blocks that
    // direction - focus stays put - rather than leaving it to the engine's spatial search, which compares top-left
    // corners and so happily jumps from the grid up into the tab bar (the tabs start right of the first column).
    static link(element, direction, target) {
        element.setNavigationNeighbor(direction, target != null ? target : element)
    }

    // A 1-2px line, stretching across its parent's width minus `inset` either side.
    static divider(parent, y, inset, z) {
        var line = UIColor.new(Vec2.new(inset, y), Vec2.new(0, 0), Vec2.new(-2 * inset, 2), Vec2.new(1, 0))
        line.setColor(PANEL_BORDER)
        line.setZIndex(z)
        parent.addChild(line)
        return line
    }
}
