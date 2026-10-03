// States/Combat/CombatResolver.wren
// The rules for what happens at the moment a combatant's charge (and so its attack animation) finishes:
// apply the move using the parry decision already made during the wind-up (CombatSession.nextParryOffer /
// ParryPromptState), including damage / stagger / parry-cancel / stall, play the impact reactions, then hand
// off to the recovery animation and message (ReturningState -> MessageState) and finally decide what comes
// after the log (win / lose / back to the move menu / back to charging). Owned by CombatSession; the phases
// themselves only call into it from their own update().
import "gameObject" for GameObject
import "Combat/Config/Moves" for Moves
import "Combat/Disruption" for Disruption

class CombatResolver {
    construct new(session) {
        _session = session
    }

    // The attack on screen has landed: look up the move's target and whether that target parried (the
    // decision was made - by the player's prompt or an AI coin flip - when the attack's parry window opened),
    // then resolve it.
    resolveMove(actor) {
        var s = _session
        var move = s.timeline.committedMove(actor)
        var isPlayer = actor == s.player
        var target = s.targetOf(actor)
        var parried = move.offensive && move.parryable && target != null && target.alive && s.parryDecision(actor)
        finishResolve(actor, move, target, isPlayer, parried)
    }

    // The actual resolution (damage, stagger/parry-cancel, impact reactions, log). parryChosen: true
    // cancels all of the move's damage (Combat/Combatant.wren) and costs the attacker a
    // Combat/Config/Moves.stalled beat; the defender's own in-flight move is also cancelled regardless of
    // side, since choosing to parry is a deliberate reaction to make. Ends by handing off to the next phase.
    finishResolve(actor, move, target, isPlayer, parryChosen) {
        var s = _session
        var cancelFraction = parryChosen ? 1 : 0

        s.clearMove(actor)

        var dealt = 0
        var staggerUnits = 0
        var defenderMoveCancelled = false
        if (target != null) {
            dealt = actor.use(move, target, cancelFraction)
            if (move.offensive && s.timeline.isCharging(target)) {
                if (parryChosen) {
                    // Choosing to parry costs the defender their own in-flight move; they have to
                    // pick again instead of the usual partial stagger.
                    s.clearMove(target)
                    defenderMoveCancelled = true
                } else {
                    staggerUnits = Disruption.delay(move, s.timeline.fraction(target))
                    s.interruptMove(target, staggerUnits)
                    s.markStaggered(target)
                }
            }
            if (parryChosen) {
                // A landed parry also punishes the attacker: instead of letting them pick a real move
                // again immediately, they sit out Combat/Config/Moves.stalled first.
                s.commitMove(actor, Moves.stalled)
            }
            if (s.stage != null) {
                s.stage.playImpact(actor, target, move, parryChosen)
            }
        } else {
            actor.use(move, actor, 0)   // heal / self-buff with nothing to hit
        }

        s.refreshViews()

        var logText = resolveLine(isPlayer, actor, move, dealt, target, staggerUnits, cancelFraction,
                                  defenderMoveCancelled)

        // With an arena the attacker eases back to its mark first (ReturningState); a stalled combatant
        // never moved, and a fight in place has no animation to wait for.
        if (s.stage != null && move.name != "Stalled") {
            s.goTo("ReturningState", {"actor": actor, "text": logText})
        } else {
            s.goTo("MessageState", {"text": logText})
        }
    }

    // What follows once a move's log line has been on screen long enough (MessageState): end the fight if
    // it's decided, otherwise back to whichever phase the player is owed.
    afterMove() {
        var s = _session
        if (s.livingEnemies().count == 0) {
            win()
        } else if (!s.player.alive) {
            lose()
        } else if (s.timeline.needsMove(s.player)) {
            // The player has no move in flight - either their own move just resolved cleanly, or a
            // parry (theirs or one landed on them) just cleared it - either way they choose again.
            // Covers every case without needing to special-case who just acted.
            s.goTo("ChoosingState")
        } else {
            s.commitEnemyMoves()
            s.goTo("ChargingState")
        }
    }

    resolveLine(isPlayer, actor, move, dealt, target, staggerUnits, cancelFraction, defenderMoveCancelled) {
        var s = _session
        var who = isPlayer ? "You" : actor.name
        if (move.name == "Stalled") {
            return "%(who) %(isPlayer ? "recover" : "recovers") from being parried."
        }
        if (move.healAmount > 0) {
            return "%(who) mends. (+%(move.healAmount) HP)"
        }
        if (move.staminaRestore > 0 && move.damage == 0) {
            return "%(who) recovers. (+%(move.staminaRestore) stamina)"
        }
        var whom = "?"
        if (target != null) {
            whom = (target == s.player) ? "you" : target.name
        }
        var line = "%(who) hit%(isPlayer ? "" : "s") %(whom) with %(move.name) - %(dealt) dmg."
        if (cancelFraction > 0) {
            line = "%(line)  Parried! No damage got through."
        }
        if (defenderMoveCancelled) {
            var whoDefended = target == s.player ? "Your" : "%(target.name)'s"
            line = "%(line)  %(whoDefended) own move was cancelled!"
        }
        if (staggerUnits > 0) {
            line = "%(line)  Staggered (-%(staggerUnits.floor))!"
        }
        return line
    }

    // --- endings -----------------------------------------------------------------------

    win() {
        var s = _session
        for (e in s.enemies) {
            if (e.entity) {
                GameObject.destroy(e.entity)
            }
        }
        endFight(s.enemies.count == 1 ? "You defeated the %(s.enemies[0].name)!" : "The pack is beaten!")
    }

    lose() {
        endFight("You were overwhelmed...")
    }

    // The player runs for it: with an arena they get a run-off animation first (FleeState), which ends
    // the fight itself.
    flee() {
        if (_session.stage != null) {
            _session.goTo("FleeState")
        } else {
            endFight("You slipped away.")
        }
    }

    endFight(text) {
        _session.goTo("OverState", {"text": text})
    }
}
