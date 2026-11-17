// UI/Combat/CombatUI.wren
// The battle screen, built from the UI kit (UI/Theme/UIKit.wren) as a frame around the arena so nothing sits on the
// fighters (Combat/BattleStage.wren places the grid in the strip between):
//   - combatant cards along the top: the party from the left, the opposition from the right (UI/Combat/CombatantUI);
//   - a message banner just under them: the battle log, call-outs and tips;
//   - a full-width bar along the bottom: the shared timeline on the left (UI/Combat/TimelineUI - every combatant's
//     committed moves on one time axis) and the command panel on the right;
//   - a prompt box over the middle for tutorial lines (speaker + text, Accept to continue);
//   - floating text over the fighters: damage, "Blocked", "Dodged", reactions.
//
// The command panel is where a chain is built. It always shows whose chain it is and how full it is (seconds of the
// budget, stamina left after it), then one grid of up to eight slot buttons whose meaning the caller sets:
//   the moves (the six loadout moves, Hold and Item), Hold's lengths, the items, or Start / Undo once it is full -
// with a detail line for whatever slot has focus and a hint line of button prompts.
//
// Pure construction + refresh: it never reads a Combatant or the engine and never calls back into game logic except
// through the two callbacks States/CombatState.wren sets (onSlotClick / onSlotFocus).
import "ui" for UIManager, UIPanel, UIColor, UILabel, UIRichLabel, TextWrapping, NavigationDirection
import "math" for Math, Vec2, Vec4
import "app" for Application, Time
import "Util/Colors" for BLANK
import "UI/Combat/CombatantUI" for CombatantUI
import "UI/Combat/TimelineUI" for TimelineUI
import "UI/Combat/MoveButtonUI" for MoveButtonUI
import "UI/Theme/KitBar" for KitBar
import "UI/Theme/KitButton" for KitButton
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
var COMMAND_WIDTH = 600
var SLOT_COUNT = 8
var SLOT_WIDTH = 296
var SLOT_HEIGHT = 32
var SLOT_GAP_X = 8
var SLOT_GAP_Y = 4
var HEADER_HEIGHT = 26
var CHAIN_BAR_WIDTH = 120
var TIMELINE_ROWS = 8
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

        // Cards: the party from the left edge, the opposition from the right - narrower if there are too many.
        var total = partySlots + enemySlots
        var width = CARD_WIDTH.min(((gw - 2 * TOP_EDGE - 40 - (total - 1) * CARD_GAP) / total.max(1)).floor)
        _partyCards = []
        for (i in 0...partySlots) {
            _partyCards.add(CombatantUI.new(_root, TOP_EDGE + i * (width + CARD_GAP), TOP_EDGE, width, font))
        }
        _enemyCards = []
        for (i in 0...enemySlots) {
            var x = gw - TOP_EDGE - width - i * (width + CARD_GAP)
            _enemyCards.add(CombatantUI.new(_root, x, TOP_EDGE, width, font))
        }

        _bar = UIKit.frame(_root, "window", Vec2.new(0, 0), Vec2.new(0, 1), Vec2.new(gw, BAR_HEIGHT), Vec2.new(0, 0), 0)
        _bar.setAnchorPoint(Vec2.new(0, 1))

        var commandX = gw - BAR_PAD - COMMAND_WIDTH
        var innerHeight = BAR_HEIGHT - 2 * BAR_PAD
        _timeline = TimelineUI.new(_bar, BAR_PAD, BAR_PAD - 4, commandX - BAR_PAD - 20, TIMELINE_ROWS, font)
        // The hint line sits under the timeline, bottom left (-5: the rich label's own padding).
        _hint = UIKit.richText(_bar, "", Vec2.new(BAR_PAD - 5, -BAR_PAD + 16), Vec2.new(0, 1), Vec2.new(0, 1), 18.0,
            UIKit.textMuted, 6)

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

    static group_(parent, x, y, w, h) {
        var panel = UIPanel.new(Vec2.new(x, y), Vec2.new(0, 0), Vec2.new(w, h), Vec2.new(0, 0))
        panel.setBackgroundColor(BLANK)
        panel.setBorderColor(BLANK)
        parent.addChild(panel)
        return panel
    }

    // --- building ----------------------------------------------------------------------

    buildCommand_(font, x, height) {
        _command = CombatUI.group_(_bar, x, BAR_PAD - 4, COMMAND_WIDTH, height + 8)

        // Header: whose chain / what is being picked, and how full the chain is.
        _header = UIKit.richText(_command, "", Vec2.new(-5, 0), Vec2.new(0, 0), Vec2.new(0, 0), 20.0, UIKit.accent, 6)
        _chainText = UIKit.richText(_command, "", Vec2.new(5, 0), Vec2.new(1, 0), Vec2.new(1, 0), 18.0, UIKit.text, 6)
        _chainBar = KitBar.new(_command, COMMAND_WIDTH - CHAIN_BAR_WIDTH - 150, 5, CHAIN_BAR_WIDTH, 14, UIKit.accent, 6)

        _slots = []
        for (i in 0...SLOT_COUNT) {
            var pos = Vec2.new((i % 2) * (SLOT_WIDTH + SLOT_GAP_X), HEADER_HEIGHT + 4 + (i / 2).floor * (SLOT_HEIGHT + SLOT_GAP_Y))
            var button = MoveButtonUI.new(_command, pos, Vec2.new(SLOT_WIDTH, SLOT_HEIGHT), font)
            var index = i
            button.setOnClick { |s, m|
                if (_onSlotClick != null) _onSlotClick.call(index)
            }
            button.setOnFocused(Fn.new {
                _focusedSlot = index
                if (_onSlotFocus != null) _onSlotFocus.call(index)
            })
            _slots.add(button)
        }
        // What the focused slot does, under the grid.
        var detailY = HEADER_HEIGHT + 4 + 4 * (SLOT_HEIGHT + SLOT_GAP_Y) + 2
        _detail = UIKit.richText(_command, "", Vec2.new(-5, detailY), Vec2.new(0, 0), Vec2.new(0, 0), 18.0, UIKit.text, 6)
        // Flee - only in fights that can be fled (setFleeAvailable).
        _fleeButton = KitButton.new(_command, "Flee", Vec2.new(0, detailY - 2), Vec2.new(1, 0), Vec2.new(84, 28), font, 17.0)
        _fleeButton.panel.setAnchorPoint(Vec2.new(1, 0))
        _fleeAvailable = false
        _fleeButton.panel.setVisible(false)
    }

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

    partyCards { _partyCards }
    enemyCards { _enemyCards }
    timeline { _timeline }
    slots { _slots }
    fleeButton { _fleeButton }
    focusedSlot { _focusedSlot }

    onSlotClick=(fn) { _onSlotClick = fn }
    onSlotFocus=(fn) { _onSlotFocus = fn }

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

    // How full the chain is: "2.6 / 4.0s   SP 54".
    setChain(text, fraction) {
        _chainText.setMarkupText(text)
        _chainBar.setFraction(fraction)
    }

    setFleeAvailable(available) {
        _fleeAvailable = available
        _fleeButton.panel.setVisible(available)
    }

    // The slot grid. entries: up to SLOT_COUNT of [name, info, enabled]; slots past the list are hidden. Links the
    // grid so the arrows stay on it, and (focus != null) puts focus on that slot.
    setSlots(entries, focus) {
        _slotCount = entries.count.min(SLOT_COUNT)
        for (i in 0...SLOT_COUNT) {
            var used = i < _slotCount
            _slots[i].panel.setVisible(used)
            if (used) {
                _slots[i].setTexts(entries[i][0], entries[i][1])
                _slots[i].setEnabled(entries[i][2])
            }
        }
        for (i in 0..._slotCount) {
            var panel = _slots[i].panel
            var column = i % 2
            CombatUI.link_(panel, NavigationDirection.LEFT, column == 1 ? _slots[i - 1].panel : null)
            CombatUI.link_(panel, NavigationDirection.RIGHT, column == 0 && i + 1 < _slotCount ? _slots[i + 1].panel : null)
            CombatUI.link_(panel, NavigationDirection.UP, i >= 2 ? _slots[i - 2].panel : null)
            var below = i + 2 < _slotCount ? _slots[i + 2].panel : (_fleeAvailable ? _fleeButton.panel : null)
            CombatUI.link_(panel, NavigationDirection.DOWN, below)
        }
        if (_fleeAvailable && _slotCount > 0) {
            var last = _slots[_slotCount - 1].panel
            CombatUI.link_(_fleeButton.panel, NavigationDirection.UP, last)
            CombatUI.link_(_fleeButton.panel, NavigationDirection.DOWN, null)
            CombatUI.link_(_fleeButton.panel, NavigationDirection.LEFT, null)
            CombatUI.link_(_fleeButton.panel, NavigationDirection.RIGHT, null)
        }
        if (focus != null && _slotCount > 0) focusSlot(focus)
    }

    focusSlot(index) {
        var i = Math.clamp(index, 0, _slotCount - 1)
        UIManager.setFocus(_slots[i].panel)
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
            var c = popup["color"]
            label.setPosition(Vec2.new(popup["x"], popup["y"] - 46 * Math.easeOut(t)), Vec2.new(0, 0))
            label.setTextColor(Vec4.new(c.x, c.y, c.z, t < 0.6 ? 255 : 255 * (1 - (t - 0.6) / 0.4)))
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
