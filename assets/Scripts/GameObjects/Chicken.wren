// GameObjects/Chicken.wren
// A wandering chicken. Two-state FSM:
//   wander - pick a random 8-way direction, walk it for a random 1-5s, then peck
//   idle   - stand still and peck for a random 1-3s, then wander again
//
// Sprite sheet: Sprites/chicken.png - 112x96, 7x6 grid, 16x16 cells, pivot (8,8).
//   frames  anim
//   0-2     wander_front   3-5   wander_back    6-8   wander_right
//   9-11    run_front      12-14 run_back       15-17 run_right
//   18-24   peck_front     25-31 peck_back      32-38 peck_right
// There is no "left" strip - left-facing reuses the right strip mirrored (Sprite.setFlipped).

import "gameObjectComponents" for Sprite, PhysicsBody, SpriteAnimation, RenderLayer
import "math" for Vec2
import "resourceManager" for Texture
import "animation" for SpriteAnimationDefinition
import "physics" for BodyDefinition, PhysicsCircleShape, BodyType, CollisionLayers
import "app" for Time
import "random" for Random
import "Colors" for WHITE
import "GameObjects/StateMachine" for StateMachine

// 8-way wander directions (diagonals pre-normalised so speed is constant in every direction).
var DIRECTIONS = [
    Vec2.new(1, 0), Vec2.new(-1, 0), Vec2.new(0, 1), Vec2.new(0, -1),
    Vec2.new(0.7071, 0.7071), Vec2.new(0.7071, -0.7071),
    Vec2.new(-0.7071, 0.7071), Vec2.new(-0.7071, -0.7071)
]

// One generator shared by every chicken. Random.new() seeds from time(NULL), so a per-instance
// generator would hand identical sequences to every chicken spawned in the same second; drawing
// from one shared stream keeps them independent.
var RNG = Random.new()

class Chicken {
    construct new(entity, args) {
        _entity = entity
        _name = args["Name"]
        _fsm = StateMachine.new()

        _speed = 0.8                       // metres/sec (physics units, like Player._speed)
        _facing = Vec2.new(0, 1)           // start facing the camera ("front")
        _moveDir = Vec2.new(0, 0)
        _phaseEndTime = 0                  // Time.scaledTime at which the current state should end
    }

    name { _name }

    // Called by C++ once the base components exist - configure them here.
    start() {
        var texture = Texture.load("Sprites/chicken.png")
        Sprite.create(_entity, texture, WHITE, Vec2.new(8, 8), 7, 6, false, 0, RenderLayer.ENTITIES, 0)
        texture.unload()

        var body = PhysicsBody.create(_entity, BodyDefinition.new(BodyType.DYNAMIC_BODY),
                                      PhysicsCircleShape.new(0.12))
        body.fixedRotation = true
        // Collide with the level (Wall bit), pass through the player and other critters (see Main.start()).
        body.setCollisionFilter(CollisionLayers.getLayer("Actor"), CollisionLayers.getLayer("Wall"))

        var anim = SpriteAnimation.create(_entity)
        // endFrame is exclusive - SpriteAnimationDefinition.new(start, end, seconds, loop).
        anim.addAnimation("WanderFront", SpriteAnimationDefinition.new(0, 3, 0.5, true))
        anim.addAnimation("WanderBack", SpriteAnimationDefinition.new(3, 6, 0.5, true))
        anim.addAnimation("WanderSide", SpriteAnimationDefinition.new(6, 9, 0.5, true))
        anim.addAnimation("RunFront", SpriteAnimationDefinition.new(9, 12, 0.35, true))
        anim.addAnimation("RunBack", SpriteAnimationDefinition.new(12, 15, 0.35, true))
        anim.addAnimation("RunSide", SpriteAnimationDefinition.new(15, 18, 0.35, true))
        anim.addAnimation("PeckFront", SpriteAnimationDefinition.new(18, 25, 1.0, true))
        anim.addAnimation("PeckBack", SpriteAnimationDefinition.new(25, 32, 1.0, true))
        anim.addAnimation("PeckSide", SpriteAnimationDefinition.new(32, 39, 1.0, true))

        _fsm.define("wander", Fn.new { enterWander() }, Fn.new { updateWander() }, null)
        _fsm.define("idle", Fn.new { enterIdle() }, Fn.new { updateIdle() }, null)
        _fsm.change("wander")
    }

    update() {}

    fixedUpdate() {
        _fsm.update()
    }

    onDestroy() {}
    onEvent(type, data) {}

    // --- wander state ---------------------------------------------------------
    enterWander() {
        _moveDir = RNG.sample(DIRECTIONS)
        _facing = _moveDir
        _phaseEndTime = Time.scaledTime + RNG.float(1.0, 5.0)
        playAnimation("Wander")
    }

    updateWander() {
        PhysicsBody.setLinearVelocity(_entity, _moveDir * _speed)
        if (Time.scaledTime >= _phaseEndTime) {
            _fsm.change("idle")
        }
    }

    // --- idle state ----------------------------------------------------------
    enterIdle() {
        PhysicsBody.setLinearVelocity(_entity, Vec2.new(0, 0))
        _phaseEndTime = Time.scaledTime + RNG.float(1.0, 3.0)
        playAnimation("Peck")
    }

    updateIdle() {
        if (Time.scaledTime >= _phaseEndTime) {
            _fsm.change("wander")
        }
    }

    // --- helpers -----------------------------------------------------------
    // Picks "<action>Front/Back/Side" from the current facing and mirrors the side strip when
    // facing left. Cheap to call every tick - setCurrentAnimation continues an already-playing clip.
    playAnimation(action) {
        var sideways = _facing.x.abs > _facing.y.abs
        var suffix = "Front"
        if (sideways) {
            suffix = "Side"
            Sprite.setFlipped(_entity, _facing.x < 0)
        } else if (_facing.y < 0) {
            suffix = "Back"
        }
        SpriteAnimation.setCurrentAnimation(_entity, "%(action)%(suffix)")
    }
}
