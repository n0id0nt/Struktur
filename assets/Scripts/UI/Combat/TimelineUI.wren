// UI/Combat/TimelineUI.wren
// The shared timeline: one row per combatant, time running left to right from "now" (the line at the left edge) to
// WINDOW seconds ahead. Every committed move is a block as long as its animation; inside it, a bright tick marks
// the moment damage or healing lands, a pale band its defensive window (with a tick at the peak), a green band its
// dodge window and a gold band a boss part's Exposed window. Because the rows share one time axis, lining your
// Brace's peak up under an enemy's impact tick is something you can see.
//
// What is shown follows the battle system doc: your own side's whole chains, and for the other side the active move
// (more of the chain with a higher Read). While a chain is being built, the chooser's row previews it from "now", and
// a marker on that row stands at the chain budget - the chain has to reach it (draw's `budget`).
// A combatant playing a damage animation gets a short red block first - everything after it is that much later.
// A foe's row always shows where its chain ends - when it picks its next one - with a bright end cap: the moves Read
// doesn't show are a dim "?" block running up to it, so the end is there even when the moves aren't.
//
// Each block starts with its move's type icon (UI/Combat/MoveIcons.wren) where the block and the row have room for it.
//
// It's a guide, not a tool: no numbers anywhere - no time axis, no seconds - just who does what, in order, against
// one shared "now" line.
//
// Rows fill the strip's height: up to FILL_ROWS combatants each get an equal share of it (FILL_ROWS rows exactly
// fill it - that's the thickest a row gets); more than that and the rows shrink so every one still fits.
//
// Soul Sight (World/SoulSight.wren) sees further into a foe's chain: a move the party's Read doesn't show but that
// starts within Soul Sight's reach is foreseen - drawn whole, see-through and tinted the soul's blue, with a slow
// shimmer running over it - and a blue gradient on each foe's row, from "now" to the reach, shows how far it sees.
// The gradient and the shimmers hang off one panel inside the strip that has its own UI batch (setBatchRoot - see
// buildSoulLayer_), so the shimmer animating every frame (animate) never dirties the rest of the battle UI.
//
// Pure view: States/CombatState.wren builds the rows (TimelineRow) from the Combatants each frame and hands them to
// draw(); the elements are pooled, so a frame only moves and resizes them - and only the ones whose place, size or
// colour actually changed (Placed), since every set marks its element for redrawing.
import "ui" for UILabel, UIColor, UIPanel, UITexture
import "app" for Time
import "math" for Vec2, Vec4
import "Util/Colors" for BLANK
import "UI/Theme/UIKit" for UIKit
import "UI/Combat/MoveIcons" for MoveIcons

var WINDOW = 5.0              // seconds of future the strip shows
var ROW_HEIGHT = 18           // a row's height when no strip height is given (and the thinnest that labels need)
var ROW_GAP = 3
var FILL_ROWS = 4             // this many rows fill the strip's height
var LABEL_HEIGHT = 18         // the 18 px font, for centring names and block labels in a row
var NAME_WIDTH = 126            // fits an 11-character name ("Thornwolf 2") at the 18 px font
var AXIS_HEIGHT = 6           // room above the first row for the budget marker's cap
var BLOCKS_PER_ROW = 8
var MARKS_PER_ROW = 10
var ICON_PAD = 3              // a block's icon sits this far in from its left edge
var ICON_TINT = Vec4.new(255, 255, 255, 255)

