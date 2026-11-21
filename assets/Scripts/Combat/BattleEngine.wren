// Combat/BattleEngine.wren
// The animation-timed battle: a shared timeline of fixed-length moves on a lanes x rows arena. Every combatant plays
// a committed chain back to back; the clock only stops when a human-controlled combatant's chain runs out and they
// must pick the next one (AI picks instantly). Pure rules - no entities, no UI: the stage and the screen read the
// combatants and drain `takeEvents()` to show what happened.
//
// The arena: lanes run across the screen's depth (0 = top), rows along it. Side 0 (the party) starts in row 0 and
// advances toward higher rows; side 1 starts in the last row and advances toward row 0. One occupant per cell.
//
// Order of operations each step (TICK seconds):
//   1. every active move advances (a combatant playing a damage animation doesn't);
//   2. movement resolves for every move reaching its move time - anyone stepping out of a cell before anyone
//      stepping into it; a move whose cell is taken stops at the nearest free cell on the way, or stays put;
//   3. every impact / effect time resolves against the updated positions - simultaneous impacts all land;
//   4. hit reactions (damage animation + set-back, or a block's skip) and on-hit statuses apply with each hit;
//   5. statuses tick, stamina comes back for anyone in Hold;
//   6. the defeated are removed and their cells freed;
//   7. finished moves hand over to the next in the chain; a finished chain asks for a new one.
// Movement resolving before impact means stepping out at the exact impact time still escapes.
//
// Hit reactions (the target's state decides - Combat/Move.wren has the per-move overrides):
//   inside a dodge window      not hit
//   inside a defensive window  the move skips forward by its own skip time; no damage animation
//   in Hold (or idle)          damage animation only, no set-back
//   before its impact          damage animation, then resume S earlier (never before 0 s)
//   past its impact            damage animation, then resume S earlier (never before the impact - it can't fire twice)
//   in a defend move's recovery  ... never before the window's end, so the window never reopens
// Set-backs within 2 s of each other have diminishing returns (100% / 50% / 25% / 0%); the third stagger inside 2 s
// buys 1 s of immunity to damage animations so nobody can be locked in them.
import "Combat/Move" for Move
import "Combat/Combatant" for ChainEntry
import "Combat/Chain" for Grid
import "Combat/Rules" for Rules, STAGGER_TO_STAMINA

var TICK = 1 / 120
var CHAIN_BUDGET = 4.0
var EPS = 0.00001
var HOLD_DAMAGE_ANIM = 0.3
var DR_WINDOW = 2
var DR_FACTORS = [1, 0.5, 0.25, 0]
var DEFLECT_GUARD = 0.8        // how close to its peak a parry must be to Deflect

class BattleEngine {
    construct new(lanes, rows) {
        _lanes = lanes
        _rows = rows
        _fighters = []
        _events = []
        _clock = 0
        _acc = 0
        _budget = CHAIN_BUDGET
        _rules = Rules.new(this)
        _specials = {}
        _defeatHooks = []
        _result = null
    }

    lanes { _lanes }
    rows { _rows }
    fighters { _fighters }
    clock { _clock }
    budget { _budget }
    budget=(value) { _budget = value }
    rules { _rules }

    // Adds a combatant to the board. side 0 = the party, 1 = the opposition.
    add(fighter, side, lane, row) {
        fighter.beginBattle(side, lane, row, forward(side))
        if (fighter.loadout.main != null && fighter.loadout.main.type.id == "crossbow") fighter.setCounter("loaded", 1)
        _fighters.add(fighter)
        return fighter
    }

    // A combatant that joins mid-fight (a boss shard): like add, plus an event for the stage.
    spawn(fighter, side, lane, row) {
        add(fighter, side, lane, row)
        emit_("spawn", fighter, null)
        return fighter
    }

    // Lines a side up in its back row, the first in the middle lane, the rest either side of it, spilling into the
    // next row if there are more of them than lanes.
    deploy(list, side) {
        var order = laneOrder_
        var i = 0
        for (fighter in list) {
            var depth = (i / _lanes).floor
            var row = side == 0 ? depth : _rows - 1 - depth
            add(fighter, side, order[i % _lanes], row)
            i = i + 1
        }
    }

