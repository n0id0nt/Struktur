// Combat/Battler.wren
// One combatant as it exists in the arena: a spawned entity wearing a def's sprite + animations
// (Combat/BattleCritter.wren for enemies, Combat/BattlePlayer.wren for the player), plus the
// Combatant it fights as. Built and owned by Combat/BattleStage.wren, torn down with the fight.
import "gameObject" for GameObject
import "gameObjectComponents" for WorldTransform, Sprite, SpriteAnimation, RenderLayer
import "math" for Vec3
import "resourceManager" for Texture
import "animation" for SpriteAnimationDefinition
import "renderer" for FlipBit
import "Colors" for WHITE

class Battler {
    // def: a BattleCritter instance or the BattlePlayer class - same getters either way
    //   (name / texturePath / cols / rows / pivot / facing / anims).
    // combatant: the Combatant this battler fights as (the player's is persistent).
    // parent: entity to parent the spawned battler under (the arena anchor).
    construct new(def, combatant, parent) {
        _def = def
        _combatant = combatant
        _entity = GameObject.create("Battler_%(def.name)", parent)

        var tex = Texture.load(def.texturePath)
        Sprite.create(_entity, tex, WHITE, def.pivot, def.cols, def.rows, FlipBit.NONE, 0, RenderLayer.ENTITIES, 0)
        tex.unload()

        var anim = SpriteAnimation.create(_entity)
        for (key in def.anims.keys) {
            var a = def.anims[key]
            anim.addAnimation(key, SpriteAnimationDefinition.new(a[0], a[1], a[2], true))
        }
        play("idle")
    }

    entity { _entity }
    combatant { _combatant }

    place(pos) { WorldTransform.setPosition(_entity, pos) }

    // Slide from fromPos toward toPos, t in 0..1.
    slide(fromPos, toPos, t) {
        WorldTransform.setPosition(_entity, Vec3.new(
            fromPos.x + (toPos.x - fromPos.x) * t,
            fromPos.y + (toPos.y - fromPos.y) * t, 0))
    }

    // dir: 1 to face right, -1 to face left. Flips the sheet when that's opposite the art's facing.
    face(dir) {
        Sprite.setFlipped(_entity, dir == _def.facing ? FlipBit.NONE : FlipBit.HORIZONTAL)
    }

    // Play a named animation ("idle" / "attack" / "hurt"); unknown keys fall back to idle.
    play(key) {
        SpriteAnimation.setCurrentAnimation(_entity, _def.anims.containsKey(key) ? key : "idle")
    }

    teardown() {
        if (GameObject.isValid(_entity)) {
            GameObject.destroy(_entity)
        }
    }
}
