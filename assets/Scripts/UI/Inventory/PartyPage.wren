// UI/Inventory/PartyPage.wren
// The inventory's Party tab: one card per party member (players first, in party order) with their HP and stamina
// and the full attribute block (Combat/Stats.wren) - attributes above the average of 10 are green, below it red -
// and, on the right, the moves of whichever card is selected. Two cards fit; more scroll sideways.
// Pure view: `members` are prepared by States/InventoryState.wren:
//   { "name", "className", "downed", "hp", "maxHp", "stamina", "maxStamina", "attributes": [[display text, value]],
//     "moves": [{ "name", "time", "damage", "staminaCost", "staminaRestore", "heal" }] }
import "math" for Vec2
import "ui" for UIScroll, NavigationDirection
import "UI/Inventory/InventoryTheme" for InventoryTheme
import "UI/Inventory/MoveCells" for MoveCells

var CARD_WIDTH = 256
var CARD_GAP = 16
var CARDS_WIDTH = 528   // two cards
var MOVES_X = 544
var MOVES_WIDTH = 528
var MAX_MOVES = 8
var MOVE_ROW_Y = 104
var MOVE_ROW_STRIDE = 42
// Right edges of the number columns in the moves panel (the move name is left-aligned at 24).
var TIME_X = 236
var DAMAGE_X = 326
var STAMINA_X = 424
var HEAL_X = 504
var AVERAGE_ATTRIBUTE = 10

