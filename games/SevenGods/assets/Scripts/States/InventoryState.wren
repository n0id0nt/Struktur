// states/InventoryState.wren
// The inventory screen. Reads the Inventory model (items by category, gold), the player's moves (Combat/MoveBook)
// and the Party (everyone's stats), turns them into the display-ready model UI/Inventory/InventoryUI.wren shows,
// owns the menu music/focus sound, switches tabs on the UITab input (Q / E, the bumpers) and closes on the Inventory
// or UICancel input (UICancel first backs out of a half-made move swap). Changing the move loadout saves it and
// re-arms the player's Combatant straight away. Game time is paused while it's open - the world freezes behind the
// screen.
//
// Tabs are listed in TABS; one with an unlock rule only shows once that rule holds (the Map tab needs the Old Map).
// Pass {"tab": id} when changing to this state to open on that tab.
//
// Weapons: selecting one in the Items tab equips it (Inventory/Equipment.wren). A weapon that can't use some of the
// equipped moves warns about it in the detail panel first; equipping it takes them out of the loadout, and the
// movesRemoved game event that sends (Events/GameEvents.wren - it reaches onEvent below, through GameWorldState)
// opens an alert listing them and flags the Moves tab until it's opened. The Moves tab lists every learned move and
// greys out the ones the equipped weapon can't use.
import "app" for Time
import "math" for Math
import "resourceManager" for Music, Sound
import "input" for Input
import "localization" for Localization
import "States/BaseState" for BaseState
import "Inventory/Inventory" for Inventory
import "Inventory/ItemDatabase" for ItemDatabase, ItemCategory
import "Party/Party" for Party
import "Combat/Stats" for ATTRIBUTE_NAMES
import "Combat/Config/BattlePlayer" for BattlePlayer
import "Combat/Config/BattleCritter" for BattleCritter
import "Combat/Config/SheetAnims" for SheetAnims
import "Combat/MoveBook" for MoveBook
import "Combat/Move" for Move
import "Combat/Config/Moves" for Moves
import "Inventory/Equipment" for Equipment
import "Events/GameEvents" for GameEvents
import "Localization/DisplayNames" for DisplayNames
import "Story/QuestLog" for QuestLog
import "UI/Inventory/InventoryUI" for InventoryUI
import "World/MapLog" for MapLog
import "World/Rooms" for Rooms
import "debug" for Debug

// The Moves tab's preview shows this much of each battle-sheet cell, centred across the fighter's pivot (their
// feet) and reaching PREVIEW_CROP_ABOVE up from it - the cells leave a lot of empty room around the fighter, and
// cropping lets the preview draw them bigger.
var PREVIEW_CROP_WIDTH = 120
var PREVIEW_CROP_HEIGHT = 80
var PREVIEW_CROP_ABOVE = 67

// Placeholders until there are levels and experience - every member shows these.
var PLACEHOLDER_LEVEL = 1
var PLACEHOLDER_XP = 0
var PLACEHOLDER_XP_TO_NEXT = 100

// The tabs in order: [id, unlock rule or null]. A rule is a Fn returning whether the tab shows - see
// InventoryState.carrying_ (an item that opens the tab, like the Old Map for "map").
var TABS = [
    ["items", null],
    ["moves", null],
    ["party", null],
    ["levels", null],
    ["quests", null],
    ["map", Fn.new { InventoryState.carrying_("map") }]
]

class InventoryState is BaseState {
    construct new() {
        super()
        name = "InventoryState"
        _view = null
        _menuMusic = null
        _itemFocusSound = null
        _lastTabStep = 0
        _partyEntries = []
    }

    enter(stateManager, params) {
        super.enter(stateManager, params)
        Debug.info("Entering Inventory")
        Time.setTimeScale(0)

        _menuMusic = Music.load("Audio/Musics/5 - Peaceful.ogg")
        _menuMusic.setLooping(true)
        _menuMusic.play()
        _itemFocusSound = Sound.load("Audio/Sounds/Menu/Move1.wav")

        _lastTabStep = tabStep_()
        var startTab = (params is Map) ? params["tab"] : null
        var callbacks = {}
        callbacks["focusMoved"] = Fn.new { _itemFocusSound.play() }
        callbacks["equipMove"] = Fn.new { |slot, id| equip_(slot, id) }
        callbacks["equipWeapon"] = Fn.new { |itemId| equipWeapon_(itemId) }
        callbacks["tabOpened"] = Fn.new { |id| tabOpened_(id) }
        _view = InventoryUI.new(buildModel_(startTab), callbacks)
    }