var COLORS = {
    "attack": Vec4.new(196, 84, 62, 255),
    "defend": Vec4.new(66, 112, 200, 255),
    "move": Vec4.new(70, 150, 104, 255),
    "heal": Vec4.new(96, 190, 140, 255),
    "hold": Vec4.new(96, 100, 104, 255),
    "other": Vec4.new(140, 110, 170, 255),
    "stagger": Vec4.new(150, 44, 52, 255),
    "preview": Vec4.new(214, 150, 70, 255),
    "impact": Vec4.new(255, 245, 200, 255),
    "peak": Vec4.new(210, 232, 255, 255),
    "guard": Vec4.new(160, 200, 255, 150),
    "dodge": Vec4.new(150, 255, 190, 150),
    "exposed": Vec4.new(255, 220, 90, 170),
    "moveTick": Vec4.new(170, 255, 200, 255),
    "hidden": Vec4.new(62, 70, 74, 255)        // the moves of a foe's chain that Read doesn't show
}
var TRACK_COLOR = Vec4.new(19, 27, 27, 200)
var GRID_COLOR = Vec4.new(245, 240, 228, 40)
var NOW_COLOR = Vec4.new(255, 173, 85, 255)
var BUDGET_COLOR = Vec4.new(245, 240, 228, 255)
var SELECTED_NAME = Vec4.new(255, 173, 85, 255)
var CHAIN_END_COLOR = Vec4.new(255, 236, 180, 255)   // the end cap: a bar down the row with a short tick top and bottom
var CHAIN_END_TICK = 6                               // px the ticks reach back (left) from the bar - a "]"

// Soul Sight: the soul's blue, how far a foreseen block's colour leans to it and how see-through it is, the gradient
// art and its tint, and the shimmer over a foreseen block (alpha low..high, a wave travelling left to right).
// The gradient is pixel art at the battle panel's scale: each texel is SOUL_PIXEL screen pixels. The sheet (white,
// tinted here) holds the fade at every length - row r is a fade r + 1 texels long: a bright edge on the "now" line,
// then stepped bands fading out - so a row picks the length that fits the reach and is drawn texel for texel, never
// stretched.
var SOUL_BLUE = Vec4.new(130, 215, 255, 255)
var FORESEEN_LEAN = 0.55
var FORESEEN_ALPHA = 150
var FORESEEN_ICON_TINT = Vec4.new(255, 255, 255, FORESEEN_ALPHA)   // a foreseen block's icon, as see-through
var SOUL_GRADIENT = "UI/Battle/soul_sight_gradient.png"
var SOUL_GRADIENT_TINT = Vec4.new(90, 170, 255, 110)
var SOUL_GRADIENT_LENGTHS = 64   // rows in the sheet - the longest fade, in texels
var SOUL_PIXEL = 4               // screen pixels per texel (the battle panel's art scale)
var SHIMMER_LOW = 10
var SHIMMER_HIGH = 70
var SHIMMER_SPEED = 3.5      // radians a second
var SHIMMER_WAVE = 0.06      // radians per px along the strip - the wave's travel
var SOUL_LAYER_Z = 0         // its batch's draw order: over the battle UI's root batch (-1)

// A foreseen block's colour, per kind - made once, so it's the same Vec4 every frame (Placed compares them).
var FORESEEN = {}
for (kind in COLORS.keys) {
    var color = COLORS[kind]
    FORESEEN[kind] = Vec4.new((color.x + (SOUL_BLUE.x - color.x) * FORESEEN_LEAN).round,
                              (color.y + (SOUL_BLUE.y - color.y) * FORESEEN_LEAN).round,
                              (color.z + (SOUL_BLUE.z - color.z) * FORESEEN_LEAN).round, FORESEEN_ALPHA)
}

// A pooled element and the place, size and colour last given to it - setting them again only when they change.
class Placed {
    construct new(element) {
        _element = element
        _x = null
        _y = null
        _width = null
        _height = null
        _color = null
    }

    element { _element }

    move(x, y) {
        if (x == _x && y == _y) return
        _x = x
        _y = y
        _element.setPosition(Vec2.new(x, y), Vec2.new(0, 0))
    }

    resize(width, height) {
        if (width == _width && height == _height) return
        _width = width
        _height = height
        _element.setSize(Vec2.new(width, height), Vec2.new(0, 0))
    }

    // Colours are compared by identity - pass the shared Vec4s (COLORS / FORESEEN), not fresh ones.
    tint(color) {
        if (Object.same(color, _color)) return
        _color = color
        _element.setColor(color)
    }

    setVisible(visible) { _element.setVisible(visible) }
}

