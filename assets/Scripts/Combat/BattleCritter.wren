// Combat/BattleCritter.wren
// The battle-side definition of one enemy type - the enemy counterpart to Combat/BattlePlayer.wren.
// Everything a critter is in a fight lives here in one place: its fighting style (which supplies
// the move set - see Combat/Archetype.wren), its HP / stamina pool, and how it looks and animates
// in the arena. Combat/Battler.wren turns one of these into a live entity + Combatant; CombatState
// looks a def up by species name when an encounter starts.
//
// "images and animations" reuse the overworld critter sheets for now - the frame ranges below
// mirror GameObjects/Chicken.wren / Chinlin.wren. Swap in battle-specific art (poses, hit/defeat
// frames) here and nothing else changes. `anims` keys Battler understands: "idle" (required),
// "attack", "hurt".
import "math" for Vec2
import "Combat/Combatant" for Combatant
import "Combat/Archetype" for Archetype

class BattleCritter {
    // anims: { key: [startFrame, endFrameExclusive, seconds] }. facing: 1 = art faces right, -1 = left.
    construct new(name, archetypeName, maxHp, maxStamina, texturePath, cols, rows, pivot, facing, anims) {
        _name = name
        _archetypeName = archetypeName
        _maxHp = maxHp
        _maxStamina = maxStamina
        _texturePath = texturePath
        _cols = cols
        _rows = rows
        _pivot = pivot
        _facing = facing
        _anims = anims
    }

    name { _name }
    archetypeName { _archetypeName }
    maxHp { _maxHp }
    maxStamina { _maxStamina }
    texturePath { _texturePath }
    cols { _cols }
    rows { _rows }
    pivot { _pivot }
    facing { _facing }
    anims { _anims }

    // A fresh Combatant (stats + move kit) for one instance of this critter. `entity` is the
    // overworld critter it came from (kept so a defeated one can be removed from the field).
    makeCombatant(entity) {
        return Combatant.new(_name, _maxHp, _maxStamina, Archetype.byName(_archetypeName).kit, entity)
    }

    static forName(name) {
        if (name == "Chinlin") {
            return BattleCritter.chinlin
        }
        return BattleCritter.chicken
    }

    // Sprites/chicken.png - 7x6 grid, 16px cells, pivot (8,8). See GameObjects/Chicken.wren.
    // anims: idle = wander_right, attack = peck_right, hurt = run_right (stand-in).
    static chicken {
        return BattleCritter.new("Chicken", "Speed", 45, 60, "Sprites/chicken.png", 7, 6, Vec2.new(8, 8), 1,
                                 {"idle": [6, 9, 0.5], "attack": [32, 39, 1.0], "hurt": [15, 18, 0.35]})
    }

    // Sprites/chinlin.png - 4x4 grid, 24px cells, pivot (12,16). See GameObjects/Chinlin.wren.
    // anims: idle = stand_right, attack / hurt = run_right (stand-in).
    static chinlin {
        return BattleCritter.new("Chinlin", "Power", 150, 55, "Sprites/chinlin.png", 4, 4, Vec2.new(12, 16), 1,
                                 {"idle": [4, 6, 0.8], "attack": [12, 15, 0.45], "hurt": [12, 15, 0.45]})
    }
}