    // Game events, forwarded by GameWorldState: a weapon change that took moves out of the loadout shows the alert
    // and flags the Moves tab.
    onEvent(type, data) {
        if (type == GameEvents.movesRemovedType && _view != null) {
            var weapon = DisplayNames.itemDisplay(data["weapon"])
            var body = Localization.get("inventory.alert.moves_removed.body").replace("{weapon}", weapon)
            var alert = {}
            alert["title"] = Localization.get("inventory.alert.moves_removed.title")
            alert["body"] = body
            alert["lines"] = data["moves"]
            alert["footer"] = Localization.get("inventory.alert.moves_removed.footer")
            alert["buttons"] = [[Localization.get("inventory.alert.ok"), null]]
            _view.showAlert(alert)
            _view.setTabAlert("moves", MoveBook.needsReview)
        }
    }

    update(stateManager) {
        var cancel = Input.isInputJustReleased("UICancel")
        // An open alert takes the keys (and holds the tabs still) until it's dismissed: its own buttons and Esc go
        // through the view's update; the Inventory key shuts it.
        if (_view.alertOpen) {
            if (Input.isInputJustReleased("Inventory")) _view.closeAlert()
            _view.update(Time.unscaledTime)
            _lastTabStep = tabStep_()
            return
        }
        if (Input.isInputJustReleased("Inventory") || (cancel && !_view.cancel())) {
            stateManager.clearCurrentState()
            return
        }
        _view.update(Time.unscaledTime)
        var step = tabStep_()
        if (step != 0 && step != _lastTabStep) {
            _view.cycleTab(step)
        }
        _lastTabStep = step
    }

    exit() {
        super.exit()
        Debug.info("Unloading Inventory...")
        Time.setTimeScale(1)

        if (_view != null) {
            _view.teardown()
            _view = null
        }
        _partyEntries = []

        _menuMusic.stop()
        _menuMusic.unload()
        _menuMusic = null
        _itemFocusSound.unload()
        _itemFocusSound = null
        Debug.info("Inventory unloaded")
    }

    // UITab is an axis - -1 / 0 / 1 - so a tab switch is the moment it leaves 0.
    tabStep_() {
        var value = Input.getInputAxis("UITab")
        return value > 0.5 ? 1 : (value < -0.5 ? -1 : 0)
    }

    // --- view model ------------------------------------------------------------------

    buildModel_(startTab) {
        var model = {}
        var tabs = []
        for (tab in TABS) {
            if (tab[1] == null || tab[1].call()) {
                tabs.add([tab[0], Localization.get("inventory.tab.%(tab[0])")])
            }
        }
        model["tabs"] = tabs
        model["startTab"] = startTab
        model["items"] = itemSections_()
        model["itemsEmpty"] = Localization.get("inventory.empty.items")
        if (tabs.any { |tab| tab[0] == "map" }) {
            model["map"] = mapModel_()
        }
        _movesModel = movesModel_()
        model["moves"] = _movesModel
        var tabAlerts = {}
        tabAlerts["moves"] = MoveBook.needsReview
        model["tabAlerts"] = tabAlerts
        _partyEntries = partyEntries_()
        model["party"] = _partyEntries
        var partyLabels = {}
        for (key in ["empty", "downed"]) {
            partyLabels[key] = Localization.get("inventory.party.%(key)")
        }
        for (key in ["hp", "stamina"]) {
            partyLabels[key] = Localization.get("inventory.stat.%(key)")
        }
        for (key in ["title", "move", "time", "damage", "stamina", "heal", "none"]) {
            partyLabels["moves.%(key)"] = Localization.get("inventory.moves.%(key)")
        }
        model["partyLabels"] = partyLabels
        model["levels"] = levelEntries_()
        var levelLabels = {}
        for (key in ["placeholder", "level", "xp"]) {
            levelLabels[key] = Localization.get("inventory.levels.%(key)")
        }
        model["levelLabels"] = levelLabels
        model["quests"] = questsModel_()
        var questLabels = {}
        for (key in ["overall", "complete", "empty", "objectives"]) {
            questLabels[key] = Localization.get("inventory.quests.%(key)")
        }
        model["questLabels"] = questLabels
        model["gold"] = Inventory.gold
        var hints = {}
        for (key in ["tabs", "move", "select", "close"]) {
            hints[key] = Localization.get("inventory.hint.%(key)")
        }
        model["hints"] = hints
        return model
    }

    // True when an item carried opens this tab (ItemDatabase "opensTab") - the unlock rule for item-backed tabs.
    static carrying_(tabId) { Inventory.stacks.any { |stack| ItemDatabase.opensTab(stack.id) == tabId } }

