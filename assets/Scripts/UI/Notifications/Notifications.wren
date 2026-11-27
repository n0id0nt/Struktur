// UI/Notifications/Notifications.wren
// Banners in the top-right corner for game events (Events/GameEvents.wren): a quest starting, moving on or finishing,
// an item or gold received, someone joining the party. It only listens - onEvent(type, data) gets every engine event
// (forwarded by States/GameWorldState.wren) and queues a banner for the ones it knows; whatever caused the event
// sends it and never calls in here. Each banner slides in from the right edge, holds for HOLD_SECONDS, then slides back out.
// Several stack downwards, newest at the bottom; when one leaves, the ones below slide up into its place. A burst of
// events (a dialogue giving three items at once) comes in one after another, SPAWN_INTERVAL apart, and at most
// MAX_VISIBLE show at a time - the rest wait their turn.
//
// A banner is an icon for its category on the left (CATEGORIES), a small header ("New quest") and the message ("The
// Stubborn Well").
//
// States/GameWorldState.wren drives it: onEvent for each engine event, update(showing) every frame - banners animate on
// game time, and freeze out of sight while the world is covered (the inventory, a fight) - and clear() when gameplay
// closes.
import "app" for Application, Time
import "math" for Vec2, Vec4
import "ui" for UIManager
import "localization" for Localization
import "UI/Theme/UIKit" for UIKit
import "UI/Inventory/InventoryTheme" for InventoryTheme
import "Localization/DisplayNames" for DisplayNames
import "Events/GameEvents" for GameEvents

var WIDTH = 420
var HEIGHT = 78
var GAP = 10
var MARGIN = 20                 // from the screen's top and right edges
var ICON_CELL = 54
var ICON_SIZE = 42
var SLIDE_SECONDS = 0.3
var HOLD_SECONDS = 3.0
var SPAWN_INTERVAL = 0.25
var MAX_VISIBLE = 5
var SETTLE_RATE = 12            // how fast banners slide up into a freed slot (higher = snappier)
var LEAVE_AT = 0.9              // how far out a leaving banner gets before the ones below start moving up

var HEADER_COLOR = Vec4.new(176, 84, 22, 255)

// category -> icon (asset path)
var CATEGORIES = {
    "quest": "Items/Scroll/Scroll.png",
    "item": "Items/Object/Bag.png",
    "gold": "Items/Treasure/GoldCoin.png",
    "party": "UI/Skill Icon/Items & Weapon/Helmet.png",
    "info": "Items/Other/Letter.png"
}

class Notifications {
    // Every engine event comes through here; the game events that deserve a banner queue one, the rest are ignored.
    static onEvent(type, data) {
        if (type == GameEvents.itemReceivedType) {
            // "Item received - Healing Herb x3"
            var name = DisplayNames.itemDisplay(data["item"])
            var count = data["count"]
            var text = count != null && count > 1 ? "%(name) x%(count)" : name
            Notifications.push_("item", Localization.get("notify.item"), text)
        } else if (type == GameEvents.goldReceivedType) {
            // "Gold received - +50"
            var amount = data["amount"]
            Notifications.push_("gold", Localization.get("notify.gold"), "+%(amount)")
        } else if (type == GameEvents.partyJoinedType) {
            // "Joined the party - Kira"
            Notifications.push_("party", Localization.get("notify.party"), DisplayNames.npcDisplay(data["name"]))
        } else if (type == GameEvents.questStartedType) {
            // "New quest - The Stubborn Well"
            Notifications.push_("quest", Localization.get("notify.quest.started"), Notifications.questTitle_(data))
        } else if (type == GameEvents.questUpdatedType) {
            // "Quest updated - <the objective just done>"
            var quest = data["quest"]
            var objective = data["objective"]
            Notifications.push_("quest", Localization.get("notify.quest.updated"),
                Localization.get("quest.%(quest).%(objective)"))
        } else if (type == GameEvents.partyRestedType) {
            // "Rested - Everyone is back on their feet."
            Notifications.push_("info", Localization.get("notify.rested"), Localization.get("notify.rested.message"))
        } else if (type == GameEvents.questCompletedType) {
            // "Quest complete - The Stubborn Well"
            Notifications.push_("quest", Localization.get("notify.quest.complete"), Notifications.questTitle_(data))
        }
    }

    static questTitle_(data) {
        var quest = data["quest"]
        return Localization.get("quest.%(quest).title")
    }

    // Queues a banner. category: a CATEGORIES key (its icon).
    static push_(category, header, message) {
        var icon = CATEGORIES.containsKey(category) ? CATEGORIES[category] : CATEGORIES["info"]
        Notifications.queue_.add([category, header, message, icon])
    }

    // Adds or replaces a category's icon.
    static setCategoryIcon(category, icon) { CATEGORIES[category] = icon }

    static queue_ {
        if (__queue == null) __queue = []
        return __queue
    }

    static banners_ {
        if (__banners == null) __banners = []
        return __banners
    }

    // --- every frame ---------------------------------------------------------------------

