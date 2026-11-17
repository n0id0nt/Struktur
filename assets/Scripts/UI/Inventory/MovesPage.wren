// UI/Inventory/MovesPage.wren
// The inventory's Moves tab: the player's loadout (Combat/MoveBook.wren - the six moves they fight with) along the
// top, every move they have unlocked listed below it, and on the right a preview of the selected move - the
// player's battle sprite playing that move's animation, then a beat of idle, on a loop - with its numbers.
//
// Swapping: Enter (UIAccept) on a loadout slot, then on a move - or the other way round - puts that move in that
// slot; a move already equipped in another slot swaps places with it. cancel() (Esc - States/InventoryState.wren
// asks before closing the screen) backs out of a half-made swap.
// Pure view: `model` comes from States/InventoryState.movesModel_:
//   { "moves": [entry], "equipped": [move ids, slot by slot], "sheet": {...}, "labels": {...} }
//   entry: { "id", "name", "time", "damage", "staminaCost", "staminaRestore", "heal",
//            "anim": [firstFrame, endFrame (exclusive), seconds] }
//   sheet: { "texture", "columns", "cellWidth", "cellHeight", "idle": [firstFrame, endFrame, seconds],
//            "crop": [x, y, width, height] - the part of each cell around the fighter that the preview shows }
// onEquip.call(slot, id) makes the change and returns the new equipped ids.
import "math" for Math, Vec2
import "ui" for UIManager, UIScroll, UITexture, NavigationDirection
import "UI/Theme/UIKit" for UIKit
import "UI/Inventory/InventoryTheme" for InventoryTheme
import "UI/Inventory/MoveCells" for MoveCells

var SLOT_COLUMNS = 3     // six loadout slots: three across the list's width (528), two rows
var SLOT_WIDTH = 168
var SLOT_STRIDE = 180
var SLOT_HEIGHT = 64
var SLOT_ROW_STRIDE = 70
var LIST_Y = 206
var LIST_INSET = 10
var ROW_INSET = 4   // room for the focus ring inside the scroll's clip
var ROW_WIDTH = 500
var ROW_HEIGHT = 40
var ROW_STRIDE = 46
// Right edges of the list's number columns, within a row.
var TIME_X = 226
var DAMAGE_X = 318
var STAMINA_X = 414
var HEAL_X = 496
var PREVIEW_X = 544
var PREVIEW_WIDTH = 528
var PREVIEW_SCALE = 3   // whole pixels only - the sheet is point-sampled pixel art
var IDLE_HOLD = 0.8   // seconds of idle between repeats of the previewed move

class MovesPage {
    construct new(parent, model, boldFont, bookFont, onFocus, onEquip) {
        _boldFont = boldFont
        _bookFont = bookFont
        _labels = model["labels"]
        _sheet = model["sheet"]
        _moves = model["moves"]
        _equipped = model["equipped"]
        _slotCount = model["slots"] != null ? model["slots"] : _equipped.count
        _onFocus = onFocus
        _onEquip = onEquip
        _slots = []
        _rows = []
        _armedSlot = null
        _armedMove = null
        _previewMove = null
        _previewStart = null
        _previewFrame = -1

        _root = InventoryTheme.group(parent, Vec2.new(0, 0), Vec2.new(1072, 476), 2)
        buildLoadout_()
        buildList_()
        buildPreview_()
        link_()
        refresh_()
        if (_equipped.count > 0) {
            preview_(entryFor_(equippedAt_(0)))
        }
    }

    root { _root }
    firstFocusable { _slots.count > 0 ? _slots[0]["panel"] : null }

    linkUpTo(tab) {
        for (slot in _slots) {
            slot["panel"].setNavigationNeighbor(NavigationDirection.UP, tab)
        }
    }

    setVisible(visible) { _root.setVisible(visible) }

    // Esc while half-way through a swap backs out of it; true when it did (so the screen stays open).
    cancel() {
        if (_armedSlot == null && _armedMove == null) {
            return false
        }
        disarm_()
        return true
    }

