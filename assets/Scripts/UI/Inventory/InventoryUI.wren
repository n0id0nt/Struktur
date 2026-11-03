// UI/Inventory/InventoryUI.wren
// The inventory screen's view: a window with a tab bar (Items / Weapons / Armor / Party / Levels), the party's gold
// in the top right, the open tab's page below and a controls hint along the bottom. Each tab's page is its own
// view - UI/Inventory/ItemGridPage.wren, PartyPage.wren, LevelsPage.wren - all built up front, only the open one
// visible (hidden ones drop out of focus navigation by themselves).
//
// Tabs switch with cycleTab (States/InventoryState.wren drives it from the UITab input - Q / E, the bumpers), or by
// moving focus up onto the tab bar and along it. Pure view: `model` is everything to show, prepared by the State:
//   { "tabs": [tab names], "itemPages": [[entries, emptyText] x3], "party": [...], "partyLabels": {...},
//     "levels": [...], "levelLabels": {...}, "gold": Num, "hint": String }
// onFocusMoved is called with no arguments whenever focus lands on something new (the screen's focus sound).
import "app" for Application
import "math" for Vec2
import "resourceManager" for Texture
import "ui" for UIManager, UITexture, NavigationDirection
import "UI/Inventory/InventoryTheme" for InventoryTheme
import "UI/Inventory/ItemGridPage" for ItemGridPage
import "UI/Inventory/PartyPage" for PartyPage
import "UI/Inventory/LevelsPage" for LevelsPage

var WINDOW_SIZE = Vec2.new(1120, 620)
var TAB_WIDTH = 140
var TAB_STRIDE = 148
var TAB_X = 64
var GOLD_ICON_PATH = "Tiles/Items/Gold Coins.png"

class InventoryUI {
    construct new(model, onFocusMoved) {
        _boldFont = InventoryTheme.loadBoldFont()
        _bookFont = InventoryTheme.loadBookFont()
        _onFocusMoved = onFocusMoved
        _current = -1

        _screenPanel = InventoryTheme.panel(null, Vec2.new(0, 0), Vec2.new(0, 0),
            Vec2.new(Application.gameWidth, Application.gameHeight), Vec2.new(0, 0), InventoryTheme.overlay, null, 0, 0)
        UIManager.addUIElement(_screenPanel)

        var window = InventoryTheme.panel(_screenPanel, Vec2.new(0, 0), Vec2.new(0.5, 0.5), WINDOW_SIZE, Vec2.new(0, 0),
            InventoryTheme.windowBg, InventoryTheme.accent, 2, 1)
        window.setAnchorPoint(Vec2.new(0.5, 0.5))

        buildTabBar_(window, model["tabs"])
        buildGold_(window, model["gold"])
        InventoryTheme.divider(window, 72, 24, 2)
        InventoryTheme.label(window, model["hint"], Vec2.new(24, -38), Vec2.new(0, 1), Vec2.new(0, 0), 18.0, _bookFont,
            InventoryTheme.textMuted, 2)

        var body = InventoryTheme.group(window, Vec2.new(24, 88), Vec2.new(1072, 476), 2)
        var onFocus = Fn.new { _onFocusMoved.call() }
        _itemPages = []
        _pages = []
        for (page in model["itemPages"]) {
            var view = ItemGridPage.new(body, page[0], page[1], _boldFont, _bookFont, onFocus)
            _itemPages.add(view)
            _pages.add(view)
        }
        _partyPage = PartyPage.new(body, model["party"], model["partyLabels"], _boldFont, _bookFont, onFocus)
        _pages.add(_partyPage)
        _levelsPage = LevelsPage.new(body, model["levels"], model["levelLabels"], _boldFont, _bookFont, onFocus)
        _pages.add(_levelsPage)

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

    buildTabBar_(window, names) {
        InventoryTheme.label(window, "Q", Vec2.new(TAB_X - 22, 26), Vec2.new(0, 0), Vec2.new(0.5, 0), 22.0, _boldFont,
            InventoryTheme.textMuted, 3)
        _tabs = []
        _tabLabels = []
        _tabFocused = []
        for (i in 0...names.count) {
            var tab = InventoryTheme.panel(window, Vec2.new(TAB_X + i * TAB_STRIDE, 18), Vec2.new(0, 0),
                Vec2.new(TAB_WIDTH, 40), Vec2.new(0, 0), InventoryTheme.slotBg, InventoryTheme.panelBorder, 2, 2)
            tab.setFocusable(true)
            var label = InventoryTheme.label(tab, names[i], Vec2.new(0, 4), Vec2.new(0.5, 0.5), Vec2.new(0.5, 0.5), 22.0,
                _boldFont, InventoryTheme.textMuted, 3)
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
        InventoryTheme.label(window, "E", Vec2.new(TAB_X + names.count * TAB_STRIDE + 14, 26), Vec2.new(0, 0),
            Vec2.new(0.5, 0), 22.0, _boldFont, InventoryTheme.textMuted, 3)
    }

    buildGold_(window, gold) {
        _goldTexture = Texture.load(GOLD_ICON_PATH)
        var icon = UITexture.new(Vec2.new(-24, 20), Vec2.new(1, 0), Vec2.new(36, 36), Vec2.new(0, 0))
        icon.setAnchorPoint(Vec2.new(1, 0))
        icon.setTexture(_goldTexture)
        icon.setZIndex(3)
        window.addChild(icon)
        InventoryTheme.label(window, "%(gold)", Vec2.new(-68, 22), Vec2.new(1, 0), Vec2.new(1, 0), 30.0, _boldFont,
            InventoryTheme.accent, 3)
    }

    selectTab_(index, moveFocus) {
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

    // Open tab: gold with dark text. Others: dark with muted text. The tab the cursor is on gets a light border.
    styleTab_(i) {
        var open = i == _current
        _tabs[i].setBackgroundColor(open ? InventoryTheme.accent : InventoryTheme.slotBg)
        _tabLabels[i].setTextColor(open ? InventoryTheme.textDark : InventoryTheme.textMuted)
        _tabs[i].setBorderColor(_tabFocused[i] ? InventoryTheme.text : InventoryTheme.panelBorder)
    }

    teardown() {
        if (_screenPanel != null) {
            UIManager.removeUIElement(_screenPanel)
            _screenPanel = null
        }
        for (page in _itemPages) {
            page.unloadTextures()
        }
        _partyPage.release()
        _levelsPage.release()
        _itemPages = []
        _pages = []
        _tabs = []
        _tabLabels = []
        if (_goldTexture != null) {
            _goldTexture.unload()
            _goldTexture = null
        }
        _boldFont.unload()
        _bookFont.unload()
    }
}
