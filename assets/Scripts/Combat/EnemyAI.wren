// Combat/EnemyAI.wren
// How the opposition picks its chains. Enemies never improvise: each one picks whole chains from a small, fixed
// combo menu (the enemy types and combo patterns doc), always opens the same way, and usually follows a combo with
// the same next one - so a player who has met an enemy before can recognise a combo from its first move.
//
//   ComboMenu   the data: named combos (complete pre-authored chains), the opener, a usual follow-up per combo,
//               weights per battle phase (healthy > 50% HP, wounded <= 50%, desperate <= 20%) and positional triggers
//   ComboBrain  picks the next combo when a chain ends, in the doc's order: opener -> trigger -> follow-up roll ->
//               phase weights, skipping anything illegal (Hold after Hold) or unaffordable (then the shared Recover
//               chain: Hold 2.0 s -> cheapest move -> Hold 2.0 s), and never the same combo three times running
//               unless it is its own follow-up
//   PlannerBrain  for fighters with no menu (a humanoid put on the other side in the battle test, a duel partner):
//               builds a legal chain move by move with the same builder the player uses - close in, hit what is in
//               reach, guard after hitting, Hold when winded
//
// A brain is anything with chainFor(engine, fighter) -> [ChainEntry] (or null to idle this step).
import "random" for Random
import "Combat/Move" for Move
import "Combat/Combatant" for ChainEntry
import "Combat/Chain" for ChainBuilder
import "Combat/Config/Moves" for Moves

var RNG = Random.new()

class ComboMenu {
    // moves: { name: Fn -> Move } - the enemy's own moves (a monster's are human moves renamed and re-aimed).
    // combos: { name: [[moveName, param], ...] } - param is a Hold's length, a movement hint ("toward" / "away" /
    //         "side") or null. A step whose name isn't in `moves` and has a number is a themed Hold ("Circle").
    construct new(moves, combos, opener) {
        _moves = moves
        _combos = combos
        _opener = opener
        _followUps = {}
        _weights = {}
        _triggers = []
    }

    moves { _moves }
    combos { _combos }
    opener { _opener }
    followUps { _followUps }
    weights { _weights }
    triggers { _triggers }

    // After `combo`, go into `next` with this chance (0..1).
    followUp(combo, next, chance) {
        _followUps[combo] = [next, chance]
        return this
    }
    // phase: "healthy" / "wounded" / "desperate"; table: { combo: weight }.
    weights(phase, table) {
        _weights[phase] = table
        return this
    }
    // when: "adjacent" (an opponent stands in one of the eight cells around it as the chain starts), "alone" (nobody
    // does), "inReach" (one stands right in front of or behind it in its lane) or "notInReach".
    trigger(when, combo, chance) {
        _triggers.add([when, combo, chance])
        return this
    }

    // The chain for `combo`: fresh ChainEntries.
    build(combo) {
        var entries = []
        for (step in _combos[combo]) {
            var name = step[0]
            var param = step.count > 1 ? step[1] : null
            if (_moves.containsKey(name)) {
                entries.add(ChainEntry.new(_moves[name].call(), param))
            } else if (param is Num) {
                entries.add(ChainEntry.new(Move.hold(param).rename(name), null))
            }
        }
        return entries
    }
}

class ComboBrain {
    construct new(menu) {
        _menu = menu
        _started = false
        _last = null
        _repeats = 0
    }

    last { _last }

    chainFor(engine, f) {
        var combo = pick_(engine, f)
        var entries = combo == null ? null : _menu.build(combo)
        if (entries == null || !affordable_(engine, f, entries)) {
            return recover_(engine, f)
        }
        if (combo == _last) {
            _repeats = _repeats + 1
        } else {
            _repeats = 1
        }
        _last = combo
        engine.combo(f, combo)
        return entries
    }

    pick_(engine, f) {
        if (!_started) {
            _started = true
            if (legal_(f, _menu.opener)) return _menu.opener
        }
        var adjacent = engine.adjacentEnemies(f).count > 0
        var inReach = engine.enemiesOf(f).any { |o| o.lane == f.lane && (o.row - f.row).abs == 1 }
        for (trigger in _menu.triggers) {
            var when = trigger[0]
            var met = (when == "adjacent" && adjacent) || (when == "alone" && !adjacent) ||
                (when == "inReach" && inReach) || (when == "notInReach" && !inReach)
            if (met && legal_(f, trigger[1]) && RNG.float() < trigger[2]) return trigger[1]
        }
        if (_last != null && _menu.followUps.containsKey(_last)) {
            var follow = _menu.followUps[_last]
            if (legal_(f, follow[0]) && RNG.float() < follow[1]) return follow[0]
        }
        var phase = "healthy"
        var fraction = f.stats.fraction
        if (fraction <= 0.2 && _menu.weights.containsKey("desperate")) {
            phase = "desperate"
        } else if (fraction <= 0.5 && _menu.weights.containsKey("wounded")) {
            phase = "wounded"
        }
        var table = _menu.weights[phase]
        if (table == null) {
            return legal_(f, _menu.opener) ? _menu.opener : null
        }
        var total = 0
        var options = []
        for (name in table.keys) {
            if (!legal_(f, name) || noRepeat_(name)) continue
            options.add(name)
            total = total + table[name]
        }
        if (options.count == 0) return null
        var roll = RNG.float() * total
        for (name in options) {
            roll = roll - table[name]
            if (roll <= 0) return name
        }
        return options[-1]
    }