    laneOrder_ {
        var mid = ((_lanes - 1) / 2).floor
        var order = [mid]
        var step = 1
        while (order.count < _lanes) {
            if (mid - step >= 0) order.add(mid - step)
            if (mid + step < _lanes) order.add(mid + step)
            step = step + 1
        }
        return order
    }

    // The next stage of a staged encounter (the lessons): the fallen leave the board, the party lines up afresh at
    // full stamina with clean chains and statuses, and the fight is open again - add the new opposition after this.
    nextStage() {
        var party = _fighters.where { |fighter| fighter.side == 0 && fighter.active }.toList
        _fighters = []
        _events = []
        _result = null
        for (fighter in party) {
            fighter.stats.gainStamina(fighter.stats.maxStamina)
        }
        deploy(party, 0)
    }

    // A move's special effect ("sunder", "reform"...): fn(engine, fighter, move), called at the move's impact time.
    onSpecial(tag, fn) { _specials[tag] = fn }
    // fn(engine, fighter) whenever a combatant is defeated.
    onDefeat(fn) { _defeatHooks.add(fn) }

    // --- events ----------------------------------------------------------------------------

    emit_(type, who, data) {
        var event = data == null ? {} : data
        event["type"] = type
        event["who"] = who
        event["time"] = _clock
        _events.add(event)
    }

    // What happened since the last call - the stage plays these, the screen logs them.
    takeEvents() {
        var out = _events
        _events = []
        return out
    }

    // Rules' side channel: a named reaction popped on `t`, a status changed, healing landed.
    note(target, text) { emit_("note", target, {"text": text}) }
    status(target, text) { emit_("status", target, {"text": text}) }
    healed(fighter, amount) {
        if (amount >= 1) emit_("heal", fighter, {"amount": amount.round})
    }
    say(text) { emit_("say", null, {"text": text}) }
    // The combo an AI combatant just went into - its name is the tell.
    combo(fighter, text) { emit_("combo", fighter, {"text": text}) }

    // --- the board -------------------------------------------------------------------------

    forward(side) { side == 0 ? 1 : -1 }
    inBounds(lane, row) { lane >= 0 && lane < _lanes && row >= 0 && row < _rows }
    frontRow(side) { side == 0 ? (_rows / 2).floor - 1 : (_rows / 2).floor }
    inFrontRow(fighter) { fighter.row == frontRow(fighter.side) }
    inEnemyHalf(fighter) { fighter.side == 0 ? fighter.row >= (_rows / 2).floor : fighter.row < (_rows / 2).floor }

    occupant(lane, row) {
        for (fighter in _fighters) {
            if (fighter.active && fighter.lane == lane && fighter.row == row) return fighter
        }
        return null
    }

    // Unoccupied (by anyone but `mover`) and not claimed by someone else's movement.
    isFree(lane, row, mover) {
        if (!inBounds(lane, row)) return false
        for (fighter in _fighters) {
            if (!fighter.active || fighter == mover) continue
            if (fighter.lane == lane && fighter.row == row) return false
            if (fighter.claim != null && fighter.claim[0] == lane && fighter.claim[1] == row) return false
        }
        return true
    }

    side(index) { _fighters.where { |fighter| fighter.side == index && fighter.active }.toList }
    enemiesOf(fighter) { _fighters.where { |other| other.side != fighter.side && other.active }.toList }
    alliesOf(fighter) { _fighters.where { |other| other.side == fighter.side && other.active && other != fighter }.toList }

    // Everyone in the eight cells around `f`.
    neighbours(fighter) {
        return _fighters.where { |other| other != fighter && other.active && (other.lane - fighter.lane).abs <= 1 && (other.row - fighter.row).abs <= 1 }.toList
    }
    adjacentEnemies(fighter) { neighbours(fighter).where { |other| other.side != fighter.side }.toList }
    // `t`'s own side standing in the lanes either side of it, same row.
    besideEnemies(target) {
        return _fighters.where { |other| other != target && other.active && other.side == target.side && other.row == target.row && (other.lane - target.lane).abs == 1 }.toList
    }
    // `a`'s opponents in the cells either side of [lane, row].
    enemiesBesideCell(fighter, lane, row) {
        return _fighters.where { |other| other.active && other.side != fighter.side && other.row == row && (other.lane - lane).abs == 1 }.toList
    }

    // True when `a` stands behind `t`, relative to the way `t` faces.
    isBehind(attacker, target) { (attacker.row - target.row) * target.facing < 0 }

