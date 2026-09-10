// Combat/Timeline.wren
// Turn ordering by charge, not by seat - Phases 2-4 of the Interrupt Combat Roadmap. Every combatant
// charges toward the Move they've committed to; whoever's bar fills first acts first, and a faster
// move means acting again sooner. Landing a hit on someone mid-charge shoves their bar backward
// (interrupt(), Phase 3); committing a move while exhausted adds a time unit to its cost (Phase 4).
// Supports any number of combatants (roadmap open decision #3 - N from day one).
//
// SECONDS_PER_TIME_UNIT is THE combat time scale (roadmap "Decide first" note): every move cost,
// interrupt delay, combo window and ultimate wind-up in the whole design is denominated in "time
// units", and this is what one unit is worth in real seconds. Change it here and the entire system
// re-times together.
var SECONDS_PER_TIME_UNIT = 0.4
var EXHAUSTION_TIME_PENALTY = 1   // extra time units on a move committed while exhausted

class Timeline {
    construct new() {
        // [{ combatant, move: Move|null, progress: Num, cost: Num (effective, incl. exhaustion) }]
        _entries = []
    }

    add(combatant) {
        _entries.add({ "combatant": combatant, "move": null, "progress": 0, "cost": 0 })
    }

    // Every combatant on the timeline, in add order (player then enemies) - for CombatState's
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
    // so an exhaustion penalty applies to a move you START while tired, not retroactively.
    commit(combatant, move) {
        var e = entryFor(combatant)
        if (e != null) {
            e["move"] = move
            e["progress"] = 0
            e["cost"] = move.timeCost + (combatant.stats.exhausted ? EXHAUSTION_TIME_PENALTY : 0)
        }
    }

    // Drop this combatant's move (they, or their AI, must choose again); progress resets.
    clear(combatant) {
        var e = entryFor(combatant)
        if (e != null) {
            e["move"] = null
            e["progress"] = 0
            e["cost"] = 0
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
