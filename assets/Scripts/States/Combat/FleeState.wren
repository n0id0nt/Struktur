// States/Combat/FleeState.wren
// The player bolts: they play the "flee" reaction (a run clip) and slide off-stage (Combat/Battler.runOff), then the
// fight ends via CombatResolver.endFight. Only used with an arena - a fight in place just ends (see
// CombatResolver.flee).
import "app" for Time
import "States/Combat/CombatSubState" for CombatSubState

var FLEE_TIME = 0.7   // game seconds - matches the "flee" reaction's duration in Combat/Config/ActionAnimations.wren

class FleeState is CombatSubState {
    construct new() {
        super()
        name = "FleeState"
        _startTime = 0
    }

    begin(params) {
        var s = session
        s.resumeTime()
        s.view.showMoveMenu(false)
        s.view.setMessage("")
        s.stage.react(s.player, "flee")
        _startTime = Time.scaledTime
    }

    update(stateManager) {
        var s = session
        var elapsed = Time.scaledTime - _startTime
        var t = elapsed / FLEE_TIME
        s.stage.runOff(s.player, t > 1 ? 1 : t)
        if (elapsed >= FLEE_TIME) {
            s.resolver.endFight("You slipped away.")
        }
    }
}
