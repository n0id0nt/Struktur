// UI/Inventory/PartyPage.wren
// The inventory's Party tab: one card per party member (players first, in party order) with their HP and stamina
// and the full attribute block (Combat/Stats.wren). Attributes above the average of 10 are green, below it red.
// Pure view: `members` are prepared by States/InventoryState.wren:
//   { "name", "className", "downed", "hp", "maxHp", "stamina", "maxStamina", "attributes": [[display text, value]] }
import "math" for Vec2
import "ui" for UIScroll, NavigationDirection
import "UI/Inventory/InventoryTheme" for InventoryTheme

var CARD_WIDTH = 256
var CARD_GAP = 16
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
            // Scrolls sideways once there are more members than fit (four do).
            var scroll = UIScroll.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(1072, 476), Vec2.new(0, 0))
            scroll.setVerticalScrollEnabled(false)
            scroll.setZIndex(3)
            _root.addChild(scroll)

            for (i in 0...members.count) {
                _cards.add(buildCard_(scroll, Vec2.new(i * (CARD_WIDTH + CARD_GAP), 0), members[i], onFocus))
            }
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

    buildCard_(scroll, position, member, onFocus) {
        var card = InventoryTheme.panel(scroll, position, Vec2.new(0, 0), Vec2.new(CARD_WIDTH, 476), Vec2.new(0, 0),
            InventoryTheme.panelBg, InventoryTheme.panelBorder, 2, 4)
        card.setFocusable(true)
        card.setOnFocus { |sender|
            card.setBorderColor(InventoryTheme.accent)
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

    release() {
        _cards = []
        _root = null
    }
}
