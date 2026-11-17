// Combat/Chain.wren
// Building a chain - the committed run of moves a combatant plays back to back. The chain rules of the battle
// system doc, shared by the player's selection screen and anything else that needs a legal chain:
//   - at least 2 moves; keep adding until their authored lengths reach the budget (4 s) - the move that reaches or
//     crosses it is the last one;
//   - Hold can't follow Hold, including across a chain boundary;
//   - stamina is checked at selection: the chain's total cost must fit in current stamina plus what earlier Holds in
//     the chain will recover (each move still pays when it starts);
//   - nothing cancels: once committed, a chain plays out.
// ChainBuilder also projects where the combatant will stand after each step, so the selection screen can show the
// cells a move will cover from there and offer the right destinations.
import "Combat/Combatant" for ChainEntry

// Movement patterns -> the [lane, row] offsets a mover may pick (row offsets already turned by `facing`).
class Grid {
    static offsets(pattern, facing) {
        if (pattern == "ortho") return [[0, facing], [0, -facing], [-1, 0], [1, 0]]
        if (pattern == "any") return [[0, facing], [0, -facing], [-1, 0], [1, 0], [-1, facing], [1, facing], [-1, -facing], [1, -facing]]
        if (pattern == "diag") return [[-1, facing], [1, facing], [-1, -facing], [1, -facing]]
        if (pattern == "fwdback") return [[0, facing], [0, -facing]]
        if (pattern == "forward") return [[0, facing]]
        if (pattern == "back") return [[0, -facing]]
        if (pattern == "forward2") return [[0, 2 * facing]]
        if (pattern == "blink") {
            var out = []
            for (dl in -2..2) {
                for (dr in -2..2) {
                    if (dl != 0 || dr != 0) out.add([dl, dr])
                }
            }
            return out
        }
        return []
    }

    // Patterns with one forced destination - nothing for the player to pick.
    static fixed(pattern) { pattern == "forward" || pattern == "back" || pattern == "forward2" }
}

class ChainBuilder {
    construct new(engine, fighter) {
        _e = engine
        _f = fighter
        _entries = []
        _time = 0
        _lane = fighter.lane
        _row = fighter.row
        _stamina = fighter.stats.stamina
        _trail = []      // per entry: [lane, row, stamina, time] before it was added - for undo
    }

    fighter { _f }
    entries { _entries }
    count { _entries.count }
    // Seconds of chain so far, and the budget it has to reach.
    time { _time }
    budget { _e.budget }
    // Where the combatant will stand once the chain so far has played (if nothing blocks it).
    lane { _lane }
    row { _row }
    facing { _e.facingAt(_f, _lane, _row) }
    // Stamina left after the chain so far (counting what its Holds recover).
    stamina { _stamina }

    complete { _entries.count >= 2 && _time >= _e.budget - 0.0001 }

    lastIsHold {
        if (_entries.count > 0) return _entries[-1].move.isHold
        return _f.lastWasHold
    }

    costOf(move) { _e.rules.moveCost(_f, move, false) }

    // null when `move` can be added now, otherwise why not ("full", "hold", "stamina", "weapon").
    blockedBy(move) {
        if (complete) return "full"
        if (move.isHold && lastIsHold) return "hold"
        if (!move.isHold && !_f.loadout.allows(move)) return "weapon"
        if (!_f.has("noStamina") && costOf(move) > _stamina + 0.0001) return "stamina"
        if (move.special == "bolt" && !loadedAfterChain_) return "weapon"
        return null
    }

    // A crossbow fires what is loaded: one bolt, reloaded by a Reload earlier in the chain.
    loadedAfterChain_ {
        var loaded = _f.counter("loaded")
        for (entry in _entries) {
            if (entry.move.special == "reload") loaded = 1
            if (entry.move.special == "bolt") loaded = 0
        }
        return loaded > 0
    }

    // True when the player has to pick where `move` goes.
    needsDestination(move) { move.isMovement && !Grid.fixed(move.pattern) }

    // The cells `move` could take the combatant to from where the chain leaves it: on the board and not standing on
    // someone (who may of course have moved by then - legality is checked again at the move time).
    destinations(move) {
        var out = []
        if (!move.isMovement) return out
        for (offset in Grid.offsets(move.pattern, facing)) {
            var lane = _lane + offset[0]
            var row = _row + offset[1]
            if (!_e.inBounds(lane, row)) continue
            var who = _e.occupant(lane, row)
            if (who != null && who != _f) continue
            out.add([lane, row])
        }
        return out
    }

    // The cells `move` would cover if added now: [[lane, row, share, isAim, isDepth], ...].
    cellsOf(move, dest) {
        var lane = _lane
        var row = _row
        if (dest != null && move.isMovement && (move.impact == null || move.moveTime <= move.impact)) {
            lane = dest[0]
            row = dest[1]
        }
        return _e.cellsAt(_f, move, lane, row, _e.facingAt(_f, lane, row))
    }

    // Adds a move. dest: the picked [lane, row] for a movement move that needs one (null otherwise).
    add(move, dest) {
        _trail.add([_lane, _row, _stamina, _time])
        var target = dest
        if (move.isMovement && Grid.fixed(move.pattern)) {
            var offset = Grid.offsets(move.pattern, facing)[0]
            target = [_lane + offset[0], _row + offset[1]]
            if (!_e.inBounds(target[0], target[1])) target = null
        }
        _entries.add(ChainEntry.new(move, target))
        _time = _time + move.completion
        if (move.isHold) {
            _stamina = (_stamina + _e.rules.holdRecovery(_f) * move.completion).min(_f.stats.maxStamina)
        } else {
            _stamina = _stamina - costOf(move)
        }
        if (target != null) {
            var who = _e.occupant(target[0], target[1])
            if (who == null || who == _f) {
                _lane = target[0]
                _row = target[1]
            }
        }
    }

    undo() {
        if (_entries.count == 0) return
        _entries.removeAt(-1)
        var before = _trail.removeAt(-1)
        _lane = before[0]
        _row = before[1]
        _stamina = before[2]
        _time = before[3]
    }
}
