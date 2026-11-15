// UI/Inventory/QuestsPage.wren
// The inventory's Quests tab. On the left, the arc's overall progress above a scrolling list of quests: the active
// ones under their category headings (Main Story, Side Quests...), each with its completion percentage and bar, then
// the quests already finished under "Completed". On the right, the focused quest: its title, category, percentage,
// summary and objectives - ticked when done. An active quest shows the objectives done so far plus the current one,
// so later steps aren't given away; a finished quest shows them all.
// Pure view: prepared by States/InventoryState.wren from Story/QuestLog.wren -
//   model: { "overall" (0..1), "sections": [ { "title", "quests": [entry] } ] }
//   entry: { "title", "category", "summary", "fraction" (0..1), "complete" (Bool),
//            "objectives": [ { "text", "done" } ] }
//   labels: { "overall", "complete", "empty", "objectives" }
import "math" for Vec2
import "ui" for UIScroll, UIRichLabel, NavigationDirection, TextWrapping
import "UI/Inventory/InventoryTheme" for InventoryTheme

var LIST_WIDTH = 520
var DETAIL_X = 536
var DETAIL_WIDTH = 536
var PAGE_HEIGHT = 476
var ROW_WIDTH = 480
var ROW_HEIGHT = 80
var ROW_GAP = 8
var HEADING_HEIGHT = 36
var ROW_INSET = 6        // room for the focus ring inside the scroll's clip
var SCROLL_Y = 84
var MAX_OBJECTIVES = 6
var OBJECTIVE_Y = 262
var OBJECTIVE_STRIDE = 34
var TICK_SIZE = 16

class QuestsPage {
    construct new(parent, model, labels, boldFont, bookFont, onFocus) {
        _rows = []
        _boldFont = boldFont
        _bookFont = bookFont
        _labels = labels
        _root = InventoryTheme.group(parent, Vec2.new(0, 0), Vec2.new(1072, PAGE_HEIGHT), 2)

        var list = InventoryTheme.frame(_root, "well", Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(LIST_WIDTH, PAGE_HEIGHT),
            Vec2.new(0, 0), 3)
        buildOverall_(list, model["overall"])

        var scroll = UIScroll.new(Vec2.new(14, SCROLL_Y), Vec2.new(0, 0),
            Vec2.new(LIST_WIDTH - 28, PAGE_HEIGHT - SCROLL_Y - 14), Vec2.new(0, 0))
        scroll.setHorizontalScrollEnabled(false)
        scroll.setZIndex(4)
        list.addChild(scroll)

        buildDetail_()

        var y = 0
        var first = null
        for (section in model["sections"]) {
            if (section["quests"].count == 0) continue
            InventoryTheme.label(scroll, section["title"], Vec2.new(ROW_INSET + 4, y + 6), Vec2.new(0, 0),
                Vec2.new(0, 0), 22.0, _boldFont, InventoryTheme.accent, 5)
            y = y + HEADING_HEIGHT
            for (entry in section["quests"]) {
                _rows.add(buildRow_(scroll, Vec2.new(ROW_INSET, y), entry, onFocus))
                if (first == null) first = entry
                y = y + ROW_HEIGHT + ROW_GAP
            }
            y = y + 6
        }

        for (i in 0..._rows.count) {
            InventoryTheme.link(_rows[i], NavigationDirection.UP, i > 0 ? _rows[i - 1] : null)
            InventoryTheme.link(_rows[i], NavigationDirection.DOWN, i + 1 < _rows.count ? _rows[i + 1] : null)
            InventoryTheme.link(_rows[i], NavigationDirection.LEFT, null)
            InventoryTheme.link(_rows[i], NavigationDirection.RIGHT, null)
        }

        if (first == null) {
            showEmpty_()
        } else {
            showEntry_(first)
        }
    }

    root { _root }
    firstFocusable { _rows.count > 0 ? _rows[0] : null }

    linkUpTo(tab) {
        if (_rows.count > 0) {
            _rows[0].setNavigationNeighbor(NavigationDirection.UP, tab)
        }
    }

