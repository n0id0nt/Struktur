// states/InventoryState.wren
// The inventory screen. Reads the Inventory model (items by category, gold) and the Party (everyone's stats),
// turns them into the display-ready model UI/Inventory/InventoryUI.wren shows, owns the menu music/focus sound,
// switches tabs on the UITab input (Q / E, the bumpers) and closes on the Inventory or UICancel input. Game time is
// paused while it's open - the world freezes behind the screen.
import "app" for Time
import "resourceManager" for Music, Sound
import "input" for Input
import "localization" for Localization
import "States/BaseState" for BaseState
import "Inventory/Inventory" for Inventory
import "Inventory/ItemDatabase" for ItemDatabase, ItemCategory
import "Party/Party" for Party
import "Combat/Stats" for ATTRIBUTE_NAMES
import "Combat/Config/Archetype" for Archetype
import "Combat/Config/BattlePlayer" for BattlePlayer
import "Combat/Config/BattleCritter" for BattleCritter
import "Localization/DisplayNames" for DisplayNames
import "UI/Inventory/InventoryUI" for InventoryUI
import "debug" for Debug

// Placeholders until there are levels and experience - every member shows these.
var PLACEHOLDER_LEVEL = 1
var PLACEHOLDER_XP = 0
var PLACEHOLDER_XP_TO_NEXT = 100

class InventoryState is BaseState {
    construct new() {
        super()
        name = "InventoryState"
        _view = null
        _menuMusic = null
        _itemFocusSound = null
        _lastTabStep = 0
    }

    enter(stateManager, params) {
        super.enter(stateManager, params)
        Debug.info("Entering Inventory")
        Time.setTimeScale(0)

        _menuMusic = Music.load("Sounds/menuMusic.wav")
        _menuMusic.setLooping(true)
        _menuMusic.play()
        _itemFocusSound = Sound.load("Sounds/scroll.wav")

        _lastTabStep = tabStep_()
        _view = InventoryUI.new(buildModel_(), Fn.new { _itemFocusSound.play() })
    }

    update(stateManager) {
        if (Input.isInputJustReleased("Inventory") || Input.isInputJustReleased("UICancel")) {
            stateManager.clearCurrentState()
            return
        }
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

    buildModel_() {
        var model = {}
        model["tabs"] = ["items", "weapons", "armor", "party", "levels"].map { |t| Localization.get("inventory.tab.%(t)") }.toList
        model["itemPages"] = [
            [itemEntries_([ItemCategory.CONSUMABLE, ItemCategory.MATERIAL]), Localization.get("inventory.empty.items")],
            [itemEntries_([ItemCategory.WEAPON]), Localization.get("inventory.empty.weapons")],
            [itemEntries_([ItemCategory.ARMOR]), Localization.get("inventory.empty.armor")]
        ]
        model["party"] = partyEntries_()
        var partyLabels = {}
        for (key in ["empty", "downed"]) {
            partyLabels[key] = Localization.get("inventory.party.%(key)")
        }
        for (key in ["hp", "stamina"]) {
            partyLabels[key] = Localization.get("inventory.stat.%(key)")
        }
        model["partyLabels"] = partyLabels
        model["levels"] = levelEntries_()
        var levelLabels = {}
        for (key in ["placeholder", "level", "xp"]) {
            levelLabels[key] = Localization.get("inventory.levels.%(key)")
        }
        model["levelLabels"] = levelLabels
        model["gold"] = Inventory.gold
        model["hint"] = Localization.get("inventory.hint")
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
                entry["className"] = InventoryState.archetypeOf_(member).name
                entry["downed"] = member.downed
                entry["hp"] = stats.hp
                entry["maxHp"] = stats.maxHp
                entry["stamina"] = stats.stamina.round
                entry["maxStamina"] = stats.maxStamina
                entry["attributes"] = attributes
                entries.add(entry)
            }
        }
        return entries
    }

    levelEntries_() {
        var entries = []
        for (member in Party.members) {
            var entry = {}
            entry["name"] = memberName_(member)
            entry["level"] = PLACEHOLDER_LEVEL
            entry["xp"] = PLACEHOLDER_XP
            entry["xpToNext"] = PLACEHOLDER_XP_TO_NEXT
            entries.add(entry)
        }
        return entries
    }

    memberName_(member) {
        return member.isPlayer ? Localization.get("inventory.party.you") : DisplayNames.npcDisplay(member.name)
    }

    // The fighting style a member's Combatant was built from (the player's from Combat/Config/BattlePlayer.wren,
    // an NPC's from their Combat/Config/BattleCritter.wren def).
    static archetypeOf_(member) {
        var style = member.isPlayer ? BattlePlayer.archetypeName : BattleCritter.forName(member.name).archetypeName
        return Archetype.byName(style)
    }
}
