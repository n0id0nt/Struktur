// UI/Dialogue/ChoiceListUI.wren
// The current dialogue node's choices: a column of UI-kit "choice" boxes sitting on the dialogue box's top-right
// corner, the last choice nearest the box. Rebuilt fresh every time a node offers choices (see
// States/InteractState.wren); the focused row is bright with an arrow cursor nudging at its left, the others are
// dimmed. onFocus is called only so the State can react to a focus change (playing a sound) without this view needing
// to know about audio. Once a choice is made, collapseTo(index) shrinks the list to just that row - muted, no cursor -
// as a record of what was picked (until InteractState replaces or clears it).
//
//            ┌──────────────────┐
//         ▶  │ First choice     │   ← focused: full brightness, cursor
//            │ Second choice    │   ← dimmed
//            └──────────────────┘
//   ┌──────────────────────────────────────────┐
//   │ the dialogue box                         │
import "ui" for UIManager, UILabel, UIPanel, NavigationDirection
import "math" for Vec2, Vec4
import "Util/Colors" for BLANK, BLACK
import "UI/Theme/UIKit" for UIKit

var ROW_HEIGHT = 48           // the kit "choice" box at 3x: 15px rim, an 18px line of text, 15px rim
var ROW_GAP = 6
var TEXT_PAD_X = 24           // text in from the row's sides (just inside the rim)
var TEXT_SIZE = 18.0
var MIN_ROW_WIDTH = 220
var BOX_GAP = 8               // between the bottom row and the dialogue box
var INSET_RIGHT = 24          // the rows' right edge, in from the dialogue box's right edge
var CURSOR_PATH = "UI/Theme/Theme Wood/arrow_right.png"
var CURSOR_SCALE = 2
var CURSOR_NUDGE = 3          // how far the cursor nudges toward the row while waiting, in pixels
var CURSOR_PERIOD = 0.8       // seconds per nudge
var FULL_TINT = Vec4.new(255, 255, 255, 255)
var DIM_TINT = Vec4.new(196, 192, 196, 255)      // the rows that aren't focused
var CHOSEN_TINT = Vec4.new(176, 172, 176, 255)   // the collapsed record of the pick
var TEXT_DIM = Vec4.new(60, 56, 60, 255)

class ChoiceListUI {
    // parent: the dialogue box (DialogueBoxUI.textBox) - the list hangs off its top-right corner. choiceTexts: the
    // localized display strings. font: DialogueBoxUI.font.
    construct new(parent, choiceTexts, font, onFocus) {
        UIKit.open()
        _parent = parent
        _rows = []
        _labels = []
        _focusedIndex = 0
        _collapsed = false
        _cursorNudge = null

        // Labels first, to size every row to the longest choice (they're re-parented into their rows below).
        var textWidth = 0
        for (text in choiceTexts) {
            var label = UILabel.new(Vec2.new(TEXT_PAD_X - 5, 0), Vec2.new(0, 0.5), text, TEXT_SIZE)
            label.setFont(font)
            label.setTextColor(BLACK)
            label.setAnchorPoint(Vec2.new(0, 0.5))
            label.setBoundingBoxToText()
            label.setZIndex(4)
            textWidth = textWidth.max(label.getSize().x)
            _labels.add(label)
        }
        _rowWidth = (textWidth + 2 * TEXT_PAD_X).ceil.max(MIN_ROW_WIDTH)

        // A transparent column on the box's top-right corner, its bottom edge BOX_GAP above the box.
        var count = choiceTexts.count
        _container = UIPanel.new(Vec2.new(-INSET_RIGHT, -BOX_GAP), Vec2.new(1, 0),
            Vec2.new(_rowWidth, count * ROW_HEIGHT + (count - 1) * ROW_GAP), Vec2.new(0, 0))
        _container.setAnchorPoint(Vec2.new(1, 1))
        _container.setBackgroundColor(BLANK)
        _container.setBorderColor(BLANK)
        parent.addChild(_container)

        for (i in 0...count) {
            var row = UIKit.frame(_container, "choice", Vec2.new(0, rowY_(i)), Vec2.new(0, 0),
                Vec2.new(_rowWidth, ROW_HEIGHT), Vec2.new(0, 0), 3)
            row.setFocusable(true)
            var index = i
            row.setOnFocus { |sender|
                if (!_collapsed) {
                    setFocused_(index)
                    onFocus.call(index)
                }
            }
            row.addChild(_labels[i])
            _rows.add(row)
        }
        // A plain column: up/down only, and nothing past either end.
        for (i in 0...count) {
            var row = _rows[i]
            row.setNavigationNeighbor(NavigationDirection.UP, i > 0 ? _rows[i - 1] : row)
            row.setNavigationNeighbor(NavigationDirection.DOWN, i < count - 1 ? _rows[i + 1] : row)
            row.setNavigationNeighbor(NavigationDirection.LEFT, row)
            row.setNavigationNeighbor(NavigationDirection.RIGHT, row)
        }

        // The arrow cursor, its right edge just short of the focused row's left edge.
        _cursor = UIKit.image(_container, CURSOR_PATH, Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(1, 0.5), CURSOR_SCALE, 4)

        // Controller/keyboard navigation starts on the first choice.
        if (count > 0) {
            UIManager.setFocus(_rows[0])
            setFocused_(0)
        }
    }

    // Which row is currently highlighted - read by the Controller when the player confirms.
    focusedIndex { _focusedIndex }

    // True once collapseTo has turned the list into a record of the pick.
    collapsed { _collapsed }

    // Nudges the cursor toward the focused row - call every frame with Time.unscaledTime.
    tick(now) {
        if (_collapsed || _rows.count == 0) {
            return
        }
        var nudge = (((now / CURSOR_PERIOD) * 2 * Num.pi).sin * 0.5 + 0.5) * CURSOR_NUDGE
        nudge = nudge.round
        if (nudge != _cursorNudge) {
            _cursorNudge = nudge
            placeCursor_()
        }
    }

    // The choice at `index` was made: every other row goes, the column shrinks to that one row (still just above the
    // box) and it takes the muted "chosen" look - no longer focusable, no cursor.
    collapseTo(index) {
        _collapsed = true
        _focusedIndex = index
        for (i in 0..._rows.count) {
            _rows[i].setFocusable(false)
            if (i != index) {
                _rows[i].setVisible(false)
            }
        }
        var chosen = _rows[index]
        chosen.setPosition(Vec2.new(0, 0), Vec2.new(0, 0))
        chosen.setTint(CHOSEN_TINT)
        _labels[index].setTextColor(TEXT_DIM)
        _cursor.setVisible(false)
        _container.setSize(Vec2.new(_rowWidth, ROW_HEIGHT), Vec2.new(0, 0))
    }

    teardown() {
        if (_container != null) {
            _parent.removeChild(_container)
            _container = null
            UIKit.close()
        }
    }

    rowY_(i) {
        return i * (ROW_HEIGHT + ROW_GAP)
    }

    // Brightens the focused row and moves the cursor to it; the rest are dimmed.
    setFocused_(index) {
        _focusedIndex = index
        for (i in 0..._rows.count) {
            var focused = i == index
            _rows[i].setTint(focused ? FULL_TINT : DIM_TINT)
            _labels[i].setTextColor(focused ? BLACK : TEXT_DIM)
        }
        placeCursor_()
    }

    placeCursor_() {
        var nudge = _cursorNudge == null ? 0 : _cursorNudge
        _cursor.setPosition(Vec2.new(nudge - 2, rowY_(_focusedIndex) + ROW_HEIGHT / 2), Vec2.new(0, 0))
    }
}
