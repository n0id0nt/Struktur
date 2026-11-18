// UI/Inventory/ItemGridPage.wren
// The inventory's Items tab: one scrolling grid on the left split into sections - Key Items, Consumables, Weapons,
// Armor (Inventory/ItemDatabase.ItemCategory.all) - each a heading over at least MIN_ROWS rows of kit cells (the
// item's icon and, bottom right, how many there are; unused cells drawn empty), with a scroll bar down its right edge,
// and a detail panel on the right for the focused slot. Icons draw at a whole-number scale of their own size (they are pixel art of different sizes), centred.
// Pure view: `sections` are already-prepared by States/InventoryState.wren:
//   [{ "title", "entries": [entry] }], an entry being
//   { "name", "icon" (texture path or null), "count", "stackable", "showCount" (the slot's count badge - stacks of
//     2+), "countText" (the detail panel's "Quantity: n"), "category" (display text),
//     "bonuses" ([[display text, amount], ...] - gear only), "opensTab" (a tab id pressing it opens, or null),
//     "actionText" (what pressing it does - shown in the detail panel, or null),
//     weapons only: "equipItem" (the item id pressing it equips), "equipped" (Bool - the slot gets an "E" marker),
//     "warning" ([headline, detail] shown in red under the panel - equipping it changes the moves - or null) }
// onOpenTab(tabId) is called when a slot with "opensTab" is pressed (the Old Map opens the Map tab); onEquip(itemId)
// when a weapon is pressed. The State updates the weapon entries in place after a change - refresh() redraws them.
import "math" for Vec2
import "ui" for UIScroll, NavigationDirection
import "UI/Theme/UIKit" for UIKit
import "UI/Inventory/InventoryTheme" for InventoryTheme

var COLUMNS = 6
var SLOT_SIZE = 80
var SLOT_STRIDE = 88
var SLOT_INSET = 6        // keeps the focus ring of the outer slots inside the scroll's clip
var HEADING_HEIGHT = 40   // a section's title, above its first row
var MIN_ROWS = 2          // every section shows at least this many rows of cells, empty or not
var GRID_WIDTH = 580      // the grid's well: the scroll (14 + 532) plus the scroll bar
var BAR_X = 552           // the scroll bar's track, right of the scroll
var BAR_WIDTH = 12
var DETAIL_X = 596
var DETAIL_WIDTH = 476
var SECTION_GAP = 10
var ICON_SIZE = 52        // 16px icons draw at 3x, leaving the cell's bottom-right corner for the count
var DETAIL_CELL = 168
var DETAIL_ICON_SIZE = 144
var MAX_BONUS_LINES = 4
var BONUS_Y = 350           // the first bonus line in the detail panel
var BONUS_LINE = 28

