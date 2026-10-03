// States/Combat/CombatResolver.wren
// The rules for what happens when a combatant's charge fills: pick the move's target, offer that
// target a parry (a prompt for the player via ParryPromptState, an instant coin flip for an AI
// enemy), apply damage / stagger / parry-cancel / stall, then hand off to the animation phases
// (AttackingState -> ReturningState -> MessageState) and finally decide what comes after the log
// (win / lose / back to the move menu / back to charging). Owned by CombatSession; the phases
// themselves only call into it from their own update()/answer handlers.
import "gameObject" for GameObject
import "random" for Random
import "Combat/Config/Moves" for Moves
import "Combat/Disruption" for Disruption

// Chance an AI-controlled defender answers "yes" to its own parry prompt (see resolveMove) - a
// plain coin flip for now, tunable like every other combat constant.
var ENEMY_PARRY_CHANCE = 0.5

var RNG = Random.new()

class CombatResolver {
    construct new(session) {
        _session = session
    }

    // Looks up the acting combatant's move/target and, for anything parryable landing on a living
    // target, offers that target a parry decision before resolving: a UI prompt for the player
    // (ParryPromptState, which calls finishResolve with the answer), or an instant random yes/no for
    // an AI-controlled enemy. Everything else (non-offensive moves, no live target) resolves
    // immediately.
    resolveMove(actor) {
        var s = _session
        var move = s.timeline.committedMove(actor)
        var isPlayer = actor == s.player
        var target = isPlayer ? s.playerTarget : s.player
        if (target == null || !target.alive) {
            target = isPlayer ? s.firstLivingEnemyCombatant() : s.player
        }

        if (move.offensive && move.parryable && target != null && target.alive) {
            if (target == s.player) {
                s.goTo("ParryPromptState", {"actor": actor, "target": target, "move": move})
                return
            }
            // AI-controlled defender - no UI, just an instant coin flip.
            finishResolve(actor, move, target, isPlayer, RNG.float() < ENEMY_PARRY_CHANCE)
            return
        }

        finishResolve(actor, move, target, isPlayer, false)
    }

    // The actual resolution (damage, stagger/parry-cancel, log, strike animation) - called from
    // resolveMove (nothing to decide, or an AI's instant decision) or from ParryPromptState once
    // the player's prompt is answered/times out. parryChosen: true cancels all of the move's damage
    // (Combat/Combatant.wren) and costs the attacker a Combat/Config/Moves.stalled beat; the
    // defender's own in-flight move is also cancelled regardless of side, since choosing to parry is
    // a deliberate reaction to make. Ends by handing off to the next phase.
    finishResolve(actor, move, target, isPlayer, parryChosen) {
        var s = _session
        var cancelFraction = parryChosen ? 1 : 0

        s.timeline.clear(actor)

        var dealt = 0
        var staggerUnits = 0
        var defenderMoveCancelled = false
        if (target != null) {
            dealt = actor.use(move, target, cancelFraction)
            if (move.offensive && s.timeline.isCharging(target)) {
                if (parryChosen) {
                    // Choosing to parry costs the defender their own in-flight move; they have to
                    // pick again instead of the usual partial stagger.
                    s.timeline.clear(target)
                    defenderMoveCancelled = true
                } else {
                    staggerUnits = Disruption.delay(move, s.timeline.fraction(target))
                    s.timeline.interrupt(target, staggerUnits)
                    s.markStaggered(target)
                }
            }
            if (parryChosen) {
                // A landed parry also punishes the attacker: instead of letting them pick a real move
                // again immediately, they sit out Combat/Config/Moves.stalled first.
                s.timeline.commit(actor, Moves.stalled)
            }
        } else {
            actor.use(move, actor, 0)   // heal / self-buff with nothing to hit
        }

        s.refreshViews()

        var logText = resolveLine(isPlayer, actor, move, dealt, target, staggerUnits, cancelFraction,
                                  defenderMoveCancelled)

        if (move.name == "Stalled") {
            if (s.stage != null) {
                s.stage.showStalled(actor)
            }
            s.goTo("MessageState", {"text": logText})
        } else if (s.stage != null && move.offensive && target != null) {
            // Offensive moves against a live target get the curve-driven lunge + impact particle
            // (Combat/BattleStage.wren, driven by AttackingState/ReturningState); everything else
            // (heals/buffs/no-target) keeps the old instant animation-switch and goes straight to the
            // message beat.
            s.goTo("AttackingState", {"actor": actor, "target": target, "move": move, "text": logText})
        } else {
            if (s.stage != null) {
                s.stage.strike(actor, null)
            }
            s.goTo("MessageState", {"text": logText})
        }
    }

    // What follows once a move's log line has been on screen long enough (MessageState): end the
    // fight if it's decided, otherwise back to whichever phase the player is owed.
    afterMove() {
        var s = _session
        if (s.stage != null) {
            s.stage.rest()
        }
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

    flee() {
        endFight("You slipped away.")
    }

    endFight(text) {
        _session.goTo("OverState", {"text": text})
    }
}
