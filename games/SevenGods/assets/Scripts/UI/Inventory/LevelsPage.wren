// UI/Inventory/LevelsPage.wren
// The inventory's Levels tab: a row per player (NPC party members don't level) with their level and progress to the
// next. There are no levels or experience in the game yet - States/InventoryState.wren fills every row with
// placeholder numbers and this page says so at the top. Pure view: `members` are { "name", "level", "xp", "xpToNext" }.
import "math" for Vec2
import "ui" for UIScroll, NavigationDirection
import "UI/Inventory/InventoryTheme" for InventoryTheme

var ROW_HEIGHT = 84
var ROW_GAP = 12
var ROW_INSET = 4   // room for the focus ring inside the scroll's clip

class LevelsPage {
    construct new(parent, members, labels, boldFont, bookFont, onFocus) {
        _rows = []
        _root = InventoryTheme.group(parent, Vec2.new(0, 0), Vec2.new(1072, 476), 2)

        InventoryTheme.label(_root, labels["placeholder"], Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(0, 0), 20.0, bookFont,
            InventoryTheme.textMuted, 3)

        var scroll = UIScroll.new(Vec2.new(0, 40), Vec2.new(0, 0), Vec2.new(1072, 436), Vec2.new(0, 0))
        scroll.setVerticalScrollEnabled(true)
        scroll.setZIndex(3)
        _root.addChild(scroll)

        for (i in 0...members.count) {
            var member = members[i]
            var row = InventoryTheme.frame(scroll, "tile", Vec2.new(ROW_INSET, ROW_INSET + i * (ROW_HEIGHT + ROW_GAP)),
                Vec2.new(0, 0), Vec2.new(1072 - 2 * ROW_INSET, ROW_HEIGHT), Vec2.new(0, 0), 4)
            InventoryTheme.focusable(row, onFocus, null)
            _rows.add(row)

            InventoryTheme.label(row, member["name"], Vec2.new(24, 5), Vec2.new(0, 0.5), Vec2.new(0, 0.5), 30.0, boldFont,
                InventoryTheme.text, 5)
            InventoryTheme.label(row, "%(labels["level"]) %(member["level"])", Vec2.new(300, 14), Vec2.new(0, 0),
                Vec2.new(0, 0), 26.0, boldFont, InventoryTheme.accent, 5)
            InventoryTheme.label(row, "%(labels["xp"]) %(member["xp"]) / %(member["xpToNext"])", Vec2.new(-24, 16),
                Vec2.new(1, 0), Vec2.new(1, 0), 18.0, bookFont, InventoryTheme.textMuted, 5)
            var fraction = member["xpToNext"] > 0 ? member["xp"] / member["xpToNext"] : 0
            InventoryTheme.bar(row, Vec2.new(300, 52), 1072 - 2 * ROW_INSET - 324, 15, fraction, InventoryTheme.xpFill, 5)
        }

        // A column of rows: up / down along it, nothing to the sides.
        for (i in 0..._rows.count) {
            InventoryTheme.link(_rows[i], NavigationDirection.UP, i > 0 ? _rows[i - 1] : null)
            InventoryTheme.link(_rows[i], NavigationDirection.DOWN, i + 1 < _rows.count ? _rows[i + 1] : null)
            InventoryTheme.link(_rows[i], NavigationDirection.LEFT, null)
            InventoryTheme.link(_rows[i], NavigationDirection.RIGHT, null)
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

    release() {
        _rows = []
        _root = null
    }
}
