// Combat/BattlePlan.wren
// Where a side's fighters will stand over the coming seconds, as far as their committed chains say - so a chain being
// built (Combat/Chain.wren) can step into a cell an ally is about to leave, and never into one an ally is about to
// take. Each ally's chain is played forward from where it is now: every movement move puts it in its picked cell at
// that move's move time (the engine checks again for real when the time comes - see BattleEngine.applyMovement_).
// An ally still waiting on a chain stands where it is. The opposition can't be read ahead, so its fighters count as
// standing where they are now for good.
//
// Times are seconds from now. A stay is [lane, row, from, until] - until null for "from then on".

class BattlePlan {
    // engine: the battle. fighter: whose chain is being built - left out of the plan.
    construct new(engine, fighter) {
        _engine = engine
        _fighter = fighter
        _stays = []
        for (other in engine.fighters) {
            if (!other.active || other == fighter) continue
            if (other.side == fighter.side) {
                for (stay in BattlePlan.staysOf(other)) _stays.add(stay)
            } else {
                _stays.add([other.lane, other.row, 0, null])
            }
        }
    }

    // True when `fighter` could step into [lane, row] at `arriveAt` and stay there: on the board and nobody standing
    // there at that time or arriving there later.
    isFree(lane, row, arriveAt) {
        if (!_engine.inBounds(lane, row)) return false
        for (stay in _stays) {
            if (stay[0] != lane || stay[1] != row) continue
            if (stay[3] == null || stay[3] > arriveAt + 0.0001) return false
        }
        return true
    }

    // Where `fighter`'s committed chain has it standing over time, from now: [[lane, row, from, until], ...].
    static staysOf(fighter) {
        var stays = []
        var lane = fighter.lane
        var row = fighter.row
        var since = 0
        for (step in BattlePlan.stepsOf(fighter)) {
            var dest = step["to"]
            if (dest != null && (dest[0] != lane || dest[1] != row)) {
                stays.add([lane, row, since, step["at"]])
                lane = dest[0]
                row = dest[1]
                since = step["at"]
            }
        }
        stays.add([lane, row, since, null])
        return stays
    }

    // The rest of `fighter`'s committed chain as timed steps from now: [{"move", "start", "end", "at", "to"}, ...] -
    // "at" / "to": when and where a movement move puts it (null for anything else, or a move that already moved).
    // Empty for a fighter waiting on a chain.
    static stepsOf(fighter) {
        var steps = []
        if (fighter.needsChain || fighter.move == null) return steps
        var start = -fighter.progress
        var index = fighter.chainIndex
        var chain = fighter.chain
        var first = true
        while (index < chain.count) {
            var entry = chain[index]
            var move = first ? fighter.move : entry.move
            var step = {"move": move, "start": start, "end": start + move.completion, "at": null, "to": null}
            var dest = entry.dest
            if (move.isMovement && dest is List && !(first && fighter.moved)) {
                step["at"] = (start + move.moveTime).max(0)
                step["to"] = dest
            }
            steps.add(step)
            start = start + move.completion
            index = index + 1
            first = false
        }
        return steps
    }
}