// One row's content, in seconds from now.
class TimelineRow {
    construct new(label, party, selected) {
        _label = label
        _party = party
        _selected = selected
        _blocks = []    // [start, end, kind, text]
        _bands = []     // [start, end, kind]
        _ticks = []     // [time, kind]
        _chainEnd = null   // seconds until the chain ends - an end cap there (foes' rows) - or null for none
    }
    label { _label }
    party { _party }
    selected { _selected }
    blocks { _blocks }
    bands { _bands }
    ticks { _ticks }
    chainEnd { _chainEnd }
    chainEnd=(value) { _chainEnd = value }

    // Appends `move` starting at `start` with `progress` seconds of it already played; returns when it ends.
    // impactDone: its impact has already fired. preview: draw it as a not-yet-committed block.
    addMove(move, start, progress, impactDone, preview) {
        var end = start + move.completion - progress
        var kind = preview ? "preview" : TimelineRow.kindOf(move)
        _blocks.add([start, end, kind, move.name, false, MoveIcons.pathFor(move)])
        if (move.impact != null && !impactDone && move.impact >= progress) {
            _ticks.add([start + move.impact - progress, "impact"])
        }
        if (move.moveTime != null && move.moveTime >= progress) {
            _ticks.add([start + move.moveTime - progress, "moveTick"])
        }
        if (move.window != null) {
            band_(start, progress, move.window[0], move.window[2], "guard")
            if (move.window[1] >= progress) _ticks.add([start + move.window[1] - progress, "peak"])
        }
        if (move.dodge != null) band_(start, progress, move.dodge[0], move.dodge[1], "dodge")
        if (move.exposed != null) band_(start, progress, move.exposed[0], move.exposed[1], "exposed")
        return end
    }

    // A move only Soul Sight sees (it starts within its reach): drawn whole, in the soul's colour.
    addForeseen(move, start) {
        var end = addMove(move, start, 0, false, false)
        _blocks[-1][4] = true
        return end
    }

    band_(start, progress, from, to, kind) {
        if (to <= progress) return
        var visibleFrom = from < progress ? progress : from
        _bands.add([start + visibleFrom - progress, start + to - progress, kind])
    }

    addBlock(start, end, kind, text) {
        _blocks.add([start, end, kind, text, false, null])
        return end
    }

    static kindOf(move) {
        if (move.isHold) return "hold"
        if (move.isAttack) return "attack"
        if (move.isHeal) return "heal"
        if (move.isDefend) return "defend"
        if (move.isMovement || move.isDodge) return "move"
        return "other"
    }
}

class TimelineUI {
    static rowStride { ROW_HEIGHT + ROW_GAP }
    static heightFor(rows) { AXIS_HEIGHT + rows * (ROW_HEIGHT + ROW_GAP) }

    // x / y / width: the strip's box inside parent. rows: how many rows to build. height: the strip's height, which
    // the rows share (see the top of the file) - left out, rows are ROW_HEIGHT.
    construct new(parent, x, y, width, rows, font) { init_(parent, x, y, width, rows, font, TimelineUI.heightFor(rows)) }
    construct new(parent, x, y, width, rows, font, height) { init_(parent, x, y, width, rows, font, height) }

    init_(parent, x, y, width, rows, font, height) {
        _trackWidth = width - NAME_WIDTH
        _scale = _trackWidth / WINDOW
        _height = height
        _rowH = ROW_HEIGHT
        _laidOut = -1
        _width = width
        _soul = null   // the Soul Sight layer (buildSoulLayer_)
        _root = UIPanel.new(Vec2.new(x, y), Vec2.new(0, 0), Vec2.new(width, height), Vec2.new(0, 0))
        _root.setBackgroundColor(BLANK)
        _root.setBorderColor(BLANK)
        _root.setZIndex(2)
        parent.addChild(_root)

        _rows = []
        for (i in 0...rows) {
            _rows.add(buildRow_(i, font))
        }
        // The "now" line over every row (sized by layout_).
        _nowLine = rect_(_root, Vec2.new(NAME_WIDTH - 1, AXIS_HEIGHT - 2), Vec2.new(2, rows * (ROW_HEIGHT + ROW_GAP) + 2), NOW_COLOR, 9)

        // The budget marker: a bright line through the chooser's row, reaching a little past it, with a cap on top.
        _budgetLine = Placed.new(rect_(_root, Vec2.new(0, 0), Vec2.new(3, ROW_HEIGHT + 8), BUDGET_COLOR, 10))
        _budgetCap = Placed.new(rect_(_root, Vec2.new(0, 0), Vec2.new(11, 4), BUDGET_COLOR, 10))
        _budgetLine.setVisible(false)
        _budgetCap.setVisible(false)
        buildSoulLayer_()
    }

