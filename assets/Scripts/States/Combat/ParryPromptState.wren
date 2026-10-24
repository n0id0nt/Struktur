// States/Combat/ParryPromptState.wren
// An attack aimed at a party member has reached its parry window (CombatSession.nextParryOffer): game time SLOWS
// (CombatSession.slowTime) so every battler crawls toward the impact while UI/Combat/ParryPromptUI asks
// "Parry?" - naming the attacker and its move, with a timing bar draining toward the moment the attack lands,
// and the attacker's stack flagged in the combat UI (CombatSession.parryAttacker). The window closes when the
// attack lands (at least PARRY_MIN_WINDOW game seconds are always given); a reply of No - or the window
// running out - lets it through. How far through the window Yes was pressed is the parry's timing (0 = at
// once, 1 = at impact), which Combat/Arbiter.parry weighs with Reflex vs Precision to pick the tier.
// Either answer is recorded on the session (CombatSession.answerParry) and
// play returns to ChargingState, where the attack still lands at its natural frame and CombatResolver
// applies the result. Entered with params actor (the attacker).
import "app" for Time
import "Combat/Timeline" for SECONDS_PER_TIME_UNIT
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
        _total = 1
    }

    begin(params) {
        _actor = params["actor"]
        _openedAt = Time.scaledTime
        _answered = false

        var s = session
        // The window is "until the attack lands" (game seconds), but never shorter than PARRY_MIN_WINDOW.
        _total = timeToImpact_() > PARRY_MIN_WINDOW ? timeToImpact_() : PARRY_MIN_WINDOW

        s.slowTime()
        s.parryAttacker = _actor
        s.refreshViews()
        // Names the move - and the defender, when it isn't the party leader the log calls "you".
        var move = s.timeline.committedMove(_actor)
        var target = s.targetOf(_actor)
        var moveText = (target == null || target == s.player) ? move.name : "%(move.name) on %(target.name)"
        s.view.showParryPrompt(_actor.name, moveText, Fn.new { answer_(true) }, Fn.new { answer_(false) })
    }

    // Game seconds until the attacker's move lands (0 once it has).
    timeToImpact_() {
        var s = session
        if (s.timeline.committedMove(_actor) == null) {
            return 0
        }
        var duration = s.timeline.costOf(_actor) * SECONDS_PER_TIME_UNIT
        return (1 - s.timeline.fraction(_actor)) * duration
    }

    update(stateManager) {
        if (_answered) {
            return
        }
        var s = session
        s.advanceCharge()

        var remaining = remaining_()
        s.view.setParryRemaining(remaining / _total)

        if (remaining <= 0) {
            answer_(false)
        }
    }

    // Game seconds left to choose: until impact, or the minimum window if impact has already been reached.
    remaining_() {
        var minLeft = PARRY_MIN_WINDOW - (Time.scaledTime - _openedAt)
        var toImpact = timeToImpact_()
        return toImpact > minLeft ? toImpact : minLeft
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
        s.parryAttacker = null
        var timing = 1 - remaining_() / _total
        s.answerParry(_actor, chosen, timing < 0 ? 0 : (timing > 1 ? 1 : timing))
        s.goTo("ChargingState")
    }

    exit() {
        super.exit()
        _actor = null
    }
}
