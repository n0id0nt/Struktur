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
//   2. movement resolves for every move reaching its move time;
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
    budget=(v) { _budget = v }
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
        var party = _fighters.where { |f| f.side == 0 && f.active }.toList
        _fighters = []
        _events = []
        _result = null
        for (f in party) {
            f.stats.gainStamina(f.stats.maxStamina)
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
    note(t, text) { emit_("note", t, {"text": text}) }
    status(t, text) { emit_("status", t, {"text": text}) }
    healed(f, amount) {
        if (amount >= 1) emit_("heal", f, {"amount": amount.round})
    }
    say(text) { emit_("say", null, {"text": text}) }
    // The combo an AI combatant just went into - its name is the tell.
    combo(f, text) { emit_("combo", f, {"text": text}) }

    // --- the board -------------------------------------------------------------------------

    forward(side) { side == 0 ? 1 : -1 }
    inBounds(lane, row) { lane >= 0 && lane < _lanes && row >= 0 && row < _rows }
    frontRow(side) { side == 0 ? (_rows / 2).floor - 1 : (_rows / 2).floor }
    inFrontRow(f) { f.row == frontRow(f.side) }
    inEnemyHalf(f) { f.side == 0 ? f.row >= (_rows / 2).floor : f.row < (_rows / 2).floor }

    occupant(lane, row) {
        for (f in _fighters) {
            if (f.active && f.lane == lane && f.row == row) return f
        }
        return null
    }

    // Unoccupied (by anyone but `mover`) and not claimed by someone else's movement.
    isFree(lane, row, mover) {
        if (!inBounds(lane, row)) return false
        for (f in _fighters) {
            if (!f.active || f == mover) continue
            if (f.lane == lane && f.row == row) return false
            if (f.claim != null && f.claim[0] == lane && f.claim[1] == row) return false
        }
        return true
    }

    side(index) { _fighters.where { |f| f.side == index && f.active }.toList }
    enemiesOf(f) { _fighters.where { |o| o.side != f.side && o.active }.toList }
    alliesOf(f) { _fighters.where { |o| o.side == f.side && o.active && o != f }.toList }

    // Everyone in the eight cells around `f`.
    neighbours(f) {
        return _fighters.where { |o| o != f && o.active && (o.lane - f.lane).abs <= 1 && (o.row - f.row).abs <= 1 }.toList
    }
    adjacentEnemies(f) { neighbours(f).where { |o| o.side != f.side }.toList }
    // `t`'s own side standing in the lanes either side of it, same row.
    besideEnemies(t) {
        return _fighters.where { |o| o != t && o.active && o.side == t.side && o.row == t.row && (o.lane - t.lane).abs == 1 }.toList
    }
    // `a`'s opponents in the cells either side of [lane, row].
    enemiesBesideCell(a, lane, row) {
        return _fighters.where { |o| o.active && o.side != a.side && o.row == row && (o.lane - lane).abs == 1 }.toList
    }

    // True when `a` stands behind `t`, relative to the way `t` faces.
    isBehind(a, t) { (a.row - t.row) * t.facing < 0 }

    // The [lane, row] step directly away from `a`, as seen from `t`.
    awayFrom(a, t) {
        var dl = (t.lane - a.lane).sign
        var dr = (t.row - a.row).sign
        if (dl == 0 && dr == 0) dr = a.facing
        return [dl, dr]
    }

    // Turn-to-face: the nearest opponent along the lane decides; with none in the lane, the nearest by row.
    facingAt(f, lane, row) {
        var best = null
        var bestDistance = 999
        var inLane = false
        for (o in _fighters) {
            if (!o.active || o.side == f.side || o.row == row) continue
            var sameLane = o.lane == lane
            var distance = (o.row - row).abs
            if ((sameLane && !inLane) || (sameLane == inLane && distance < bestDistance)) {
                best = o
                bestDistance = distance
                inLane = sameLane
            }
        }
        if (best == null) return f.facing
        return best.row > row ? 1 : -1
    }

    // Moves `t` one cell by [dl, dr] if it can be moved and the cell is free. Returns whether it moved.
    push(t, dl, dr) {
        if (!t.active || t.has("immovable")) return false
        var lane = t.lane + dl
        var row = t.row + dr
        if (!isFree(lane, row, t)) return false
        relocate_(t, lane, row, "push")
        return true
    }

    relocate_(f, lane, row, how) {
        var from = [f.lane, f.row]
        f.lane = lane
        f.row = row
        emit_("move", f, {"from": from, "how": how})
    }

    // --- cells a move covers -----------------------------------------------------------------

    // [[lane, row, share, isAim, isDepth], ...] for `move` used by `f` standing at [lane, row] facing `facing`.
    // The aim cell takes 100%; the stencil adds the rest, the higher share winning where they overlap.
    cellsAt(f, move, lane, row, facing) {
        var out = []
        if (!move.isAttack && !move.isHeal) return out
        if (move.aim == "self") {
            out.add([lane, row, 1, true, false])
            return out
        }
        if (move.aim == "around") {
            for (c in move.stencil) {
                addCell_(out, lane + c[1], row + facing * c[0], c[2], true, false)
            }
            return out
        }
        var aimRow = row + facing * move.reach
        if (move.aim == "first") {
            aimRow = null
            var r = row + facing
            var last = null
            while (inBounds(lane, r)) {
                last = r
                var who = occupant(lane, r)
                if (who != null && who != f && ((who.side != f.side) == move.isAttack)) {
                    aimRow = r
                    break
                }
                r = r + facing
            }
            if (aimRow == null) aimRow = last
            if (aimRow == null) return out
        }
        addCell_(out, lane, aimRow, 1, true, false)
        for (c in move.stencil) {
            addCell_(out, lane + c[1], aimRow + facing * c[0], c[2], false, c[1] == 0)
        }
        // What the weapon adds: Piercing, the warbow's piercing Aimed Shot, the pike's long reach.
        var pierce = Rules.r([0, 0.2, 0.3, 0.4], f.loadout.weaponRank("Piercing"))
        var item = f.loadout.itemFor(move)[1]
        if (item != null) {
            if (item.type.id == "warbow" && move.id == "Aimed Shot") pierce = pierce.max(0.5)
            if (item.type.id == "pike" && move.aim == "reach" && move.reach >= 2) pierce = pierce.max(0.5)
        }
        if (pierce > 0 && move.isAttack) addCell_(out, lane, aimRow + facing, pierce, false, true)
        return out
    }

    addCell_(out, lane, row, share, isAim, isDepth) {
        if (!inBounds(lane, row)) return
        for (c in out) {
            if (c[0] == lane && c[1] == row) {
                if (share > c[2]) c[2] = share
                return
            }
        }
        out.add([lane, row, share, isAim, isDepth])
    }

    // --- chains ------------------------------------------------------------------------------

    // Human-controlled combatants waiting on a chain - while there are any, the clock is stopped.
    waiting { _fighters.where { |f| f.active && f.needsChain && f.brain == null && !f.has("passive") }.toList }

    // Commits `entries` (ChainEntry list) as `f`'s chain and starts its first move.
    commit(f, entries) {
        if (entries == null || entries.count == 0) return
        f.setChain(entries)
        _rules.onChainStart(f)
        startNext_(f, 0)
    }

    // Lets every AI combatant that needs a chain pick one. Called before the clock starts and every step.
    think() {
        for (f in _fighters) {
            if (f.active && f.needsChain && f.brain != null) {
                var entries = f.brain.chainFor(this, f)
                if (entries != null) commit(f, entries)
            }
        }
    }

    // Starts the chain's current move, `carry` seconds into it (what the last move overran its completion by).
    startNext_(f, carry) {
        var entry = f.chain[f.chainIndex]
        var move = entry.move
        f.facing = facingAt(f, f.lane, f.row)

        var cost = _rules.moveCost(f, move, true)
        var falter = !f.has("noStamina") && f.stats.stamina < cost - EPS
        if (move.special == "bolt" && f.counter("loaded") < 1) falter = true
        if (falter) {
            // It can't be paid for: a Falter - a 0.5 s stumble with no effect.
            move = Move.falter()
            cost = 0
            emit_("falter", f, null)
        } else {
            f.stats.spendStamina(cost)
            if (move.special == "bolt") f.setCounter("loaded", 0)
        }
        f.startMove(entry, move)
        f.cost = cost
        f.progress = carry
        f.startedAfterHold = f.lastWasHold
        f.lastWasHold = move.isHold

        var lane = f.lane
        var row = f.row
        if (move.isMovement) {
            var dest = destinationFor_(f, entry, move)
            entry.dest = dest
            if (dest != null && isFree(dest[0], dest[1], f)) {
                f.claim = dest
                if (move.impact == null || move.moveTime <= move.impact) {
                    lane = dest[0]
                    row = dest[1]
                }
            }
        }
        f.cells = cellsAt(f, move, lane, row, facingAt(f, lane, row))
        _rules.onMoveStart(f, move)
        emit_("start", f, {"move": move})
    }

    // Where a movement move is headed: a fixed pattern's forced cell, the cell the player picked, or an AI hint
    // ("toward" / "away" / "side" / a [lane, row]) resolved against the board as it is now.
    destinationFor_(f, entry, move) {
        if (Grid.fixed(move.pattern)) {
            var offset = Grid.offsets(move.pattern, f.facing)[0]
            return [f.lane + offset[0], f.row + offset[1]]
        }
        var dest = entry.dest
        if (dest is List) return dest
        if (dest is String) return hintedCell_(f, move, dest)
        if (dest is Fn) return dest.call(this, f)
        return null
    }

    hintedCell_(f, move, hint) {
        var target = nearestOpponent(f)
        var best = null
        var bestScore = null
        for (offset in Grid.offsets(move.pattern, f.facing)) {
            var lane = f.lane + offset[0]
            var row = f.row + offset[1]
            if (!isFree(lane, row, f)) continue
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

    nearestOpponent(f) {
        var best = null
        var bestDistance = 999
        for (o in _fighters) {
            if (!o.active || o.side == f.side) continue
            var distance = (o.lane - f.lane).abs * 2 + (o.row - f.row).abs
            if (distance < bestDistance) {
                best = o
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
        for (f in _fighters) {
            if (!f.active || f.move == null) continue
            if (f.stun > 0) {
                f.stun = f.stun - dt
                if (f.stun < 0) {
                    f.progress = f.progress - f.stun
                    f.stun = 0
                }
            } else {
                f.progress = f.progress + dt
            }
        }

        // 2. movement
        for (f in _fighters) {
            var move = f.move
            if (f.active && move != null && move.isMovement && !f.moved && f.progress >= move.moveTime - EPS) {
                applyMovement_(f)
            }
        }

        // 3 + 4. impacts - everything due this step is collected first, so simultaneous impacts all land
        var due = []
        for (f in _fighters) {
            var move = f.move
            if (f.active && move != null && move.impact != null && !f.impactDone && f.progress >= move.impact - EPS) {
                f.impactDone = true
                due.add(f)
            }
        }
        for (f in due) {
            resolveImpact_(f)
        }

        // 5. statuses, and stamina for anyone in Hold
        _rules.tick(dt)
        for (f in _fighters) {
            if (f.active && f.move != null && f.move.isHold && f.stun <= 0 && !f.has("noStamina")) {
                f.stats.gainStamina(_rules.holdRecovery(f) * dt)
            }
        }

        // 6. the defeated
        sweepDefeated_()

        // 7. completions
        for (f in _fighters) {
            var move = f.move
            if (f.active && move != null && f.stun <= 0 && f.progress >= move.completion - EPS) {
                completeMove_(f)
            }
        }
        think()

        if (side(0).count == 0) {
            _result = "lost"
        } else if (side(1).count == 0) {
            _result = "won"
        }
    }

    completeMove_(f) {
        var move = f.move
        var over = f.progress - move.completion
        if (over < 0) over = 0
        if (move.special == "reload") f.setCounter("loaded", 1)
        emit_("end", f, {"move": move})
        f.clearMove()
        f.chainIndex = f.chainIndex + 1
        if (f.chainIndex < f.chain.count) {
            startNext_(f, over)
        } else {
            f.needsChain = true
            emit_("chainEnd", f, null)
        }
    }

    sweepDefeated_() {
        for (f in _fighters) {
            if (f.stats.hp > 0 || f.counter("dead") > 0 || f.removed) continue
            f.setCounter("dead", 1)
            f.clearMove()
            f.clearStatuses()
            emit_("defeat", f, null)
            for (hook in _defeatHooks) {
                hook.call(this, f)
            }
            // A boss's shared pool running dry takes every part with it.
            for (part in _fighters) {
                if (part.active && part.trait("pool") == f) part.stats.takeDamage(part.stats.hp)
            }
        }
    }

    // --- movement ------------------------------------------------------------------------------

    applyMovement_(f) {
        f.moved = true
        var move = f.move
        var dest = f.entry.dest
        f.claim = null
        if (dest == null) return
        var target = null
        if (move.pattern == "forward2") {
            // A fixed two-cell path: all the way, or - stopping short - as far as it is clear.
            var mid = [f.lane, f.row + f.facing]
            if (isFree(mid[0], mid[1], f)) {
                target = isFree(dest[0], dest[1], f) ? dest : (move.blocked == "short" ? mid : null)
            }
        } else if (isFree(dest[0], dest[1], f)) {
            target = dest
        }
        if (target == null) {
            emit_("blocked", f, null)
            return
        }
        var from = [f.lane, f.row]
        relocate_(f, target[0], target[1], "move")
        // The cells were locked for where the move expected to stand; if that isn't where it ended up, re-lock.
        if (!f.impactDone && move.impact != null && (move.isAttack || move.isHeal)) {
            f.cells = cellsAt(f, move, f.lane, f.row, f.facing)
        }
    }

    // --- impacts -------------------------------------------------------------------------------

    resolveImpact_(a) {
        var move = a.move
        if (move == null) return
        if (move.special != null && _specials.containsKey(move.special)) {
            _specials[move.special].call(this, a, move)
        }
        if (move.isAttack) {
            // One hit per target, at the best share of the cells it stands in.
            var hits = []
            for (cell in a.cells) {
                var t = occupant(cell[0], cell[1])
                if (t == null || t.side == a.side) continue
                var known = false
                for (hit in hits) {
                    if (hit[0] == t) {
                        known = true
                        if (cell[2] > hit[1][2]) hit[1] = cell
                    }
                }
                if (!known) hits.add([t, cell])
            }
            if (hits.count == 0) emit_("miss", a, null)
            for (hit in hits) {
                hit_(a, hit[0], move, hit[1])
            }
        }
        if (move.isHeal) {
            for (cell in a.cells) {
                var t = occupant(cell[0], cell[1])
                if (t == null || t.side != a.side) continue
                var amount = move.special == "potion" ? (t.stats.maxHp * move.healAmount / 100).round : _rules.healAmount(a, move, cell[2])
                var over = t.stats.heal(amount)
                healed(t, amount - over)
            }
        }
        if (move.special == "salts") a.clearStatuses()
        if (move.item != null) emit_("item", a, {"item": move.item})
    }

    hit_(a, t, move, cell) {
        if (t.dodgingNow) {
            emit_("dodge", t, {"by": a})
            _rules.onDodge(t, a)
            return
        }
        var share = cell[2]
        var g = t.stun > 0 ? 0 : t.guardNow
        var own = _rules.blockValue(t, g)

        // Guard range: an ally's Defend move covering this cell - the stronger of the two applies.
        var cover = 0
        var guarder = null
        var coverGuard = 0
        for (u in alliesOf(t)) {
            if (u.stun > 0 || u.move == null || !u.move.isDefend) continue
            var gu = u.guardNow
            if (gu <= 0) continue
            var value = _rules.blockValue(u, gu) * coverShare_(u, t)
            if (value > cover) {
                cover = value
                guarder = u
                coverGuard = gu
            }
        }
        var covered = cover > own
        var block = covered ? cover : own

        var stagger = _rules.staggerOf(a, t, move, share)
        var result = _rules.damage(a, t, move, share, cell[3], cell[4], block, covered ? coverGuard : g)
        var dealt = result[0]
        applyDamage_(t, dealt)
        emit_("hit", t, {"by": a, "amount": dealt, "blocked": block > 0, "guard": covered ? coverGuard : g, "move": move})
        if (dealt > 0) _rules.onDamaged(t, a, dealt)

        if (covered) {
            // The cover was used: the guarder takes the skip and the ally isn't staggered.
            _rules.onBlock(guarder, a, coverGuard, peakDistance_(guarder), stagger, dealt)
            blockReaction_(guarder, a, coverGuard, stagger)
        } else if (g > 0) {
            _rules.onBlock(t, a, g, peakDistance_(t), stagger, dealt)
            blockReaction_(t, a, g, stagger)
        } else {
            stagger_(t, stagger, a)
        }
        if (dealt > 0) _rules.onHit(a, t, move, dealt, cell, cell[3], stagger)
        // A Fire Flask sets alight whatever it splashes.
        if (move.special == "flask") _rules.addBurn(t, a, 1, false)

        if (move.push > 0 && t.active) {
            var dir = awayFrom(a, t)
            push(t, dir[0], dir[1])
        }
        if (move.special == "throw" && t.active && !t.has("immovable")) {
            // Throw: the target lands in the cell behind the thrower, if it is free.
            var row = a.row - a.facing
            if (isFree(a.lane, row, t)) relocate_(t, a.lane, row, "push")
        }
        // A reactive prop (the swing-post) answers a hit taken while idle.
        if (t.active && t.move == null && t.has("reactive")) {
            t.stun = 0
            commit(t, [ChainEntry.new(t.trait("reactive").call(), null)])
        }
    }

    peakDistance_(u) { (u.progress - u.move.window[1]).abs }

    // A hit landed inside `u`'s defensive window: the skip (or Absorb's stamina), and Deflect near the peak.
    blockReaction_(u, a, g, stagger) {
        var move = u.move
        if (move.reaction == "absorb") {
            u.stats.gainStamina(stagger * STAGGER_TO_STAMINA)
        } else if (!a.loadout.capstone("Breaker")) {
            var to = u.progress + move.skip + _rules.skipBonus(u)
            u.progress = to > move.completion ? move.completion : to
        }
        emit_("block", u, {"by": a, "guard": g})
        var deflects = move.reaction == "deflect" && g >= DEFLECT_GUARD
        if (u.loadout.capstone("Riposte") && g >= 0.9) deflects = true
        if (deflects) {
            emit_("note", u, {"text": "Deflect"})
            stagger_(a, stagger, u)
        }
    }

    // Guard range: the share of `u`'s block that reaches `t` (0 = not covered).
    coverShare_(u, t) {
        var move = u.move
        var cells = []
        if (move != null) {
            for (c in move.guardRange) cells.add(c)
        }
        var shield = u.loadout.shield
        if (shield != null && (move == null || move.addsShield)) {
            if (shield.type.rangeSides > 0) {
                cells.add([0, -1, shield.type.rangeSides])
                cells.add([0, 1, shield.type.rangeSides])
            }
            if (shield.type.rangeBehind > 0) cells.add([-1, 0, shield.type.rangeBehind])
        }
        if (u.loadout.capstone("Sentinel")) cells.add([-1, 0, 1])
        var best = 0
        for (c in cells) {
            if (t.at(u.lane + c[1], u.row + u.facing * c[0])) {
                var share = _rules.guardShare(u, c[2], c[1])
                if (share > best) best = share
            }
        }
        return best
    }
    covers(u, t) { coverShare_(u, t) > 0 }

    applyDamage_(t, amount) {
        if (amount <= 0) return
        var left = amount
        if (t.shield > 0) {
            var soak = left.min(t.shield)
            t.shield = t.shield - soak
            left = left - soak
        }
        t.stats.takeDamage(left)
        // A boss part's damage also comes off the shared pool.
        var pool = t.trait("pool")
        if (pool != null) pool.stats.takeDamage(left)
    }

    // Damage outside a hit (statuses, reactions, Reprisal). tag: shown on the target (null = silent, e.g. a tick).
    dealDirect(source, t, amount, tag) {
        if (!t.active || amount <= 0) return
        applyDamage_(t, amount)
        if (tag != null) emit_("direct", t, {"amount": amount.round, "text": tag})
    }

    // A set-back from something other than a plain hit landing (Thermal Shock, a Deflect, Permafrost...).
    setback(t, seconds, source) {
        if (seconds > 0) stagger_(t, seconds, source)
    }

    // Staggers `t` by `s` seconds: its move's damage animation plays, then the move resumes from a set-back point.
    stagger_(t, s, source) {
        if (!t.active) return
        var move = t.move
        if (move == null || move.isHold) {
            // In Hold (or idle): the Hold damage animation only - the Hold just ends that much later.
            if (_clock >= t.animImmuneUntil) t.stun = t.stun.max(HOLD_DAMAGE_ANIM)
            emit_("stagger", t, {"setback": 0})
            return
        }
        var recent = 0
        for (time in t.staggerTimes) {
            if (_clock - time <= DR_WINDOW) recent = recent + 1
        }
        t.staggerTimes.add(_clock)
        if (t.staggerTimes.count > 6) t.staggerTimes.removeAt(0)

        var amount = s * DR_FACTORS[recent.min(3)]
        if (!t.has("noStamina") && t.stats.stamina <= 0) amount = amount * 1.5
        if (t.frozen > 0) {
            amount = amount * t.frozen
            t.frozen = 0
        }
        if (source != null && source.counter("halveSetback") > 0) {
            amount = amount * 0.5
            source.setCounter("halveSetback", 0)
        }
        amount = amount * (1 - _rules.setbackReduction(t))
        // Steadfast / Unflinching - which an Exposed boss part loses.
        if (!t.exposedNow) {
            if (move.reaction == "steadfast") amount = amount * 0.5
            if (move.reaction == "unflinching" && t.impactDone) amount = 0
        }
        // Focus capstone: a spell started straight after a Hold can't be set back before its impact.
        if (t.loadout.capstone("Focus") && t.startedAfterHold && move.scaling == "mana" && !t.impactDone) amount = 0

        var floor = 0
        if (move.impact != null && t.impactDone) {
            floor = move.impact
        } else if (move.window != null && t.progress > move.window[2]) {
            floor = move.window[2]
        }
        var back = amount.min(t.progress - floor)
        if (back < 0) back = 0
        t.progress = t.progress - back
        if (_clock >= t.animImmuneUntil) {
            t.stun = t.stun.max(move.damageAnim)
            if (recent >= 2) t.animImmuneUntil = _clock + move.damageAnim + 1
        }
        // Opportunist capstone: set back during the wind-up counts as "in recovery" for 1 s.
        if (source != null && !t.impactDone && source.loadout.capstone("Opportunist")) {
            t.setCounter("recoverUntil", _clock + 1)
        }
        emit_("stagger", t, {"setback": back})
    }
}