    // Advances the preview animation - called every frame while this tab is open, with unscaled time (the
    // inventory pauses game time).
    tick(now) {
        if (_previewMove == null) {
            return
        }
        if (_previewStart == null) {
            _previewStart = now
        }
        var anim = _previewMove["anim"]
        var idle = _sheet["idle"]
        var t = (now - _previewStart) % (anim[2] + IDLE_HOLD)
        var frame = 0
        if (t < anim[2]) {
            var count = anim[1] - anim[0]
            frame = anim[0] + Math.min((t / anim[2] * count).floor, count - 1)
        } else {
            var count = idle[1] - idle[0]
            frame = idle[0] + ((t - anim[2]) / idle[2] * count).floor % count
        }
        showFrame_(frame)
    }

    // --- building ---------------------------------------------------------------------------

    buildLoadout_() {
        InventoryTheme.label(_root, _labels["loadout"], Vec2.new(0, 6), Vec2.new(0, 0), Vec2.new(0, 0), 18.0, _boldFont,
            InventoryTheme.accent, 3)
        for (i in 0..._slotCount) {
            var pos = Vec2.new((i % SLOT_COLUMNS) * SLOT_STRIDE, 36 + (i / SLOT_COLUMNS).floor * SLOT_ROW_STRIDE)
            var panel = InventoryTheme.frame(_root, "tile", pos, Vec2.new(0, 0),
                Vec2.new(SLOT_WIDTH, SLOT_HEIGHT), Vec2.new(0, 0), 3)
            InventoryTheme.label(panel, "%(_labels["slot"]) %(i + 1)", Vec2.new(12, 6), Vec2.new(0, 0), Vec2.new(0, 0),
                16.0, _bookFont, InventoryTheme.textMuted, 4)
            var slot = {}
            slot["panel"] = panel
            slot["name"] = InventoryTheme.label(panel, "", Vec2.new(12, 30), Vec2.new(0, 0), Vec2.new(0, 0), 20.0,
                _boldFont, InventoryTheme.text, 4)
            _slots.add(slot)

            InventoryTheme.focusable(panel, Fn.new {
                if (_armedMove == null) {
                    preview_(entryFor_(equippedAt_(i)))
                }
                _onFocus.call()
            }, null)
            panel.setOnClick { |sender, mousePos| slotPressed_(i) }
        }
    }

    buildList_() {
        InventoryTheme.label(_root, _labels["unlocked"], Vec2.new(0, LIST_Y - 26), Vec2.new(0, 0), Vec2.new(0, 0), 18.0,
            _boldFont, InventoryTheme.accent, 3)
        header_(_labels["time"], TIME_X)
        header_(_labels["damage"], DAMAGE_X)
        header_(_labels["stamina"], STAMINA_X)
        header_(_labels["heal"], HEAL_X)

        var frame = InventoryTheme.frame(_root, "well", Vec2.new(0, LIST_Y), Vec2.new(0, 0), Vec2.new(528, 476 - LIST_Y),
            Vec2.new(0, 0), 3)
        var scroll = UIScroll.new(Vec2.new(LIST_INSET, LIST_INSET), Vec2.new(0, 0),
            Vec2.new(ROW_WIDTH + 2 * ROW_INSET, 476 - LIST_Y - 2 * LIST_INSET), Vec2.new(0, 0))
        scroll.setHorizontalScrollEnabled(false)
        // Six rows fit; past them, scroll a row at a time (110 = the sixth row's centre below the viewport's).
        scroll.setFocusDeadzone(Vec2.new(0, 110))
        scroll.setZIndex(4)
        frame.addChild(scroll)

        for (r in 0..._moves.count) {
            var move = _moves[r]
            var panel = InventoryTheme.frame(scroll, "tile", Vec2.new(ROW_INSET, ROW_INSET + r * ROW_STRIDE), Vec2.new(0, 0),
                Vec2.new(ROW_WIDTH, ROW_HEIGHT), Vec2.new(0, 0), 5)
            var row = {}
            row["panel"] = panel
            row["badge"] = InventoryTheme.label(panel, "", Vec2.new(14, 8), Vec2.new(0, 0), Vec2.new(0, 0), 20.0,
                _boldFont, InventoryTheme.accent, 6)
            InventoryTheme.label(panel, move["name"], Vec2.new(40, 7), Vec2.new(0, 0), Vec2.new(0, 0), 22.0, _boldFont,
                InventoryTheme.text, 6)
            MoveCells.apply(cell_(panel, TIME_X), MoveCells.time(move))
            MoveCells.apply(cell_(panel, DAMAGE_X), MoveCells.damage(move))
            MoveCells.apply(cell_(panel, STAMINA_X), MoveCells.stamina(move))
            MoveCells.apply(cell_(panel, HEAL_X), MoveCells.heal(move))
            _rows.add(row)

            InventoryTheme.focusable(panel, Fn.new {
                if (_armedMove == null) {
                    preview_(move)
                }
                _onFocus.call()
            }, null)
            panel.setOnClick { |sender, mousePos| rowPressed_(r) }
        }
    }