    rect_(parent, pos, size, color, z) {
        var rect = UIColor.new(pos, Vec2.new(0, 0), size, Vec2.new(0, 0))
        rect.setColor(color)
        rect.setZIndex(z)
        parent.addChild(rect)
        return rect
    }

    buildRow_(index, font) {
        var y = AXIS_HEIGHT + index * (ROW_HEIGHT + ROW_GAP)
        var row = {}
        var name = UILabel.new(Vec2.new(0, y + 1), Vec2.new(0, 0), "", 18.0)
        name.setFont(font)
        name.setTextColor(UIKit.text)
        name.setBoundingBoxToText()
        name.setZIndex(6)
        _root.addChild(name)
        row["name"] = name
        row["nameText"] = ""
        row["nameColor"] = null
        row["track"] = rect_(_root, Vec2.new(NAME_WIDTH, y), Vec2.new(_trackWidth, ROW_HEIGHT), TRACK_COLOR, 3)
        row["y"] = y
        var blocks = []
        var labels = []
        var texts = []
        var icons = []
        for (i in 0...BLOCKS_PER_ROW) {
            var icon = UITexture.new(Vec2.new(NAME_WIDTH, y), Vec2.new(0, 0), Vec2.new(24, 24), Vec2.new(0, 0))
            icon.setZIndex(7)
            icon.setVisible(false)
            _root.addChild(icon)
            icons.add({"image": Placed.new(icon), "path": null, "tint": null})
            var block = Placed.new(rect_(_root, Vec2.new(NAME_WIDTH, y), Vec2.new(10, ROW_HEIGHT), COLORS["hold"], 4))
            block.setVisible(false)
            blocks.add(block)
            var label = UILabel.new(Vec2.new(NAME_WIDTH, y + 1), Vec2.new(0, 0), "", 18.0)
            label.setFont(font)
            label.setTextColor(UIKit.text)
            label.setBoundingBoxToText()
            label.setZIndex(8)
            label.setVisible(false)
            _root.addChild(label)
            labels.add(Placed.new(label))
            texts.add("")
        }
        row["blocks"] = blocks
        row["labels"] = labels
        row["texts"] = texts
        row["icons"] = icons
        var marks = []
        for (i in 0...MARKS_PER_ROW) {
            var mark = Placed.new(rect_(_root, Vec2.new(NAME_WIDTH, y), Vec2.new(2, ROW_HEIGHT), COLORS["impact"], 7))
            mark.setVisible(false)
            marks.add(mark)
        }
        row["marks"] = marks
        // The chain's end cap: the bar and its two ticks.
        row["endBar"] = Placed.new(rect_(_root, Vec2.new(NAME_WIDTH, y), Vec2.new(2, ROW_HEIGHT), CHAIN_END_COLOR, 9))
        row["endTop"] = Placed.new(rect_(_root, Vec2.new(NAME_WIDTH, y), Vec2.new(CHAIN_END_TICK, 2), CHAIN_END_COLOR, 9))
        row["endBottom"] = Placed.new(rect_(_root, Vec2.new(NAME_WIDTH, y), Vec2.new(CHAIN_END_TICK, 2), CHAIN_END_COLOR, 9))
        showEnd_(row, false)
        row["shown"] = true
        return row
    }

    // rows: [TimelineRow] - one per combatant, in the order they should appear. Rows past the list are hidden.
    draw(rows) { draw(rows, null, 0) }
    draw(rows, budget) { draw(rows, budget, 0) }

