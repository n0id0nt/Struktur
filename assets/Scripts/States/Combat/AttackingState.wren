// States/Combat/AttackingState.wren
// The forward half of a strike: the attacker lunges toward the target along the move's curve and
// the impact (hurt clip + particle burst) fires part-way through - all driven by Combat/
// BattleStage.beginStrike/updateStrike. Entered with params actor / target / move / text (the log
// line to show afterwards); hands off to ReturningState.
import "app" for Time
import "States/Combat/CombatSubState" for CombatSubState

var STRIKE_TIME = 0.35   // curve-driven lunge toward the target (BattleStage.updateStrike)

class AttackingState is CombatSubState {
    construct new() {
        super()
        name = "AttackingState"
        _text = null
        _startTime = 0
    }

    begin(params) {
        _text = params["text"]
        _startTime = Time.unscaledTime
        session.stage.beginStrike(params["actor"], params["target"], params["move"])
    }

    update(stateManager) {
        var s = session
        var elapsed = Time.unscaledTime - _startTime
        var t = elapsed / STRIKE_TIME
        if (t > 1) {
            t = 1
        }
        s.stage.updateStrike(t)
        if (elapsed >= STRIKE_TIME) {
            s.goTo("ReturningState", {"text": _text})
        }
    }
}
