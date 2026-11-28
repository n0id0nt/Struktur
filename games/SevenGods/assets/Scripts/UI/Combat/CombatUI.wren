// UI/Combat/CombatUI.wren
// The battle screen, built from the UI kit (UI/Theme/UIKit.wren) as a frame around the arena so nothing sits on the
// fighters (Combat/BattleStage.wren places the grid in the strip between):
//   - combatant cards stacked down the top corners: the party on the left, the opposition (health only) on the right
//     (UI/Combat/CombatantUI);
//   - a message banner just under them: the battle log, call-outs and tips;
//   - a full-width bar along the bottom: the shared timeline on the left (UI/Combat/TimelineUI - every combatant's
//     committed moves on one time axis) and the command panel on the right;
//   - a prompt box over the middle for tutorial lines (speaker + text, Accept to continue);
//   - floating text over the fighters: damage, "Blocked", "Dodged", reactions.
//
// The command panel is where a chain is built (how full it is shows on the timeline's budget marker). It always shows
// whose chain it is, then one screen at a time (States/CombatState.wren decides which):
//   main menu   Moves / Items / Flee - one row of big buttons (setSlots, style "menu");
//   Moves       the five loadout moves and Hold - a 3 x 2 grid, each a name over its time and cost ("grid");
//   Hold        the six Hold lengths - a 3 x 2 grid ("grid");
//   Items       a scrolling list (icon, name, count) with the focused item's picture, effect and description beside it
//               (showItems);
//   ready       Start / Undo last ("pair").
// A detail line under the buttons describes whatever has focus, and the hint line of button prompts runs along the
// panel's bottom edge, under everything. There's no Back button - Esc goes back (the State reads it).
//
// Pure construction + refresh: it never reads a Combatant or the engine and never calls back into game logic except
// through the callbacks States/CombatState.wren sets (onSlotClick / onSlotFocus, onItemClick / onItemFocus).
import "ui" for UIManager, UIPanel, UIColor, UILabel, UIRichLabel, UIScroll, TextWrapping, NavigationDirection
import "math" for Math, Vec2, Vec4
import "app" for Application, Time
import "Util/Colors" for BLANK
import "UI/Combat/CombatantUI" for CombatantUI
import "UI/Combat/TimelineUI" for TimelineUI
import "UI/Combat/MoveButtonUI" for MoveButtonUI
import "UI/Theme/UIKit" for UIKit

var TOP_EDGE = 12
var CARD_WIDTH = 196
var CARD_GAP = 8
var MESSAGE_Y = 132           // the message banner's centre, down from the screen's top
var MESSAGE_SIZE = 18.0
var MESSAGE_PAD_X = 22
var MESSAGE_PAD_Y = 10
var BAR_HEIGHT = 252          // the bar along the bottom
var BAR_PAD = 24              // inside the bar's window (its rim is 5px art at 4x = 20px)
var COMMAND_WIDTH = 738       // the command panel: everything right of the timeline (24 + 470 + 24 gap)
var HINT_HEIGHT = 30          // the button-prompt line along the panel's bottom (its glyphs stand a little taller)
var SLOT_COUNT = 6             // the button pool: the biggest screen is a 3 x 2 grid
var SLOT_GAP_X = 8
var SLOT_GAP_Y = 6
// Per screen style: [columns, button height, button layout] - see MoveButtonUI.setLayout.
var STYLES = {
    "menu": [3, 58, "center"],
    "grid": [3, 50, "stacked"],
    "pair": [2, 58, "center"]
}
var ITEM_ROWS = 12             // the Items list's row pool
var ITEM_LIST_WIDTH = 290
var ITEM_BAR_WIDTH = 8         // the list's scroll bar, shown only when the list is longer than it is tall
var ITEM_HINT_GAP = 8          // air between the item list's frame and the hint line
var ITEM_ROW_HEIGHT = 34
var ITEM_ROW_GAP = 4
var ITEM_ICON = 24             // px an item's icon is fitted to in its row
var ITEM_CELL = 64             // the focused item's picture cell
var ITEM_PICTURE = 40
var HEADER_HEIGHT = 26
var DETAIL_SIZE = 18.0
var DETAIL_LINE = 22          // px per wrapped line of the detail text
var DETAIL_LIFT = 4           // the rich label's text starts this far below its box top
var TIMELINE_ROWS = 8
var TIMELINE_WIDTH = 470      // the timeline strip, names included - a guide, kept compact
var POPUPS = 16
var POPUP_TIME = 0.9
var PROMPT_WIDTH = 760
var PROMPT_HEIGHT = 170

