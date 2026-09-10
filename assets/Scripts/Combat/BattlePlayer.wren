// Combat/BattlePlayer.wren
// The battle-side definition of the player - the same shape as one Combat/BattleCritter.wren entry
// (name, fighting style -> move set, HP / stamina pool, sprite sheet + pivot + grid, animation
// frame ranges), so Combat/Battler.wren can build a live battler from either without caring which.
//
// The player's *persistent* Combatant still lives on the Player script (HP carries between fights -
// see GameObjects/Player.combatant); this is where its numbers and its arena look are declared.
// Reused overworld sprite for now: Sprites/player.png. `anims` keys Battler understands: "idle"
// (required), "attack", "hurt".
import "math" for Vec2
import "Combat/Combatant" for Combatant
import "Combat/Archetype" for Archetype

class BattlePlayer {
    static name { "You" }
    static archetypeName { "Speed" }
    static maxHp { 120 }
    static maxStamina { 90 }

    // Sprites/player.png - 10x10 grid, pivot (19,17). See GameObjects/Player.wren for the full strip
    // list; battle only needs an idle plus stand-ins for attack / hurt.
    static texturePath { "Sprites/player.png" }
    static cols { 10 }
    static rows { 10 }
    static pivot { Vec2.new(19, 17) }
    static facing { 1 }
    // idle = right idle strip, attack = right attack strip, hurt = left idle (stand-in).
    static anims { ({"idle": [3, 4, 1.0], "attack": [58, 65, 0.7], "hurt": [2, 3, 1.0]}) }

    // A fresh Combatant - GameObjects/Player.combatant calls this once to mint the persistent one.
    static makeCombatant(entity) {
        return Combatant.new(BattlePlayer.name, BattlePlayer.maxHp, BattlePlayer.maxStamina,
                             Archetype.byName(BattlePlayer.archetypeName).kit, entity)
    }
}