    // The [lane, row] step directly away from `a`, as seen from `t`.
    awayFrom(attacker, target) {
        var dl = (target.lane - attacker.lane).sign
        var dr = (target.row - attacker.row).sign
        if (dl == 0 && dr == 0) dr = attacker.facing
        return [dl, dr]
    }

    // Turn-to-face: the nearest opponent along the lane decides; with none in the lane, the nearest by row.
    facingAt(fighter, lane, row) {
        var best = null
        var bestDistance = 999
        var inLane = false
        for (other in _fighters) {
            if (!other.active || other.side == fighter.side || other.row == row) continue
            var sameLane = other.lane == lane
            var distance = (other.row - row).abs
            if ((sameLane && !inLane) || (sameLane == inLane && distance < bestDistance)) {
                best = other
                bestDistance = distance
                inLane = sameLane
            }
        }
        if (best == null) return fighter.facing
        return best.row > row ? 1 : -1
    }

    // Moves `t` one cell by [dl, dr] if it can be moved and the cell is free. Returns whether it moved.
    push(target, dl, dr) {
        if (!target.active || target.has("immovable")) return false
        var lane = target.lane + dl
        var row = target.row + dr
        if (!isFree(lane, row, target)) return false
        relocate_(target, lane, row, "push")
        return true
    }

    relocate_(fighter, lane, row, how) {
        var from = [fighter.lane, fighter.row]
        fighter.lane = lane
        fighter.row = row
        emit_("move", fighter, {"from": from, "how": how})
    }

    // --- cells a move covers -----------------------------------------------------------------

    // [[lane, row, share, isAim, isDepth], ...] for `move` used by `f` standing at [lane, row] facing `facing`.
    // The aim cell takes 100%; the stencil adds the rest, the higher share winning where they overlap.
    cellsAt(fighter, move, lane, row, facing) {
        var out = []
        if (!move.isAttack && !move.isHeal) return out
        if (move.aim == "self") {
            out.add([lane, row, 1, true, false])
            return out
        }
        if (move.aim == "around") {
            for (cell in move.stencil) {
                addCell_(out, lane + cell[1], row + facing * cell[0], cell[2], true, false)
            }
            return out
        }
        var aimRow = row + facing * move.reach
        if (move.aim == "first") {
            aimRow = null
            var targetRow = row + facing
            var last = null
            while (inBounds(lane, targetRow)) {
                last = targetRow
                var who = occupant(lane, targetRow)
                if (who != null && who != fighter && ((who.side != fighter.side) == move.isAttack)) {
                    aimRow = targetRow
                    break
                }
                targetRow = targetRow + facing
            }
            if (aimRow == null) aimRow = last
            if (aimRow == null) return out
        }
        addCell_(out, lane, aimRow, 1, true, false)
        for (cell in move.stencil) {
            addCell_(out, lane + cell[1], aimRow + facing * cell[0], cell[2], false, cell[1] == 0)
        }
        // What the weapon adds: Piercing, the warbow's piercing Aimed Shot, the pike's long reach.
        var pierce = Rules.r([0, 0.2, 0.3, 0.4], fighter.loadout.weaponRank("Piercing"))
        var item = fighter.loadout.itemFor(move)[1]
        if (item != null) {
            if (item.type.id == "warbow" && move.id == "Aimed Shot") pierce = pierce.max(0.5)
            if (item.type.id == "pike" && move.aim == "reach" && move.reach >= 2) pierce = pierce.max(0.5)
        }
        if (pierce > 0 && move.isAttack) addCell_(out, lane, aimRow + facing, pierce, false, true)
        return out
    }

    addCell_(out, lane, row, share, isAim, isDepth) {
        if (!inBounds(lane, row)) return
        for (cell in out) {
            if (cell[0] == lane && cell[1] == row) {
                if (share > cell[2]) cell[2] = share
                return
            }
        }
        out.add([lane, row, share, isAim, isDepth])
    }

    // --- chains ------------------------------------------------------------------------------

    // Human-controlled combatants waiting on a chain - while there are any, the clock is stopped.
    waiting { _fighters.where { |fighter| fighter.active && fighter.needsChain && fighter.brain == null && !fighter.has("passive") }.toList }

    // Commits `entries` (ChainEntry list) as `f`'s chain and starts its first move.
    commit(fighter, entries) {
        if (entries == null || entries.count == 0) return
        fighter.setChain(entries)
        _rules.onChainStart(fighter)
        startNext_(fighter, 0)
        answerChainStart_(fighter)
    }

