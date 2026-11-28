// Combat/MoveBook.wren
// The player's moves outside a fight: every move they have learned, and the up-to-LOADOUT_SIZE of those they carry
// into battle (plus Hold and Use Item, which everyone always has) - Combat/Config/BattlePlayer.kit reads the loadout.
// Moves are learned in the world: a new game starts with the starter kit (Step and Slash - first arc script), the
// "unlockMove" dialogue command teaches more (a newly learned move goes straight into a free loadout slot), and the
// inventory's Moves tab swaps which are equipped. Moves are stored by name ("Slash", "Slash II" -
// Combat/Config/Moves.byName). Saved to moves.sav.
//
// The equipped weapon decides which moves work (Inventory/Equipment.wren sets equipFilter): MoveBook only fills a
// slot by itself - a newly learned move, an old save's empty slots - with a move that passes it, and equipping a new
// weapon unequips the moves it can't use. needsReview stays true from then until the player opens the Moves tab.
import "serialisation" for Json, MapUtil
import "fileSystem" for FileSystem
import "Combat/Config/Moves" for Moves
import "debug" for Debug

var LOADOUT_SIZE = 5   // five moves - with Hold, a 3 x 2 grid in the battle's Moves menu

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

    // Fn(move) -> whether it may go into the loadout by itself (null = anything may) - see the top of the file.
    static equipFilter=(fn) { __equipFilter = fn }
    static canAutoEquip_(name) { __equipFilter == null || __equipFilter.call(Moves.byName(name)) }

    // True after moves were taken out of the loadout (a new weapon), until the Moves tab is opened.
    static needsReview { __needsReview == true }
    static needsReview=(value) { __needsReview = value }

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
        if (__equipped.count < LOADOUT_SIZE && MoveBook.canAutoEquip_(name)) {
            __equipped.add(name)
        }
        return true
    }

    // Takes a move out of the loadout (the slots after it move up); it stays learned.
    static unequip(name) {
        var index = __equipped.indexOf(name)
        if (index >= 0) __equipped.removeAt(index)
        MoveBook.needsReview = true
    }

    // Puts an unlocked move in loadout slot `slot`. If it was already in another slot, the two swap - so the
    // loadout never holds a move twice.
    static equip(slot, name) {
        if (!MoveBook.isUnlocked(name) || slot < 0 || slot >= LOADOUT_SIZE) {
            return
        }
        // An empty slot past the loadout's end: the move joins it (moving out of the slot it was in, if any).
        if (slot >= __equipped.count) {
            if (__equipped.contains(name)) __equipped.removeAt(__equipped.indexOf(name))
            __equipped.add(name)
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
        // An older save may carry more than the loadout holds now (it was six): keep the first ones.
        if (equipped.count > LOADOUT_SIZE) equipped = equipped.take(LOADOUT_SIZE).toList
        for (name in __starter) {
            if (!unlocked.contains(name)) {
                unlocked.add(name)
            }
        }
        __unlocked = unlocked
        // A loadout with room left (an older save, or one whose moves no longer exist) fills up from the learned ones.
        for (name in unlocked) {
            if (equipped.count < LOADOUT_SIZE && !equipped.contains(name) && MoveBook.canAutoEquip_(name)) {
                equipped.add(name)
            }
        }
        // A saved loadout is kept even if it's empty (a weapon change can take every move out).
        if (equipped.count > 0 || data["equipped"] != null) {
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