    // Sizes the rows for `count` combatants (only when that count changes): each gets an equal share of the strip,
    // never thicker than a FILL_ROWS share, never thinner than ROW_HEIGHT.
    layout_(count) {
        if (count == _laidOut) return
        _laidOut = count
        var share = (_height - AXIS_HEIGHT - (FILL_ROWS - 1) * ROW_GAP) / FILL_ROWS
        var fit = (_height - AXIS_HEIGHT - (count.max(1) - 1) * ROW_GAP) / count.max(1)
        _rowH = share.min(fit).floor.max(ROW_HEIGHT)
        for (i in 0..._rows.count) {
            var view = _rows[i]
            var y = AXIS_HEIGHT + i * (_rowH + ROW_GAP)
            view["y"] = y
            view["name"].setPosition(Vec2.new(0, y + ((_rowH - LABEL_HEIGHT) / 2).floor), Vec2.new(0, 0))
            view["track"].setPosition(Vec2.new(NAME_WIDTH, y), Vec2.new(0, 0))
            view["track"].setSize(Vec2.new(_trackWidth, _rowH), Vec2.new(0, 0))
        }
        _nowLine.setSize(Vec2.new(2, count.min(_rows.count) * (_rowH + ROW_GAP) + 2), Vec2.new(0, 0))
        _budgetLine.resize(3, _rowH + 8)
    }

    // budget: seconds the chain being built has to reach - marked on the selected row - or null for no marker.
    // reach: seconds of the foes' future Soul Sight sees (0 without it) - its gradient on every foe's row.
    draw(rows, budget, reach) {
        layout_(rows.count)
        var marked = false
        for (i in 0...rows.count.min(_rows.count)) {
            if (budget != null && rows[i].selected) {
                var x = NAME_WIDTH + (budget.min(WINDOW) * _scale).round - 1
                var y = _rows[i]["y"]
                _budgetLine.move(x, y - 4)
                _budgetCap.move(x - 4, y - 6)
                marked = true
            }
        }
        _budgetLine.setVisible(marked)
        _budgetCap.setVisible(marked)

        for (i in 0..._rows.count) {
            var view = _rows[i]
            var data = i < rows.count ? rows[i] : null
            var shown = data != null
            if (view["shown"] != shown) {
                view["shown"] = shown
                view["name"].setVisible(shown)
                view["track"].setVisible(shown)
            }
            if (!shown) {
                hideFrom_(view["blocks"], 0)
                hideFrom_(view["labels"], 0)
                hideIconsFrom_(view["icons"], 0)
                hideFrom_(view["marks"], 0)
                showEnd_(view, false)
                if (_soul != null) hideSoul_(view)
                continue
            }
            drawRow_(view, data)
            if (_soul != null) drawSoul_(view, data, reach)
        }
    }

    hideFrom_(list, from) {
        for (i in from...list.count) {
            list[i].setVisible(false)
        }
    }

    // A block's icon (segment[5] - an image path, or null): shown at the block's start when it fits. Returns the
    // icon's width, or 0 when none is shown.
    showIcon_(icon, segment, x0, y, width) {
        var path = segment[5]
        var image = icon["image"]
        if (path == null) {
            image.setVisible(false)
            return 0
        }
        var texture = UIKit.texture(path)
        if (width < texture.width + 2 * ICON_PAD || _rowH < texture.height + 2) {
            image.setVisible(false)
            return 0
        }
        if (icon["path"] != path) {
            icon["path"] = path
            image.element.setTexture(texture)
        }
        var tint = segment[4] ? FORESEEN_ICON_TINT : ICON_TINT
        if (!Object.same(icon["tint"], tint)) {
            icon["tint"] = tint
            image.element.setTint(tint)
        }
        image.move(x0 + ICON_PAD, y + ((_rowH - texture.height) / 2).floor)
        image.resize(texture.width, texture.height)
        image.setVisible(true)
        return texture.width
    }

    hideIconsFrom_(icons, from) {
        for (i in from...icons.count) {
            icons[i]["image"].setVisible(false)
        }
    }