    // showing: false while something covers the world (the inventory, a fight) - banners hold where they are, out of
    // sight, and pick up again after.
    static update(showing) {
        var dt = Time.scaledDelta
        if (!showing || dt <= 0) {
            if (__root != null) __root.setVisible(false)
            return
        }
        if (__root == null) {
            if (Notifications.queue_.count == 0) return
            Notifications.open_()
        }
        __root.setVisible(true)

        // Bring in the next queued event when there's room.
        __sinceSpawn = __sinceSpawn + dt
        var banners = Notifications.banners_
        var stacked = banners.where { |banner| banner.inStack }.count
        if (Notifications.queue_.count > 0 && __sinceSpawn >= SPAWN_INTERVAL && stacked < MAX_VISIBLE) {
            var next = Notifications.queue_.removeAt(0)
            banners.add(Banner.new(__root, next[1], next[2], next[3], stacked, __font))
            __sinceSpawn = 0
        }

        // Slots: banners still in the stack, top to bottom.
        var slot = 0
        for (banner in banners) {
            if (banner.inStack) {
                banner.targetSlot = slot
                slot = slot + 1
            }
        }
        var gone = []
        for (banner in banners) {
            if (banner.step(dt)) gone.add(banner)
        }
        for (banner in gone) {
            banner.remove()
            banners.remove(banner)
        }
    }

    // Gameplay is closing: every banner and queued event goes.
    static clear() {
        for (banner in Notifications.banners_) {
            banner.remove()
        }
        __banners = []
        __queue = []
        if (__root != null) {
            UIManager.removeUIElement(__root)
            __root = null
            __font.unload()
            __font = null
            UIKit.close()
        }
    }

    static open_() {
        UIKit.open()
        __font = UIKit.font()
        __sinceSpawn = SPAWN_INTERVAL
        __root = InventoryTheme.group(null, Vec2.new(0, 0), Vec2.new(Application.gameWidth, Application.gameHeight), 0)
        UIManager.addUIElement(__root)
    }
}

// One banner: slides in, holds, slides out; moves up to its slot (targetSlot) as the ones above it leave.
class Banner {
    construct new(root, header, message, icon, slot, font) {
        _root = root
        _phase = "in"
        _t = 0
        _y = Banner.slotY_(slot)
        _targetSlot = slot

        _panel = UIKit.frame(root, "choice", Vec2.new(0, 0), Vec2.new(1, 0), Vec2.new(WIDTH, HEIGHT), Vec2.new(0, 0), 10)
        _panel.setAnchorPoint(Vec2.new(1, 0))

        var pad = UIKit.padding("choice")
        var cell = UIKit.frame(_panel, "cell", Vec2.new(pad[0] + 4, 0), Vec2.new(0, 0.5),
            Vec2.new(ICON_CELL, ICON_CELL), Vec2.new(0, 0), 11)
        cell.setAnchorPoint(Vec2.new(0, 0.5))
        var cellPad = UIKit.padding("cell")
        var offset = Vec2.new((cellPad[0] - cellPad[1]) / 2, (cellPad[2] - cellPad[3]) / 2)
        UIKit.image(cell, icon, offset, Vec2.new(0.5, 0.5), Vec2.new(0.5, 0.5), UIKit.fitScale(icon, ICON_SIZE), 12)

        var textX = pad[0] + 4 + ICON_CELL + 14
        InventoryTheme.label(_panel, header, Vec2.new(textX, 10), Vec2.new(0, 0), Vec2.new(0, 0), 18.0, font,
            HEADER_COLOR, 11)
        InventoryTheme.label(_panel, message, Vec2.new(textX, 34), Vec2.new(0, 0), Vec2.new(0, 0), 22.0, font,
            UIKit.textDark, 11)
        place_()
    }

    // In the stack (has a slot): until it's far enough out on its way off.
    inStack { _phase != "out" || _t < SLIDE_SECONDS * LEAVE_AT }

    targetSlot=(value) { _targetSlot = value }

    // Advances the animation; true once it has slid fully off.
    step(dt) {
        _t = _t + dt
        if (_phase == "in" && _t >= SLIDE_SECONDS) {
            _phase = "hold"
            _t = 0
        } else if (_phase == "hold" && _t >= HOLD_SECONDS) {
            _phase = "out"
            _t = 0
        } else if (_phase == "out" && _t >= SLIDE_SECONDS) {
            return true
        }
        if (inStack) {
            var target = Banner.slotY_(_targetSlot)
            var k = dt * SETTLE_RATE
            _y = _y + (target - _y) * (k > 1 ? 1 : k)
        }
        place_()
        return false
    }

    // How far right of its resting place the banner is: off-screen -> 0 sliding in, 0 -> off-screen sliding out.
    slideX_ {
        var offscreen = WIDTH + MARGIN + 8
        var progress = _t / SLIDE_SECONDS
        if (progress > 1) progress = 1
        if (_phase == "in") {
            var eased = 1 - (1 - progress) * (1 - progress) * (1 - progress)   // ease out
            return offscreen * (1 - eased)
        }
        if (_phase == "out") return offscreen * progress * progress * progress   // ease in
        return 0
    }

    place_() {
        _panel.setPosition(Vec2.new(-MARGIN + slideX_, _y), Vec2.new(1, 0))
    }

    remove() {
        if (_panel != null) {
            _root.removeChild(_panel)
            _panel = null
        }
    }

    static slotY_(slot) { MARGIN + slot * (HEIGHT + GAP) }
}