    // A prop that snaps at a chain started in its reach (Thomas's snap-post) starts its move as `starter`'s chain
    // starts - like a starving wolf biting the moment you move next to it.
    answerChainStart_(starter) {
        for (prop in _fighters) {
            if (prop.side == starter.side || !prop.active || prop.move != null || !prop.has("snaps")) continue
            var move = prop.trait("snaps").call()
            var cells = cellsAt(prop, move, prop.lane, prop.row, facingAt(prop, prop.lane, prop.row))
            if (cells.any { |cell| cell[0] == starter.lane && cell[1] == starter.row }) {
                commit(prop, [ChainEntry.new(move, null)])
            }
        }
    }

    // Lets every AI combatant that needs a chain pick one. Called before the clock starts and every step.
    think() {
        for (fighter in _fighters) {
            if (fighter.active && fighter.needsChain && fighter.brain != null) {
                var entries = fighter.brain.chainFor(this, fighter)
                if (entries != null) commit(fighter, entries)
            }
        }
    }

    // Starts the chain's current move, `carry` seconds into it (what the last move overran its completion by).
    startNext_(fighter, carry) {
        var entry = fighter.chain[fighter.chainIndex]
        var move = entry.move
        fighter.facing = facingAt(fighter, fighter.lane, fighter.row)

        var cost = _rules.moveCost(fighter, move, true)
        var falter = !fighter.has("noStamina") && fighter.stats.stamina < cost - EPS
        if (move.special == "bolt" && fighter.counter("loaded") < 1) falter = true
        if (falter) {
            // It can't be paid for: a Falter - a 0.5 s stumble with no effect.
            move = Move.falter()
            cost = 0
            emit_("falter", fighter, null)
        } else {
            fighter.stats.spendStamina(cost)
            if (move.special == "bolt") fighter.setCounter("loaded", 0)
        }
        fighter.startMove(entry, move)
        fighter.cost = cost
        fighter.progress = carry
        fighter.startedAfterHold = fighter.lastWasHold
        fighter.lastWasHold = move.isHold

        var lane = fighter.lane
        var row = fighter.row
        if (move.isMovement) {
            var dest = destinationFor_(fighter, entry, move)
            entry.dest = dest
            if (dest != null && isFree(dest[0], dest[1], fighter)) {
                fighter.claim = dest
                if (move.impact == null || move.moveTime <= move.impact) {
                    lane = dest[0]
                    row = dest[1]
                }
            }
        }
        fighter.cells = cellsAt(fighter, move, lane, row, facingAt(fighter, lane, row))
        _rules.onMoveStart(fighter, move)
        emit_("start", fighter, {"move": move})
    }

    // Where a movement move is headed: a fixed pattern's forced cell, the cell the player picked, or an AI hint
    // ("toward" / "away" / "side" / a [lane, row]) resolved against the board as it is now.
    destinationFor_(fighter, entry, move) {
        if (Grid.fixed(move.pattern)) {
            var offset = Grid.offsets(move.pattern, fighter.facing)[0]
            return [fighter.lane + offset[0], fighter.row + offset[1]]
        }
        var dest = entry.dest
        if (dest is List) return dest
        if (dest is String) return hintedCell_(fighter, move, dest)
        if (dest is Fn) return dest.call(this, fighter)
        return null
    }

    hintedCell_(fighter, move, hint) {
        var target = nearestOpponent(fighter)
        var best = null
        var bestScore = null
        for (offset in Grid.offsets(move.pattern, fighter.facing)) {
            var lane = fighter.lane + offset[0]
            var row = fighter.row + offset[1]
            if (!isFree(lane, row, fighter)) continue
            var score = 0
            if (target != null) {
                // Distance to the cell in front of the target (where a Reach 1 attack can hit it from).
                var rowGap = (target.row - row).abs
                var laneGap = (target.lane - lane).abs
                var distance = laneGap * 2 + (rowGap - 1).abs
                if (rowGap == 0) distance = distance + 2
                if (hint == "toward") score = -distance
                if (hint == "away") score = rowGap * 2 + laneGap
                if (hint == "side") score = offset[0] != 0 ? 5 - distance : -distance
            }
            if (bestScore == null || score > bestScore) {
                best = [lane, row]
                bestScore = score
            }
        }
        return best
    }