    header_(text, x) {
        InventoryTheme.label(_root, text, Vec2.new(LIST_INSET + ROW_INSET + x, LIST_Y - 26), Vec2.new(0, 0), Vec2.new(1, 0), 16.0,
            _bookFont, InventoryTheme.textMuted, 3)
    }

    cell_(row, x) {
        return InventoryTheme.label(row, "", Vec2.new(x, 9), Vec2.new(0, 0), Vec2.new(1, 0), 20.0, _boldFont,
            InventoryTheme.text, 6)
    }

    buildPreview_() {
        var panel = InventoryTheme.frame(_root, "well", Vec2.new(PREVIEW_X, 0), Vec2.new(0, 0), Vec2.new(PREVIEW_WIDTH, 476),
            Vec2.new(0, 0), 3)

        var crop = _sheet["crop"]
        var width = crop[2] * PREVIEW_SCALE
        var height = crop[3] * PREVIEW_SCALE
        // The fighter stands in a kit cell (its border is heavier top-left, so its inside is centred down-right).
        var pad = UIKit.padding("cell")
        var stage = InventoryTheme.frame(panel, "cell", Vec2.new(0, 16), Vec2.new(0.5, 0),
            Vec2.new(width + pad[0] + pad[1], height + pad[2] + pad[3]), Vec2.new(0, 0), 4)
        stage.setAnchorPoint(Vec2.new(0.5, 0))
        _sprite = UITexture.new(Vec2.new((pad[0] - pad[1]) / 2, (pad[2] - pad[3]) / 2), Vec2.new(0.5, 0.5),
            Vec2.new(width, height), Vec2.new(0, 0))
        _sprite.setAnchorPoint(Vec2.new(0.5, 0.5))
        _sprite.setTexture(UIKit.texture(_sheet["texture"]))
        _sprite.setZIndex(5)
        stage.addChild(_sprite)

        var y = 16 + height + pad[2] + pad[3] + 10
        _previewName = InventoryTheme.label(panel, "", Vec2.new(0, y), Vec2.new(0.5, 0), Vec2.new(0.5, 0), 34.0,
            _boldFont, InventoryTheme.text, 4)

        // Four stat columns: header over value.
        _previewCells = []
        var headers = [_labels["time"], _labels["damage"], _labels["stamina"], _labels["heal"]]
        for (i in 0...4) {
            var x = PREVIEW_WIDTH / 8 * (2 * i + 1)
            InventoryTheme.label(panel, headers[i], Vec2.new(x, y + 40), Vec2.new(0, 0), Vec2.new(0.5, 0), 16.0,
                _bookFont, InventoryTheme.textMuted, 4)
            _previewCells.add(InventoryTheme.label(panel, "", Vec2.new(x, y + 58), Vec2.new(0, 0), Vec2.new(0.5, 0),
                26.0, _boldFont, InventoryTheme.text, 4))
        }

        _status = InventoryTheme.label(panel, "", Vec2.new(0, y + 102), Vec2.new(0.5, 0), Vec2.new(0.5, 0), 20.0,
            _bookFont, InventoryTheme.textMuted, 4)
        // Rich text, so the button prompts in the hint strings ([input=UIAccept], [input=UICancel]) draw as art.
        _hint = UIKit.richText(panel, "", Vec2.new(0, -60), Vec2.new(0.5, 1), Vec2.new(0.5, 0), 18.0,
            InventoryTheme.accent, 4)
        _hintSub = UIKit.richText(panel, _labels["hintCancel"], Vec2.new(0, -32), Vec2.new(0.5, 1), Vec2.new(0.5, 0),
            16.0, InventoryTheme.textMuted, 4)
    }

