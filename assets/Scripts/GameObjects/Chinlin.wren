// GameObjects/Chinlin.wren
// A wandering critter that chases the player when it gets close. Three-state FSM:
//   wander - pick a random 8-way direction, run it for a random 1-5s, then stand
//   idle   - stand still for a random 1-3s, then wander again
//   chase  - run straight at the player; entered from any state when the player comes within
//            CHASE_RANGE, left back to wander once the player is further than LOSE_RANGE
//
// Sprite sheet: Sprites/chinlin.png - 96x96, 4x4 grid, 24x24 cells, pivot (12,16).
//   frames  anim
//   0-1     stand_down    2-3   stand_up    4-5   stand_right
//   6-8     run_down      9-11  run_up      12-14 run_right
// No "left" strip - left-facing reuses the right strip mirrored (Sprite.setFlipped).

import "gameObject" for GameObject
import "gameObjectComponents" for Sprite, PhysicsBody, SpriteAnimation, RenderLayer, WorldTransform
import "math" for Vec2, Vec3
import "resourceManager" for Texture
import "animation" for SpriteAnimationDefinition
import "physics" for BodyDefinition, PhysicsCircleShape, BodyType, CollisionLayers
import "app" for Time
import "random" for Random
import "Colors" for WHITE
import "GameObjects/StateMachine" for StateMachine

var DIRECTIONS = [
    Vec2.new(1, 0), Vec2.new(-1, 0), Vec2.new(0, 1), Vec2.new(0, -1),
    Vec2.new(0.7071, 0.7071), Vec2.new(0.7071, -0.7071),
    Vec2.new(-0.7071, 0.7071), Vec2.new(-0.7071, -0.7071)
]

// World-space pixel distances (physics is 64 px/m, so ~1.4m / ~2.5m). LOSE_RANGE > CHASE_RANGE
// gives the chase some hysteresis so it doesn't flicker on/off at the boundary.
var CHASE_RANGE = 90
var LOSE_RANGE = 160

// One generator shared by every chinlin - see the note in Chicken.wren for why it isn't per-instance.
var RNG = Random.new()

class Chinlin {
    construct new(entity, args) {
        _entity = entity
        _name = args["Name"]
        _fsm = StateMachine.new()

        _wanderSpeed = 1.1
        _chaseSpeed = 1.9      // a hair under Player._speed (2), so a running player can break away
        _facing = Vec2.new(0, 1)
        _moveDir = Vec2.new(0, 0)
        _phaseEndTime = 0
    }

    name { _name }

    start() {
        var texture = Texture.load("Sprites/chinlin.png")
        Sprite.create(_entity, texture, WHITE, Vec2.new(12, 16), 4, 4, false, 0, RenderLayer.ENTITIES, 0)
        texture.unload()

        var body = PhysicsBody.create(_entity, BodyDefinition.new(BodyType.DYNAMIC_BODY),
                                      PhysicsCircleShape.new(0.18))
        body.fixedRotation = true
        // Collide with the level (Wall bit), pass through the player and other critters (see Main.start()).
        body.setCollisionFilter(CollisionLayers.getLayer("Actor"), CollisionLayers.getLayer("Wall"))

        var anim = SpriteAnimation.create(_entity)
        anim.addAnimation("StandFront", SpriteAnimationDefinition.new(0, 2, 0.8, true))
        anim.addAnimation("StandBack", SpriteAnimationDefinition.new(2, 4, 0.8, true))
        anim.addAnimation("StandSide", SpriteAnimationDefinition.new(4, 6, 0.8, true))
        anim.addAnimation("RunFront", SpriteAnimationDefinition.new(6, 9, 0.45, true))
        anim.addAnimation("RunBack", SpriteAnimationDefinition.new(9, 12, 0.45, true))
        anim.addAnimation("RunSide", SpriteAnimationDefinition.new(12, 15, 0.45, true))

        _fsm.define("wander", Fn.new { enterWander() }, Fn.new { updateWander() }, null)
        _fsm.define("idle", Fn.new { enterIdle() }, Fn.new { updateIdle() }, null)
        _fsm.define("chase", Fn.new { playAnimation("Run") }, Fn.new { updateChase() }, null)
        _fsm.change("wander")
    }

    update() {}

    fixedUpdate() {
        // Player proximity overrides whatever the FSM is doing (enter chase) or ends a chase
        // (drop back to wander). Checked before the per-state update each tick.
        var distance = playerDistance()
        if (distance != null) {
            if (_fsm.name != "chase" && distance <= CHASE_RANGE) {
                _fsm.change("chase")
            } else if (_fsm.name == "chase" && distance > LOSE_RANGE) {
                _fsm.change("wander")
            }
        }
        _fsm.update()
    }

    onDestroy() {}
    onEvent(type, data) {}

    // --- wander state ---------------------------------------------------------
    enterWander() {
        _moveDir = RNG.sample(DIRECTIONS)
        _facing = _moveDir
        _phaseEndTime = Time.scaledTime + RNG.float(1.0, 5.0)
        playAnimation("Run")
    }

    updateWander() {
        PhysicsBody.setLinearVelocity(_entity, _moveDir * _wanderSpeed)
        if (Time.scaledTime >= _phaseEndTime) {
            _fsm.change("idle")
        }
    }

    // --- idle state ----------------------------------------------------------
    enterIdle() {
        PhysicsBody.setLinearVelocity(_entity, Vec2.new(0, 0))
        _phaseEndTime = Time.scaledTime + RNG.float(1.0, 3.0)
        playAnimation("Stand")
    }

    updateIdle() {
        if (Time.scaledTime >= _phaseEndTime) {
            _fsm.change("wander")
        }
    }

    // --- chase state --------------------------------------------------------
    updateChase() {
        var target = playerPosition()
        if (!target) {
            _fsm.change("wander")
            return
        }
        var here = WorldTransform.getPosition(_entity)
        var toPlayer = Vec2.new(target.x - here.x, target.y - here.y)
        if (toPlayer.length() > 0.001) {
            _moveDir = toPlayer.normalize()
            _facing = _moveDir
        }
        PhysicsBody.setLinearVelocity(_entity, _moveDir * _chaseSpeed)
        playAnimation("Run")
    }

    // --- helpers -----------------------------------------------------------
    player() {
        var players = GameObject.getAllWithIdentifier("Player")
        return players.count > 0 ? players[0] : null
    }

    playerPosition() {
        var p = player()
        return p ? WorldTransform.getPosition(p) : null
    }

    playerDistance() {
        var target = playerPosition()
        if (!target) {
            return null
        }
        var here = WorldTransform.getPosition(_entity)
        if (!here) {
            return null
        }
        return Vec3.distance(here, target)
    }

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
