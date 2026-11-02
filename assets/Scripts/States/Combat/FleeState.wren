// States/Combat/FleeState.wren
// The party bolts: everyone plays the "flee" reaction (a run clip) and slides off-stage (Combat/Battler.runOff), then the
// fight ends via CombatResolver.endFight. Only used with an arena - a fight in place just ends (see
// CombatResolver.flee).
import "app" for Time
import "math" for Math
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
        for (member in s.party) {
            s.stage.react(member, "flee")
        }
        _startTime = Time.scaledTime
    }

    update(stateManager) {
        var s = session
        var elapsed = Time.scaledTime - _startTime
        var t = elapsed / FLEE_TIME
        for (member in s.party) {
            s.stage.runOff(member, Math.min(t, 1))
        }
        if (elapsed >= FLEE_TIME) {
            s.resolver.endFight("You slipped away.", "fled", [])
        }
    }
}
