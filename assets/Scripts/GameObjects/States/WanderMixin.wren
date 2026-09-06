// GameObjects/States/WanderMixin.wren
// Reusable "wander" state: pick a random 8-way direction, move it for a random duration, then hand
// off to another state. Shared by Chicken and Chinlin.
//
// Not tied to Critter specifically - works with any owner exposing this small protocol (see
// GameObjects/Critter.wren for the concrete implementation both critters use):
//   owner.moveDir=(vec2)            - current movement direction (also read back by e.g. ChaseMixin)
//   owner.facing=(vec2)             - current facing (used to pick front/back/side animations)
//   owner.velocity=(vec2)           - velocity this state wants (Critter commits it, plus any
//                                     steering nudges, once per tick - see Critter.tickStates())
//   owner.playAnimation(action)     - play "<action>Front/Back/Side" for the owner's current facing
//   owner.changeState(name)         - force the owner's FSM into another state
import "math" for Vec2
import "app" for Time
import "random" for Random

// 8-way directions, diagonals pre-normalised so speed is constant in every direction.
var DIRECTIONS = [
    Vec2.new(1, 0), Vec2.new(-1, 0), Vec2.new(0, 1), Vec2.new(0, -1),
    Vec2.new(0.7071, 0.7071), Vec2.new(0.7071, -0.7071),
    Vec2.new(-0.7071, 0.7071), Vec2.new(-0.7071, -0.7071)
]

// One generator shared by every WanderMixin instance, across every owner (Chicken's and Chinlin's
// alike). Random.new() seeds from time(NULL), so a per-instance generator would hand identical
// sequences to every critter created in the same second - drawing from one shared stream keeps them
// independent.
var RNG = Random.new()

class WanderMixin {
    // speed - movement speed while wandering.
    // animationAction - passed to owner.playAnimation(action), e.g. "Wander" or "Run".
    // minDuration/maxDuration - seconds to wander for, re-rolled every time this state is entered.
    // nextState - state name to switch to once the duration elapses.
    construct new(speed, animationAction, minDuration, maxDuration, nextState) {
        _speed = speed
        _animationAction = animationAction
        _minDuration = minDuration
        _maxDuration = maxDuration
        _nextState = nextState
        _endTime = 0
    }

    enter(owner) {
        owner.moveDir = RNG.sample(DIRECTIONS)
        owner.facing = owner.moveDir
        owner.velocity = owner.moveDir * _speed
        _endTime = Time.scaledTime + RNG.float(_minDuration, _maxDuration)
        owner.playAnimation(_animationAction)
    }

    // Direction/speed don't change for the rest of this state's duration (set once in enter()), so
    // there's nothing to do here but watch the clock. See IdleMixin's own comment for why this is
    // Time.scaledTime-based, not delta-accumulated.
    update(owner) {
        if (Time.scaledTime >= _endTime) {
            owner.changeState(_nextState)
        }
    }
}
