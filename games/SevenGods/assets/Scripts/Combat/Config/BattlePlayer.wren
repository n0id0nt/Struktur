// Combat/Config/BattlePlayer.wren
// The battle-side definition of the player - the same shape as one Combat/Config/BattleCritter.wren entry (name,
// stats, loadout, moves, sprite sheet + pivot + grid, animation frame ranges), so Combat/Battler.wren can build a
// live battler from either without caring which.
//
// The player's *persistent* Combatant still lives on the Player script (HP carries between fights - see
// GameObjects/Player.combatant); this is where its numbers, its gear and its arena look are declared. Stats are
// the early tier of the battle system doc. Gear is what the player has equipped (Inventory/Equipment.wren): the weapon
// picked in the inventory (Finn's practice sword - an arming sword - by default), and Thomas's leather vest once it
// has been handed over.
// Uses the battle sheet generated from the 3D model (Actors/explorer_battle.png - left and right facing clips).
// `anims` / `animsLeft` map every battle role to one of its clips - see Combat/Config/SheetAnims.wren.
import "math" for Vec2
import "Combat/Combatant" for Combatant
import "Combat/Stats" for Stats
import "Combat/Gear" for GearItem, Loadout
import "Combat/Config/Moves" for Moves
import "Combat/Config/SheetAnims" for SheetAnims
import "Combat/EnemyAI" for PlannerBrain
import "Sprites/ExplorerBattleSprite" for ExplorerBattleSprite
import "Sprites/ExplorerBattleWeaponSprite" for ExplorerBattleWeaponSprite
import "Combat/MoveBook" for MoveBook
import "Inventory/Equipment" for Equipment

var STARTER_MOVES = ["Step", "Slash"]
// The weapon layer sheet (see weaponTexturePath): a placeholder sword in the right hand (a bow in the left for Shoot),
// rendered by 3dModelsToSprites (characters/explorer/job.battle_weapon.json) on the same grid as the body sheet.
var WEAPON_LAYER = ExplorerBattleWeaponSprite.data["texture"]

class BattlePlayer {
    static name { "You" }
    static maxHp { 5 }
    static maxStamina { 80 }
    static tint { null }
    static hasMenu { false }
    static passive { false }

    // Five hits of a tier I move (docs/CombatBalance.md).
    static newStats { Stats.new(5, 80, 0, 0, 0, 12) }

    // What the player fights with - their equipped gear (Inventory/Equipment.wren).
    static loadout { Equipment.loadout }

    // The battle sheet, Actors/explorer_battle.png (generated - see Sprites/ExplorerBattleSprite.wren).
    static texturePath { ExplorerBattleSprite.data["texture"] }
    static cols { ExplorerBattleSprite.data["columns"] }
    static rows { ExplorerBattleSprite.data["rows"] }
    // One frame's size in pixels.
    static cellSize { Vec2.new(ExplorerBattleSprite.data["cellSize"][0], ExplorerBattleSprite.data["cellSize"][1]) }
    static pivot { Vec2.new(ExplorerBattleSprite.data["origin"][0], ExplorerBattleSprite.data["origin"][1]) }
    static facing { 1 }
    // The weapon layer drawn over the sheet, frame for frame (rendered by 3dModelsToSprites alongside it) - null
    // until one has been built.
    static weaponTexturePath { WEAPON_LAYER }

    // role -> [startFrame, endFrameExclusive, seconds] - see Combat/Config/SheetAnims.wren for which clip plays what.
    static anims {
        if (__anims == null) {
            __anims = SheetAnims.forSheet(ExplorerBattleSprite.data, "right", "ExplorerBattleSprite")
        }
        return __anims
    }

    // The same roles facing left (Battler plays these instead of flipping when the player faces left).
    static animsLeft {
        if (__animsLeft == null) {
            __animsLeft = SheetAnims.forSheet(ExplorerBattleSprite.data, "left", "ExplorerBattleSprite")
        }
        return __animsLeft
    }

    // The moves a new game starts with learned and equipped (Combat/MoveBook.wren): Step and Slash. Brace is taught
    // by Thomas, Soul Step comes with the fusion.
    static starterKit { STARTER_MOVES.map { |name| Moves.byName(name) }.toList }

    // The moves the player fights with - their equipped loadout (same shape as BattleCritter.kit), less any the
    // equipped weapon can't use (an older save can still have one in a slot).
    static kit {
        var loadout = BattlePlayer.loadout
        return MoveBook.equippedMoves.where { |move| loadout.allows(move) }.toList
    }
    static kitNames { MoveBook.equipped }

    // A fresh Combatant - GameObjects/Player.combatant calls this once to mint the persistent one.
    static makeCombatant(entity) { BattlePlayer.makeCombatant(entity, BattlePlayer.name, null, null) }

    // The same with its own name, loadout and moves (the battle test's pickers) - null keeps the player's.
    static makeCombatant(entity, name, loadout, moves) {
        var combatant = Combatant.new(name, BattlePlayer.newStats, loadout == null ? BattlePlayer.loadout : loadout,
            moves == null ? BattlePlayer.kit : moves, entity)
        combatant.def = BattlePlayer
        return combatant
    }

    // Brings a kept Combatant up to date with what the player now carries and knows - called as each fight starts.
    static refresh(combatant) {
        combatant.loadout = BattlePlayer.loadout
        combatant.moves = BattlePlayer.kit
    }

    // On the other side (the battle test) the planner plays it.
    static makeBrain() { PlannerBrain.new() }
}