    // Explicit neighbours everywhere (see InventoryTheme.link): the slots in a row, the list as a column, down from
    // any slot into the list and up off its top back onto the first slot.
    link_() {
        var firstRow = _rows.count > 0 ? _rows[0]["panel"] : null
        for (i in 0..._slots.count) {
            var panel = _slots[i]["panel"]
            var column = i % SLOT_COLUMNS
            InventoryTheme.link(panel, NavigationDirection.LEFT, column > 0 ? _slots[i - 1]["panel"] : null)
            InventoryTheme.link(panel, NavigationDirection.RIGHT,
                column + 1 < SLOT_COLUMNS && i + 1 < _slots.count ? _slots[i + 1]["panel"] : null)
            InventoryTheme.link(panel, NavigationDirection.UP, i >= SLOT_COLUMNS ? _slots[i - SLOT_COLUMNS]["panel"] : null)
            InventoryTheme.link(panel, NavigationDirection.DOWN,
                i + SLOT_COLUMNS < _slots.count ? _slots[i + SLOT_COLUMNS]["panel"] : firstRow)
        }
        for (r in 0..._rows.count) {
            var panel = _rows[r]["panel"]
            InventoryTheme.link(panel, NavigationDirection.UP, r > 0 ? _rows[r - 1]["panel"] : firstFocusable)
            InventoryTheme.link(panel, NavigationDirection.DOWN, r + 1 < _rows.count ? _rows[r + 1]["panel"] : null)
            InventoryTheme.link(panel, NavigationDirection.LEFT, null)
            InventoryTheme.link(panel, NavigationDirection.RIGHT, null)
        }
    }

    // --- swapping ---------------------------------------------------------------------------

    slotPressed_(slot) {
        if (_armedMove != null) {
            equip_(slot, _armedMove)
        } else if (_armedSlot == slot) {
            disarm_()
        } else {
            // Pick what goes here: jump to the move currently in the slot, in the list.
            _armedSlot = slot
            refresh_()
            var r = rowIndexOf_(equippedAt_(slot))
            if (_rows.count > 0) UIManager.setFocus(_rows[r >= 0 ? r : 0]["panel"])
        }
    }

    rowPressed_(r) {
        var id = _moves[r]["id"]
        if (_armedSlot != null) {
            var slot = _armedSlot
            equip_(slot, id)
            UIManager.setFocus(_slots[slot]["panel"])
        } else if (_armedMove == id) {
            disarm_()
        } else {
            // Pick where it goes: up to the loadout, the preview staying on this move meanwhile.
            _armedMove = id
            preview_(_moves[r])
            refresh_()
            UIManager.setFocus(_slots[0]["panel"])
        }
    }

    equip_(slot, id) {
        _equipped = _onEquip.call(slot, id)
        _armedSlot = null
        _armedMove = null
        refresh_()
        preview_(entryFor_(id))
    }

    disarm_() {
        _armedSlot = null
        _armedMove = null
        refresh_()
    }

