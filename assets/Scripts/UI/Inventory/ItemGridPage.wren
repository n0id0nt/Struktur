// UI/Inventory/ItemGridPage.wren
// One item tab of the inventory (Items / Weapons / Armor): a scrolling grid of slots on the left - each with the
// item's icon and, bottom right, how many there are - and a detail panel on the right for the focused slot.
// Pure view: `entries` are already-prepared maps from States/InventoryState.wren:
//   { "name", "icon" (texture path or null), "count", "stackable", "showCount" (the slot's count badge - stacks of
//     2+), "countText" (the detail panel's "Quantity: n"), "category" (display text),
//     "bonuses" ([[display text, amount], ...] - gear only) }
import "math" for Vec2
import "resourceManager" for Texture
import "ui" for UIScroll, UITexture, NavigationDirection
import "UI/Inventory/InventoryTheme" for InventoryTheme

var COLUMNS = 6
var SLOT_SIZE = 76
var SLOT_STRIDE = 88
var MIN_SLOTS = 30   // five full rows, so the grid never looks empty
var ICON_SIZE = 60
var DETAIL_ICON_SIZE = 144
var MAX_BONUS_LINES = 4

class ItemGridPage {
    // parent: the inventory body. onFocus: called with no arguments whenever a slot gains focus (the screen's
    // focus sound).
    construct new(parent, entries, emptyText, boldFont, bookFont, onFocus) {
        _textures = []
        _slots = []
        _slotTextures = []   // per filled slot, its icon texture (null when it has no art)
        _boldFont = boldFont
        _bookFont = bookFont

        _root = InventoryTheme.group(parent, Vec2.new(0, 0), Vec2.new(1072, 476), 2)

        var gridFrame = InventoryTheme.panel(_root, Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(560, 476), Vec2.new(0, 0),
            InventoryTheme.panelBg, InventoryTheme.panelBorder, 2, 3)
        var scroll = UIScroll.new(Vec2.new(20, 20), Vec2.new(0, 0), Vec2.new(524, 440), Vec2.new(0, 0))
        scroll.setHorizontalScrollEnabled(false)
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
            var position = Vec2.new(2 + (i % COLUMNS) * SLOT_STRIDE, 2 + (i / COLUMNS).floor * SLOT_STRIDE)
            if (i < entries.count) {
                _slots.add(buildSlot_(scroll, position, entries[i], onFocus))
            } else {
                InventoryTheme.panel(scroll, position, Vec2.new(0, 0), Vec2.new(SLOT_SIZE, SLOT_SIZE), Vec2.new(0, 0),
                    InventoryTheme.slotEmptyBg, InventoryTheme.panelBorder, 1, 5)
            }
        }

        linkSlots_()
        if (entries.count == 0) {
            showEmpty_(emptyText)
        } else {
            // So the panel isn't blank when the tab is opened from the tab bar, before any slot has focus.
            showEntry_(entries[0], _slotTextures[0])
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
        var slot = InventoryTheme.panel(scroll, position, Vec2.new(0, 0), Vec2.new(SLOT_SIZE, SLOT_SIZE), Vec2.new(0, 0),
            InventoryTheme.slotBg, InventoryTheme.panelBorder, 2, 5)
        slot.setFocusable(true)

        var texture = null
        if (entry["icon"] != null) {
            texture = Texture.load(entry["icon"])
            _textures.add(texture)
            var icon = UITexture.new(Vec2.new(0, 0), Vec2.new(0.5, 0.5), Vec2.new(ICON_SIZE, ICON_SIZE), Vec2.new(0, 0))
            icon.setAnchorPoint(Vec2.new(0.5, 0.5))
            icon.setTexture(texture)
            icon.setZIndex(6)
            slot.addChild(icon)
        } else {
            InventoryTheme.label(slot, "?", Vec2.new(0, 4), Vec2.new(0.5, 0.5), Vec2.new(0.5, 0.5), 40.0, _boldFont,
                InventoryTheme.textMuted, 6)
        }

        if (entry["showCount"]) {
            InventoryTheme.label(slot, "%(entry["count"])", Vec2.new(-6, -2), Vec2.new(1, 1), Vec2.new(1, 1), 22.0,
                _boldFont, InventoryTheme.text, 7)
        }

        _slotTextures.add(texture)
        slot.setOnFocus { |sender|
            slot.setBackgroundColor(InventoryTheme.slotFocusBg)
            slot.setBorderColor(InventoryTheme.accent)
            showEntry_(entry, texture)
            onFocus.call()
        }
        slot.setOnLoseFocus { |sender|
            slot.setBackgroundColor(InventoryTheme.slotBg)
            slot.setBorderColor(InventoryTheme.panelBorder)
        }
        return slot
    }