    drawRow_(view, data) {
        var y = view["y"]
        var label = data.label.count > 11 ? data.label[0...10] + "." : data.label
        if (view["nameText"] != label) {
            view["nameText"] = label
            view["name"].setText(label)
            view["name"].setBoundingBoxToText()
        }
        var nameColor = data.selected ? SELECTED_NAME : (data.party ? UIKit.text : UIKit.textMuted)
        if (view["nameColor"] != nameColor) {
            view["nameColor"] = nameColor
            view["name"].setTextColor(nameColor)
        }

        // Blocks (and their names, where there is room).
        var blocks = view["blocks"]
        var labels = view["labels"]
        var texts = view["texts"]
        var icons = view["icons"]
        var used = 0
        for (segment in data.blocks) {
            if (used >= blocks.count) break
            var start = segment[0] < 0 ? 0 : segment[0]
            var end = segment[1] > WINDOW ? WINDOW : segment[1]
            if (end - start <= 0.001) continue
            var x0 = NAME_WIDTH + (start * _scale).round
            var width = ((end - start) * _scale).round - 1
            if (width < 1) width = 1
            var block = blocks[used]
            block.move(x0, y)
            block.resize(width, _rowH)
            block.tint(segment[4] ? FORESEEN[segment[2]] : COLORS[segment[2]])
            block.setVisible(true)
            // The move's icon at the block's start, if the block is wide enough and the row tall enough for it.
            var textX = x0 + 3
            var iconWidth = showIcon_(icons[used], segment, x0, y, width)
            if (iconWidth > 0) textX = x0 + ICON_PAD + iconWidth + 3
            var label = labels[used]
            var text = segment[3]
            // ~9px a character at this size: only name blocks wide enough to hold it.
            if (text != "" && x0 + width - textX >= text.count * 9 + 3) {
                if (texts[used] != text) {
                    texts[used] = text
                    label.element.setText(text)
                    label.element.setBoundingBoxToText()
                }
                label.move(textX, y + ((_rowH - LABEL_HEIGHT) / 2).floor)
                label.setVisible(true)
            } else {
                label.setVisible(false)
            }
            used = used + 1
        }
        hideFrom_(blocks, used)
        hideFrom_(labels, used)
        hideIconsFrom_(icons, used)

        // Bands and ticks share one pool.
        var marks = view["marks"]
        var latestEnd = 0
        for (band in data.bands) {
            if (latestEnd >= marks.count) break
            var start = band[0] < 0 ? 0 : band[0]
            var end = band[1] > WINDOW ? WINDOW : band[1]
            if (end - start <= 0.001) continue
            var mark = marks[latestEnd]
            mark.move(NAME_WIDTH + (start * _scale).round, y + _rowH - 6)
            mark.resize(((end - start) * _scale).round.max(2), 6)
            mark.tint(COLORS[band[2]])
            mark.setVisible(true)
            latestEnd = latestEnd + 1
        }
        for (tick in data.ticks) {
            if (latestEnd >= marks.count) break
            if (tick[0] < 0 || tick[0] > WINDOW) continue
            var mark = marks[latestEnd]
            mark.move(NAME_WIDTH + (tick[0] * _scale).round - 1, y)
            mark.resize(tick[1] == "impact" ? 3 : 2, _rowH)
            mark.tint(COLORS[tick[1]])
            mark.setVisible(true)
            latestEnd = latestEnd + 1
        }
        hideFrom_(marks, latestEnd)

        // The end cap, when the chain ends inside the strip.
        var end = data.chainEnd
        var capped = end != null && end > 0 && end <= WINDOW
        if (capped) {
            var x = NAME_WIDTH + (end * _scale).round - 2
            view["endBar"].move(x, y - 2)
            view["endBar"].resize(3, _rowH + 4)
            view["endTop"].move(x - CHAIN_END_TICK + 3, y - 2)
            view["endBottom"].move(x - CHAIN_END_TICK + 3, y + _rowH)
        }
        showEnd_(view, capped)
    }

    showEnd_(view, visible) {
        if (view["endVisible"] == visible) return
        view["endVisible"] = visible
        view["endBar"].setVisible(visible)
        view["endTop"].setVisible(visible)
        view["endBottom"].setVisible(visible)
    }

    // --- Soul Sight -------------------------------------------------------------------------------------------

