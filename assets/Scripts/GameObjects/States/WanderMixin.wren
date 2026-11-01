// GameObjects/States/WanderMixin.wren
// Reusable "wander" state: walk to a random spot inside the owner's home room, then hand off to another state.
// Shared by Chicken and Chinlin. Two flavours:
//   new(...)      - ordinary wandering: a nearby spot, about as far as it can walk in a random duration
//   anywhere(...) - a spot anywhere in the room (Chinlin heads off like this after giving up a chase)
// Either way the spot is inside the room, so the walk there never leaves it. The state ends on arriving, or when
// its time runs out (a wall in the way, say). An owner with no home room falls back to walking a random 8-way
// direction for the duration.
//
// Not tied to Critter specifically - works with any owner exposing this small protocol (see
// GameObjects/Critter.wren for the concrete implementation both critters use):
//   owner.position                  - Vec3 world position
//   owner.randomPointInHome(maxDistance) - a Vec2 spot inside its home room (within maxDistance px of it,
//                                     or anywhere in the room for null), or null if it has no home room
//   owner.moveDir=(vec2)            - current movement direction (also read back by e.g. ChaseMixin)
//   owner.facing=(vec2)             - current facing (used to pick front/back/side animations)
//   owner.velocity=(vec2)           - velocity this state wants (Critter commits it, plus any
//                                     steering nudges, once per tick - see Critter.tickStates())
//   owner.playAnimation(action)     - play "<action>Front/Back/Side" for the owner's current facing
//   owner.changeState(name)         - force the owner's FSM into another state
import "math" for Vec2
import "app" for Time, Application
import "random" for Random

// 8-way directions, diagonals pre-normalised so speed is constant in every direction.
var DIRECTIONS = [
    Vec2.new(1, 0), Vec2.new(-1, 0), Vec2.new(0, 1), Vec2.new(0, -1),
    Vec2.new(0.7071, 0.7071), Vec2.new(0.7071, -0.7071),
    Vec2.new(-0.7071, 0.7071), Vec2.new(-0.7071, -0.7071)
]

// px from the spot that counts as arrived.
var ARRIVE_RADIUS = 4

// One generator shared by every WanderMixin instance, across every owner (Chicken's and Chinlin's
// alike). Random.new() seeds from time(NULL), so a per-instance generator would hand identical
// sequences to every critter created in the same second - drawing from one shared stream keeps them
// independent.
var RNG = Random.new()

class WanderMixin {
    // speed - movement speed while wandering.
    // animationAction - passed to owner.playAnimation(action), e.g. "Wander" or "Run".
    // minDuration/maxDuration - seconds to wander for, re-rolled every time this state is entered; the spot
    // picked is about as far as that walks.
    // nextState - state name to switch to on arriving, or once the duration elapses.
    construct new(speed, animationAction, minDuration, maxDuration, nextState) {
        init_(speed, animationAction, minDuration, maxDuration, nextState, false)
    }

    // Walks to a spot anywhere in the home room. maxDuration caps how long it tries for.
    construct anywhere(speed, animationAction, maxDuration, nextState) {
        init_(speed, animationAction, maxDuration, maxDuration, nextState, true)
    }

    init_(speed, animationAction, minDuration, maxDuration, nextState, anywhere) {
        _speed = speed
        _animationAction = animationAction
        _minDuration = minDuration
        _maxDuration = maxDuration
        _nextState = nextState
        _anywhere = anywhere
        _endTime = 0
        _target = null
    }

    enter(owner) {
        var duration = RNG.float(_minDuration, _maxDuration)
        _endTime = Time.scaledTime + duration
        _target = owner.randomPointInHome(_anywhere ? null : _speed * duration * Application.pixelsPerMeter)
        if (_target == null) {
            owner.moveDir = RNG.sample(DIRECTIONS)
            owner.facing = owner.moveDir
            owner.velocity = owner.moveDir * _speed
        }
        owner.playAnimation(_animationAction)
    }

    // Steers at the spot every tick (separation pushes it off line). See IdleMixin's own comment for why the
    // timer is Time.scaledTime-based, not delta-accumulated.
    update(owner) {
        if (Time.scaledTime >= _endTime) {
            owner.changeState(_nextState)
            return
        }
        if (_target == null) {
            return
        }
        var here = owner.position
        if (here == null) {
            return
        }
        var toTarget = Vec2.new(_target.x - here.x, _target.y - here.y)
        if (toTarget.length() <= ARRIVE_RADIUS) {
            owner.velocity = Vec2.new(0, 0)
            owner.changeState(_nextState)
            return
        }
        owner.moveDir = toTarget.normalize()
        owner.facing = owner.moveDir
        owner.velocity = owner.moveDir * _speed
        owner.playAnimation(_animationAction)
    }
}