    nearestOpponent(fighter) {
        var best = null
        var bestDistance = 999
        for (other in _fighters) {
            if (!other.active || other.side == fighter.side) continue
            var distance = (other.lane - fighter.lane).abs * 2 + (other.row - fighter.row).abs
            if (distance < bestDistance) {
                best = other
                bestDistance = distance
            }
        }
        return best
    }

    // --- the clock -----------------------------------------------------------------------------

    // "won" / "lost" once a side has nobody left, else null.
    result { _result }

    // Advances the fight by `dt` seconds of game time (in TICK steps). Does nothing while a human owes a chain.
    update(dt) {
        if (_result != null) return
        think()
        _acc = _acc + dt
        var steps = 0
        while (_acc >= TICK && steps < 30) {
            if (_result != null || waiting.count > 0) {
                _acc = 0
                return
            }
            step_(TICK)
            _acc = _acc - TICK
            steps = steps + 1
        }
        if (steps >= 30) _acc = 0
    }

    step_(dt) {
        _clock = _clock + dt

        // 1. advance
        for (fighter in _fighters) {
            if (!fighter.active || fighter.move == null) continue
            if (fighter.stun > 0) {
                fighter.stun = fighter.stun - dt
                if (fighter.stun < 0) {
                    fighter.progress = fighter.progress - fighter.stun
                    fighter.stun = 0
                }
            } else {
                fighter.progress = fighter.progress + dt
            }
        }

        // 2. movement - whoever's cell is free goes first, which frees theirs for the next; whoever is still blocked
        // once nobody else can go falls back (applyMovement_).
        var pending = []
        for (fighter in _fighters) {
            var move = fighter.move
            if (fighter.active && move != null && move.isMovement && !fighter.moved && fighter.progress >= move.moveTime - EPS) {
                pending.add(fighter)
            }
        }
        var progressed = true
        while (pending.count > 0 && progressed) {
            progressed = false
            var still = []
            for (fighter in pending) {
                var dest = fighter.entry.dest
                if (dest is List && isFree(dest[0], dest[1], fighter) && pathClear_(fighter, dest)) {
                    applyMovement_(fighter)
                    progressed = true
                } else {
                    still.add(fighter)
                }
            }
            pending = still
        }
        for (fighter in pending) {
            applyMovement_(fighter)
        }

        // 3 + 4. impacts - everything due this step is collected first, so simultaneous impacts all land
        var due = []
        for (fighter in _fighters) {
            var move = fighter.move
            if (fighter.active && move != null && move.impact != null && !fighter.impactDone && fighter.progress >= move.impact - EPS) {
                fighter.impactDone = true
                due.add(fighter)
            }
        }
        for (fighter in due) {
            resolveImpact_(fighter)
        }

        // 5. statuses, and stamina for anyone in Hold
        _rules.tick(dt)
        for (fighter in _fighters) {
            if (fighter.active && fighter.move != null && fighter.move.isHold && fighter.stun <= 0 && !fighter.has("noStamina")) {
                fighter.stats.gainStamina(_rules.holdRecovery(fighter) * dt)
            }
        }

        // 6. the defeated
        sweepDefeated_()

        // 7. completions
        for (fighter in _fighters) {
            var move = fighter.move
            if (fighter.active && move != null && fighter.stun <= 0 && fighter.progress >= move.completion - EPS) {
                completeMove_(fighter)
            }
        }
        think()

        if (side(0).count == 0) {
            _result = "lost"
        } else if (side(1).count == 0) {
            _result = "won"
        }
    }

    completeMove_(fighter) {
        var move = fighter.move
        var over = fighter.progress - move.completion
        if (over < 0) over = 0
        if (move.special == "reload") fighter.setCounter("loaded", 1)
        emit_("end", fighter, {"move": move})
        fighter.clearMove()
        fighter.chainIndex = fighter.chainIndex + 1
        if (fighter.chainIndex < fighter.chain.count) {
            startNext_(fighter, over)
        } else {
            fighter.needsChain = true
            emit_("chainEnd", fighter, null)
        }
    }

