// Combat/Timeline.wren
// Turn ordering by charge, not by seat - Phase 2 of the Interrupt Combat Roadmap. Every combatant
// charges toward the Move they've committed to; whoever's bar fills first acts first, and a faster
// move means acting again sooner. Supports any number of combatants (the roadmap's open decision #3
// - N from day one, not hard-coded to two).
//
// SECONDS_PER_TIME_UNIT is THE combat time scale (roadmap "Decide first" note): every move cost,
// interrupt delay, combo window and ultimate wind-up in the whole design is denominated in "time
// units", and this is what one unit is worth in real seconds. Change it here and the entire system
// re-times together.
var SECONDS_PER_TIME_UNIT = 0.4

class Timeline {
    construct new() {
        _entries = []   // [{ "combatant": c, "move": Move|null, "progress": Num (time units) }]
    }

    add(combatant) {
        _entries.add({ "combatant": combatant, "move": null, "progress": 0 })
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

    // Start (or restart) this combatant's charge toward `move`.
    commit(combatant, move) {
        var e = entryFor(combatant)
        if (e != null) {
            e["move"] = move
            e["progress"] = 0
        }
    }

    // Drop this combatant's move (they, or their AI, must choose again); progress resets.
    clear(combatant) {
        var e = entryFor(combatant)
        if (e != null) {
            e["move"] = null
            e["progress"] = 0
        }
    }

    // Advance every committed, still-living entry by dtSeconds of real time.
    tick(dtSeconds) {
        var units = dtSeconds / SECONDS_PER_TIME_UNIT
        for (e in _entries) {
            if (e["move"] != null && e["combatant"].alive) {
                e["progress"] = e["progress"] + units
            }
        }
    }

    // 0..1 charge toward the committed move (0 when no move is committed).
    fraction(combatant) {
        var e = entryFor(combatant)
        if (e == null || e["move"] == null) {
            return 0
        }
        var f = e["progress"] / e["move"].timeCost
        return f > 1 ? 1 : f
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
            var overflow = e["progress"] - e["move"].timeCost
            if (overflow >= 0 && (best == null || overflow > bestOverflow)) {
                best = e["combatant"]
                bestOverflow = overflow
            }
        }
        return best
    }
}
