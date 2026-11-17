// UI/Combat/TimelineUI.wren
// The shared timeline: one row per combatant, time running left to right from "now" (the line at the left edge) to
// WINDOW seconds ahead. Every committed move is a block as long as its animation; inside it, a bright tick marks
// the moment damage or healing lands, a pale band its defensive window (with a tick at the peak), a green band its
// dodge window and a gold band a boss part's Exposed window. Because the rows share one time axis, lining your
// Brace's peak up under an enemy's impact tick is something you can see.
//
// What is shown follows the battle system doc: your own side's whole chains, and for the other side the active move
// (more of the chain with a higher Read). While a chain is being built, the chooser's row previews it from "now".
// A combatant playing a damage animation gets a short red block first - everything after it is that much later.
//
// Pure view: States/CombatState.wren builds the rows (TimelineRow) from the Combatants each frame and hands them to
// draw(); the elements are pooled, so a frame only moves and resizes them.
import "ui" for UILabel, UIColor, UIPanel
import "math" for Vec2, Vec4
import "Util/Colors" for BLANK
import "UI/Theme/UIKit" for UIKit

var WINDOW = 5.0              // seconds of future the strip shows
var ROW_HEIGHT = 20
var ROW_GAP = 2
var NAME_WIDTH = 132
var AXIS_HEIGHT = 18
var BLOCKS_PER_ROW = 8
var MARKS_PER_ROW = 10

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
    "moveTick": Vec4.new(170, 255, 200, 255)
}
var TRACK_COLOR = Vec4.new(19, 27, 27, 200)
var GRID_COLOR = Vec4.new(245, 240, 228, 40)
var NOW_COLOR = Vec4.new(255, 173, 85, 255)
var SELECTED_NAME = Vec4.new(255, 173, 85, 255)

// One row's content, in seconds from now.
class TimelineRow {
    construct new(label, party, selected) {
        _label = label
        _party = party
        _selected = selected
        _blocks = []    // [start, end, kind, text]
        _bands = []     // [start, end, kind]
        _ticks = []     // [time, kind]
    }
    label { _label }
    party { _party }
    selected { _selected }
    blocks { _blocks }
    bands { _bands }
    ticks { _ticks }

    // Appends `move` starting at `start` with `progress` seconds of it already played; returns when it ends.
    // impactDone: its impact has already fired. preview: draw it as a not-yet-committed block.
    addMove(move, start, progress, impactDone, preview) {
        var end = start + move.completion - progress
        var kind = preview ? "preview" : TimelineRow.kindOf(move)
        _blocks.add([start, end, kind, move.name])
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

    band_(start, progress, from, to, kind) {
        if (to <= progress) return
        var a = from < progress ? progress : from
        _bands.add([start + a - progress, start + to - progress, kind])
    }

    addBlock(start, end, kind, text) {
        _blocks.add([start, end, kind, text])
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

    // x / y / width: the strip's box inside parent. rows: how many rows to build.
    construct new(parent, x, y, width, rows, font) {
        _trackWidth = width - NAME_WIDTH
        _scale = _trackWidth / WINDOW
        _root = UIPanel.new(Vec2.new(x, y), Vec2.new(0, 0), Vec2.new(width, TimelineUI.heightFor(rows)), Vec2.new(0, 0))
        _root.setBackgroundColor(BLANK)
        _root.setBorderColor(BLANK)
        _root.setZIndex(2)
        parent.addChild(_root)

        // The time axis: a faint line and a label every second.
        for (s in 0..WINDOW.floor) {
            var x0 = NAME_WIDTH + s * _scale
            if (s > 0) {
                var line = rect_(_root, Vec2.new(x0, AXIS_HEIGHT), Vec2.new(1, rows * (ROW_HEIGHT + ROW_GAP)), GRID_COLOR, 5)
            }
            var text = s == 0 ? "now" : "%(s)s"
            var label = UILabel.new(Vec2.new(x0 + (s == 0 ? 0 : -6), 0), Vec2.new(0, 0), text, 9.0)
            label.setFont(font)
            label.setTextColor(s == 0 ? NOW_COLOR : UIKit.textMuted)
            label.setBoundingBoxToText()
            label.setZIndex(6)
            _root.addChild(label)
        }

        _rows = []
        for (i in 0...rows) {
            _rows.add(buildRow_(i, font))
        }
        // The "now" line over every row.
        rect_(_root, Vec2.new(NAME_WIDTH - 1, AXIS_HEIGHT - 2), Vec2.new(2, rows * (ROW_HEIGHT + ROW_GAP) + 2), NOW_COLOR, 9)
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
        for (i in 0...BLOCKS_PER_ROW) {
            var block = rect_(_root, Vec2.new(NAME_WIDTH, y), Vec2.new(10, ROW_HEIGHT), COLORS["hold"], 4)
            block.setVisible(false)
            blocks.add(block)
            var label = UILabel.new(Vec2.new(NAME_WIDTH, y + 1), Vec2.new(0, 0), "", 18.0)
            label.setFont(font)
            label.setTextColor(UIKit.text)
            label.setBoundingBoxToText()
            label.setZIndex(8)
            label.setVisible(false)
            _root.addChild(label)
            labels.add(label)
            texts.add("")
        }
        row["blocks"] = blocks
        row["labels"] = labels
        row["texts"] = texts
        var marks = []
        for (i in 0...MARKS_PER_ROW) {
            var mark = rect_(_root, Vec2.new(NAME_WIDTH, y), Vec2.new(2, ROW_HEIGHT), COLORS["impact"], 7)
            mark.setVisible(false)
            marks.add(mark)
        }
        row["marks"] = marks
        row["shown"] = true
        return row
    }

    // rows: [TimelineRow] - one per combatant, in the order they should appear. Rows past the list are hidden.
    draw(rows) {
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
                hideFrom_(view["marks"], 0)
                continue
            }
            drawRow_(view, data)
        }
    }

