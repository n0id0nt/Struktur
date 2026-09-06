// GameObjects/Chinlin.wren
// A wandering critter that chases the player when it gets close, built from Critter + the shared
// state mixins in GameObjects/States/. Three states: wander <-> idle, plus chase - entered/left by
// proximity rather than a timer (see ChaseMixin.checkProximity).
//
// Sprite sheet: Sprites/chinlin.png - 96x96, 4x4 grid, 24x24 cells, pivot (12,16).
//   frames  anim
//   0-1     stand_down    2-3   stand_up    4-5   stand_right
//   6-8     run_down      9-11  run_up      12-14 run_right
// No "left" strip - left-facing reuses the right strip mirrored (see Critter.playAnimation).

import "gameObjectComponents" for Sprite, PhysicsBody, SpriteAnimation, RenderLayer
import "renderer" for FlipBit
import "math" for Vec2
import "resourceManager" for Texture
import "animation" for SpriteAnimationDefinition
import "physics" for BodyDefinition, PhysicsCircleShape, BodyType, CollisionLayers
import "Colors" for WHITE
import "GameObjects/Critter" for Critter
import "GameObjects/States/IdleMixin" for IdleMixin
import "GameObjects/States/WanderMixin" for WanderMixin
import "GameObjects/States/ChaseMixin" for ChaseMixin
import "GameObjects/Steering/Separation" for Separation

class Chinlin is Critter {
    construct new(entity, args) {
        super(entity, args)
    }

    // Aggressive: catching the player forces them into combat rather than offering it (see
    // ExperimentState.checkCombatTriggers()) - the chase state above is what actually closes the
    // distance to make that happen.
    aggressive { true }

    // Tougher than a chicken - a real (still winnable) fight. Slower attack (5u charge) than a
    // chicken's (3u), so it acts less often but hits harder.
    combatMaxHp { 20 }
    combatAttack { 5 }
    combatMoveCost { 5 }

    start() {
        var texture = Texture.load("Sprites/chinlin.png")
        Sprite.create(entity, texture, WHITE, Vec2.new(12, 16), 4, 4, FlipBit.NONE, 0, RenderLayer.ENTITIES, 0)
        texture.unload()

        var body = PhysicsBody.create(entity, BodyDefinition.new(BodyType.DYNAMIC_BODY),
                                      PhysicsCircleShape.new(0.18))
        body.fixedRotation = true
        // Collide with the level (Wall bit), pass through the player and other critters (see Main.start()).
        body.setCollisionFilter(CollisionLayers.getLayer("Actor"), CollisionLayers.getLayer("Wall"))

        var anim = SpriteAnimation.create(entity)
        anim.addAnimation("StandFront", SpriteAnimationDefinition.new(0, 2, 0.8, true))
        anim.addAnimation("StandBack", SpriteAnimationDefinition.new(2, 4, 0.8, true))
        anim.addAnimation("StandSide", SpriteAnimationDefinition.new(4, 6, 0.8, true))
        anim.addAnimation("RunFront", SpriteAnimationDefinition.new(6, 9, 0.45, true))
        anim.addAnimation("RunBack", SpriteAnimationDefinition.new(9, 12, 0.45, true))
        anim.addAnimation("RunSide", SpriteAnimationDefinition.new(12, 15, 0.45, true))

        // Kept so fixedUpdate() can run its proximity check every tick - see ChaseMixin's own comment
        // on why entering/leaving chase can't just be another timed state like wander/idle. Speed
        // 1.9 is a hair under Player._speed (2), so a running player can break away.
        _chase = ChaseMixin.new(1.9, "Run", 90, 160, "wander")

        addState("wander", WanderMixin.new(1.1, "Run", 1.0, 5.0, "idle"))
        addState("idle", IdleMixin.new("Stand", 1.0, 3.0, "wander"))
        addState("chase", _chase)
        // Boid-style separation so chinlins (and chickens - same registry, see Critter.all) visually
        // avoid overlapping - see Separation's own comment for why this is a steering nudge rather
        // than turning physical collision back on.
        addSteering(Separation.new(28, 1.6))
        changeState("wander")
    }

    fixedUpdate() {
        _chase.checkProximity(this, "chase")
        tickStates()
    }
}
