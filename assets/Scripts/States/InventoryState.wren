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
        _view = InventoryUI.new(buildModel_(startTab), Fn.new { _itemFocusSound.play() }, Fn.new { |slot, id| equip_(slot, id) })
    }

    update(stateManager) {
        var cancel = Input.isInputJustReleased("UICancel")
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
        if (tabs.any { |t| t[0] == "map" }) {
            model["map"] = mapModel_()
        }
        model["moves"] = movesModel_()
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
    static carrying_(tabId) { Inventory.stacks.any { |s| ItemDatabase.opensTab(s.id) == tabId } }

    // The Items tab: a section per category (ItemCategory.all - key items, consumables, weapons, armour). Old
    // "material" items show with the key items. See UI/Inventory/ItemGridPage.wren for the shape.
    itemSections_() {
        var sections = []
        for (category in ItemCategory.all) {
            var categories = category == ItemCategory.KEY ? [ItemCategory.KEY, ItemCategory.MATERIAL] : [category]
            var section = {}
            section["title"] = Localization.get("inventory.section.%(category)")
            section["entries"] = itemEntries_(categories)
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
            var categoryText = Localization.get(ItemCategory.labelKey(category))
            for (stack in Inventory.stacksIn(category)) {
                var bonusMap = ItemDatabase.bonuses(stack.id)
                var bonuses = []
                for (attribute in ATTRIBUTE_NAMES) {
                    if (bonusMap.containsKey(attribute)) {
                        bonuses.add([Localization.get("inventory.stat.%(attribute)"), bonusMap[attribute]])
                    }
                }
                var entry = {}
                entry["name"] = DisplayNames.itemDisplay(stack.id)
                entry["icon"] = ItemDatabase.iconPath(stack.id)
                entry["count"] = stack.count
                entry["stackable"] = stack.stacks
                entry["showCount"] = stack.stacks && stack.count > 1
                entry["countText"] = "%(Localization.get("inventory.quantity")) %(stack.count)"
                entry["category"] = categoryText
                entry["bonuses"] = bonuses
                var opensTab = ItemDatabase.opensTab(stack.id)
                entry["opensTab"] = opensTab
                entry["actionText"] = opensTab != null ? Localization.get("inventory.open.%(opensTab)") : null
                // One-of-a-kind key items don't show "Quantity: 1".
                if (ItemDatabase.unique(stack.id)) entry["stackable"] = false
                entries.add(entry)
            }
        }
        return entries
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
        for (member in Party.members.where { |m| m.isPlayer }) {
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
            section["quests"] = quests.where { |q| q["category"] == category && q["state"] == QuestLog.active }.map { |q| InventoryState.questEntry_(q) }.toList
            sections.add(section)
        }
        var completed = {}
        completed["title"] = Localization.get("inventory.quests.completed")
        completed["quests"] = quests.where { |q| q["state"] == QuestLog.complete }.map { |q| InventoryState.questEntry_(q) }.toList
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
        entry["objectives"] = quest["objectives"].map { |o|
            var objectiveId = o["id"]
            var line = {}
            line["text"] = Localization.get("quest.%(id).%(objectiveId)")
            line["done"] = o["done"]
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
                    ["stamina", "stamina"], ["heal", "heal"]]
        for (key in keys) {
            labels[key[0]] = Localization.get("inventory.moves.%(key[1])")
        }
        model["labels"] = labels
        return model
    }

    // The Moves tab put `id` in loadout slot `slot`: save it, give every player's Combatant the new kit for their
    // next fight, and update the Party tab's copy of the player's moves. Returns the new loadout for the view.
    equip_(slot, id) {
        MoveBook.equip(slot, id)
        MoveBook.save("moves.sav")
        for (member in Party.members) {
            var combatant = member.isPlayer ? Party.combatantOf(member) : null
            if (combatant != null) {
                combatant.moves = MoveBook.equippedMoves
            }
        }
        for (entry in _partyEntries) {
            if (entry["isPlayer"]) {
                entry["moves"] = MoveBook.equippedMoves.map { |move| InventoryState.moveEntry_(move) }.toList
            }
        }
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
    static styleOf_(combatant) {
        var main = combatant.loadout.main
        return main == null ? "Unarmed" : main.type.name
    }
}