    // The Items tab: a section per category (ItemCategory.all - key items, consumables, weapons, armour). Old
    // "material" items show with the key items. See UI/Inventory/ItemGridPage.wren for the shape.
    itemSections_() {
        var sections = []
        for (category in ItemCategory.all) {
            var categories = category == ItemCategory.KEY ? [ItemCategory.KEY, ItemCategory.MATERIAL] : [category]
            var section = {}
            section["title"] = Localization.get("inventory.section.%(category)")
            section["entries"] = category == ItemCategory.WEAPON ? weaponEntries_() : itemEntries_(categories)
            sections.add(section)
        }
        return sections
    }

    // The Map tab: every room visited (World/MapLog.wren) - see UI/Inventory/MapPage.wren for the shape.
    mapModel_() {
        var model = {}
        model["rooms"] = MapLog.rooms(Rooms.world)
        model["doors"] = MapLog.doors(Rooms.world)
        model["current"] = MapLog.current
        model["currentName"] = MapLog.displayName(MapLog.current)
        var labels = {}
        for (key in ["title", "here", "empty", "door"]) {
            labels[key] = Localization.get("inventory.map.%(key)")
        }
        model["labels"] = labels
        var names = {}
        for (room in model["rooms"]) {
            var terrain = room["terrain"]
            if (!names.containsKey(terrain)) {
                names[terrain] = terrain == "" ? Localization.get("inventory.map.unknown") : Localization.get("terrain.%(terrain)")
            }
        }
        model["terrainNames"] = names
        return model
    }

    // The stacks in these categories, category by category - see UI/Inventory/ItemGridPage.wren for the shape.
    itemEntries_(categories) {
        var entries = []
        for (category in categories) {
            for (stack in Inventory.stacksIn(category)) {
                entries.add(itemEntry_(stack.id, category, stack.count, stack.stacks))
            }
        }
        return entries
    }

    itemEntry_(id, category, count, stacks) {
        var bonusMap = ItemDatabase.bonuses(id)
        var bonuses = []
        for (attribute in ATTRIBUTE_NAMES) {
            if (bonusMap.containsKey(attribute)) {
                bonuses.add([Localization.get("inventory.stat.%(attribute)"), bonusMap[attribute]])
            }
        }
        var quantity = Localization.get("inventory.quantity")
        var entry = {}
        entry["name"] = DisplayNames.itemDisplay(id)
        entry["icon"] = ItemDatabase.iconPath(id)
        entry["count"] = count
        entry["stackable"] = stacks
        entry["showCount"] = stacks && count > 1
        entry["countText"] = "%(quantity) %(count)"
        entry["category"] = Localization.get(ItemCategory.labelKey(category))
        entry["bonuses"] = bonuses
        var opensTab = ItemDatabase.opensTab(id)
        entry["opensTab"] = opensTab
        entry["actionText"] = opensTab != null ? Localization.get("inventory.open.%(opensTab)") : null
        // One-of-a-kind key items don't show "Quantity: 1".
        if (ItemDatabase.unique(id)) entry["stackable"] = false
        return entry
    }

    // The Weapons section: every weapon that can be equipped (Inventory/Equipment.weapons - the practice sword
    // first), each pressable. Kept, so a weapon change can update them in place (refreshWeapons_).
    weaponEntries_() {
        _weaponEntries = []
        for (id in Equipment.weapons) {
            var entry = itemEntry_(id, ItemCategory.WEAPON, 1, false)
            entry["equipItem"] = id
            _weaponEntries.add(entry)
        }
        refreshWeapons_()
        return _weaponEntries
    }

    // What equipping each weapon would do now: "Equipped" on the one in hand, otherwise "Select to equip" - plus a
    // warning when it would take equipped moves out of the loadout.
    refreshWeapons_() {
        for (entry in _weaponEntries) {
            var id = entry["equipItem"]
            var equipped = Equipment.isEquipped(id)
            entry["equipped"] = equipped
            entry["actionText"] = Localization.get(equipped ? "inventory.equip.equipped" : "inventory.equip.select")
            var dropped = equipped ? [] : Equipment.movesDroppedBy(id)
            if (dropped.count == 0) {
                entry["warning"] = null
            } else {
                var key = dropped.count == 1 ? "inventory.equip.removes_one" : "inventory.equip.removes_many"
                var count = "%(dropped.count)"
                entry["warning"] = [Localization.get("inventory.equip.warning"), Localization.get(key).replace("{n}", count)]
            }
        }
    }