    // --- detail panel -------------------------------------------------------------------

    buildDetail_() {
        var detail = InventoryTheme.panel(_root, Vec2.new(576, 0), Vec2.new(0, 0), Vec2.new(496, 476), Vec2.new(0, 0),
            InventoryTheme.panelBg, InventoryTheme.panelBorder, 2, 3)
        _detail = detail

        _iconFrame = InventoryTheme.panel(detail, Vec2.new(0, 28), Vec2.new(0.5, 0), Vec2.new(168, 168), Vec2.new(0, 0),
            InventoryTheme.slotBg, InventoryTheme.panelBorder, 2, 4)
        _iconFrame.setAnchorPoint(Vec2.new(0.5, 0))
        _detailIcon = UITexture.new(Vec2.new(0, 0), Vec2.new(0.5, 0.5), Vec2.new(DETAIL_ICON_SIZE, DETAIL_ICON_SIZE), Vec2.new(0, 0))
        _detailIcon.setAnchorPoint(Vec2.new(0.5, 0.5))
        _detailIcon.setZIndex(5)
        _iconFrame.addChild(_detailIcon)
        _detailNoIcon = InventoryTheme.label(_iconFrame, "?", Vec2.new(0, 8), Vec2.new(0.5, 0.5), Vec2.new(0.5, 0.5), 80.0,
            _boldFont, InventoryTheme.textMuted, 5)

        _nameLabel = InventoryTheme.label(detail, "", Vec2.new(0, 216), Vec2.new(0.5, 0), Vec2.new(0.5, 0), 34.0,
            _boldFont, InventoryTheme.text, 4)
        _categoryLabel = InventoryTheme.label(detail, "", Vec2.new(0, 260), Vec2.new(0.5, 0), Vec2.new(0.5, 0), 20.0,
            _bookFont, InventoryTheme.accent, 4)
        _countLabel = InventoryTheme.label(detail, "", Vec2.new(0, 290), Vec2.new(0.5, 0), Vec2.new(0.5, 0), 20.0,
            _bookFont, InventoryTheme.textMuted, 4)

        _bonusDivider = InventoryTheme.divider(detail, 330, 40, 4)
        _bonusLines = []
        for (i in 0...MAX_BONUS_LINES) {
            _bonusLines.add(InventoryTheme.label(detail, "", Vec2.new(0, 346 + i * 28), Vec2.new(0.5, 0),
                Vec2.new(0.5, 0), 22.0, _bookFont, InventoryTheme.good, 4))
        }

        _emptyLabel = InventoryTheme.label(detail, "", Vec2.new(0, 0), Vec2.new(0.5, 0.5), Vec2.new(0.5, 0.5), 24.0,
            _bookFont, InventoryTheme.textMuted, 4)
        _emptyLabel.setVisible(false)
    }

    showEntry_(entry, texture) {
        _iconFrame.setVisible(true)
        _emptyLabel.setVisible(false)
        if (texture != null) {
            _detailIcon.setTexture(texture)
            _detailIcon.setVisible(true)
            _detailNoIcon.setVisible(false)
        } else {
            _detailIcon.setVisible(false)
            _detailNoIcon.setVisible(true)
        }
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
        _iconFrame.setVisible(false)
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

    // Call after the UI tree is removed - the item icons stay loaded until then.
    unloadTextures() {
        for (texture in _textures) {
            texture.unload()
        }
        _textures = []
        _slots = []
        _slotTextures = []
        _root = null
        _detail = null
        _iconFrame = null
        _detailIcon = null
        _detailNoIcon = null
        _nameLabel = null
        _categoryLabel = null
        _countLabel = null
        _bonusDivider = null
        _bonusLines = []
        _emptyLabel = null
    }
}
