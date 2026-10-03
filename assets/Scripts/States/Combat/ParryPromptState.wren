// States/Combat/ParryPromptState.wren
// An attack aimed at the player has reached its parry window (CombatSession.nextParryOffer): game time SLOWS
// (CombatSession.slowTime) so every battler crawls toward the impact while UI/Combat/ParryPromptUI asks
// "Parry?". The window closes when the attack lands (at least PARRY_MIN_WINDOW game seconds are always given); a
// reply of No - or the window running out - lets it through. Either answer is recorded on the session
// (CombatSession.answerParry) and play returns to ChargingState, where the attack still lands at its natural
// frame and CombatResolver applies the result. Entered with params actor (the attacker).
import "app" for Time
import "States/Combat/CombatSubState" for CombatSubState

// Game seconds the player always gets to answer, even if the attack was already at impact when the prompt opened.
var PARRY_MIN_WINDOW = 0.3

class ParryPromptState is CombatSubState {
    construct new() {
        super()
        name = "ParryPromptState"
        _actor = null
        _openedAt = 0
        _answered = false
    }

    begin(params) {
        _actor = params["actor"]
        _openedAt = Time.scaledTime
        _answered = false

        var s = session
        s.slowTime()
        s.view.showParryPrompt(Fn.new { answer_(true) }, Fn.new { answer_(false) })
    }

    update(stateManager) {
        if (_answered) {
            return
        }
        var s = session
        s.advanceCharge()
        var landed = s.timeline.committedMove(_actor) == null || s.timeline.fraction(_actor) >= 1
        if (landed && Time.scaledTime - _openedAt >= PARRY_MIN_WINDOW) {
            answer_(false)
        }
    }

    // The prompt's buttons and the window closing all funnel through here; _answered guards against a click
    // and the timeout landing the same frame.
    answer_(chosen) {
        if (_answered) {
            return
        }
        _answered = true
        var s = session
        s.view.hideParryPrompt()
        s.answerParry(_actor, chosen)
        s.goTo("ChargingState")
    }

    exit() {
        super.exit()
        _actor = null
    }
}