class ItemGridPage {
    // parent: the inventory body. onFocus: called with no arguments whenever a slot gains focus (the screen's
    // focus sound). emptyText: the detail panel's text when there are no items at all. onOpenTab: see above.
    construct new(parent, sections, emptyText, boldFont, bookFont, onFocus, onOpenTab, onEquip) {
        _slots = []
        _rows = []          // focusable slots, row by row, across every section - for up / down
        _boldFont = boldFont
        _bookFont = bookFont
        _onOpenTab = onOpenTab
        _onEquip = onEquip
        _gear = []          // [entry, "E" marker] for every weapon slot
        _focused = null
        _focusedEntry = null

        _root = InventoryTheme.group(parent, Vec2.new(0, 0), Vec2.new(1072, 476), 2)

        var gridFrame = InventoryTheme.frame(_root, "well", Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(GRID_WIDTH, 476),
            Vec2.new(0, 0), 3)
        var scroll = UIScroll.new(Vec2.new(14, 14), Vec2.new(0, 0), Vec2.new(532, 448), Vec2.new(0, 0))
        scroll.setVerticalScrollEnabled(true)
        // Rows only scroll once the focused one nears an edge, instead of re-centring on every move.
        scroll.setFocusDeadzone(Vec2.new(0, 150))
        scroll.setZIndex(4)
        gridFrame.addChild(scroll)

        // Scroll bar: the engine sizes and moves the thumb to match the scroll (UIScroll.setScrollIndicator).
        var track = InventoryTheme.fill(gridFrame, Vec2.new(BAR_X, 14), Vec2.new(0, 0), Vec2.new(BAR_WIDTH, 448),
            Vec2.new(0, 0), InventoryTheme.rowBand, 4)
        var thumb = InventoryTheme.fill(track, Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(BAR_WIDTH, 40), Vec2.new(0, 0),
            InventoryTheme.accent, 5)
        scroll.setScrollIndicator(thumb)

        buildDetail_()

        var y = 0
        var first = null
        for (section in sections) {
            InventoryTheme.label(scroll, section["title"], Vec2.new(SLOT_INSET + 2, y + 8), Vec2.new(0, 0),
                Vec2.new(0, 0), 24.0, _boldFont, InventoryTheme.accent, 5)
            y = y + HEADING_HEIGHT
            var entries = section["entries"]
            if (first == null && entries.count > 0) first = entries[0]
            var slotCount = entries.count
            if (slotCount % COLUMNS != 0) {
                slotCount = slotCount + COLUMNS - slotCount % COLUMNS
            }
            if (slotCount < MIN_ROWS * COLUMNS) {
                slotCount = MIN_ROWS * COLUMNS
            }
            // Empty cells take focus too, so the arrows walk the whole grid (and scroll it) - they clear the panel.
            var row = null
            for (i in 0...slotCount) {
                var position = Vec2.new(SLOT_INSET + (i % COLUMNS) * SLOT_STRIDE,
                    y + SLOT_INSET + (i / COLUMNS).floor * SLOT_STRIDE)
                if (i % COLUMNS == 0) {
                    row = []
                    _rows.add(row)
                }
                var slot = i < entries.count ? buildSlot_(scroll, position, entries[i], onFocus) :
                    buildEmptySlot_(scroll, position, onFocus)
                _slots.add(slot)
                row.add(slot)
            }
            y = y + (slotCount / COLUMNS) * SLOT_STRIDE + SLOT_INSET
            y = y + SECTION_GAP
        }

        linkSlots_()
        if (first == null) {
            showEmpty_(emptyText)
        } else {
            // So the panel isn't blank when the tab is opened from the tab bar, before any slot has focus.
            showEntry_(first)
        }
    }

    root { _root }

    // The slot to focus when this tab opens - null when there is nothing in it.
    firstFocusable { _slots.count > 0 ? _slots[0] : null }

    // The slot that last had focus (or the first one).
    focused { _focused != null ? _focused : firstFocusable }

    // The weapon entries changed (another weapon equipped): moves the "E" marker and redraws the panel.
    refresh() {
        for (gear in _gear) {
            gear[1].setVisible(gear[0]["equipped"] == true)
        }
        if (_focusedEntry != null) showEntry_(_focusedEntry)
    }

    // Up from the top row goes back to this page's tab (instead of whichever tab happens to sit above that column).
    linkUpTo(tab) {
        if (_rows.count == 0) return
        for (slot in _rows[0]) {
            slot.setNavigationNeighbor(NavigationDirection.UP, tab)
        }
    }

    // Arrow keys move one slot at a time and stop at the grid's ends. Up / down step a row, jumping over section
    // headings into the next section; a column the next row doesn't reach lands on that row's last item.
    linkSlots_() {
        for (rowIndex in 0..._rows.count) {
            var row = _rows[rowIndex]
            for (columnIndex in 0...row.count) {
                var slot = row[columnIndex]
                InventoryTheme.link(slot, NavigationDirection.LEFT, columnIndex > 0 ? row[columnIndex - 1] : null)
                InventoryTheme.link(slot, NavigationDirection.RIGHT, columnIndex + 1 < row.count ? row[columnIndex + 1] : null)
                InventoryTheme.link(slot, NavigationDirection.UP, rowIndex > 0 ? ItemGridPage.columnOf_(_rows[rowIndex - 1], columnIndex) : null)
                InventoryTheme.link(slot, NavigationDirection.DOWN,
                    rowIndex + 1 < _rows.count ? ItemGridPage.columnOf_(_rows[rowIndex + 1], columnIndex) : null)
            }
        }
    }

