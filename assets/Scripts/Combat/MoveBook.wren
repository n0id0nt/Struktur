// Combat/MoveBook.wren
// The player's moves outside a fight: every move they have unlocked, and the LOADOUT_SIZE of those they carry into
// battle - Combat/Config/BattlePlayer.kit reads the loadout, so a freshly minted player Combatant fights with it
// (States/InventoryState.wren pushes a changed loadout onto the live one). A new game starts with the starter kit
// (the archetype's) unlocked and equipped; the "unlockMove" dialogue command unlocks more, and the inventory's
// Moves tab swaps which are equipped. Moves are stored by name (Combat/Config/Moves.byName). Saved to moves.sav.
import "serialisation" for Json, MapUtil
import "fileSystem" for FileSystem
import "Combat/Config/Moves" for Moves
import "debug" for Debug

var LOADOUT_SIZE = 3

class MoveBook {
    static loadoutSize { LOADOUT_SIZE }

    // starterKit: the Moves a new game begins with unlocked and equipped (Main passes BattlePlayer.starterKit).
    static init(starterKit) {
        __starter = starterKit.map { |move| move.name }.toList
        MoveBook.reset_()
    }

    static reset_() {
        __unlocked = __starter.toList
        __equipped = __starter.take(LOADOUT_SIZE).toList
    }

    // Move names, in the order they were unlocked.
    static unlocked { __unlocked }
    static unlockedMoves { __unlocked.map { |name| Moves.byName(name) }.toList }

    // The equipped move names, slot by slot.
    static equipped { __equipped }
    static equippedMoves { __equipped.map { |name| Moves.byName(name) }.toList }

    static isUnlocked(name) { __unlocked.contains(name) }
    static isEquipped(name) { __equipped.contains(name) }

    // Unlocks a move from the catalog; false if there is no such move or it was already unlocked.
    static unlock(name) {
        if (Moves.byName(name) == null) {
            Debug.warning("[MoveBook] No move called %(name) to unlock")
            return false
        }
        if (MoveBook.isUnlocked(name)) {
            return false
        }
        __unlocked.add(name)
        return true
    }

    // Puts an unlocked move in loadout slot `slot`. If it was already in another slot, the two swap - so the
    // loadout never holds a move twice.
    static equip(slot, name) {
        if (!MoveBook.isUnlocked(name) || slot < 0 || slot >= __equipped.count) {
            return
        }
        var from = __equipped.indexOf(name)
        if (from >= 0) {
            __equipped[from] = __equipped[slot]
        }
        __equipped[slot] = name
    }

    // --- Serialisation ---

    static serialise() {
        var map = { "unlocked": __unlocked, "equipped": __equipped }
        return Json.stringify(MapUtil.mapToPairs(map))
    }

    // Unknown names (a move renamed or removed since the save) are dropped; a loadout that no longer fills every
    // slot falls back to the starter kit.
    static parse(json) {
        var data = Json.parse(json)
        MoveBook.reset_()
        if (data == null || data["unlocked"] == null) {
            return
        }
        var unlocked = data["unlocked"].where { |name| Moves.byName(name) != null }.toList
        var equipped = data["equipped"] == null ? [] : data["equipped"].where { |name| unlocked.contains(name) }.toList
        for (name in __starter) {
            if (!unlocked.contains(name)) {
                unlocked.add(name)
            }
        }
        __unlocked = unlocked
        if (equipped.count == LOADOUT_SIZE) {
            __equipped = equipped
        }
    }

    static save(path) {
        var result = FileSystem.writeString(path, MoveBook.serialise())
        if (result.success) {
            FileSystem.syncSaves()
        }
    }

    static load(path) {
        var result = FileSystem.readString(path)
        if (result.success) {
            MoveBook.parse(result.value)
        }
    }
}
