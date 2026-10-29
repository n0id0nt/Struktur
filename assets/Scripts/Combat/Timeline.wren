// Combat/Timeline.wren
// Turn ordering by charge, not by seat - Phases 2-4 of the Interrupt Combat Roadmap. Every combatant
// charges toward the Move they've committed to; whoever's bar fills first acts first, and a faster
// move means acting again sooner. Landing a hit on someone mid-charge shoves their bar backward
// (interrupt(), Phase 3); a move's effective cost (agility, +1 time unit when exhausted - Phase 4) and each
// combatant's opening head start (initiative) come from Combat/Arbiter.wren.
// Supports any number of combatants (roadmap open decision #3 - N from day one).
//
// SECONDS_PER_TIME_UNIT is THE combat time scale (roadmap "Decide first" note): every move cost,
// interrupt delay, combo window and ultimate wind-up in the whole design is denominated in "time
// units", and this is what one unit is worth in real seconds. Change it here and the entire system
// re-times together.
var SECONDS_PER_TIME_UNIT = 0.4

import "Combat/Arbiter" for Arbiter

class Timeline {
    construct new() {
        // [{ combatant, move: Move|null, progress: Num, cost: Num (effective - Arbiter.moveCost),
        //    headStart: Num (initiative, spent by the first commit) }]
        _entries = []
    }

    // Joins the fight; the combatant's initiative is rolled now and spent on its first charge.
    add(combatant) {
        _entries.add({ "combatant": combatant, "move": null, "progress": 0, "cost": 0, "parryOffered": false, "parryResult": null, "target": null, "headStart": Arbiter.initiative(combatant) })
    }

    // Every combatant on the timeline, in add order (player then enemies) - for States/Combat/CombatSession.wren
    // per-frame view refresh.
    combatants {
        var out = []
        for (e in _entries) {
            out.add(e["combatant"])
        }
        return out
    }

    entryFor(combatant) {
        for (e in _entries) {
            if (e["combatant"] == combatant) {
                return e
            }
        }
        return null
    }

    committedMove(combatant) {
        var e = entryFor(combatant)
        return e == null ? null : e["move"]
    }

    needsMove(combatant) {
        return committedMove(combatant) == null
    }

    // Start (or restart) this combatant's charge toward `move`. The effective cost is locked in here,
    // so an exhaustion penalty applies to a move you START while tired, not retroactively. The first commit
    // of the fight starts from the initiative head start instead of 0.
    commit(combatant, move) {
        var e = entryFor(combatant)
        if (e != null) {
            e["move"] = move
            e["cost"] = Arbiter.moveCost(combatant, move)
            e["progress"] = Arbiter.capInitiative(e["headStart"], e["cost"])
            e["headStart"] = 0
            e["parryOffered"] = false
            e["parryResult"] = null
            e["target"] = null
        }
    }

    // Drop this combatant's move (they, or their AI, must choose again); progress resets.
    clear(combatant) {
        var e = entryFor(combatant)
        if (e != null) {
            e["move"] = null
            e["progress"] = 0
            e["cost"] = 0
            e["parryOffered"] = false
            e["parryResult"] = null
            e["target"] = null
        }
    }

    // Advance every committed, still-living entry by dtSeconds of real time. Returns the time units
    // that elapsed, so the caller can drive stamina regen off the same clock.
    tick(dtSeconds) {
        var units = dtSeconds / SECONDS_PER_TIME_UNIT
        for (e in _entries) {
            if (e["move"] != null && e["combatant"].alive) {
                e["progress"] = e["progress"] + units
            }
        }
        return units
    }

    // 0..1 charge toward the committed move (0 with no move, or while pushed to negative progress
    // by an interrupt).
    fraction(combatant) {
        var e = entryFor(combatant)
        if (e == null || e["move"] == null) {
            return 0
        }
        var f = e["progress"] / e["cost"]
        if (f < 0) {
            return 0
        }
        return f > 1 ? 1 : f
    }

    // Who the committed move is aimed at (set right after commit by CombatSession.commitMove; reset by commit/clear).
    target(combatant) {
        var e = entryFor(combatant)
        return e == null ? null : e["target"]
    }

    setTarget(combatant, target) {
        var e = entryFor(combatant)
        if (e != null) {
            e["target"] = target
        }
    }

    // Parry bookkeeping for the committed move (reset by commit/clear): whether its parry window has opened
    // (so the target is only asked once) and, if the target tried to parry, how well it went (a
    // Combat/Arbiter.wren ParryResult; null = no parry attempt). See CombatSession.
    parryOffered(combatant) {
        var e = entryFor(combatant)
        return e != null && e["parryOffered"]
    }

    markParryOffered(combatant) {
        var e = entryFor(combatant)
        if (e != null) {
            e["parryOffered"] = true
        }
    }

    parryResult(combatant) {
        var e = entryFor(combatant)
        return e == null ? null : e["parryResult"]
    }

    setParryResult(combatant, result) {
        var e = entryFor(combatant)
        if (e != null) {
            e["parryResult"] = result
        }
    }

    // Effective cost in time units of this combatant's committed move (Arbiter.moveCost, locked in at
    // commit), or 0 with no move - also the move's animation length in time units
    // (see Combat/Config/ActionAnimations.wren: charge time and animation time are the same thing).
    costOf(combatant) {
        var e = entryFor(combatant)
        return (e == null || e["move"] == null) ? 0 : e["cost"]
    }

    // Raw progress in time units (can be negative after an interrupt), or 0 with no move.
    progress(combatant) {
        var e = entryFor(combatant)
        return (e == null || e["move"] == null) ? 0 : e["progress"]
    }

    // True while this combatant has committed a move and hasn't fired it yet - i.e. a valid
    // interrupt target.
    isCharging(combatant) {
        var e = entryFor(combatant)
        return e != null && e["move"] != null && e["progress"] < e["cost"]
    }

    // Shove a mid-charge combatant's progress backward by `units` (Phase 3 interrupt). Can drop the
    // progress below zero; nextReady()/fraction() both cope.
    interrupt(combatant, units) {
        var e = entryFor(combatant)
        if (e != null && e["move"] != null) {
            e["progress"] = e["progress"] - units
        }
    }

    // The living combatant whose charge has completed and is furthest past it (so if two fill on the
    // same tick, the more-overdue one goes first), or null if nobody is ready.
    nextReady() {
        var best = null
        var bestOverflow = 0
        for (e in _entries) {
            if (e["move"] == null || !e["combatant"].alive) {
                continue
            }
            var overflow = e["progress"] - e["cost"]
            if (overflow >= 0 && (best == null || overflow > bestOverflow)) {
                best = e["combatant"]
                bestOverflow = overflow
            }
        }
        return best
    }
}