    static columnOf_(row, column) { column < row.count ? row[column] : row[row.count - 1] }

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
            _focused = slot
            _focusedEntry = entry
            showEntry_(entry)
            onFocus.call()
        }, null)
        var tab = entry["opensTab"]
        if (tab != null) {
            slot.setOnClick { |sender, mousePos| _onOpenTab.call(tab) }
        }
        var equipItem = entry["equipItem"]
        if (equipItem != null) {
            // "E" in the top-left corner of the weapon in hand.
            var marker = InventoryTheme.label(slot, "E", Vec2.new(9, 5), Vec2.new(0, 0), Vec2.new(0, 0), 22.0, _boldFont,
                InventoryTheme.bad, 7)
            marker.setVisible(entry["equipped"] == true)
            _gear.add([entry, marker])
            slot.setOnClick { |sender, mousePos| _onEquip.call(equipItem) }
        }
        return slot
    }

    // A darkened, empty cell - focusable, so navigation reaches every row; focusing it clears the detail panel.
    buildEmptySlot_(scroll, position, onFocus) {
        var slot = InventoryTheme.frame(scroll, "cell", position, Vec2.new(0, 0), Vec2.new(SLOT_SIZE, SLOT_SIZE),
            Vec2.new(0, 0), 5)
        slot.setTint(InventoryTheme.emptyTint)
        InventoryTheme.focusable(slot, Fn.new {
            _focused = slot
            _focusedEntry = null
            showBlank_()
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
        var detail = InventoryTheme.frame(_root, "well", Vec2.new(DETAIL_X, 0), Vec2.new(0, 0), Vec2.new(DETAIL_WIDTH, 476),
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
            _bonusLines.add(InventoryTheme.label(detail, "", Vec2.new(0, BONUS_Y + i * BONUS_LINE), Vec2.new(0.5, 0),
                Vec2.new(0.5, 0), 22.0, _bookFont, InventoryTheme.good, 4))
        }

        // What pressing it does - "Select to view the map", "Select to equip" - under the bonuses (placeDetail_).
        _actionLabel = InventoryTheme.label(detail, "", Vec2.new(0, BONUS_Y), Vec2.new(0.5, 0), Vec2.new(0.5, 0), 22.0,
            _bookFont, InventoryTheme.accent, 4)
        // A weapon that would take equipped moves out of the loadout says so under that, in red.
        _warningLabel = InventoryTheme.label(detail, "", Vec2.new(0, BONUS_Y), Vec2.new(0.5, 0), Vec2.new(0.5, 0), 22.0,
            _boldFont, InventoryTheme.bad, 4)
        _warningDetail = InventoryTheme.label(detail, "", Vec2.new(0, BONUS_Y), Vec2.new(0.5, 0), Vec2.new(0.5, 0), 20.0,
            _bookFont, InventoryTheme.bad, 4)

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
        setText_(_actionLabel, entry["actionText"] != null ? entry["actionText"] : "")
        var warning = entry["warning"]
        setText_(_warningLabel, warning != null ? warning[0] : "")
        setText_(_warningDetail, warning != null ? warning[1] : "")

        var bonuses = entry["bonuses"]
        placeDetail_(bonuses.count)
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

    // The action line goes under however many bonus lines there are, the warning under that.
    placeDetail_(bonusCount) {
        var y = BONUS_Y + bonusCount * BONUS_LINE + (bonusCount > 0 ? 8 : 0)
        _actionLabel.setPosition(Vec2.new(0, y), Vec2.new(0.5, 0))
        _warningLabel.setPosition(Vec2.new(0, y + 34), Vec2.new(0.5, 0))
        _warningDetail.setPosition(Vec2.new(0, y + 62), Vec2.new(0.5, 0))
    }

    // Nothing focused worth describing (an empty cell): the panel goes blank.
    showBlank_() {
        if (_detailCell != null) {
            _detail.removeChild(_detailCell)
            _detailCell = null
        }
        showEmpty_("")
    }

    showEmpty_(text) {
        setText_(_nameLabel, "")
        setText_(_categoryLabel, "")
        setText_(_countLabel, "")
        setText_(_actionLabel, "")
        setText_(_warningLabel, "")
        setText_(_warningDetail, "")
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
        _rows = []
        _gear = []
        _focused = null
        _focusedEntry = null
        _onOpenTab = null
        _onEquip = null
        _warningLabel = null
        _warningDetail = null
        _root = null
        _detail = null
        _detailCell = null
        _nameLabel = null
        _categoryLabel = null
        _countLabel = null
        _actionLabel = null
        _bonusDivider = null
        _bonusLines = []
        _emptyLabel = null
    }
}
