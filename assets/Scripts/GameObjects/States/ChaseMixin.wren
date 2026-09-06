// GameObjects/States/ChaseMixin.wren
// Reusable "chase" state: run straight at the player. Unlike Idle/Wander, entering/leaving chase
// isn't driven by a timer - checkProximity() is a *global* check the owner calls every fixedUpdate,
// regardless of its current state, to force a transition in once the player gets close and back out
// once they're far enough away again.
//
// Not tied to Critter specifically - works with any owner exposing this small protocol (see
// GameObjects/Critter.wren for the concrete implementation Chinlin uses):
//   owner.position                  - Vec3 world position
//   owner.moveDir=(vec2)            - current movement direction
//   owner.facing=(vec2)             - current facing (used to pick front/back/side animations)
//   owner.velocity=(vec2)           - velocity this state wants (Critter commits it, plus any
//                                     steering nudges, once per tick - see Critter.tickStates())
//   owner.playAnimation(action)     - play "<action>Front/Back/Side" for the owner's current facing
//   owner.changeState(name)         - force the owner's FSM into another state
//   owner.currentStateName          - name of the owner's currently active state
import "gameObject" for GameObject
import "gameObjectComponents" for WorldTransform
import "math" for Vec2, Vec3

class ChaseMixin {
    // speed - movement speed while chasing.
    // animationAction - passed to owner.playAnimation(action), e.g. "Run".
    // chaseRange/loseRange - world-pixel distances; loseRange > chaseRange gives some hysteresis so
    // the chase doesn't flicker on/off right at the boundary.
    // giveUpState - state to fall back to once the player is lost (out of range, or gone entirely).
    construct new(speed, animationAction, chaseRange, loseRange, giveUpState) {
        _speed = speed
        _animationAction = animationAction
        _chaseRange = chaseRange
        _loseRange = loseRange
        _giveUpState = giveUpState
    }

    // Call every fixedUpdate(), regardless of the owner's current state. `stateName` is whatever
    // name this mixin was registered under (owner.addState(stateName, thisMixin)) - needed since the
    // mixin itself doesn't know what it was named.
    checkProximity(owner, stateName) {
        var distance = distanceToPlayer(owner)
        if (distance == null) {
            return
        }
        if (owner.currentStateName != stateName && distance <= _chaseRange) {
            owner.changeState(stateName)
        } else if (owner.currentStateName == stateName && distance > _loseRange) {
            owner.changeState(_giveUpState)
        }
    }

    enter(owner) {
        owner.playAnimation(_animationAction)
    }

    update(owner) {
        var target = playerPosition()
        var here = owner.position
        if (!target || !here) {
            owner.changeState(_giveUpState)
            return
        }
        var toPlayer = Vec2.new(target.x - here.x, target.y - here.y)
        if (toPlayer.length() > 0.001) {
            owner.moveDir = toPlayer.normalize()
            owner.facing = owner.moveDir
        }
        owner.velocity = owner.moveDir * _speed
        owner.playAnimation(_animationAction)
    }

    player_() {
        var players = GameObject.getAllWithIdentifier("Player")
        return players.count > 0 ? players[0] : null
    }

    playerPosition() {
        var p = player_()
        return p ? WorldTransform.getPosition(p) : null
    }

    distanceToPlayer(owner) {
        var target = playerPosition()
        var here = owner.position
        if (!target || !here) {
            return null
        }
        return Vec3.distance(here, target)
    }
}
