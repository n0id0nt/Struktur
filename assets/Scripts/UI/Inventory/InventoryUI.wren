// UI/Inventory/InventoryUI.wren
// The inventory screen's view, built from the UI kit (UI/Theme/UIKit.wren): a metal window with a row of tabs
// (Items / Weapons / Armor / Moves / Party / Levels) between Q / E key prompts, the open tab's page below, and a
// footer of key prompts with the party's gold on the right. Each tab's page is its own view -
// UI/Inventory/ItemGridPage.wren, MovesPage.wren, PartyPage.wren, LevelsPage.wren - all built up front, only the
// open one visible (hidden ones drop out of focus navigation by themselves).
//
// Tabs switch with cycleTab (States/InventoryState.wren drives it from the UITab input - Q / E, the bumpers), or by
// moving focus up onto the tab bar and along it. Pure view: `model` is everything to show, prepared by the State:
//   { "tabs": [tab names], "itemPages": [[entries, emptyText] x3], "moves": {...}, "party": [...],
//     "partyLabels": {...}, "levels": [...], "levelLabels": {...}, "gold": Num,
//     "hints": { "tabs", "move", "select", "close" } }
// onFocusMoved is called with no arguments whenever focus lands on something new (the screen's focus sound);
// onEquip(slot, moveId) changes the player's loadout and returns the new one (see MovesPage).
import "app" for Application
import "math" for Vec2
import "ui" for UIManager, UIColor, NavigationDirection
import "UI/Theme/UIKit" for UIKit
import "UI/Inventory/InventoryTheme" for InventoryTheme
import "UI/Inventory/ItemGridPage" for ItemGridPage
import "UI/Inventory/MovesPage" for MovesPage
import "UI/Inventory/PartyPage" for PartyPage
import "UI/Inventory/LevelsPage" for LevelsPage

var WINDOW_SIZE = Vec2.new(1120, 620)
var CONTENT_X = 24            // inside the window frame's border
var TAB_Y = 24
var TAB_HEIGHT = 36           // the kit tab's 12px at 3x
var TAB_WIDTH = 150
var TAB_STRIDE = 156
var TAB_X = 80                // after the previous-tab prompt (room for a pad's LB, which is wider than Q)
var TAB_LABEL_Y = 10          // the tab art's top border is heavier and its bottom open - this centres the text inside
var BODY_Y = 72
var KEY_SCALE = 3             // the previous/next tab prompts by the tabs
var KEY_GAP = 6               // between those prompts and the tabs
var FOOTER_SIZE = 18.0
var FOOTER_Y = -44            // from the window's bottom edge
var GOLD_ICON_PATH = "Items/Treasure/GoldCoin.png"

class InventoryUI {
    construct new(model, onFocusMoved, onEquip) {
        UIKit.open()
        _boldFont = InventoryTheme.loadBoldFont()
        _bookFont = InventoryTheme.loadBookFont()
        _onFocusMoved = onFocusMoved
        _current = -1

        _screenPanel = InventoryTheme.fill(null, Vec2.new(0, 0), Vec2.new(0, 0),
            Vec2.new(Application.gameWidth, Application.gameHeight), Vec2.new(0, 0), InventoryTheme.overlay, 0)
        UIManager.addUIElement(_screenPanel)

        var window = InventoryTheme.frame(_screenPanel, "window", Vec2.new(0, 0), Vec2.new(0.5, 0.5), WINDOW_SIZE,
            Vec2.new(0, 0), 1)
        window.setAnchorPoint(Vec2.new(0.5, 0.5))

        buildTabBar_(window, model["tabs"])
        buildFooter_(window, model["hints"], model["gold"])

        var body = InventoryTheme.group(window, Vec2.new(CONTENT_X, BODY_Y), Vec2.new(1072, 476), 2)
        var onFocus = Fn.new { _onFocusMoved.call() }
        _pages = []
        for (page in model["itemPages"]) {
            _pages.add(ItemGridPage.new(body, page[0], page[1], _boldFont, _bookFont, onFocus))
        }
        _movesPage = MovesPage.new(body, model["moves"], _boldFont, _bookFont, onFocus, onEquip)
        _pages.add(_movesPage)
        _partyPage = PartyPage.new(body, model["party"], model["partyLabels"], _boldFont, _bookFont, onFocus)
        _pages.add(_partyPage)
        _pages.add(LevelsPage.new(body, model["levels"], model["levelLabels"], _boldFont, _bookFont, onFocus))

        // Along the tab bar left / right (stopping at the ends); down from a tab enters its page, and up from the
        // page's first row comes back to that tab.
        for (i in 0..._tabs.count) {
            InventoryTheme.link(_tabs[i], NavigationDirection.LEFT, i > 0 ? _tabs[i - 1] : null)
            InventoryTheme.link(_tabs[i], NavigationDirection.RIGHT, i + 1 < _tabs.count ? _tabs[i + 1] : null)
            InventoryTheme.link(_tabs[i], NavigationDirection.UP, null)
            var first = _pages[i].firstFocusable
            InventoryTheme.link(_tabs[i], NavigationDirection.DOWN, first)
            if (first != null) {
                _pages[i].linkUpTo(_tabs[i])
            }
        }

        selectTab_(0, true)
    }

    // Steps to the next (direction 1) or previous (-1) tab, wrapping round, and focuses the new page (whatever gains
    // focus plays the focus sound through its own onFocus).
    cycleTab(direction) {
        if (_screenPanel == null) {
            return
        }
        selectTab_((_current + direction + _tabs.count) % _tabs.count, true)
    }