    setVisible(visible) { _root.setVisible(visible) }

    // "Arc progress            42%" over a bar, across the top of the list.
    buildOverall_(list, fraction) {
        InventoryTheme.label(list, _labels["overall"], Vec2.new(24, 18), Vec2.new(0, 0), Vec2.new(0, 0), 22.0,
            _boldFont, InventoryTheme.text, 4)
        InventoryTheme.label(list, QuestsPage.percent_(fraction), Vec2.new(-24, 18), Vec2.new(1, 0), Vec2.new(1, 0),
            22.0, _boldFont, InventoryTheme.accent, 4)
        InventoryTheme.bar(list, Vec2.new(24, 52), LIST_WIDTH - 48, 15, fraction, InventoryTheme.xpFill, 4)
    }

    // A quest's row: its title with the percentage on the right and a bar below - or, once finished, its category
    // and "Complete" instead.
    buildRow_(scroll, position, entry, onFocus) {
        var row = InventoryTheme.frame(scroll, "tile", position, Vec2.new(0, 0), Vec2.new(ROW_WIDTH, ROW_HEIGHT),
            Vec2.new(0, 0), 5)
        var complete = entry["complete"]
        InventoryTheme.label(row, entry["title"], Vec2.new(18, 6), Vec2.new(0, 0), Vec2.new(0, 0), 24.0, _boldFont,
            complete ? InventoryTheme.textMuted : InventoryTheme.text, 6)
        if (complete) {
            InventoryTheme.label(row, _labels["complete"], Vec2.new(-18, 10), Vec2.new(1, 0), Vec2.new(1, 0), 20.0,
                _boldFont, InventoryTheme.good, 6)
            InventoryTheme.label(row, entry["category"], Vec2.new(18, 46), Vec2.new(0, 0), Vec2.new(0, 0), 18.0,
                _bookFont, InventoryTheme.textMuted, 6)
        } else {
            InventoryTheme.label(row, QuestsPage.percent_(entry["fraction"]), Vec2.new(-18, 10), Vec2.new(1, 0),
                Vec2.new(1, 0), 20.0, _boldFont, InventoryTheme.accent, 6)
            InventoryTheme.bar(row, Vec2.new(18, 52), ROW_WIDTH - 36, 12, entry["fraction"], InventoryTheme.xpFill, 6)
        }
        InventoryTheme.focusable(row, Fn.new {
            showEntry_(entry)
            onFocus.call()
        }, null)
        return row
    }

    // --- detail panel -------------------------------------------------------------------