    // Everyone in the party with a Combatant, in party order - see UI/Inventory/PartyPage.wren for the shape.
    partyEntries_() {
        var entries = []
        for (member in Party.members) {
            var combatant = Party.combatantOf(member)
            if (combatant != null) {
                var stats = combatant.stats
                var attributes = []
                for (attribute in ATTRIBUTE_NAMES) {
                    attributes.add([Localization.get("inventory.stat.%(attribute)"), stats.attr(attribute)])
                }
                var entry = {}
                entry["name"] = memberName_(member)
                entry["isPlayer"] = member.isPlayer
                entry["className"] = InventoryState.styleOf_(combatant)
                entry["downed"] = member.downed
                entry["hp"] = stats.hp
                entry["maxHp"] = stats.maxHp
                entry["stamina"] = stats.stamina.round
                entry["maxStamina"] = stats.maxStamina
                entry["attributes"] = attributes
                entry["moves"] = combatant.moves.map { |move| InventoryState.moveEntry_(move) }.toList
                entries.add(entry)
            }
        }
        return entries
    }

    // The players only - their level is the one that matters (NPC members don't level).
    levelEntries_() {
        var entries = []
        for (member in Party.members.where { |candidate| candidate.isPlayer }) {
            var entry = {}
            entry["name"] = memberName_(member)
            entry["level"] = PLACEHOLDER_LEVEL
            entry["xp"] = PLACEHOLDER_XP
            entry["xpToNext"] = PLACEHOLDER_XP_TO_NEXT
            entries.add(entry)
        }
        return entries
    }

    // The Quests tab (Story/QuestLog.wren): active quests by category, then the finished ones - see
    // UI/Inventory/QuestsPage.wren for the shape. Quests not started yet aren't listed.
    questsModel_() {
        var quests = QuestLog.all
        var sections = []
        for (category in QuestLog.categories) {
            var section = {}
            section["title"] = Localization.get("quest.category.%(category)")
            section["quests"] = quests.where { |quest| quest["category"] == category && quest["state"] == QuestLog.active }.map { |quest| InventoryState.questEntry_(quest) }.toList
            sections.add(section)
        }
        var completed = {}
        completed["title"] = Localization.get("inventory.quests.completed")
        completed["quests"] = quests.where { |quest| quest["state"] == QuestLog.complete }.map { |quest| InventoryState.questEntry_(quest) }.toList
        sections.add(completed)
        var model = {}
        model["overall"] = QuestLog.overallFraction
        model["sections"] = sections
        return model
    }

    static questEntry_(quest) {
        var id = quest["id"]
        var category = quest["category"]
        var entry = {}
        entry["title"] = Localization.get("quest.%(id).title")
        entry["category"] = Localization.get("quest.category.%(category)")
        entry["summary"] = Localization.get("quest.%(id).summary")
        entry["fraction"] = quest["fraction"]
        entry["complete"] = quest["state"] == QuestLog.complete
        entry["objectives"] = quest["objectives"].map { |objective|
            var objectiveId = objective["id"]
            var line = {}
            line["text"] = Localization.get("quest.%(id).%(objectiveId)")
            line["done"] = objective["done"]
            return line
        }.toList
        return entry
    }

    // The Moves tab: every unlocked move (with the battle-sheet clip it plays), the loadout, and the sheet the
    // preview animates - see UI/Inventory/MovesPage.wren.
    movesModel_() {
        var anims = BattlePlayer.anims
        var model = {}
        model["moves"] = MoveBook.unlockedMoves.map { |move|
            var entry = InventoryState.moveEntry_(move)
            entry["anim"] = InventoryState.clipFor_(move, anims)
            return entry
        }.toList
        InventoryState.markCompatible_(model)
        model["equipped"] = MoveBook.equipped.toList
        model["slots"] = MoveBook.loadoutSize

        var sheet = {}
        sheet["texture"] = BattlePlayer.texturePath
        sheet["columns"] = BattlePlayer.cols
        sheet["cellWidth"] = BattlePlayer.cellSize.x
        sheet["cellHeight"] = BattlePlayer.cellSize.y
        sheet["idle"] = anims["idle"]
        var pivot = BattlePlayer.pivot
        var cropX = Math.clamp(pivot.x - PREVIEW_CROP_WIDTH / 2, 0, sheet["cellWidth"] - PREVIEW_CROP_WIDTH)
        var cropY = Math.clamp(pivot.y - PREVIEW_CROP_ABOVE, 0, sheet["cellHeight"] - PREVIEW_CROP_HEIGHT)
        sheet["crop"] = [cropX, cropY, PREVIEW_CROP_WIDTH, PREVIEW_CROP_HEIGHT]
        model["sheet"] = sheet

        var labels = {}
        var keys = [["loadout", "loadout"], ["unlocked", "unlocked"], ["slot", "slot"], ["equippedIn", "equipped_in"],
                    ["notEquipped", "not_equipped"], ["hintIdle", "hint_idle"], ["hintSlot", "hint_slot"],
                    ["hintMove", "hint_move"], ["hintCancel", "hint_cancel"], ["time", "time"], ["damage", "damage"],
                    ["stamina", "stamina"], ["heal", "heal"], ["incompatible", "incompatible"]]
        for (key in keys) {
            labels[key[0]] = Localization.get("inventory.moves.%(key[1])")
        }
        model["labels"] = labels
        return model
    }

