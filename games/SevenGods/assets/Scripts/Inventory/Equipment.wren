// Inventory/Equipment.wren
// What the player has equipped: the weapon they fight with (an item id), and the battle Loadout that makes
// (Combat/Gear.wren - Combat/Config/BattlePlayer.loadout reads it). Saved to equipment.sav.
//
// The weapons on offer are DEFAULT_WEAPON (Finn's practice sword - always carried, so there is always something to
// go back to) plus every weapon item in the inventory; Inventory/ItemDatabase.wren says which WeaponType each one is.
// Armour isn't picked yet: leather once the player owns a vest (Thomas, first arc scene 4), clothes before.
//
// A weapon decides which moves can be used (Loadout.allows - the move's weapon tags). equipWeapon takes every
// equipped move the new weapon can't use out of the loadout (Combat/MoveBook.wren) and, when it took any, sends the
// movesRemoved game event (Events/GameEvents.wren) - the inventory listens for it to show its alert and flag the
// Moves tab. MoveBook also asks here (its equip filter) before it puts a move into the loadout by itself.
import "serialisation" for Json, MapUtil
import "fileSystem" for FileSystem
import "Combat/Gear" for GearItem, Loadout
import "Combat/MoveBook" for MoveBook
import "Combat/Config/Moves" for Moves
import "Inventory/Inventory" for Inventory
import "Inventory/ItemDatabase" for ItemDatabase, ItemCategory
import "Events/GameEvents" for GameEvents

var DEFAULT_WEAPON = "Practice Sword"
var SAVE_PATH = "equipment.sav"

class Equipment {
    static defaultWeapon { DEFAULT_WEAPON }

    // Main calls this once, after MoveBook.init: from then on MoveBook only fills a loadout slot by itself (a newly
    // learned move, an old save's empty slots) with a move the equipped weapon can use.
    static init() {
        __weapon = DEFAULT_WEAPON
        MoveBook.equipFilter = Fn.new { |move| Equipment.loadout.allows(move) }
    }

    // The equipped weapon's item id - the default again if the one equipped is no longer carried.
    static weapon {
        if (__weapon != DEFAULT_WEAPON && !Inventory.contains(__weapon)) __weapon = DEFAULT_WEAPON
        return __weapon
    }

    // Every weapon the player can equip, item ids: the practice sword first, then the inventory's weapons.
    static weapons {
        var ids = [DEFAULT_WEAPON]
        for (stack in Inventory.stacksIn(ItemCategory.WEAPON)) {
            if (!ids.contains(stack.id) && ItemDatabase.weaponType(stack.id) != null) ids.add(stack.id)
        }
        return ids
    }

    static isEquipped(itemId) { Equipment.weapon == itemId }

    // The battle Loadout with what is equipped now / with `itemId` in the main hand instead.
    static loadout { Equipment.loadoutWith(Equipment.weapon) }
    static loadoutWith(itemId) {
        var type = ItemDatabase.weaponType(itemId)
        var vest = Inventory.count("Leather Vest") > 0 || Inventory.count("Leather Armor") > 0
        return Loadout.new(type == null ? null : GearItem.weapon(type), null, GearItem.armour(vest ? "leather" : "clothes"))
    }

    // The equipped move names that wouldn't work with `itemId` (empty when it changes nothing) - the inventory's
    // warning before the player equips it.
    static movesDroppedBy(itemId) {
        var loadout = Equipment.loadoutWith(itemId)
        return MoveBook.equipped.where { |name| !loadout.allows(Moves.byName(name)) }.toList
    }

    // Equips weapon `itemId` and takes the moves it can't use out of the loadout, saving both. Sends movesRemoved
    // with their names when there were any. Returns the names removed (empty when none, or nothing changed).
    static equipWeapon(itemId) {
        if (itemId == Equipment.weapon || !Equipment.weapons.contains(itemId)) return []
        var dropped = Equipment.movesDroppedBy(itemId)
        __weapon = itemId
        Equipment.save()
        if (dropped.count > 0) {
            for (name in dropped) {
                MoveBook.unequip(name)
            }
            MoveBook.save("moves.sav")
            GameEvents.movesRemoved(itemId, dropped)
        }
        return dropped
    }

    // --- Serialisation ---

    static save() {
        var result = FileSystem.writeString(SAVE_PATH, Json.stringify(MapUtil.mapToPairs({"weapon": __weapon})))
        if (result.success) FileSystem.syncSaves()
    }

    // A missing file, or a weapon that's gone from the item list, leaves the practice sword equipped.
    static load() {
        __weapon = DEFAULT_WEAPON
        var result = FileSystem.readString(SAVE_PATH)
        if (!result.success) return
        var data = Json.parse(result.value)
        if (data is Map && data["weapon"] is String && ItemDatabase.weaponType(data["weapon"]) != null) {
            __weapon = data["weapon"]
        }
    }
}
