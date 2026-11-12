// UI/Inventory/ItemGridPage.wren
// One item tab of the inventory (Items / Weapons / Armor): a scrolling grid of kit cells on the left - each with
// the item's icon and, bottom right, how many there are - and a detail panel on the right for the focused slot.
// Icons draw at a whole-number scale of their own size (they are pixel art of different sizes), centred.
// Pure view: `entries` are already-prepared maps from States/InventoryState.wren:
//   { "name", "icon" (texture path or null), "count", "stackable", "showCount" (the slot's count badge - stacks of
//     2+), "countText" (the detail panel's "Quantity: n"), "category" (display text),
//     "bonuses" ([[display text, amount], ...] - gear only) }
import "math" for Vec2
import "ui" for UIScroll, NavigationDirection
import "UI/Theme/UIKit" for UIKit
import "UI/Inventory/InventoryTheme" for InventoryTheme

var COLUMNS = 6
var SLOT_SIZE = 80
var SLOT_STRIDE = 88
var SLOT_INSET = 6   // keeps the focus ring of the outer slots inside the scroll's clip
var MIN_SLOTS = 30   // five full rows, so the grid never looks empty
var ICON_SIZE = 52   // 16px icons draw at 3x, leaving the cell's bottom-right corner for the count
var DETAIL_CELL = 168
var DETAIL_ICON_SIZE = 144
var MAX_BONUS_LINES = 4

class ItemGridPage {
    // parent: the inventory body. onFocus: called with no arguments whenever a slot gains focus (the screen's
    // focus sound).
    construct new(parent, entries, emptyText, boldFont, bookFont, onFocus) {
        _slots = []
        _boldFont = boldFont
        _bookFont = bookFont

        _root = InventoryTheme.group(parent, Vec2.new(0, 0), Vec2.new(1072, 476), 2)

        var gridFrame = InventoryTheme.frame(_root, "well", Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(560, 476),
            Vec2.new(0, 0), 3)
        var scroll = UIScroll.new(Vec2.new(14, 14), Vec2.new(0, 0), Vec2.new(532, 448), Vec2.new(0, 0))
        scroll.setHorizontalScrollEnabled(false)
        // Rows only scroll once the focused one is past the five that fit, a row at a time (174 = the fifth row's
        // centre below the viewport's centre), instead of re-centring on every move.
        scroll.setFocusDeadzone(Vec2.new(0, 174))
        scroll.setZIndex(4)
        gridFrame.addChild(scroll)

        buildDetail_()

        var slotCount = entries.count
        if (slotCount % COLUMNS != 0) {
            slotCount = slotCount + COLUMNS - slotCount % COLUMNS
        }
        if (slotCount < MIN_SLOTS) {
            slotCount = MIN_SLOTS
        }
        for (i in 0...slotCount) {
            var position = Vec2.new(SLOT_INSET + (i % COLUMNS) * SLOT_STRIDE,
                SLOT_INSET + (i / COLUMNS).floor * SLOT_STRIDE)
            if (i < entries.count) {
                _slots.add(buildSlot_(scroll, position, entries[i], onFocus))
            } else {
                var empty = InventoryTheme.frame(scroll, "cell", position, Vec2.new(0, 0), Vec2.new(SLOT_SIZE, SLOT_SIZE),
                    Vec2.new(0, 0), 5)
                empty.setTint(InventoryTheme.emptyTint)
            }
        }

        linkSlots_()
        if (entries.count == 0) {
            showEmpty_(emptyText)
        } else {
            // So the panel isn't blank when the tab is opened from the tab bar, before any slot has focus.
            showEntry_(entries[0])
        }
    }

    root { _root }

    // The slot to focus when this tab opens - null when there is nothing in it.
    firstFocusable { _slots.count > 0 ? _slots[0] : null }

    // Up from the top row goes back to this page's tab (instead of whichever tab happens to sit above that column).
    linkUpTo(tab) {
        var top = _slots.count < COLUMNS ? _slots.count : COLUMNS
        for (i in 0...top) {
            _slots[i].setNavigationNeighbor(NavigationDirection.UP, tab)
        }
    }

    // Arrow keys move one slot at a time and stop at the grid's edges. Down from a column the next row doesn't
    // reach lands on that row's last item.
    linkSlots_() {
        var count = _slots.count
        for (i in 0...count) {
            var slot = _slots[i]
            var column = i % COLUMNS
            InventoryTheme.link(slot, NavigationDirection.LEFT, column > 0 ? _slots[i - 1] : null)
            InventoryTheme.link(slot, NavigationDirection.RIGHT, (column < COLUMNS - 1 && i + 1 < count) ? _slots[i + 1] : null)
            InventoryTheme.link(slot, NavigationDirection.UP, i >= COLUMNS ? _slots[i - COLUMNS] : null)
            var below = null
            if (i + COLUMNS < count) {
                below = _slots[i + COLUMNS]
            } else if ((i / COLUMNS).floor < ((count - 1) / COLUMNS).floor) {
                below = _slots[count - 1]
            }
            InventoryTheme.link(slot, NavigationDirection.DOWN, below)
        }
    }

    setVisible(visible) { _root.setVisible(visible) }

