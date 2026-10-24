// Combat/Config/BattlePlayer.wren
// The battle-side definition of the player - the same shape as one Combat/Config/BattleCritter.wren entry
// (name, fighting style -> move set, HP / stamina pool, sprite sheet + pivot + grid, animation
// frame ranges), so Combat/Battler.wren can build a live battler from either without caring which.
//
// The player's *persistent* Combatant still lives on the Player script (HP carries between fights -
// see GameObjects/Player.combatant); this is where its numbers and its arena look are declared.
// Uses the overworld sheet generated from the 3D model (Sprites/explorer.png). `anims` maps every battle
// role to one of its clips - see Combat/Config/SheetAnims.wren and ActionAnimations.wren.
import "math" for Vec2
import "Combat/Combatant" for Combatant
import "Combat/Config/Archetype" for Archetype
import "Combat/Config/SheetAnims" for SheetAnims
import "Sprites/ExplorerSprite" for ExplorerSprite

class BattlePlayer {
    static name { "You" }
    static archetypeName { "Speed" }
    static maxHp { 120 }
    static maxStamina { 90 }
    // Overrides on top of the archetype's attribute baseline (Combat/Config/Archetype.wren) - null keeps it as is.
    static attributes { null }

    // The overworld sheet, Sprites/explorer.png (generated - see Sprites/ExplorerSprite.wren). Battle plays
    // its right-facing clips; Battler flips them for the other side.
    static texturePath { ExplorerSprite.data["texture"] }
    static cols { ExplorerSprite.data["columns"] }
    static rows { ExplorerSprite.data["rows"] }
    static pivot { Vec2.new(ExplorerSprite.data["origin"][0], ExplorerSprite.data["origin"][1]) }
    static facing { 1 }

    // role -> [startFrame, endFrameExclusive, seconds] - see Combat/Config/SheetAnims.wren for which clip plays what.
    static anims {
        if (__anims == null) {
            __anims = SheetAnims.forSheet(ExplorerSprite.data, "ExplorerSprite")
        }
        return __anims
    }

    // A fresh Combatant - GameObjects/Player.combatant calls this once to mint the persistent one.
    static makeCombatant(entity) {
        var archetype = Archetype.byName(BattlePlayer.archetypeName)
        return Combatant.new(BattlePlayer.name, BattlePlayer.maxHp, BattlePlayer.maxStamina,
                             archetype.attributesWith(BattlePlayer.attributes), archetype.kit, entity)
    }
}
