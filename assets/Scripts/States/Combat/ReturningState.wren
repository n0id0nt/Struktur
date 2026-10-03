// States/Combat/ReturningState.wren
// The back half of a strike: the attacker eases back from the target to its home mark
// (Combat/BattleStage.updateReturn). Entered with params text (the log line to show next); hands off
// to MessageState.
import "app" for Time
import "States/Combat/CombatSubState" for CombatSubState

var RETURN_TIME = 0.25   // ease back to home afterwards (BattleStage.updateReturn)

class ReturningState is CombatSubState {
    construct new() {
        super()
        name = "ReturningState"
        _text = null
        _startTime = 0
    }

    begin(params) {
        _text = params["text"]
        _startTime = Time.unscaledTime
    }

    update(stateManager) {
        var s = session
        var elapsed = Time.unscaledTime - _startTime
        var t = elapsed / RETURN_TIME
        if (t > 1) {
            t = 1
        }
        s.stage.updateReturn(t)
        if (elapsed >= RETURN_TIME) {
            s.goTo("MessageState", {"text": _text})
        }
    }
}
