// UI/Theme/UIKit.wren
// The game's UI kit: the pixel-art pieces under assets/UI/ as named styles any screen can build from. Every style
// is a nine-slice - corners keep their shape, edges and centre stretch - drawn at a whole-number scale so the 16px
// art keeps square, chunky pixels at 1280x720.
//
//   frames   "window"  a screen's main panel (UI/Theme/Wip/<WINDOW_THEME> - ThemeMetal; the borders below are
//                      measured for it, so re-measure them when switching theme)
//            "well"    a recessed dark panel for a section inside a window
//            "tile"    a bevelled olive tile - list rows, cards, loadout slots
//            "cell"    an inventory slot (a recessed light cell for an icon)
//            "dialogue" the dialogue box (UI/Dialog - light, orange-rimmed); "choice" a dialogue choice row in the same
//                      art, and "plate" the same art at 2x for small tags like the dialogue's prompt
//   states   "focus"   an orange ring drawn over whatever has focus (focusRing)
//            "tab" / "tabSelected" / "tabHover", "button" / "buttonHover" / "buttonPressed" / "buttonDisabled"
//   bars     "barTrack" / "barHealth" (UI/Receptacle/LifeBarMini*)
// plus KitButton / KitBar (UI/Theme/) for focusable buttons and filled bars in these styles, button prompts for input
// actions (hint(...), and [input=Action] tags in richText(...)) - they show the art for
// whichever binding the player would use right now, keyboard/mouse or pad, and redraw themselves when that changes -
// and any other image at a whole-number scale (image(...)).
//
// Textures are loaded once and shared: call UIKit.open() before building a screen and UIKit.close() after its UI
// has been removed (UIManager.removeUIElement) - opens are counted, so screens can stack.
//
// The game's font is the kit's too: UIKit.font() - the pack's pixel font, drawn nearest-neighbour at whole multiples
// of its 9px design size (a label asking for size 20 draws at 18, 24 at 27 - see Text::GlyphScale), so it stays crisp.
import "math" for Vec2, Vec4
import "resourceManager" for Font, Texture
import "ui" for UINineSlice, UITexture, UIInputHint, UIRichLabel

var WINDOW_THEME = "ThemeMetal"
var FONT_PATH = "UI/Font/NormalFont.ttf"
var FONT_DESIGN_SIZE = 9
// The kit art's text palette - shared, so don't change these Vec4s in place.
var TEXT_COLOR = Vec4.new(245, 240, 228, 255)   // light text, on dark or saturated surfaces
var TEXT_DARK = Vec4.new(19, 27, 27, 255)       // on light surfaces
var TEXT_MUTED = Vec4.new(171, 181, 156, 255)
var ACCENT = Vec4.new(255, 173, 85, 255)
var HINT_FONT_SIZE = 18                         // the font at 2x, next to 2x button art

// style -> [texture path, [left, right, top, bottom] slice borders in source pixels, scale]
var STYLES = {
    "window": ["UI/Theme/Wip/%(WINDOW_THEME)/nine_path_panel.png", [5, 5, 5, 6], 4],
    "well": ["UI/Theme/Theme Wood/nine_path_bg.png", [2, 2, 2, 2], 4],
    "tile": ["UI/Theme/Theme Wood/unchecked.png", [4, 4, 4, 4], 2],
    "cell": ["UI/Theme/Theme Wood/inventory_cell.png", [3, 1, 3, 1], 3],
    "focus": ["UI/Theme/Theme Wood/nine_path_focus.png", [3, 3, 3, 3], 3],
    "tab": ["UI/Theme/Theme Wood/tab_unselected.png", [4, 6, 5, 0], 3],
    "tabSelected": ["UI/Theme/Theme Wood/tab_selected.png", [4, 6, 4, 0], 3],
    "tabHover": ["UI/Theme/Theme Wood/tab_hover.png", [4, 6, 4, 0], 3],
    "button": ["UI/Theme/Theme Wood/button_normal.png", [2, 2, 2, 2], 3],
    "buttonHover": ["UI/Theme/Theme Wood/button_hover.png", [2, 2, 2, 2], 3],
    "buttonPressed": ["UI/Theme/Theme Wood/button_pressed.png", [2, 2, 2, 2], 3],
    "buttonDisabled": ["UI/Theme/Theme Wood/button_disabled.png", [2, 2, 2, 2], 3],
    "dialogue": ["UI/Dialog/DialogueBoxSimple.png", [8, 8, 8, 8], 3],
    "choice": ["UI/Dialog/ChoiceBox.png", [5, 5, 5, 5], 3],
    "plate": ["UI/Dialog/ChoiceBox.png", [5, 5, 5, 5], 2],
    "barTrack": ["UI/Receptacle/LifeBarMiniUnder.png", [1, 1, 1, 1], 3],
    "barHealth": ["UI/Receptacle/LifeBarMiniProgress.png", [1, 1, 1, 1], 3]
}

// How far the focus ring reaches past the element it surrounds, in screen pixels.
var FOCUS_OUTSET = 3

