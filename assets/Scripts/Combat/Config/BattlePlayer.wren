// Combat/Config/BattlePlayer.wren
// The battle-side definition of the player - the same shape as one Combat/Config/BattleCritter.wren entry
// (name, fighting style -> move set, HP / stamina pool, sprite sheet + pivot + grid, animation
// frame ranges), so Combat/Battler.wren can build a live battler from either without caring which.
//
// The player's *persistent* Combatant still lives on the Player script (HP carries between fights -
// see GameObjects/Player.combatant); this is where its numbers and its arena look are declared.
// Uses the overworld sheet generated from the 3D model (Sprites/explorer.png). `anims` maps every battle
// role to one of its clips - see ROLE_CLIPS and Combat/Config/ActionAnimations.wren.
import "math" for Vec2
import "Combat/Combatant" for Combatant
import "Combat/Config/Archetype" for Archetype
import "Sprites/ExplorerSprite" for ExplorerSprite

// Which clip of the explorer sheet each battle role plays: [role, animation name in ExplorerSprite].
// Roles are ActionAnimations' - a generic one ("idle" "attack" "hurt" "run"), one per move (the move's
// name, lower-cased) or a reaction ("parry" "defeat"). A role left out falls back to its generic one.
var ROLE_CLIPS = [
    ["idle", "Idle"],
    ["run", "Run"],
    ["attack", "Attack"],
    ["hurt", "Hurt"],
    // Speed kit
    ["slash", "Slash2"],         // quick forward cut
    ["flurry", "Slash"],         // spinning slash
    ["dodge", "StandRoll"],      // rolls away (Battler turns it to face the way it retreats)
    // Power kit
    ["cleave", "Slash"],         // spinning heavy swing
    ["crush", "Attack"],         // leaping overhead slam
    ["onslaught", "Flip"],       // flipping charge
    // Control kit - no casting clips yet, so a thrust for the spells and a stand for the heal
    ["bolt", "Slash2"],
    ["slow", "Slash2"],
    ["mend", "Idle"],
    // Reactions
    ["parry", "Slash2"],         // the counter-cut when the player parries
    ["defeat", "Death"]
]

class BattlePlayer {
    static name { "You" }
    static archetypeName { "Speed" }
    static maxHp { 120 }
    static maxStamina { 90 }

    // The overworld sheet, Sprites/explorer.png (generated - see Sprites/ExplorerSprite.wren). Battle plays
    // its right-facing clips; Battler flips them for the other side.
    static texturePath { ExplorerSprite.data["texture"] }
    static cols { ExplorerSprite.data["columns"] }
    static rows { ExplorerSprite.data["rows"] }
    static pivot { Vec2.new(ExplorerSprite.data["origin"][0], ExplorerSprite.data["origin"][1]) }
    static facing { 1 }

    // role -> [startFrame, endFrameExclusive, seconds], read from the generated sheet via ROLE_CLIPS.
    static anims {
        if (__anims == null) {
            var clips = {}
            for (clip in ExplorerSprite.data["animations"]) {
                clips[clip[0]] = [clip[1], clip[2], clip[3]]
            }
            var anims = {}
            for (pair in ROLE_CLIPS) {
                var name = "right%(pair[1])Animation"
                if (!clips.containsKey(name)) {
                    Fiber.abort("BattlePlayer: %(name) is not in ExplorerSprite - rerun build_sprites.bat explorer")
                }
                anims[pair[0]] = clips[name]
            }
            __anims = anims
        }
        return __anims
    }

    // A fresh Combatant - GameObjects/Player.combatant calls this once to mint the persistent one.
    static makeCombatant(entity) {
        return Combatant.new(BattlePlayer.name, BattlePlayer.maxHp, BattlePlayer.maxStamina,
                             Archetype.byName(BattlePlayer.archetypeName).kit, entity)
    }
}