    // Re-reads the loadout into the slots, the list's slot badges, the highlight colours and the hint.
    refresh_() {
        for (i in 0..._slots.count) {
            var move = entryFor_(equippedAt_(i))
            setText_(_slots[i]["name"], move == null ? "-" : move["name"])
        }
        for (row in _rows) {
            row["badge"].setText("")
        }
        for (i in 0..._equipped.count) {
            var r = rowIndexOf_(_equipped[i])
            if (r >= 0) {
                setText_(_rows[r]["badge"], "%(i + 1)")
            }
        }
        style_()

        if (_armedSlot != null) {
            _hint.setMarkupText("%(_labels["hintSlot"]) %(_armedSlot + 1)")
        } else if (_armedMove != null) {
            _hint.setMarkupText("%(_labels["hintMove"]) %(entryFor_(_armedMove)["name"])")
        } else {
            _hint.setMarkupText(_labels["hintIdle"])
        }
        _hintSub.setVisible(_armedSlot != null || _armedMove != null)
        if (_previewMove != null) {
            showStatus_()
        }
    }

    // The armed tile (the half of a swap already picked) is tinted warm; focus is the kit's ring.
    style_() {
        for (i in 0..._slots.count) {
            styleElement_(_slots[i]["panel"], _armedSlot == i)
        }
        for (r in 0..._rows.count) {
            styleElement_(_rows[r]["panel"], _armedMove == _moves[r]["id"])
        }
    }

    styleElement_(tile, armed) {
        tile.setTint(armed ? InventoryTheme.armedTint : InventoryTheme.noTint)
    }

    // --- preview ----------------------------------------------------------------------------

    preview_(move) {
        if (move == null) {
            return
        }
        if (_previewMove != move) {
            _previewMove = move
            _previewStart = null
            showFrame_(move["anim"][0])
        }
        setText_(_previewName, move["name"])
        MoveCells.apply(_previewCells[0], MoveCells.time(move))
        MoveCells.apply(_previewCells[1], MoveCells.damage(move))
        MoveCells.apply(_previewCells[2], MoveCells.stamina(move))
        MoveCells.apply(_previewCells[3], MoveCells.heal(move))
        showStatus_()
    }

    showStatus_() {
        var slot = _equipped.indexOf(_previewMove["id"])
        if (slot >= 0) {
            setText_(_status, "%(_labels["equippedIn"]) %(slot + 1)")
            _status.setTextColor(InventoryTheme.accent)
        } else {
            setText_(_status, _labels["notEquipped"])
            _status.setTextColor(InventoryTheme.textMuted)
        }
    }

    // Points the sprite at the cropped middle of one cell of the battle sheet (cells run left to right, top to
    // bottom).
    showFrame_(frame) {
        if (frame == _previewFrame) {
            return
        }
        _previewFrame = frame
        var columns = _sheet["columns"]
        var crop = _sheet["crop"]
        var x = (frame % columns) * _sheet["cellWidth"] + crop[0]
        var y = (frame / columns).floor * _sheet["cellHeight"] + crop[1]
        _sprite.setSourceRect(x, y, crop[2], crop[3])
    }

    // --- helpers ----------------------------------------------------------------------------

    // The move id in loadout slot `i`, or null for an empty slot.
    equippedAt_(i) { i < _equipped.count ? _equipped[i] : null }

    entryFor_(id) {
        if (id == null) return null
        var r = rowIndexOf_(id)
        return r >= 0 ? _moves[r] : null
    }

    rowIndexOf_(id) {
        for (r in 0..._moves.count) {
            if (_moves[r]["id"] == id) {
                return r
            }
        }
        return -1
    }

    setText_(label, text) {
        label.setText(text)
        label.setBoundingBoxToText()
    }

    // Call after the UI tree is removed.
    release() {
        _slots = []
        _rows = []
        _previewCells = []
        _sprite = null
        _previewName = null
        _status = null
        _hint = null
        _hintSub = null
        _root = null
    }
}