class UIKit {
    static text { TEXT_COLOR }
    static textDark { TEXT_DARK }
    static textMuted { TEXT_MUTED }
    static accent { ACCENT }

    static open() {
        if (__opens == null || __opens <= 0) {
            __opens = 0
            __textures = {}
        }
        __opens = __opens + 1
    }

    static close() {
        __opens = __opens - 1
        if (__opens <= 0) {
            for (texture in __textures.values) {
                texture.unload()
            }
            __textures = {}
            __opens = 0
        }
    }

    // The game's font, for any label size (labels pick their size; the font snaps it to its pixel grid). Each call
    // returns a new handle onto the one shared atlas - unload it when the screen is torn down, like any Font.
    static font() { Font.load(FONT_PATH, FONT_DESIGN_SIZE, true) }

    // A texture by path, loaded the first time it is asked for and kept until the last close().
    static texture(path) {
        if (!__textures.containsKey(path)) {
            __textures[path] = Texture.load(path)
        }
        return __textures[path]
    }

    // A nine-slice in `style`, added to parent (null for a root you add to UIManager yourself).
    static frame(parent, style, absPos, relPos, size, relSize, z) {
        var slice = UINineSlice.new(absPos, relPos, size, relSize)
        UIKit.applyStyle(slice, style)
        slice.setZIndex(z)
        if (parent != null) {
            parent.addChild(slice)
        }
        return slice
    }

    // Re-skins a frame - e.g. a tab switching between "tab" / "tabSelected" / "tabHover".
    static applyStyle(slice, style) {
        var def = STYLES[style]
        var border = def[1]
        slice.setTexture(UIKit.texture(def[0]))
        slice.setBorder(border[0], border[1], border[2], border[3])
        slice.setScale(def[2])
    }

    // The on-screen thickness [left, right, top, bottom] of a style's border - where content inside it can start.
    static padding(style) {
        var def = STYLES[style]
        return def[1].map { |b| b * def[2] }.toList
    }

    // A hidden focus ring over `element` (a child, so it moves and hides with it) - show it from the element's
    // onFocus, hide it from onLoseFocus.
    static focusRing(element, z) {
        var ring = UIKit.frame(element, "focus", Vec2.new(-FOCUS_OUTSET, -FOCUS_OUTSET), Vec2.new(0, 0),
            Vec2.new(2 * FOCUS_OUTSET, 2 * FOCUS_OUTSET), Vec2.new(1, 1), z)
        ring.setVisible(false)
        return ring
    }

    // An image at its native size times `scale` (keep it whole for pixel art). anchor picks the point of the image
    // that sits at absPos/relPos, like a label's.
    static image(parent, path, absPos, relPos, anchor, scale, z) {
        var texture = UIKit.texture(path)
        var image = UITexture.new(absPos, relPos, Vec2.new(texture.width * scale, texture.height * scale), Vec2.new(0, 0))
        image.setAnchorPoint(anchor)
        image.setTexture(texture)
        image.setZIndex(z)
        parent.addChild(image)
        return image
    }

    // The largest whole-number scale at which the image at `path` fits inside `size` pixels both ways (at least 1).
    static fitScale(path, size) {
        var texture = UIKit.texture(path)
        var longest = texture.width > texture.height ? texture.width : texture.height
        var scale = (size / longest).floor
        return scale < 1 ? 1 : scale
    }

    // A button prompt for an input action (an InputConfig.json name like "UIAccept", or "Action:part" for one part
    // of an axis - "UITab:negative", "UIDir:up"). Shows the art for whichever binding the player would use right
    // now - keyboard/mouse or pad, the one last used for this action first - and redraws itself when that changes.
    // `label` (or null) is drawn after the button. Its width changes with the art (Space is wider than A), so
    // `anchor` picks the point that stays at absPos/relPos. `parent` may be null for a top-level hint (add it with
    // UIManager.addUIElement yourself).
    static hint(parent, action, label, absPos, relPos, anchor, z) {
        var hint = UIInputHint.new(absPos, relPos, action)
        var font = UIKit.font()
        hint.setFont(font)
        font.unload()
        hint.setFontSize(HINT_FONT_SIZE)
        hint.setTextColor(TEXT_COLOR)
        if (label != null) hint.setLabel(label)
        hint.setAnchorPoint(anchor)
        hint.setZIndex(z)
        if (parent != null) parent.addChild(hint)
        return hint
    }

    // Kit-font rich text. [input=Action] tags draw button prompts inline (same rules as hint), and the label sizes
    // itself to its content, so `anchor` works like a label's even as the prompts change width.
    static richText(parent, markup, absPos, relPos, anchor, size, color, z) {
        var label = UIRichLabel.new(absPos, relPos, markup, size)
        var font = UIKit.font()
        label.setFont(font)
        font.unload()
        label.setTextColor(color)
        label.setAutoSize(true)
        label.setAnchorPoint(anchor)
        label.setZIndex(z)
        if (parent != null) parent.addChild(label)
        return label
    }
}