class CombatUI {
    // partySlots / enemySlots: how many cards each side gets (the opposition's may fill up mid-fight - a boss's
    // shards).
    construct new(partySlots, enemySlots) {
        UIKit.open()
        var gw = Application.gameWidth
        var gh = Application.gameHeight
        var font = UIKit.font()
        _font = font
        _onSlotClick = null
        _onSlotFocus = null
        _slotCount = 0
        _messageUntil = null

        _root = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(gw, gh), Vec2.new(0, 0))
        _root.setBackgroundColor(BLANK)
        _root.setBorderColor(BLANK)
        _root.setZIndex(-1)
        UIManager.addUIElement(_root)

        // Cards: a column down each top corner, beside the grid - the party on the left, the opposition on the right.
        _partyCards = CombatUI.cardColumn_(_root, TOP_EDGE, partySlots, true, font)
        _enemyCards = CombatUI.cardColumn_(_root, gw - TOP_EDGE - CARD_WIDTH, enemySlots, false, font)

        _bar = UIKit.frame(_root, "window", Vec2.new(0, 0), Vec2.new(0, 1), Vec2.new(gw, BAR_HEIGHT), Vec2.new(0, 0), 0)
        _bar.setAnchorPoint(Vec2.new(0, 1))

        var commandX = gw - BAR_PAD - COMMAND_WIDTH
        var innerHeight = BAR_HEIGHT - 2 * BAR_PAD
        // The timeline's rows share the panel's whole height (TimelineUI: four rows fill it, more shrink to fit).
        _timeline = TimelineUI.new(_bar, BAR_PAD, BAR_PAD - 4, TIMELINE_WIDTH.min(commandX - BAR_PAD - 20), TIMELINE_ROWS, font,
            innerHeight + 2)
        buildCommand_(font, commandX, innerHeight)
        // Battle log / call-outs: in the command area while the clock runs (the panel is hidden then), on a dark
        // well, word-wrapped - off the arena, so it never covers a fighter.
        _messageWell = UIKit.frame(_bar, "well", Vec2.new(commandX, BAR_PAD), Vec2.new(0, 0), Vec2.new(COMMAND_WIDTH, innerHeight),
            Vec2.new(0, 0), 2)
        _messageLabel = UIRichLabel.new(Vec2.new(14, 10), Vec2.new(0, 0), "", MESSAGE_SIZE)
        _messageLabel.setFont(font)
        _messageLabel.setTextColor(UIKit.text)
        _messageLabel.setWordWrap(TextWrapping.WORD_WRAP)
        _messageLabel.setSize(Vec2.new(COMMAND_WIDTH - 28, innerHeight - 20), Vec2.new(0, 0))
        _messageLabel.setZIndex(3)
        _messageWell.addChild(_messageLabel)
        setMessage("")
        buildPrompt_(font, gw, gh)
        buildPopups_(font)
        showCommand(false)
    }

    static group_(parent, x, y, width, height) {
        var panel = UIPanel.new(Vec2.new(x, y), Vec2.new(0, 0), Vec2.new(width, height), Vec2.new(0, 0))
        panel.setBackgroundColor(BLANK)
        panel.setBorderColor(BLANK)
        parent.addChild(panel)
        return panel
    }

    // --- building ----------------------------------------------------------------------

    buildCommand_(font, x, height) {
        // Ends 2 px inside the window's bottom rim (BAR_HEIGHT - its 24 px border), so nothing sits on the frame.
        _commandHeight = height + 2
        _command = CombatUI.group_(_bar, x, BAR_PAD - 4, COMMAND_WIDTH, _commandHeight)
        // The button prompts, along the bottom under everything else (-5: the rich label's own padding).
        _hint = UIKit.richText(_command, "", Vec2.new(-5, 0), Vec2.new(0, 1), Vec2.new(0, 1), 18.0, UIKit.textMuted, 6)

        // Header: whose chain / what is being picked.
        _header = UIKit.richText(_command, "", Vec2.new(-5, 0), Vec2.new(0, 0), Vec2.new(0, 0), 20.0, UIKit.accent, 6)

        // The button pool, laid out per screen by setSlots.
        _slots = []
        for (i in 0...SLOT_COUNT) {
            var button = MoveButtonUI.new(_command, Vec2.new(0, HEADER_HEIGHT + 4), Vec2.new(100, 40), font)
            var index = i
            button.setOnClick { |sender, mousePos|
                if (_onSlotClick != null) _onSlotClick.call(index)
            }
            button.setOnFocused(Fn.new {
                _focusedSlot = index
                if (_onSlotFocus != null) _onSlotFocus.call(index)
            })
            button.panel.setVisible(false)
            _slots.add(button)
        }

        // What the focused button does: right under the last row in use (layoutDetail_), word-wrapped over the height
        // that's left.
        _detail = UIRichLabel.new(Vec2.new(-5, rowTop_(2, 50)), Vec2.new(0, 0), "", DETAIL_SIZE)
        _detail.setFont(font)
        _detail.setTextColor(UIKit.text)
        _detail.setWordWrap(TextWrapping.WORD_WRAP)
        _detail.setZIndex(6)
        _command.addChild(_detail)
        _detailLines = 1
        layoutDetail_(rowTop_(2, 50))

        buildItems_(font)
    }

    // The Items screen: the list on the left (a scroll of pooled rows), the focused item on the right.
    buildItems_(font) {
        var top = HEADER_HEIGHT + 4
        // Stops short of the hint line with a little air (ITEM_HINT_GAP), so the list's frame never touches it.
        var height = _commandHeight - top - HINT_HEIGHT - ITEM_HINT_GAP
        _itemsPanel = CombatUI.group_(_command, 0, top, COMMAND_WIDTH, height)
        _itemsPanel.setVisible(false)
        var well = UIKit.frame(_itemsPanel, "well", Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(ITEM_LIST_WIDTH, height),
            Vec2.new(0, 0), 3)
        var pad = UIKit.padding("well")
        var viewHeight = height - pad[2] - pad[3]
        _itemScroll = UIScroll.new(Vec2.new(pad[0], pad[2]), Vec2.new(0, 0),
            Vec2.new(ITEM_LIST_WIDTH - pad[0] - pad[1] - ITEM_BAR_WIDTH - 4, viewHeight), Vec2.new(0, 0))
        _itemScroll.setVerticalScrollEnabled(true)
        _itemScroll.setFocusDeadzone(Vec2.new(0, 30))
        _itemScroll.setZIndex(4)
        well.addChild(_itemScroll)
        _itemViewHeight = viewHeight
        // The scroll bar down the list's right edge - the engine sizes and moves the thumb (setScrollIndicator).
        _itemTrack = UIPanel.new(Vec2.new(ITEM_LIST_WIDTH - pad[1] - ITEM_BAR_WIDTH, pad[2]), Vec2.new(0, 0),
            Vec2.new(ITEM_BAR_WIDTH, viewHeight), Vec2.new(0, 0))
        _itemTrack.setBackgroundColor(Vec4.new(19, 27, 27, 120))
        _itemTrack.setBorderColor(BLANK)
        _itemTrack.setZIndex(5)
        well.addChild(_itemTrack)
        var thumb = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(ITEM_BAR_WIDTH, 40), Vec2.new(0, 0))
        thumb.setBackgroundColor(UIKit.accent)
        thumb.setBorderColor(BLANK)
        thumb.setZIndex(6)
        _itemTrack.addChild(thumb)
        _itemScroll.setScrollIndicator(thumb)
        _itemRows = []
        var rowWidth = ITEM_LIST_WIDTH - pad[0] - pad[1] - ITEM_BAR_WIDTH - 8
        for (i in 0...ITEM_ROWS) {
            var row = MoveButtonUI.new(_itemScroll, Vec2.new(2, 2 + i * (ITEM_ROW_HEIGHT + ITEM_ROW_GAP)),
                Vec2.new(rowWidth, ITEM_ROW_HEIGHT), font)
            row.inset = ITEM_ICON + 6
            var index = i
            row.setOnClick { |sender, mousePos|
                if (_onItemClick != null) _onItemClick.call(index)
            }
            row.setOnFocused(Fn.new {
                showItemDetail_(index)
                if (_onItemFocus != null) _onItemFocus.call(index)
            })
            row.panel.setVisible(false)
            _itemRows.add({"button": row, "icon": null})
        }

        // The focused item: its picture in a kit cell, name, effect and description.
        var x = ITEM_LIST_WIDTH + 14
        _itemCell = UIKit.frame(_itemsPanel, "cell", Vec2.new(x, 0), Vec2.new(0, 0), Vec2.new(ITEM_CELL, ITEM_CELL),
            Vec2.new(0, 0), 4)
        _itemPicture = null
        var textX = x + ITEM_CELL + 12
        var textWidth = COMMAND_WIDTH - textX
        _itemName = UIKit.richText(_itemsPanel, "", Vec2.new(textX - 5, -2), Vec2.new(0, 0), Vec2.new(0, 0), 20.0, UIKit.text, 6)
        _itemEffect = UIRichLabel.new(Vec2.new(textX - 5, 26), Vec2.new(0, 0), "", 18.0)
        _itemEffect.setFont(font)
        _itemEffect.setTextColor(UIKit.accent)
        _itemEffect.setWordWrap(TextWrapping.WORD_WRAP)
        _itemEffect.setSize(Vec2.new(textWidth + 5, 44), Vec2.new(0, 0))
        _itemEffect.setZIndex(6)
        _itemsPanel.addChild(_itemEffect)
        _itemDesc = UIRichLabel.new(Vec2.new(x - 5, ITEM_CELL + 12), Vec2.new(0, 0), "", 18.0)
        _itemDesc.setFont(font)
        _itemDesc.setTextColor(UIKit.textMuted)
        _itemDesc.setWordWrap(TextWrapping.WORD_WRAP)
        _itemDesc.setSize(Vec2.new(COMMAND_WIDTH - x + 5, height - ITEM_CELL - 20), Vec2.new(0, 0))
        _itemDesc.setZIndex(6)
        _itemsPanel.addChild(_itemDesc)
        _items = []
    }

    // The top of button row `row` for buttons `height` tall (row = the row count: just past the last one).
    rowTop_(row, height) { HEADER_HEIGHT + 4 + row * (height + SLOT_GAP_Y) }

    // Puts the detail line at `top`, as wide as the panel and as tall as what's left - detailLines says how many
    // lines that is.
    layoutDetail_(top) {
        var height = _commandHeight - HINT_HEIGHT - top
        _detail.setPosition(Vec2.new(-5, top), Vec2.new(0, 0))
        _detail.setSize(Vec2.new(COMMAND_WIDTH + 5, height), Vec2.new(0, 0))
        _detailLines = (height / DETAIL_LINE).floor.max(1)
    }

    // How many lines the detail line has room for under the current buttons.
    detailLines { _detailLines }

    buildPrompt_(font, gw, gh) {
        _prompt = UIKit.frame(_root, "dialogue", Vec2.new(0, -40), Vec2.new(0.5, 0.5), Vec2.new(PROMPT_WIDTH, PROMPT_HEIGHT),
            Vec2.new(0, 0), 20)
        _prompt.setAnchorPoint(Vec2.new(0.5, 0.5))
        _promptSpeaker = UIKit.richText(_prompt, "", Vec2.new(26, 20), Vec2.new(0, 0), Vec2.new(0, 0), 22.0, UIKit.accent, 21)
        _promptText = UIRichLabel.new(Vec2.new(26, 52), Vec2.new(0, 0), "", 20.0)
        _promptText.setFont(font)
        _promptText.setTextColor(UIKit.textDark)
        _promptText.setWordWrap(TextWrapping.WORD_WRAP)
        _promptText.setSize(Vec2.new(PROMPT_WIDTH - 52, PROMPT_HEIGHT - 86), Vec2.new(0, 0))
        _promptText.setUseUnscaledTime(true)
        _promptText.setZIndex(21)
        _prompt.addChild(_promptText)
        UIKit.richText(_prompt, "[input=UIAccept] Continue", Vec2.new(-22, -14), Vec2.new(1, 1), Vec2.new(1, 1), 18.0,
            UIKit.textDark, 21)
        _prompt.setVisible(false)
    }

    buildPopups_(font) {
        _popups = []
        for (i in 0...POPUPS) {
            var label = UILabel.new(Vec2.new(0, 0), Vec2.new(0, 0), "", 20.0)
            label.setFont(font)
            label.setBoundingBoxToText()
            label.setAnchorPoint(Vec2.new(0.5, 1))
            label.setZIndex(12)
            label.setVisible(false)
            _root.addChild(label)
            var popup = {}
            popup["label"] = label
            popup["start"] = null
            _popups.add(popup)
        }
    }

    // --- cards, timeline, message --------------------------------------------------------

    // `count` cards stacked downward from the top at x - with the stamina bar or not (CombatantUI).
    static cardColumn_(parent, x, count, showStamina, font) {
        var cards = []
        var step = CombatantUI.heightFor(showStamina) + CARD_GAP
        for (i in 0...count) {
            cards.add(CombatantUI.new(parent, x, TOP_EDGE + i * step, CARD_WIDTH, font, showStamina))
        }
        return cards
    }

    partyCards { _partyCards }
    enemyCards { _enemyCards }
    timeline { _timeline }
    slots { _slots }
    focusedSlot { _focusedSlot }

    onSlotClick=(fn) { _onSlotClick = fn }
    onSlotFocus=(fn) { _onSlotFocus = fn }
    onItemClick=(fn) { _onItemClick = fn }
    onItemFocus=(fn) { _onItemFocus = fn }

    setVisible(visible) { _root.setVisible(visible) }

    // A line on the banner. seconds: how long it stays (null = until replaced).
    setMessage(text) { setMessage(text, null) }
    setMessage(text, seconds) {
        _messageLabel.setMarkupText(text)
        _messageUntil = seconds == null ? null : Time.unscaledTime + seconds
    }

    // --- the command panel -----------------------------------------------------------------

    showCommand(visible) {
        _command.setVisible(visible)
        _messageWell.setVisible(!visible)
        if (!visible) _hint.setMarkupText("")
    }

    setHeader(text) { _header.setMarkupText(text) }
    setDetail(text) { _detail.setMarkupText(text) }
    setHint(markup) { _hint.setMarkupText(markup) }

    // A screen of buttons. entries: up to SLOT_COUNT of [name, info, enabled] or [name, info, enabled, icon path]; style: "menu", "grid" or "pair" (see
    // STYLES - columns, height, layout). Links the buttons so the arrows stay on them, puts focus on `focus` (if not
    // null), and hides the Items panel.
    setSlots(entries, focus) { setSlots(entries, focus, "grid") }
    setSlots(entries, focus, style) {
        _itemsPanel.setVisible(false)
        var spec = STYLES[style]
        var columns = spec[0]
        var height = spec[1]
        var width = ((COMMAND_WIDTH - (columns - 1) * SLOT_GAP_X) / columns).floor
        _slotCount = entries.count.min(SLOT_COUNT)
        for (i in 0...SLOT_COUNT) {
            var used = i < _slotCount
            var button = _slots[i]
            button.panel.setVisible(used)
            if (used) {
                button.place(Vec2.new((i % columns) * (width + SLOT_GAP_X), rowTop_((i / columns).floor, height)),
                    Vec2.new(width, height))
                button.setLayout(spec[2])
                button.setTexts(entries[i][0], entries[i][1])
                button.setEnabled(entries[i][2])
                button.setIcon(entries[i].count > 3 ? entries[i][3] : null)
            }
        }
        for (i in 0..._slotCount) {
            var panel = _slots[i].panel
            var column = i % columns
            CombatUI.link_(panel, NavigationDirection.LEFT, column > 0 ? _slots[i - 1].panel : null)
            CombatUI.link_(panel, NavigationDirection.RIGHT, column + 1 < columns && i + 1 < _slotCount ? _slots[i + 1].panel : null)
            CombatUI.link_(panel, NavigationDirection.UP, i >= columns ? _slots[i - columns].panel : null)
            CombatUI.link_(panel, NavigationDirection.DOWN, i + columns < _slotCount ? _slots[i + columns].panel : null)
        }
        var rows = ((_slotCount + columns - 1) / columns).floor
        _detail.setVisible(true)
        // Centred between the last row of buttons and the hint line (a one-line description; the rich label draws a
        // few px below its box top, hence DETAIL_LIFT).
        var gridBottom = rowTop_(rows, height) - SLOT_GAP_Y
        var room = _commandHeight - HINT_HEIGHT - gridBottom
        layoutDetail_(gridBottom + ((room - DETAIL_LINE) / 2).floor.max(0) - DETAIL_LIFT)
        if (focus != null && _slotCount > 0) focusSlot(focus)
    }

    focusSlot(index) {
        var i = Math.clamp(index, 0, _slotCount - 1)
        UIManager.setFocus(_slots[i].panel)
    }

    // The Items screen. items: [{ "name", "count" (text), "icon" (path or null), "effect", "description", "enabled" }],
    // up to ITEM_ROWS. Focus goes to row `focus`.
    showItems(items, focus) {
        for (button in _slots) {
            button.panel.setVisible(false)
        }
        _detail.setVisible(false)
        _itemsPanel.setVisible(true)
        _items = items.take(ITEM_ROWS).toList
        for (i in 0...ITEM_ROWS) {
            var row = _itemRows[i]
            var button = row["button"]
            var used = i < _items.count
            button.panel.setVisible(used)
            if (row["icon"] != null) {
                button.panel.removeChild(row["icon"])
                row["icon"] = null
            }
            if (!used) continue
            var item = _items[i]
            button.setTexts(item["name"], item["count"])
            button.setEnabled(item["enabled"] != false)
            if (item["icon"] != null) {
                row["icon"] = UIKit.image(button.panel, item["icon"], Vec2.new(8, 0), Vec2.new(0, 0.5), Vec2.new(0, 0.5),
                    UIKit.fitScale(item["icon"], ITEM_ICON), 7)
            }
        }
        // The scroll bar only when the rows don't all fit.
        _itemTrack.setVisible(_items.count * (ITEM_ROW_HEIGHT + ITEM_ROW_GAP) > _itemViewHeight)
        for (i in 0..._items.count) {
            var panel = _itemRows[i]["button"].panel
            CombatUI.link_(panel, NavigationDirection.UP, i > 0 ? _itemRows[i - 1]["button"].panel : null)
            CombatUI.link_(panel, NavigationDirection.DOWN, i + 1 < _items.count ? _itemRows[i + 1]["button"].panel : null)
            CombatUI.link_(panel, NavigationDirection.LEFT, null)
            CombatUI.link_(panel, NavigationDirection.RIGHT, null)
        }
        if (_items.count > 0) {
            var index = Math.clamp(focus, 0, _items.count - 1)
            showItemDetail_(index)
            UIManager.setFocus(_itemRows[index]["button"].panel)
        }
    }

    // The right-hand side of the Items screen for item `index`.
    showItemDetail_(index) {
        if (index >= _items.count) return
        var item = _items[index]
        if (_itemPicture != null) {
            _itemCell.removeChild(_itemPicture)
            _itemPicture = null
        }
        if (item["icon"] != null) {
            var pad = UIKit.padding("cell")
            _itemPicture = UIKit.image(_itemCell, item["icon"], Vec2.new((pad[0] - pad[1]) / 2, (pad[2] - pad[3]) / 2),
                Vec2.new(0.5, 0.5), Vec2.new(0.5, 0.5), UIKit.fitScale(item["icon"], ITEM_PICTURE), 5)
        }
        _itemName.setMarkupText(item["name"])
        _itemEffect.setMarkupText(item["effect"] == null ? "" : item["effect"])
        _itemDesc.setMarkupText(item["description"] == null ? "" : item["description"])
    }

    // Sets where `direction` moves focus from `element`; null blocks it (focus stays put).
    static link_(element, direction, target) {
        element.setNavigationNeighbor(direction, target != null ? target : element)
    }

    // --- the prompt box ----------------------------------------------------------------------

    showPrompt(speaker, text) {
        _promptSpeaker.setMarkupText(speaker == null ? "" : speaker)
        _promptText.setMarkupText(text)
        _prompt.setVisible(true)
    }
    hidePrompt() { _prompt.setVisible(false) }

    // --- floating text -----------------------------------------------------------------------

    // Text that rises from `screenPos` (a Vec2 in screen pixels) and fades. big: the larger size (damage numbers).
    popup(screenPos, text, color) {
        var slot = null
        var oldest = null
        for (popup in _popups) {
            if (popup["start"] == null) {
                slot = popup
                break
            }
            if (oldest == null || popup["start"] < oldest["start"]) oldest = popup
        }
        if (slot == null) slot = oldest
        // A little sideways scatter so two numbers on one fighter don't stack exactly.
        var jitter = ((Time.unscaledTime * 977) % 21) - 10
        slot["start"] = Time.unscaledTime
        slot["x"] = screenPos.x + jitter
        slot["y"] = screenPos.y
        slot["color"] = color
        var label = slot["label"]
        label.setText(text)
        label.setBoundingBoxToText()
        label.setTextColor(color)
        label.setPosition(Vec2.new(slot["x"], slot["y"]), Vec2.new(0, 0))
        label.setVisible(true)
    }

    // Every frame: floats the popups, clears a timed message.
    update() {
        var now = Time.unscaledTime
        _timeline.animate()
        for (popup in _popups) {
            var start = popup["start"]
            if (start == null) continue
            var t = (now - start) / POPUP_TIME
            var label = popup["label"]
            if (t >= 1) {
                popup["start"] = null
                label.setVisible(false)
                continue
            }
            var color = popup["color"]
            label.setPosition(Vec2.new(popup["x"], popup["y"] - 46 * Math.easeOut(t)), Vec2.new(0, 0))
            label.setTextColor(Vec4.new(color.x, color.y, color.z, t < 0.6 ? 255 : 255 * (1 - (t - 0.6) / 0.4)))
        }
        if (_messageUntil != null && now >= _messageUntil) setMessage("")
    }

    teardown() {
        if (_root != null) {
            UIManager.removeUIElement(_root)
            _root = null
            _font.unload()
            _font = null
            UIKit.close()
        }
    }
}