    // Which moves the equipped weapon can use ("compatible" on each entry - the rest are greyed out), and its name for
    // the "Can't be used with the ..." line.
    static markCompatible_(model) {
        var loadout = Equipment.loadout
        for (entry in model["moves"]) {
            entry["compatible"] = loadout.allows(Moves.byName(entry["id"]))
        }
        model["weaponName"] = DisplayNames.itemDisplay(Equipment.weapon)
    }

    // The Items tab equipped weapon `itemId`: Equipment swaps it in and takes out the moves it can't use (sending
    // movesRemoved if it did - see onEvent). Brings the player's Combatant and every tab's copy up to date.
    equipWeapon_(itemId) {
        if (Equipment.isEquipped(itemId)) return
        Equipment.equipWeapon(itemId)
        rearmPlayer_()
        refreshWeapons_()
        InventoryState.markCompatible_(_movesModel)
        _movesModel["equipped"] = MoveBook.equipped.toList
        _view.refreshGear(_movesModel)
    }

    // A tab was opened (its badge hides itself): opening Moves clears its "update your moves" flag.
    tabOpened_(id) {
        if (id == "moves") MoveBook.needsReview = false
    }

    // The player's Combatant fights with the gear and moves now equipped, and the Party tab shows them.
    rearmPlayer_() {
        for (member in Party.members) {
            var combatant = member.isPlayer ? Party.combatantOf(member) : null
            if (combatant != null) {
                BattlePlayer.refresh(combatant)
            }
        }
        for (entry in _partyEntries) {
            if (entry["isPlayer"]) {
                entry["moves"] = BattlePlayer.kit.map { |move| InventoryState.moveEntry_(move) }.toList
                entry["className"] = InventoryState.styleName_(BattlePlayer.loadout)
            }
        }
    }

    // The Moves tab put `id` in loadout slot `slot`: save it, give every player's Combatant the new kit for their
    // next fight, and update the Party tab's copy of the player's moves. Returns the new loadout for the view.
    equip_(slot, id) {
        MoveBook.equip(slot, id)
        MoveBook.save("moves.sav")
        rearmPlayer_()
        return MoveBook.equipped.toList
    }

    // The battle-sheet clip [firstFrame, endFrame, seconds] the player plays for `move` - the same role
    // Combat/Battler.wren would pick (the move's `anim`, falling back to a generic one - Combat/Config/SheetAnims.wren).
    static clipFor_(move, anims) {
        var role = move.anim
        if (anims.containsKey(role)) {
            return anims[role]
        }
        var fallback = SheetAnims.fallbackRole(role)
        if (fallback != null && anims.containsKey(fallback)) {
            return anims[fallback]
        }
        return anims["idle"]
    }

    // One move row for the Party tab's moves panel and the Moves tab - see UI/Inventory/MoveCells.wren.
    static moveEntry_(move) {
        var entry = {}
        entry["id"] = move.name
        entry["name"] = move.name
        entry["time"] = Move.fmt(move.completion)
        entry["damage"] = move.isAttack ? move.damage.round : 0
        entry["staminaCost"] = move.cost
        entry["staminaRestore"] = 0
        entry["heal"] = move.isHeal ? move.healAmount.round : 0
        return entry
    }

    memberName_(member) {
        return member.isPlayer ? Localization.get("inventory.party.you") : DisplayNames.npcDisplay(member.name)
    }

    // What a member fights with, in a word or two: their main-hand weapon (Combat/Gear.wren).
    static styleOf_(combatant) { InventoryState.styleName_(combatant.loadout) }

    static styleName_(loadout) {
        var main = loadout.main
        return main == null ? "Unarmed" : main.type.name
    }
}