    buildSlot_(scroll, position, entry, onFocus) {
        var slot = InventoryTheme.frame(scroll, "cell", position, Vec2.new(0, 0), Vec2.new(SLOT_SIZE, SLOT_SIZE),
            Vec2.new(0, 0), 5)
        iconInCell_(slot, entry["icon"], ICON_SIZE, 40.0)

        if (entry["showCount"]) {
            InventoryTheme.label(slot, "%(entry["count"])", Vec2.new(-7, -3), Vec2.new(1, 1), Vec2.new(1, 1), 22.0,
                _boldFont, InventoryTheme.textDark, 7)
        }

        InventoryTheme.focusable(slot, Fn.new {
            showEntry_(entry)
            onFocus.call()
        }, null)
        return slot
    }

    // The item's icon centred in a kit cell, at the biggest whole-number scale that fits `size` - or a "?" when
    // the item has no art. The cell's border is heavier top-left, so its inside is centred a little down-right.
    iconInCell_(cell, path, size, glyphSize) {
        var pad = UIKit.padding("cell")
        var offset = Vec2.new((pad[0] - pad[1]) / 2, (pad[2] - pad[3]) / 2)
        if (path != null) {
            return UIKit.image(cell, path, offset, Vec2.new(0.5, 0.5), Vec2.new(0.5, 0.5), UIKit.fitScale(path, size), 6)
        }
        return InventoryTheme.label(cell, "?", Vec2.new(offset.x, offset.y + glyphSize / 10), Vec2.new(0.5, 0.5),
            Vec2.new(0.5, 0.5), glyphSize, _boldFont, InventoryTheme.textDark, 6)
    }

    // --- detail panel -------------------------------------------------------------------

    buildDetail_() {
        var detail = InventoryTheme.frame(_root, "well", Vec2.new(576, 0), Vec2.new(0, 0), Vec2.new(496, 476),
            Vec2.new(0, 0), 3)
        _detail = detail
        _detailCell = null

        _nameLabel = InventoryTheme.label(detail, "", Vec2.new(0, 220), Vec2.new(0.5, 0), Vec2.new(0.5, 0), 34.0,
            _boldFont, InventoryTheme.text, 4)
        _categoryLabel = InventoryTheme.label(detail, "", Vec2.new(0, 264), Vec2.new(0.5, 0), Vec2.new(0.5, 0), 20.0,
            _bookFont, InventoryTheme.accent, 4)
        _countLabel = InventoryTheme.label(detail, "", Vec2.new(0, 294), Vec2.new(0.5, 0), Vec2.new(0.5, 0), 20.0,
            _bookFont, InventoryTheme.textMuted, 4)

        _bonusDivider = InventoryTheme.divider(detail, 334, 40, 4)
        _bonusLines = []
        for (i in 0...MAX_BONUS_LINES) {
            _bonusLines.add(InventoryTheme.label(detail, "", Vec2.new(0, 350 + i * 28), Vec2.new(0.5, 0),
                Vec2.new(0.5, 0), 22.0, _bookFont, InventoryTheme.good, 4))
        }

        _emptyLabel = InventoryTheme.label(detail, "", Vec2.new(0, 0), Vec2.new(0.5, 0.5), Vec2.new(0.5, 0.5), 24.0,
            _bookFont, InventoryTheme.textMuted, 4)
        _emptyLabel.setVisible(false)
    }

    // The big icon sits in its own cell, rebuilt for each item - icons differ in size, so each gets its own scale.
    showEntry_(entry) {
        _emptyLabel.setVisible(false)
        if (_detailCell != null) {
            _detail.removeChild(_detailCell)
        }
        _detailCell = InventoryTheme.frame(_detail, "cell", Vec2.new(0, 28), Vec2.new(0.5, 0),
            Vec2.new(DETAIL_CELL, DETAIL_CELL), Vec2.new(0, 0), 4)
        _detailCell.setAnchorPoint(Vec2.new(0.5, 0))
        iconInCell_(_detailCell, entry["icon"], DETAIL_ICON_SIZE, 80.0)

        setText_(_nameLabel, entry["name"])
        setText_(_categoryLabel, entry["category"])
        setText_(_countLabel, entry["stackable"] ? entry["countText"] : "")

        var bonuses = entry["bonuses"]
        _bonusDivider.setVisible(bonuses.count > 0)
        for (i in 0...MAX_BONUS_LINES) {
            if (i < bonuses.count) {
                var amount = bonuses[i][1]
                setText_(_bonusLines[i], "%(amount >= 0 ? "+" : "")%(amount) %(bonuses[i][0])")
                _bonusLines[i].setTextColor(amount >= 0 ? InventoryTheme.good : InventoryTheme.bad)
            } else {
                setText_(_bonusLines[i], "")
            }
        }
    }

    showEmpty_(text) {
        setText_(_nameLabel, "")
        setText_(_categoryLabel, "")
        setText_(_countLabel, "")
        _bonusDivider.setVisible(false)
        for (line in _bonusLines) {
            setText_(line, "")
        }
        setText_(_emptyLabel, text)
        _emptyLabel.setVisible(true)
    }

    setText_(label, text) {
        label.setText(text)
        label.setBoundingBoxToText()
    }

    // Call after the UI tree is removed.
    release() {
        _slots = []
        _root = null
        _detail = null
        _detailCell = null
        _nameLabel = null
        _categoryLabel = null
        _countLabel = null
        _bonusDivider = null
        _bonusLines = []
        _emptyLabel = null
    }
}