    hideFrom_(list, from) {
        for (i in from...list.count) {
            list[i].setVisible(false)
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
        var used = 0
        for (b in data.blocks) {
            if (used >= blocks.count) break
            var start = b[0] < 0 ? 0 : b[0]
            var end = b[1] > WINDOW ? WINDOW : b[1]
            if (end - start <= 0.001) continue
            var x0 = NAME_WIDTH + (start * _scale).round
            var width = ((end - start) * _scale).round - 1
            if (width < 1) width = 1
            var block = blocks[used]
            block.setPosition(Vec2.new(x0, y), Vec2.new(0, 0))
            block.setSize(Vec2.new(width, ROW_HEIGHT), Vec2.new(0, 0))
            block.setColor(COLORS[b[2]])
            block.setVisible(true)
            var label = labels[used]
            var text = b[3]
            // ~9px a character at this size: only name blocks wide enough to hold it.
            if (text != "" && width >= text.count * 9 + 6) {
                if (texts[used] != text) {
                    texts[used] = text
                    label.setText(text)
                    label.setBoundingBoxToText()
                }
                label.setPosition(Vec2.new(x0 + 3, y + 1), Vec2.new(0, 0))
                label.setVisible(true)
            } else {
                label.setVisible(false)
            }
            used = used + 1
        }
        hideFrom_(blocks, used)
        hideFrom_(labels, used)

        // Bands and ticks share one pool.
        var marks = view["marks"]
        var m = 0
        for (band in data.bands) {
            if (m >= marks.count) break
            var start = band[0] < 0 ? 0 : band[0]
            var end = band[1] > WINDOW ? WINDOW : band[1]
            if (end - start <= 0.001) continue
            var mark = marks[m]
            mark.setPosition(Vec2.new(NAME_WIDTH + (start * _scale).round, y + ROW_HEIGHT - 6), Vec2.new(0, 0))
            mark.setSize(Vec2.new(((end - start) * _scale).round.max(2), 6), Vec2.new(0, 0))
            mark.setColor(COLORS[band[2]])
            mark.setVisible(true)
            m = m + 1
        }
        for (tick in data.ticks) {
            if (m >= marks.count) break
            if (tick[0] < 0 || tick[0] > WINDOW) continue
            var mark = marks[m]
            mark.setPosition(Vec2.new(NAME_WIDTH + (tick[0] * _scale).round - 1, y), Vec2.new(0, 0))
            mark.setSize(Vec2.new(tick[1] == "impact" ? 3 : 2, ROW_HEIGHT), Vec2.new(0, 0))
            mark.setColor(COLORS[tick[1]])
            mark.setVisible(true)
            m = m + 1
        }
        hideFrom_(marks, m)
    }
}