    buildDetail_() {
        var detail = InventoryTheme.frame(_root, "well", Vec2.new(DETAIL_X, 0), Vec2.new(0, 0),
            Vec2.new(DETAIL_WIDTH, PAGE_HEIGHT), Vec2.new(0, 0), 3)
        _detail = detail
        _titleLabel = InventoryTheme.label(detail, "", Vec2.new(28, 22), Vec2.new(0, 0), Vec2.new(0, 0), 32.0,
            _boldFont, InventoryTheme.text, 4)
        _categoryLabel = InventoryTheme.label(detail, "", Vec2.new(28, 66), Vec2.new(0, 0), Vec2.new(0, 0), 20.0,
            _bookFont, InventoryTheme.accent, 4)
        _percentLabel = InventoryTheme.label(detail, "", Vec2.new(-28, 66), Vec2.new(1, 0), Vec2.new(1, 0), 20.0,
            _boldFont, InventoryTheme.accent, 4)

        // Wrapped to the panel's width (-5 / -2: the rich label's own padding).
        _summaryLabel = UIRichLabel.new(Vec2.new(28 - 5, 102 - 2), Vec2.new(0, 0), "", 20.0)
        _summaryLabel.setFont(_bookFont)
        _summaryLabel.setTextColor(InventoryTheme.text)
        _summaryLabel.setWordWrap(TextWrapping.WORD_WRAP)
        _summaryLabel.setSize(Vec2.new(DETAIL_WIDTH - 56 + 10, 110), Vec2.new(0, 0))
        _summaryLabel.setZIndex(4)
        detail.addChild(_summaryLabel)

        _divider = InventoryTheme.divider(detail, 218, 28, 4)
        _objectivesHeading = InventoryTheme.label(detail, _labels["objectives"], Vec2.new(28, 228), Vec2.new(0, 0),
            Vec2.new(0, 0), 18.0, _boldFont, InventoryTheme.textMuted, 4)

        _ticks = []
        _tickFills = []
        _objectiveLabels = []
        for (i in 0...MAX_OBJECTIVES) {
            var y = OBJECTIVE_Y + i * OBJECTIVE_STRIDE
            var tick = InventoryTheme.fill(detail, Vec2.new(30, y + 4), Vec2.new(0, 0), Vec2.new(TICK_SIZE, TICK_SIZE),
                Vec2.new(0, 0), InventoryTheme.emptyTint, 4)
            var fill = InventoryTheme.fill(tick, Vec2.new(3, 3), Vec2.new(0, 0), Vec2.new(TICK_SIZE - 6, TICK_SIZE - 6),
                Vec2.new(0, 0), InventoryTheme.good, 5)
            _ticks.add(tick)
            _tickFills.add(fill)
            _objectiveLabels.add(InventoryTheme.label(detail, "", Vec2.new(58, y), Vec2.new(0, 0), Vec2.new(0, 0), 20.0,
                _bookFont, InventoryTheme.text, 4))
        }

        _emptyLabel = InventoryTheme.label(detail, "", Vec2.new(0, 0), Vec2.new(0.5, 0.5), Vec2.new(0.5, 0.5), 22.0,
            _bookFont, InventoryTheme.textMuted, 4)
        _emptyLabel.setVisible(false)
    }

    showEntry_(entry) {
        _emptyLabel.setVisible(false)
        setText_(_titleLabel, entry["title"])
        setText_(_categoryLabel, entry["category"])
        setText_(_percentLabel, entry["complete"] ? _labels["complete"] : QuestsPage.percent_(entry["fraction"]))
        _percentLabel.setTextColor(entry["complete"] ? InventoryTheme.good : InventoryTheme.accent)
        _summaryLabel.setMarkupText(entry["summary"])
        _divider.setVisible(true)
        _objectivesHeading.setVisible(true)

        // Done objectives, then the current one - the rest stay hidden until reached.
        var shown = []
        for (objective in entry["objectives"]) {
            shown.add(objective)
            if (!objective["done"]) break
        }
        for (i in 0...MAX_OBJECTIVES) {
            var visible = i < shown.count
            _ticks[i].setVisible(visible)
            _objectiveLabels[i].setVisible(visible)
            if (visible) {
                var done = shown[i]["done"]
                _tickFills[i].setVisible(done)
                setText_(_objectiveLabels[i], shown[i]["text"])
                _objectiveLabels[i].setTextColor(done ? InventoryTheme.textMuted : InventoryTheme.text)
            }
        }
    }

    showEmpty_() {
        setText_(_titleLabel, "")
        setText_(_categoryLabel, "")
        setText_(_percentLabel, "")
        _summaryLabel.setMarkupText("")
        _divider.setVisible(false)
        _objectivesHeading.setVisible(false)
        for (i in 0...MAX_OBJECTIVES) {
            _ticks[i].setVisible(false)
            _objectiveLabels[i].setVisible(false)
        }
        setText_(_emptyLabel, _labels["empty"])
        _emptyLabel.setVisible(true)
    }

    setText_(label, text) {
        label.setText(text)
        label.setBoundingBoxToText()
    }

    static percent_(fraction) { "%((fraction * 100).round)\%" }

    // Call after the UI tree is removed.
    release() {
        _rows = []
        _root = null
        _detail = null
        _titleLabel = null
        _categoryLabel = null
        _percentLabel = null
        _summaryLabel = null
        _divider = null
        _objectivesHeading = null
        _ticks = []
        _tickFills = []
        _objectiveLabels = []
        _emptyLabel = null
    }
}