    sweepDefeated_() {
        for (fighter in _fighters) {
            if (fighter.stats.hp > 0 || fighter.counter("dead") > 0 || fighter.removed) continue
            fighter.setCounter("dead", 1)
            fighter.clearMove()
            fighter.clearStatuses()
            emit_("defeat", fighter, null)
            for (hook in _defeatHooks) {
                hook.call(this, fighter)
            }
            // A boss's shared pool running dry takes every part with it.
            for (part in _fighters) {
                if (part.active && part.trait("pool") == fighter) part.stats.takeDamage(part.stats.hp)
            }
        }
    }

    // --- movement ------------------------------------------------------------------------------

    applyMovement_(fighter) {
        fighter.moved = true
        var move = fighter.move
        var dest = fighter.entry.dest
        fighter.claim = null
        if (!(dest is List)) return
        var target = null
        if (isFree(dest[0], dest[1], fighter) && pathClear_(fighter, dest)) {
            target = dest
        } else {
            // Taken (the plan couldn't know where the opposition would be): the nearest free cell on the way, if any.
            target = fallbackCell_(fighter, dest)
        }
        if (target == null) {
            emit_("blocked", fighter, null)
            return
        }
        var from = [fighter.lane, fighter.row]
        relocate_(fighter, target[0], target[1], "move")
        // The cells were locked for where the move expected to stand; if that isn't where it ended up, re-lock.
        if (!fighter.impactDone && move.impact != null && (move.isAttack || move.isHeal)) {
            fighter.cells = cellsAt(fighter, move, fighter.lane, fighter.row, fighter.facing)
        }
    }

    // The cells between where `fighter` stands and `dest`, nearest `dest` first: the midpoint of a two-cell trip.
    static pathCells_(fighter, dest) {
        var laneGap = dest[0] - fighter.lane
        var rowGap = dest[1] - fighter.row
        var steps = laneGap.abs.max(rowGap.abs)
        var out = []
        var step = steps - 1
        while (step >= 1) {
            out.add([fighter.lane + (laneGap * step / steps).round, fighter.row + (rowGap * step / steps).round])
            step = step - 1
        }
        return out
    }

    // A walk along the rows (forward2) needs every cell on the way free; a jump (blink) doesn't.
    pathClear_(fighter, dest) {
        if (fighter.move.pattern != "forward2") return true
        for (cell in BattleEngine.pathCells_(fighter, dest)) {
            if (!isFree(cell[0], cell[1], fighter)) return false
        }
        return true
    }

    // Where a blocked move ends up: the free cell on the way nearest its destination - for a walk, only as far as the
    // way is clear - or null to stay put.
    fallbackCell_(fighter, dest) {
        var walk = fighter.move.pattern == "forward2"
        var cells = BattleEngine.pathCells_(fighter, dest)
        if (walk) {
            // Walking: the furthest cell reached before the first taken one.
            var reached = null
            var i = cells.count - 1
            while (i >= 0 && isFree(cells[i][0], cells[i][1], fighter)) {
                reached = cells[i]
                i = i - 1
            }
            return reached
        }
        for (cell in cells) {
            if (isFree(cell[0], cell[1], fighter)) return cell
        }
        return null
    }

    // --- impacts -------------------------------------------------------------------------------

    resolveImpact_(attacker) {
        var move = attacker.move
        if (move == null) return
        if (move.special != null && _specials.containsKey(move.special)) {
            _specials[move.special].call(this, attacker, move)
        }
        if (move.isAttack) {
            // One hit per target, at the best share of the cells it stands in.
            var hits = []
            for (cell in attacker.cells) {
                var target = occupant(cell[0], cell[1])
                if (target == null || target.side == attacker.side) continue
                var known = false
                for (hit in hits) {
                    if (hit[0] == target) {
                        known = true
                        if (cell[2] > hit[1][2]) hit[1] = cell
                    }
                }
                if (!known) hits.add([target, cell])
            }
            if (hits.count == 0) emit_("miss", attacker, null)
            for (hit in hits) {
                hit_(attacker, hit[0], move, hit[1])
            }
        }
        if (move.isHeal) {
            for (cell in attacker.cells) {
                var target = occupant(cell[0], cell[1])
                if (target == null || target.side != attacker.side) continue
                var amount = move.special == "potion" ? (target.stats.maxHp * move.healAmount / 100).round : _rules.healAmount(attacker, move, cell[2])
                var over = target.stats.heal(amount)
                healed(target, amount - over)
            }
        }
        if (move.special == "salts") attacker.clearStatuses()
        if (move.item != null) emit_("item", attacker, {"item": move.item})
    }

