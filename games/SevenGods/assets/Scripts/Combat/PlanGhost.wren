// Combat/PlanGhost.wren
// A see-through copy of a party member that plays through its planned chain while moves are being picked - the chain
// being built, or one an ally already committed - so the player can see where everyone will be and which cells they
// leave free (Combat/BattlePlan.wren decides which cells a new chain may use). With Soul Sight an opponent gets one
// too, in the soul's blue, that plays only as far into its chain as Soul Sight reaches. Every ghost on the stage runs on one
// shared, looping clock (BattleStage.drawPlans), so allies' plans play side by side, in step.
//
// It replays timed steps ({"move", "start", "end", "at", "to"} - ChainBuilder.steps / BattlePlan.stepsOf): each move's
// clip anchored to how far into the move the clock is, a slide into the new cell at each movement's move time. It runs
// on real time, so it plays while the fight itself is frozen.
import "gameObject" for GameObject
import "gameObjectComponents" for WorldTransform, Sprite, SpriteAnimation, RenderLayer
import "math" for Math, Vec3, Vec4
import "resourceManager" for Texture
import "animation" for SpriteAnimationDefinition
import "renderer" for FlipBit
import "Combat/Config/SheetAnims" for SheetAnims

var GHOST_TINT = Vec4.new(170, 205, 255, 130)
var SOUL_GHOST_TINT = Vec4.new(110, 200, 255, 120)   // an opponent's ghost, seen with Soul Sight
var SLIDE_TIME = 0.19        // how long the slide into a new cell takes (about a battler's own slide)

class PlanGhost {
    // def: the battle def the fighter wears (Battler.def). wearsWeapon: whether it draws the weapon layer too.
    // parent: entity to spawn under. soul: an opponent's ghost, seen with Soul Sight (drawn in the soul's blue).
    construct new(def, wearsWeapon, parent, soul) {
        _def = def
        var tint = soul ? SOUL_GHOST_TINT : GHOST_TINT
        _entity = GameObject.create("PlanGhost_%(def.name)", parent)
        var texture = Texture.load(def.texturePath)
        Sprite.create(_entity, texture, tint, def.pivot, def.cols, def.rows, FlipBit.NONE, 0, RenderLayer.ENTITIES, 0)
        texture.unload()
        _anim = SpriteAnimation.create(_entity)
        _weapon = null
        if (wearsWeapon) {
            _weapon = GameObject.create("PlanGhostWeapon_%(def.name)", _entity)
            var weaponTexture = Texture.load(def.weaponTexturePath)
            Sprite.create(_weapon, weaponTexture, tint, def.pivot, def.cols, def.rows, FlipBit.NONE, 0, RenderLayer.ENTITIES, 1)
            weaponTexture.unload()
        }
        _registered = {}   // clip key -> its length in seconds (addAnimation rejects duplicates)
        _side = null
    }

    // Shows the ghost `clock` seconds into its plan. steps: the timed steps; fighter: whose plan it is (it starts from
    // the fighter's cell); stage: the BattleStage (cell positions, and the engine for facing).
    show(clock, steps, fighter, stage) {
        // Where it stands: the last cell reached by `clock`, sliding in from the one before.
        var start = [fighter.lane, fighter.row]
        var from = start
        var cell = start
        var arrivedAt = null
        for (step in steps) {
            if (step["to"] == null || step["at"] > clock) continue
            from = cell
            cell = step["to"]
            arrivedAt = step["at"]
        }
        var position = stage.cellWorld(cell[0], cell[1])
        if (arrivedAt != null && clock - arrivedAt < SLIDE_TIME) {
            position = Vec3.lerp(stage.cellWorld(from[0], from[1]), position, (clock - arrivedAt) / SLIDE_TIME)
        }
        WorldTransform.setPosition(_entity, position)
        order = cell[0] * 10 + 5
        face_(stage.engine.facingAt(fighter, cell[0], cell[1]))

        // What it is doing: the move under the clock, or idling before / after the chain.
        var current = null
        for (step in steps) {
            if (clock >= step["start"] && clock < step["end"]) current = step
        }
        if (current == null || current["move"].isHold) {
            var idle = clip_("idle", null, true)
            var length = _registered[idle]
            SpriteAnimation.playAnimationAt(_entity, idle, (clock % length) / length)
        } else {
            var move = current["move"]
            var key = clip_(move.anim, move.completion, false)
            SpriteAnimation.playAnimationAt(_entity, key, Math.clamp01((clock - current["start"]) / move.completion))
        }
        syncWeapon_()
    }

    // Draw order: over the battlers in its lane (a planned cell can be where someone stands now), under nearer lanes.
    order=(value) {
        Sprite.setOrderInLayer(_entity, value)
        if (_weapon != null) Sprite.setOrderInLayer(_weapon, value + 1)
    }

    teardown() {
        if (GameObject.isValid(_entity)) GameObject.destroy(_entity)
    }

    // --- like Battler's facing and clips ---------------------------------------------------

    face_(dir) {
        if (dir == _side) return
        _side = dir
        var flip = (_def.animsLeft == null && dir != _def.facing) ? FlipBit.HORIZONTAL : FlipBit.NONE
        Sprite.setFlipped(_entity, flip)
        if (_weapon != null) Sprite.setFlipped(_weapon, flip)
    }

    anims_ { (_side == -1 && _def.animsLeft != null) ? _def.animsLeft : _def.anims }

    // The clip for `role` at `duration` seconds (null = the sheet's own length), registered on first use - a role the
    // sheet lacks falls back to its generic role, then to idle. Returns its key.
    clip_(role, duration, loop) {
        var anims = anims_
        if (!anims.containsKey(role)) {
            var generic = SheetAnims.fallbackRole(role)
            role = (generic != null && anims.containsKey(generic)) ? generic : "idle"
        }
        var frames = anims[role]
        var seconds = duration == null ? frames[2] : duration
        var side = (_side == -1 && _def.animsLeft != null) ? "left" : "right"
        var key = "%(role)|%(seconds)|%(loop)|%(side)"
        if (!_registered.containsKey(key)) {
            _anim.addAnimation(key, SpriteAnimationDefinition.new(frames[0], frames[1], seconds, loop))
            _registered[key] = seconds   // its length, seconds
        }
        return key
    }

    syncWeapon_() {
        if (_weapon == null) return
        var body = Sprite.get(_entity)
        var weaponSprite = Sprite.get(_weapon)
        if (body != null && weaponSprite != null) weaponSprite.index = body.index
    }
}
