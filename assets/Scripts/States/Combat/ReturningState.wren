// States/Combat/ReturningState.wren
// After a move lands, the attacker eases back from where its lunge/retreat left it to its mark (Combat/
// Battler.recover) while the hit's reactions play out. Entered with params actor and text (the log line to show
// next); a combatant that never left its mark passes straight through. Hands off to MessageState.
import "app" for Time
import "Combat/Config/ActionAnimations" for RECOVER_TIME
import "States/Combat/CombatSubState" for CombatSubState

class ReturningState is CombatSubState {
    construct new() {
        super()
        name = "ReturningState"
        _actor = null
        _text = null
        _startTime = 0
        _duration = 0
    }

    begin(params) {
        var s = session
        s.resumeTime()
        _actor = params["actor"]
        _text = params["text"]
        _startTime = Time.scaledTime
        _duration = 0
        if (s.stage != null && s.stage.needsRecover(_actor)) {
            _duration = RECOVER_TIME
            s.stage.beginRecover(_actor)
        }
    }

    update(stateManager) {
        var s = session
        var elapsed = Time.scaledTime - _startTime
        if (_duration > 0) {
            var t = elapsed / _duration
            s.stage.recover(_actor, t > 1 ? 1 : t)
        }
        if (elapsed >= _duration) {
            s.goTo("MessageState", {"text": _text})
        }
    }

    exit() {
        super.exit()
        _actor = null
    }
}
