// Combat/Config/BattlePlayer.wren
// The battle-side definition of the player - the same shape as one Combat/Config/BattleCritter.wren entry
// (name, fighting style -> move set, HP / stamina pool, sprite sheet + pivot + grid, animation
// frame ranges), so Combat/Battler.wren can build a live battler from either without caring which.
//
// The player's *persistent* Combatant still lives on the Player script (HP carries between fights -
// see GameObjects/Player.combatant); this is where its numbers and its arena look are declared.
// Uses the battle sheet generated from the 3D model (Sprites/explorer_battle.png - left and right facing clips).
// `anims` / `animsLeft` map every battle role to one of its clips - see Combat/Config/SheetAnims.wren.
import "math" for Vec2
import "Combat/Combatant" for Combatant
import "Combat/Config/Archetype" for Archetype
import "Combat/Config/SheetAnims" for SheetAnims
import "Sprites/ExplorerBattleSprite" for ExplorerBattleSprite

class BattlePlayer {
    static name { "You" }
    static archetypeName { "Speed" }
    static maxHp { 120 }
    static maxStamina { 90 }
    // Overrides on top of the archetype's attribute baseline (Combat/Config/Archetype.wren) - null keeps it as is.
    static attributes { null }

    // The battle sheet, Sprites/explorer_battle.png (generated - see Sprites/ExplorerBattleSprite.wren).
    static texturePath { ExplorerBattleSprite.data["texture"] }
    static cols { ExplorerBattleSprite.data["columns"] }
    static rows { ExplorerBattleSprite.data["rows"] }
    static pivot { Vec2.new(ExplorerBattleSprite.data["origin"][0], ExplorerBattleSprite.data["origin"][1]) }
    static facing { 1 }

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

    // The moves the player knows by default - the archetype's kit (same shape as BattleCritter.kit).
    static kit { Archetype.byName(BattlePlayer.archetypeName).kit }

    // A fresh Combatant - GameObjects/Player.combatant calls this once to mint the persistent one.
    static makeCombatant(entity) {
        var archetype = Archetype.byName(BattlePlayer.archetypeName)
        return Combatant.new(BattlePlayer.name, BattlePlayer.maxHp, BattlePlayer.maxStamina,
                             archetype.attributesWith(BattlePlayer.attributes), BattlePlayer.kit, entity)
    }
}