    // Per-frame: animates the Moves tab's preview while it is open. `now` is unscaled time.
    update(now) {
        if (_screenPanel != null && _pages[_current] == _movesPage) {
            _movesPage.tick(now)
        }
    }

    // Esc: true when the open page used it (backing out of a half-made move swap) - otherwise the screen closes.
    cancel() {
        return _screenPanel != null && _pages[_current] == _movesPage && _movesPage.cancel()
    }

    // The kit's tabs between previous/next-tab prompts (Q / E, or LB / RB on a pad), sitting on a dark rule that runs
    // the width of the window. The prompts hug the tabs from outside, so a wider pad button grows away from them.
    buildTabBar_(window, names) {
        var middle = TAB_Y + TAB_HEIGHT / 2
        UIKit.hint(window, "UITab:negative", null, Vec2.new(TAB_X - KEY_GAP, middle), Vec2.new(0, 0), Vec2.new(1, 0.5),
            3).setGlyphScale(KEY_SCALE)
        _tabs = []
        _tabLabels = []
        _tabFocused = []
        for (i in 0...names.count) {
            var tab = InventoryTheme.frame(window, "tab", Vec2.new(TAB_X + i * TAB_STRIDE, TAB_Y), Vec2.new(0, 0),
                Vec2.new(TAB_WIDTH, TAB_HEIGHT), Vec2.new(0, 0), 2)
            tab.setFocusable(true)
            var label = InventoryTheme.label(tab, names[i], Vec2.new(0, TAB_LABEL_Y), Vec2.new(0.5, 0.5),
                Vec2.new(0.5, 0.5), 22.0, _boldFont, InventoryTheme.text, 3)
            _tabs.add(tab)
            _tabLabels.add(label)
            _tabFocused.add(false)

            // Moving onto a tab opens it; focus stays on the tab until the player steps down into the page.
            tab.setOnFocus { |sender|
                _tabFocused[i] = true
                selectTab_(i, false)
                _onFocusMoved.call()
            }
            tab.setOnLoseFocus { |sender|
                _tabFocused[i] = false
                styleTab_(i)
            }
        }
        UIKit.hint(window, "UITab:positive", null, Vec2.new(TAB_X + names.count * TAB_STRIDE, middle), Vec2.new(0, 0),
            Vec2.new(0, 0.5), 3).setGlyphScale(KEY_SCALE)

        var rule = UIColor.new(Vec2.new(CONTENT_X, TAB_Y + TAB_HEIGHT), Vec2.new(0, 0), Vec2.new(-2 * CONTENT_X, 3),
            Vec2.new(1, 0))
        rule.setColor(InventoryTheme.textDark)
        rule.setZIndex(3)
        window.addChild(rule)
    }

    // "[Q][E] Switch tab   [^][<][v][>] Move   [Space] Select   [I] [Esc] Close" along the bottom (the pad's
    // buttons when one is in use - one rich-text row, so it reflows as they change), gold on the right.
    buildFooter_(window, hints, gold) {
        var gap = "    "
        var markup = "[input=UITab] %(hints["tabs"])%(gap)[input=UIDir] %(hints["move"])%(gap)" +
            "[input=UIAccept] %(hints["select"])%(gap)[input=Inventory] [input=UICancel] %(hints["close"])"
        // -5: the rich label's own left padding, so the first prompt lines up with the content edge.
        UIKit.richText(window, markup, Vec2.new(CONTENT_X - 1, FOOTER_Y), Vec2.new(0, 1), Vec2.new(0, 0.5),
            FOOTER_SIZE, InventoryTheme.text, 3)

        var coin = UIKit.image(window, GOLD_ICON_PATH, Vec2.new(-CONTENT_X - 4, FOOTER_Y), Vec2.new(1, 1),
            Vec2.new(1, 0.5), 4, 3)
        InventoryTheme.label(window, "%(gold)", Vec2.new(-CONTENT_X - 4 - coin.getSize().x - 10, FOOTER_Y + 2),
            Vec2.new(1, 1), Vec2.new(1, 0.5), 28.0, _boldFont, InventoryTheme.accent, 3)
    }

    selectTab_(index, moveFocus) {
        if (_current != index) {
            _movesPage.cancel()
        }
        if (_pages[index] == _partyPage) {
            _partyPage.refresh()
        }
        _current = index
        for (i in 0..._pages.count) {
            _pages[i].setVisible(i == index)
            styleTab_(i)
        }
        if (moveFocus) {
            var first = _pages[index].firstFocusable
            UIManager.setFocus(first != null ? first : _tabs[index])
        }
    }

    // The kit's three tab looks: hover while the cursor is on it, selected while its page is open, otherwise the
    // darker unselected one. The first two have light insides, so their text goes dark.
    styleTab_(i) {
        var open = i == _current
        var style = _tabFocused[i] ? "tabHover" : (open ? "tabSelected" : "tab")
        UIKit.applyStyle(_tabs[i], style)
        _tabLabels[i].setTextColor((open || _tabFocused[i]) ? InventoryTheme.textDark : InventoryTheme.text)
    }

    teardown() {
        if (_screenPanel != null) {
            UIManager.removeUIElement(_screenPanel)
            _screenPanel = null
        }
        for (page in _pages) {
            page.release()
        }
        _pages = []
        _movesPage = null
        _partyPage = null
        _tabs = []
        _tabLabels = []
        _boldFont.unload()
        _bookFont.unload()
        UIKit.close()
    }
}