    hit_(attacker, target, move, cell) {
        if (target.dodgingNow) {
            emit_("dodge", target, {"by": attacker})
            _rules.onDodge(target, attacker)
            return
        }
        // The first hit from one named attacker is caught outright, whatever the timing (Finn's tongs - "catches").
        if (target.has("catches") && target.trait("catches") == attacker.name && target.counter("caught") == 0) {
            target.setCounter("caught", 1)
            emit_("caught", target, {"by": attacker})
            return
        }
        var share = cell[2]
        var guard = target.stun > 0 ? 0 : target.guardNow
        var own = _rules.blockValue(target, guard)

        // Guard range: an ally's Defend move covering this cell - the stronger of the two applies.
        var cover = 0
        var guarder = null
        var coverGuard = 0
        for (defender in alliesOf(target)) {
            if (defender.stun > 0 || defender.move == null || !defender.move.isDefend) continue
            var gu = defender.guardNow
            if (gu <= 0) continue
            var value = _rules.blockValue(defender, gu) * coverShare_(defender, target)
            if (value > cover) {
                cover = value
                guarder = defender
                coverGuard = gu
            }
        }
        var covered = cover > own
        var block = covered ? cover : own

        var stagger = _rules.staggerOf(attacker, target, move, share)
        var result = _rules.damage(attacker, target, move, share, cell[3], cell[4], block, covered ? coverGuard : guard)
        var dealt = result[0]
        applyDamage_(target, dealt)
        var element = _rules.weaponFor(attacker, move)[2]
        emit_("hit", target, {"by": attacker, "amount": dealt, "blocked": block > 0, "guard": covered ? coverGuard : guard, "move": move,
            "element": element})
        if (dealt > 0) _rules.onDamaged(target, attacker, dealt)

        if (covered) {
            // The cover was used: the guarder takes the skip and the ally isn't staggered.
            _rules.onBlock(guarder, attacker, coverGuard, peakDistance_(guarder), stagger, dealt)
            blockReaction_(guarder, attacker, coverGuard, stagger)
        } else if (guard > 0) {
            _rules.onBlock(target, attacker, guard, peakDistance_(target), stagger, dealt)
            blockReaction_(target, attacker, guard, stagger)
        } else {
            stagger_(target, stagger, attacker)
        }
        if (dealt > 0) _rules.onHit(attacker, target, move, dealt, cell, cell[3], stagger)
        // A Fire Flask sets alight whatever it splashes.
        if (move.special == "flask") _rules.addBurn(target, attacker, 1, false)

        if (move.push > 0 && target.active) {
            var dir = awayFrom(attacker, target)
            push(target, dir[0], dir[1])
        }
        if (move.special == "throw" && target.active && !target.has("immovable")) {
            // Throw: the target lands in the cell behind the thrower, if it is free.
            var row = attacker.row - attacker.facing
            if (isFree(attacker.lane, row, target)) relocate_(target, attacker.lane, row, "push")
        }
        // A reactive prop (the swing-post) answers a hit taken while idle.
        if (target.active && target.move == null && target.has("reactive")) {
            target.stun = 0
            commit(target, [ChainEntry.new(target.trait("reactive").call(), null)])
        }
    }

    peakDistance_(defender) { (defender.progress - defender.move.window[1]).abs }

    // A hit landed inside `u`'s defensive window: the skip (or Absorb's stamina), and Deflect near the peak.
    blockReaction_(defender, attacker, guard, stagger) {
        var move = defender.move
        if (move.reaction == "absorb") {
            defender.stats.gainStamina(stagger * STAGGER_TO_STAMINA)
        } else if (!attacker.loadout.capstone("Breaker")) {
            var to = defender.progress + move.skip + _rules.skipBonus(defender)
            defender.progress = to > move.completion ? move.completion : to
        }
        emit_("block", defender, {"by": attacker, "guard": guard})
        var deflects = move.reaction == "deflect" && guard >= DEFLECT_GUARD
        if (defender.loadout.capstone("Riposte") && guard >= 0.9) deflects = true
        if (deflects) {
            emit_("note", defender, {"text": "Deflect"})
            stagger_(attacker, stagger, defender)
        }
    }

