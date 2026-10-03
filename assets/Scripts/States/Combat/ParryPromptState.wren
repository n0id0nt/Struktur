// States/Combat/ParryPromptState.wren
// A parryable move is about to land on the player: the attacker winds up (BattleStage.beginTelegraph)
// while UI/Combat/ParryPromptUI asks "Parry?". Either button - or the move's parryPromptTime running
// out, which counts as "No" - hands the answer to CombatResolver.finishResolve, which applies the
// result and leads on to AttackingState / MessageState. Entered with params actor / target / move.
import "app" for Time
import "States/Combat/CombatSubState" for CombatSubState

class ParryPromptState is CombatSubState {
    construct new() {
        super()
        name = "ParryPromptState"
        _actor = null
        _target = null
        _move = null
        _startTime = 0
        _answered = false
    }

    begin(params) {
        _actor = params["actor"]
        _target = params["target"]
        _move = params["move"]
        _startTime = Time.unscaledTime
        _answered = false

        var s = session
        if (s.stage != null) {
            s.stage.beginTelegraph(_actor)
        }
        s.view.showParryPrompt(Fn.new { answer_(true) }, Fn.new { answer_(false) })
    }

    update(stateManager) {
        if (!_answered && Time.unscaledTime - _startTime >= _move.parryPromptTime) {
            answer_(false)
        }
    }

    // The prompt's buttons and the timeout all funnel through here; _answered guards against a click
    // and the timeout landing the same frame.
    answer_(chosen) {
        if (_answered) {
            return
        }
        _answered = true
        var s = session
        s.view.hideParryPrompt()
        s.resolver.finishResolve(_actor, _move, _target, false, chosen)
    }

    exit() {
        super.exit()
        _actor = null
        _target = null
        _move = null
    }
}