class PartyPage {
    construct new(parent, members, labels, boldFont, bookFont, onFocus) {
        _cards = []
        _boldFont = boldFont
        _bookFont = bookFont
        _labels = labels

        _root = InventoryTheme.group(parent, Vec2.new(0, 0), Vec2.new(1072, 476), 2)
        if (members.count == 0) {
            InventoryTheme.label(_root, labels["empty"], Vec2.new(0, 0), Vec2.new(0.5, 0.5), Vec2.new(0.5, 0.5), 24.0,
                bookFont, InventoryTheme.textMuted, 3)
        } else {
            // Scrolls sideways once there are more members than fit (two do).
            var scroll = UIScroll.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(CARDS_WIDTH, 476), Vec2.new(0, 0))
            scroll.setVerticalScrollEnabled(false)
            // Only scroll once the selected card would be off screen, and then by one card - so both visible cards
            // stay whole (the default deadzone re-centres on every card, leaving halves at the edges).
            scroll.setFocusDeadzone(Vec2.new((CARDS_WIDTH - CARD_WIDTH) / 2, 0))
            scroll.setZIndex(3)
            _root.addChild(scroll)

            buildMoves_()
            for (i in 0...members.count) {
                _cards.add(buildCard_(scroll, Vec2.new(i * (CARD_WIDTH + CARD_GAP), 0), members[i], onFocus))
            }
            showMoves_(members[0])
            // A row of cards: left / right along it, nothing below.
            for (i in 0..._cards.count) {
                InventoryTheme.link(_cards[i], NavigationDirection.LEFT, i > 0 ? _cards[i - 1] : null)
                InventoryTheme.link(_cards[i], NavigationDirection.RIGHT, i + 1 < _cards.count ? _cards[i + 1] : null)
                InventoryTheme.link(_cards[i], NavigationDirection.UP, null)
                InventoryTheme.link(_cards[i], NavigationDirection.DOWN, null)
            }
        }
    }

    root { _root }
    firstFocusable { _cards.count > 0 ? _cards[0] : null }

    linkUpTo(tab) {
        for (card in _cards) {
            card.setNavigationNeighbor(NavigationDirection.UP, tab)
        }
    }

    setVisible(visible) { _root.setVisible(visible) }

    // Redraws the moves panel for whoever it shows - the player's moves may have changed in the Moves tab (the
    // State updates the member maps this page holds).
    refresh() {
        if (_shownMember != null) {
            showMoves_(_shownMember)
        }
    }

    buildCard_(scroll, position, member, onFocus) {
        var card = InventoryTheme.panel(scroll, position, Vec2.new(0, 0), Vec2.new(CARD_WIDTH, 476), Vec2.new(0, 0),
            InventoryTheme.panelBg, InventoryTheme.panelBorder, 2, 4)
        card.setFocusable(true)
        card.setOnFocus { |sender|
            card.setBorderColor(InventoryTheme.accent)
            showMoves_(member)
            onFocus.call()
        }
        card.setOnLoseFocus { |sender| card.setBorderColor(InventoryTheme.panelBorder) }

        InventoryTheme.label(card, member["name"], Vec2.new(20, 16), Vec2.new(0, 0), Vec2.new(0, 0), 30.0, _boldFont,
            InventoryTheme.text, 5)
        InventoryTheme.label(card, member["className"], Vec2.new(20, 56), Vec2.new(0, 0), Vec2.new(0, 0), 18.0, _bookFont,
            InventoryTheme.accent, 5)
        if (member["downed"]) {
            InventoryTheme.label(card, _labels["downed"], Vec2.new(-20, 56), Vec2.new(1, 0), Vec2.new(1, 0), 18.0,
                _bookFont, InventoryTheme.bad, 5)
        }

        poolRow_(card, 94, _labels["hp"], member["hp"], member["maxHp"], InventoryTheme.hpFill)
        poolRow_(card, 146, _labels["stamina"], member["stamina"], member["maxStamina"], InventoryTheme.staminaFill)

        InventoryTheme.divider(card, 200, 20, 5)

        var y = 214
        for (attribute in member["attributes"]) {
            var value = attribute[1]
            var color = InventoryTheme.text
            if (value > AVERAGE_ATTRIBUTE) {
                color = InventoryTheme.good
            } else if (value < AVERAGE_ATTRIBUTE) {
                color = InventoryTheme.bad
            }
            InventoryTheme.label(card, attribute[0], Vec2.new(20, y), Vec2.new(0, 0), Vec2.new(0, 0), 20.0, _bookFont,
                InventoryTheme.textMuted, 5)
            InventoryTheme.label(card, "%(value)", Vec2.new(-20, y), Vec2.new(1, 0), Vec2.new(1, 0), 20.0, _boldFont,
                color, 5)
            y = y + 31
        }
        return card
    }

    // "HP          95 / 120" over a bar.
    poolRow_(card, y, name, value, max, fill) {
        InventoryTheme.label(card, name, Vec2.new(20, y), Vec2.new(0, 0), Vec2.new(0, 0), 18.0, _bookFont,
            InventoryTheme.textMuted, 5)
        InventoryTheme.label(card, "%(value) / %(max)", Vec2.new(-20, y), Vec2.new(1, 0), Vec2.new(1, 0), 18.0, _bookFont,
            InventoryTheme.text, 5)
        InventoryTheme.bar(card, Vec2.new(20, y + 26), CARD_WIDTH - 40, 10, max > 0 ? value / max : 0, fill, 5)
    }

    // --- moves panel ----------------------------------------------------------------------

    // Built once with MAX_MOVES empty rows; showMoves_ fills in the selected member's.
    buildMoves_() {
        var panel = InventoryTheme.panel(_root, Vec2.new(MOVES_X, 0), Vec2.new(0, 0), Vec2.new(MOVES_WIDTH, 476),
            Vec2.new(0, 0), InventoryTheme.panelBg, InventoryTheme.panelBorder, 2, 3)
        InventoryTheme.label(panel, _labels["moves.title"], Vec2.new(24, 16), Vec2.new(0, 0), Vec2.new(0, 0), 30.0,
            _boldFont, InventoryTheme.text, 5)
        _movesOwner = InventoryTheme.label(panel, "", Vec2.new(-24, 24), Vec2.new(1, 0), Vec2.new(1, 0), 20.0, _bookFont,
            InventoryTheme.accent, 5)
        InventoryTheme.divider(panel, 62, 20, 5)

        header_(panel, _labels["moves.move"], 24, false)
        header_(panel, _labels["moves.time"], TIME_X, true)
        header_(panel, _labels["moves.damage"], DAMAGE_X, true)
        header_(panel, _labels["moves.stamina"], STAMINA_X, true)
        header_(panel, _labels["moves.heal"], HEAL_X, true)

        _moveRows = []
        for (i in 0...MAX_MOVES) {
            var y = MOVE_ROW_Y + i * MOVE_ROW_STRIDE
            var row = InventoryTheme.panel(panel, Vec2.new(12, y), Vec2.new(0, 0), Vec2.new(MOVES_WIDTH - 24, 36),
                Vec2.new(0, 0), InventoryTheme.slotEmptyBg, InventoryTheme.panelBorder, 0, 4)
            var cells = {}
            cells["row"] = row
            cells["name"] = cell_(row, 12, false, 22.0)
            cells["time"] = cell_(row, TIME_X - 12, true, 20.0)
            cells["damage"] = cell_(row, DAMAGE_X - 12, true, 20.0)
            cells["stamina"] = cell_(row, STAMINA_X - 12, true, 20.0)
            cells["heal"] = cell_(row, HEAL_X - 12, true, 20.0)
            _moveRows.add(cells)
        }
        _noMoves = InventoryTheme.label(panel, _labels["moves.none"], Vec2.new(0, 0), Vec2.new(0.5, 0.5),
            Vec2.new(0.5, 0.5), 24.0, _bookFont, InventoryTheme.textMuted, 5)
    }

    header_(panel, text, x, rightAligned) {
        InventoryTheme.label(panel, text, Vec2.new(x, 76), Vec2.new(0, 0), Vec2.new(rightAligned ? 1 : 0, 0), 16.0,
            _bookFont, InventoryTheme.textMuted, 5)
    }

    // One cell of a move row - x is its left edge, or its right edge when rightAligned (the number columns).
    cell_(row, x, rightAligned, size) {
        return InventoryTheme.label(row, "", Vec2.new(x, 7), Vec2.new(0, 0), Vec2.new(rightAligned ? 1 : 0, 0), size,
            _boldFont, InventoryTheme.text, 6)
    }

    showMoves_(member) {
        _shownMember = member
        setText_(_movesOwner, member["name"])
        var moves = member["moves"]
        _noMoves.setVisible(moves.count == 0)
        for (i in 0...MAX_MOVES) {
            var cells = _moveRows[i]
            cells["row"].setVisible(i < moves.count)
            if (i < moves.count) {
                var move = moves[i]
                setText_(cells["name"], move["name"])
                MoveCells.apply(cells["time"], MoveCells.time(move))
                MoveCells.apply(cells["damage"], MoveCells.damage(move))
                MoveCells.apply(cells["stamina"], MoveCells.stamina(move))
                MoveCells.apply(cells["heal"], MoveCells.heal(move))
            }
        }
    }

    setText_(label, text) {
        label.setText(text)
        label.setBoundingBoxToText()
    }

    release() {
        _cards = []
        _moveRows = []
        _movesOwner = null
        _noMoves = null
        _shownMember = null
        _root = null
    }
}