    // Guard range: the share of `u`'s block that reaches `t` (0 = not covered).
    coverShare_(defender, target) {
        var move = defender.move
        var cells = []
        if (move != null) {
            for (cell in move.guardRange) cells.add(cell)
        }
        var shield = defender.loadout.shield
        if (shield != null && (move == null || move.addsShield)) {
            if (shield.type.rangeSides > 0) {
                cells.add([0, -1, shield.type.rangeSides])
                cells.add([0, 1, shield.type.rangeSides])
            }
            if (shield.type.rangeBehind > 0) cells.add([-1, 0, shield.type.rangeBehind])
        }
        if (defender.loadout.capstone("Sentinel")) cells.add([-1, 0, 1])
        var best = 0
        for (cell in cells) {
            if (target.at(defender.lane + cell[1], defender.row + defender.facing * cell[0])) {
                var share = _rules.guardShare(defender, cell[2], cell[1])
                if (share > best) best = share
            }
        }
        return best
    }
    covers(defender, target) { coverShare_(defender, target) > 0 }

    applyDamage_(target, amount) {
        if (amount <= 0) return
        var left = amount
        if (target.shield > 0) {
            var soak = left.min(target.shield)
            target.shield = target.shield - soak
            left = left - soak
        }
        target.stats.takeDamage(left)
        // A boss part's damage also comes off the shared pool.
        var pool = target.trait("pool")
        if (pool != null) pool.stats.takeDamage(left)
    }

    // Damage outside a hit (statuses, reactions, Reprisal). tag: shown on the target (null = silent, e.g. a tick).
    dealDirect(source, target, amount, tag) {
        if (!target.active || amount <= 0) return
        applyDamage_(target, amount)
        if (tag != null) emit_("direct", target, {"amount": amount.round, "text": tag})
    }

    // A set-back from something other than a plain hit landing (Thermal Shock, a Deflect, Permafrost...).
    setback(target, seconds, source) {
        if (seconds > 0) stagger_(target, seconds, source)
    }

    // Staggers `t` by `s` seconds: its move's damage animation plays, then the move resumes from a set-back point.
    stagger_(target, staggerAmount, source) {
        if (!target.active) return
        var move = target.move
        if (move == null || move.isHold) {
            // In Hold (or idle): the Hold damage animation only - the Hold just ends that much later.
            if (_clock >= target.animImmuneUntil) target.stun = target.stun.max(HOLD_DAMAGE_ANIM)
            emit_("stagger", target, {"setback": 0})
            return
        }
        var recent = 0
        for (time in target.staggerTimes) {
            if (_clock - time <= DR_WINDOW) recent = recent + 1
        }
        target.staggerTimes.add(_clock)
        if (target.staggerTimes.count > 6) target.staggerTimes.removeAt(0)

        var amount = staggerAmount * DR_FACTORS[recent.min(3)]
        if (!target.has("noStamina") && target.stats.stamina <= 0) amount = amount * 1.5
        if (target.frozen > 0) {
            amount = amount * target.frozen
            target.frozen = 0
        }
        if (source != null && source.counter("halveSetback") > 0) {
            amount = amount * 0.5
            source.setCounter("halveSetback", 0)
        }
        amount = amount * (1 - _rules.setbackReduction(target))
        // Steadfast / Unflinching - which an Exposed boss part loses.
        if (!target.exposedNow) {
            if (move.reaction == "steadfast") amount = amount * 0.5
            if (move.reaction == "unflinching" && target.impactDone) amount = 0
        }
        // Focus capstone: a spell started straight after a Hold can't be set back before its impact.
        if (target.loadout.capstone("Focus") && target.startedAfterHold && move.scaling == "mana" && !target.impactDone) amount = 0

        var floor = 0
        if (move.impact != null && target.impactDone) {
            floor = move.impact
        } else if (move.window != null && target.progress > move.window[2]) {
            floor = move.window[2]
        }
        var back = amount.min(target.progress - floor)
        if (back < 0) back = 0
        target.progress = target.progress - back
        if (_clock >= target.animImmuneUntil) {
            target.stun = target.stun.max(move.damageAnim)
            if (recent >= 2) target.animImmuneUntil = _clock + move.damageAnim + 1
        }
        // Opportunist capstone: set back during the wind-up counts as "in recovery" for 1 s.
        if (source != null && !target.impactDone && source.loadout.capstone("Opportunist")) {
            target.setCounter("recoverUntil", _clock + 1)
        }
        emit_("stagger", target, {"setback": back})
    }
}
