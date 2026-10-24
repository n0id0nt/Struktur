// States/Combat/CombatResolver.wren
// The rules for what happens at the moment a combatant's charge (and so its attack animation) finishes:
// apply the move using the parry decision already made during the wind-up (CombatSession.nextParryOffer /
// ParryPromptState), including damage / stagger / parry-cancel / stall, play the impact reactions, then hand
// off to the recovery animation and message (ReturningState -> MessageState) and finally decide what comes
// after the log (win / lose / back to the move menu / back to charging). Owned by CombatSession; the phases
// themselves only call into it from their own update().
import "gameObject" for GameObject
import "gameObjectComponents" for Script, WorldTransform
import "reflect" for Reflect
import "Party" for Party
import "Combat/Config/Moves" for Moves
import "Combat/Disruption" for Disruption

class CombatResolver {
    construct new(session) {
        _session = session
    }

    // The attack on screen has landed: look up the move's target and how that target's parry went (it was
    // attempted - from the player's prompt or an AI coin flip - and arbitrated by Combat/Arbiter.parry when
    // the attack's parry window opened), then resolve it.
    resolveMove(actor) {
        var s = _session
        var move = s.timeline.committedMove(actor)
        var isPlayer = actor == s.player
        var target = s.targetOf(actor)
        var parry = null
        if (move.offensive && move.parryable && target != null && target.alive) {
            parry = s.parryResult(actor)
        }
        finishResolve(actor, move, target, isPlayer, parry)
    }

    // The actual resolution (damage, stagger/parry, impact reactions, log). parry: the Combat/Arbiter.wren
    // ParryResult, or null with no parry attempt. Its tier decides how much damage gets through
    // (Combat/Combatant.use), whether the attacker sits out a Combat/Config/Moves.stalled beat, and whether
    // the defender keeps its own in-flight move (only a perfect parry does - otherwise it was dropped to
    // guard, in place of the usual stagger). Ends by handing off to the next phase.
    finishResolve(actor, move, target, isPlayer, parry) {
        var s = _session
        var cancelFraction = parry == null ? 0 : 1 - parry.damageThrough

        s.clearMove(actor)

        var dealt = 0
        var staggerUnits = 0
        var defenderMoveCancelled = false
        if (target != null) {
            dealt = actor.use(move, target, cancelFraction)
            if (move.offensive && s.timeline.isCharging(target)) {
                if (parry != null) {
                    // Trying to parry costs the defender their own in-flight move (they pick again instead
                    // of the usual partial stagger) - unless the parry was perfect.
                    if (!parry.defenderKeepsMove) {
                        s.clearMove(target)
                        defenderMoveCancelled = true
                    }
                } else {
                    staggerUnits = Disruption.delay(move, s.timeline.fraction(target))
                    s.interruptMove(target, staggerUnits)
                    s.markStaggered(target)
                }
            }
            if (parry != null && parry.stallsAttacker) {
                // A perfect parry also punishes the attacker: instead of letting them pick a real move
                // again immediately, they sit out Combat/Config/Moves.stalled first.
                s.commitMove(actor, Moves.stalled)
            }
            if (s.stage != null) {
                s.stage.playImpact(actor, target, move, parry)
            }
        } else {
            actor.use(move, actor, 0)   // heal / self-buff with nothing to hit
        }

        s.refreshViews()

        var logText = resolveLine(isPlayer, actor, move, dealt, target, staggerUnits, parry,
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
        } else if (s.livingParty().count == 0) {
            lose()
        } else if (s.chooser != null) {
            // Some living party member has no move in flight - their own move just resolved, or a parry
            // (theirs or one landed on them) cleared it - so the human picks again. Covers every case
            // without needing to special-case who just acted.
            s.goTo("ChoosingState")
        } else {
            s.commitEnemyMoves()
            s.goTo("ChargingState")
        }
    }

    resolveLine(isPlayer, actor, move, dealt, target, staggerUnits, parry, defenderMoveCancelled) {
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
        if (parry != null) {
            if (parry.tier == "perfect") {
                line = "%(line)  Perfect parry! No damage got through."
            } else if (parry.tier == "partial") {
                line = "%(line)  Partly parried - some damage got through."
            } else {
                line = "%(line)  The parry failed!"
            }
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

    // Every opponent is down: beaten overworld entities are removed - and any who "join the party when
    // defeated" (a recruitable NPC, like the Duelist - see GameObjects/NPC.joinsPartyOnDefeat) are added to the
    // party instead of just vanishing; their follower appears where they stood.
    win() {
        var s = _session
        var recruited = []
        for (e in s.enemies) {
            if (e.entity) {
                var script = Script.getInstance(e.entity)
                // Duck-typed (not `is NPC`): registered scripts live in their own module instance, so only shared
                // base classes like Critter pass an `is` check.
                if (script != null && Reflect.hasMethod(script, "joinsPartyOnDefeat()") && script.joinsPartyOnDefeat()) {
                    Party.join(script.name, WorldTransform.getPosition(e.entity))
                    recruited.add(script.name)
                }
                GameObject.destroy(e.entity)
            }
        }
        var line = s.enemies.count == 1 ? "You defeated the %(s.enemies[0].name)!" : "The pack is beaten!"
        if (recruited.count > 0) {
            line = "%(line)  %(recruited.join(", ")) joins your party!"
        }
        endFight(line)
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
