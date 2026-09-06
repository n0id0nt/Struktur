// GameObjects/States/IdleMixin.wren
// Reusable "idle" state: stop moving, play an animation, and hold for a random duration before
// handing off to another state. Shared by Chicken (-> "Peck") and Chinlin (-> "Stand").
//
// Not tied to Critter specifically - works with any owner exposing this small protocol (see
// GameObjects/Critter.wren for the concrete implementation both critters use):
//   owner.velocity=(vec2)           - velocity this state wants (Critter commits it, plus any
//                                     steering nudges, once per tick - see Critter.tickStates())
//   owner.playAnimation(action)     - play "<action>Front/Back/Side" for the owner's current facing
//   owner.changeState(name)         - force the owner's FSM into another state
import "math" for Vec2
import "app" for Time
import "random" for Random

// Shared by every IdleMixin instance across every owner - see WanderMixin.wren's own comment for
// why a per-instance generator would be the wrong call here.
var RNG = Random.new()

class IdleMixin {
    // animationAction - passed to owner.playAnimation(action), e.g. "Peck" or "Stand".
    // minDuration/maxDuration - seconds to hold idle for, re-rolled every time this state is entered.
    // nextState - state name to switch to once the duration elapses.
    construct new(animationAction, minDuration, maxDuration, nextState) {
        _animationAction = animationAction
        _minDuration = minDuration
        _maxDuration = maxDuration
        _nextState = nextState
        _endTime = 0
    }

    enter(owner) {
        owner.velocity = Vec2.new(0, 0)
        _endTime = Time.scaledTime + RNG.float(_minDuration, _maxDuration)
        owner.playAnimation(_animationAction)
    }

    // Timing is driven off Time.scaledTime rather than an accumulated delta: fixedUpdate() can run
    // more than once per rendered frame, and Time.scaledDelta during that loop is still the *frame*
    // delta, so accumulating it here would drift with framerate instead of tracking real time.
    update(owner) {
        if (Time.scaledTime >= _endTime) {
            owner.changeState(_nextState)
        }
    }
}