    // A combo is illegal if it would put a Hold straight after a Hold.
    legal_(f, combo) {
        if (!_menu.combos.containsKey(combo)) return false
        var first = _menu.combos[combo][0]
        var startsWithHold = !_menu.moves.containsKey(first[0])
        return !(startsWithHold && f.lastWasHold)
    }

    // The same combo can't run three times in a row unless it is its own listed follow-up.
    noRepeat_(combo) {
        if (combo != _last || _repeats < 2) return false
        var follow = _menu.followUps[combo]
        return follow == null || follow[0] != combo
    }

    affordable_(engine, f, entries) {
        if (f.has("noStamina")) return true
        var stamina = f.stats.stamina
        for (entry in entries) {
            if (entry.move.isHold) {
                stamina = (stamina + engine.rules.holdRecovery(f) * entry.move.completion).min(f.stats.maxStamina)
            } else {
                stamina = stamina - engine.rules.moveCost(f, entry.move, false)
                if (stamina < 0) return false
            }
        }
        return true
    }

    // The shared fallback: Hold 2.0 s -> cheapest move -> Hold 2.0 s (the Hold first only if the last move wasn't one).
    recover_(engine, f) {
        var cheapest = null
        var cheapestCost = null
        for (name in _menu.moves.keys) {
            var move = _menu.moves[name].call()
            var cost = engine.rules.moveCost(f, move, false)
            if (cheapestCost == null || cost < cheapestCost) {
                cheapest = move
                cheapestCost = cost
            }
        }
        var entries = []
        if (!f.lastWasHold) entries.add(ChainEntry.new(Move.hold(2.0).rename("Recover"), null))
        if (cheapest != null && f.stats.stamina >= cheapestCost) {
            entries.add(ChainEntry.new(cheapest, cheapest.isMovement ? "toward" : null))
            entries.add(ChainEntry.new(Move.hold(2.0).rename("Recover"), null))
        }
        if (entries.count == 0) entries.add(ChainEntry.new(Move.falter(), null))
        _last = null
        return entries
    }
}

class PlannerBrain {
    construct new() {}

    chainFor(engine, f) {
        var builder = ChainBuilder.new(engine, f)
        var guards = f.moves.where { |m| m.isDefend && !m.isAttack }.toList
        var justHit = false
        var guard = 0
        while (!builder.complete && guard < 12) {
            guard = guard + 1
            var move = null
            var dest = null
            var winded = !f.has("noStamina") && builder.stamina < f.stats.maxStamina * 0.3
            if (winded && !builder.lastIsHold) {
                move = Moves.hold(builder.stamina < f.stats.maxStamina * 0.15 ? 2.0 : 1.2)
            } else if (justHit && guards.count > 0 && RNG.float() < 0.6 && builder.blockedBy(guards[0]) == null) {
                // Hit, then block.
                move = guards[0]
            } else {
                var attack = bestAttack_(engine, f, builder)
                if (attack != null) {
                    move = attack[0]
                    dest = attack[1]
                } else {
                    var step = approach_(engine, f, builder)
                    if (step != null) {
                        move = step[0]
                        dest = step[1]
                    }
                }
            }
            if (move == null) {
                // Nothing useful: wait a beat, or guard if the last move was already a Hold.
                if (!builder.lastIsHold) {
                    move = Moves.hold(0.8)
                } else {
                    move = fallback_(f, builder)
                    if (move != null && builder.needsDestination(move)) {
                        var cells = builder.destinations(move)
                        dest = cells.count > 0 ? cells[0] : null
                    }
                }
            }
            if (move == null) break
            justHit = move.isAttack
            builder.add(move, dest)
        }
        if (builder.count == 0) return [ChainEntry.new(Move.hold(1.2), null)]
        return builder.entries
    }

    // The affordable attack that would hurt the most from where the chain leaves `f`: [move, dest] or null.
    bestAttack_(engine, f, builder) {
        var best = null
        var bestScore = 0
        for (move in f.moves) {
            if (!move.isAttack || builder.blockedBy(move) != null) continue
            var options = builder.needsDestination(move) ? builder.destinations(move) : [null]
            for (dest in options) {
                var score = 0
                for (cell in builder.cellsOf(move, dest)) {
                    var who = engine.occupant(cell[0], cell[1])
                    if (who != null && who.side != f.side) score = score + move.damage * cell[2] + move.stagger * 10 + 1
                }
                score = score / (move.completion + 0.5)
                if (score > bestScore) {
                    best = [move, dest]
                    bestScore = score
                }
            }
        }
        return best
    }

    // A movement move that brings `f` in front of its nearest opponent: [move, dest] or null (already there / none).
    approach_(engine, f, builder) {
        var target = engine.nearestOpponent(f)
        if (target == null) return null
        var now = distance_(builder.lane, builder.row, target)
        var best = null
        var bestDistance = now
        for (move in f.moves) {
            if (!move.isMovement || move.isAttack || builder.blockedBy(move) != null) continue
            var options = builder.needsDestination(move) ? builder.destinations(move) : [null]
            for (dest in options) {
                if (dest == null) continue
                var d = distance_(dest[0], dest[1], target)
                if (d < bestDistance) {
                    best = [move, dest]
                    bestDistance = d
                }
            }
        }
        return best
    }

    // How far [lane, row] is from standing right in front of `target` in its lane.
    distance_(lane, row, target) {
        var rowGap = (target.row - row).abs
        var d = (target.lane - lane).abs * 2 + (rowGap - 1).abs
        return rowGap == 0 ? d + 2 : d
    }

    fallback_(f, builder) {
        for (move in f.moves) {
            if (builder.blockedBy(move) == null) return move
        }
        return null
    }
}