    // The Soul Sight layer: a panel over the strip holding each row's gradient and shimmers, in a batch of its own
    // (setBatchRoot) so the shimmer's per-frame colour changes redraw only it. A child of the strip, so it moves, hides
    // and goes away with the rest of the battle UI.
    buildSoulLayer_() {
        _soul = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(_width, _height), Vec2.new(0, 0))
        _soul.setBackgroundColor(BLANK)
        _soul.setBorderColor(BLANK)
        _soul.setZIndex(SOUL_LAYER_Z)
        _soul.setBatchRoot(true)
        _root.addChild(_soul)
        var texture = UIKit.texture(SOUL_GRADIENT)
        for (view in _rows) {
            var gradient = UITexture.new(Vec2.new(NAME_WIDTH, view["y"]), Vec2.new(0, 0), Vec2.new(1, ROW_HEIGHT), Vec2.new(0, 0))
            gradient.setTexture(texture)
            gradient.setTint(SOUL_GRADIENT_TINT)
            gradient.setZIndex(1)
            gradient.setVisible(false)
            _soul.addChild(gradient)
            view["gradient"] = Placed.new(gradient)
            view["gradientTexels"] = null   // the fade length (sheet row) it's showing
            var shimmers = []
            for (i in 0...BLOCKS_PER_ROW) {
                var shimmer = Placed.new(rect_(_soul, Vec2.new(NAME_WIDTH, view["y"]), Vec2.new(10, ROW_HEIGHT), BLANK, 2))
                shimmer.setVisible(false)
                shimmers.add(shimmer)
            }
            view["shimmers"] = shimmers
            view["shimmerX"] = []   // the visible shimmers' x, for the wave
        }
    }

    // A foe's row: the gradient out to the reach, and a shimmer over each foreseen block.
    drawSoul_(view, data, reach) {
        var gradient = view["gradient"]
        var seeing = !data.party && reach > 0
        if (seeing) {
            // As many whole texels as cover the reach, and the sheet row with a fade exactly that long.
            var texels = (reach.min(WINDOW) * _scale / SOUL_PIXEL).round.max(2).min(SOUL_GRADIENT_LENGTHS)
            if (view["gradientTexels"] != texels) {
                view["gradientTexels"] = texels
                gradient.element.setSourceRect(0, texels - 1, texels, 1)
            }
            gradient.move(NAME_WIDTH, view["y"])
            gradient.resize(texels * SOUL_PIXEL, _rowH)
        }
        gradient.setVisible(seeing)

        var shimmers = view["shimmers"]
        var xs = []
        for (segment in data.blocks) {
            if (!segment[4] || xs.count >= shimmers.count) continue
            var start = segment[0] < 0 ? 0 : segment[0]
            var end = segment[1] > WINDOW ? WINDOW : segment[1]
            if (end - start <= 0.001) continue
            var x = NAME_WIDTH + (start * _scale).round
            var shimmer = shimmers[xs.count]
            shimmer.move(x, view["y"])
            shimmer.resize((((end - start) * _scale).round - 1).max(1), _rowH)
            shimmer.setVisible(true)
            xs.add(x)
        }
        hideFrom_(shimmers, xs.count)
        view["shimmerX"] = xs
    }

    hideSoul_(view) {
        view["gradient"].setVisible(false)
        hideFrom_(view["shimmers"], 0)
        view["shimmerX"] = []
    }

    // Every frame (UI/Combat/CombatUI.update): the shimmers breathe, a slow wave running left to right along the strip.
    // Only the Soul Sight layer's own batch is redrawn for it.
    animate() {
        if (_soul == null) return
        var now = Time.unscaledTime
        for (view in _rows) {
            var shimmers = view["shimmers"]
            var xs = view["shimmerX"]
            for (i in 0...xs.count) {
                var wave = 0.5 + 0.5 * (now * SHIMMER_SPEED - xs[i] * SHIMMER_WAVE).sin
                var alpha = (SHIMMER_LOW + (SHIMMER_HIGH - SHIMMER_LOW) * wave).round
                shimmers[i].element.setColor(Vec4.new(SOUL_BLUE.x, SOUL_BLUE.y, SOUL_BLUE.z, alpha))
            }
        }
    }
}
