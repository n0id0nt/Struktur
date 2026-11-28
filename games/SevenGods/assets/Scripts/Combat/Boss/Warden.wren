// Combat/Boss/Warden.wren
// The Warden - the living statue that guards the old shrine: the tutorial's lead-up boss (boss design doc), fought
// by the player and Kira on the normal 3 x 4 grid. It teaches the boss vocabulary: slow, heavy, readable hits,
// guards you mustn't swing into, Exposed windows, and one enemy that becomes three.
//
//   Twist - Sunder and Reform: merged, the Warden is one statue in one cell. Sunder splits it into three shards
//     across its row (Left shard / Heart / Right shard), each its own enemy with its own line; Reform pulls them
//     back together. Its menu has two halves, one per form.
//   Granite Skin: defence 30 and only half a hit gets through - except while Exposed (defence 5, full damage x1.5).
//   One HP pool (14): damage to any shard comes off the pool. The side shards also have 4 HP of their own;
//     breaking one crumbles it - the Warden Reforms without that arm and loses Ground Slam (it needs both), and with
//     both broken it can't Sunder any more.
//   Easy by design: no stamina, no feints, and it never stays split for more than three combos (Three Stones, at
//     most one Crossfire, then always Reform).
//
// A boss is a group of enemies with one mind: every part picks its line at one decision point (when all of them
// have finished), so the whole boss produces one read. WardenBrain is that mind - Combat/BattleEngine.wren asks it
// for the core's chain and it commits the shards' lines alongside.
import "random" for Random
import "math" for Vec4
import "Combat/Move" for Move
import "Combat/Stats" for Stats
import "Combat/Combatant" for Combatant, ChainEntry
import "Combat/Chain" for Grid
import "Combat/Config/BattleCritter" for BattleCritter
import "Sprites/MutantBattleSprite" for MutantBattleSprite

var RNG = Random.new()
var POOL_HP = 14
var SHARD_HP = 4
// Defence 30 (5 while Exposed); outside an Exposed window only 30% of a hit gets through - with the defence formula
// alone a 30 still lets three quarters of a hit in, which is not "barely scratching".
var GRANITE = [30, 5, 0.5]
var STONE_TINT = Vec4.new(165, 175, 190, 255)
var SHARD_TINT = Vec4.new(130, 150, 185, 255)

// The Warden's custom moves - authored with their final numbers (boss moves don't have to be reskins).
class WardenMoves {
    // The fist stays in the ground after impact: Exposed from impact to the end.
    static stoneFist {
        return Move.new("Stone Fist", 1, 2.2, 0).attackFlat(1.5, 3, 0.8).reach(1).exposed(1.5, 2.2).heavy().special("wardenFist").anim("heavy").damageAnim(0.4)
    }
    // Marks the whole row in front - step back a row.
    static groundSlam {
        return Move.new("Ground Slam", 1, 2.6, 0).attackFlat(1.8, 2, 0.8).reach(1).sides(1).cell(0, -2, 1).cell(0, 2, 1).exposed(1.8, 2.6).heavy().anim("spin").damageAnim(0.4)
    }
    // Arms crossed, covering both sides. Swinging into the peak feeds its skip.
    static bastion {
        return Move.new("Bastion", 1, 2.0, 0).defendFlat(0.2, 1.0, 1.8, 40, 0.3).guardSides(1).anim("guard")
    }
    static lumber { Move.new("Lumber", 1, 1.0, 0).movement(0.5, "ortho", "stay").anim("step") }
    // At 1.2 s it cracks apart into Left shard / Heart / Right shard across its row.
    static sunder { Move.new("Sunder", 1, 1.8, 0).special("sunder").at(1.2).anim("spin") }

    static shardSlam {
        return Move.new("Shard Slam", 1, 1.8, 0).attackFlat(1.2, 2, 0.5).reach(1).exposed(1.2, 1.8).heavy().anim("heavy")
    }
    static shardRoll { Move.new("Shard Roll", 1, 1.0, 0).movement(0.5, "ortho", "stay").anim("step") }
    // Rolls back toward the Heart for Reform - Exposed for the whole move.
    static gather { Move.new("Gather", 1, 1.0, 0).movement(0.5, "ortho", "stay").exposed(0, 1.0).anim("step") }
    // Punishes anyone standing next to the Heart.
    static heartPulse {
        return Move.new("Heart Pulse", 1, 2.0, 0).attackFlat(1.4, 2, 0.5).around8(0.5).heavy().anim("spin")
    }
    // Its guard range covers the shards beside it.
    static heartGuard {
        return Move.new("Heart Guard", 1, 1.6, 0).defendFlat(0.2, 0.8, 1.4, 35, 0.3).guardSides(1).anim("guard")
    }
    // The big window: the glowing core rune is Exposed for the whole Reform; the shards lock back in at 1.6 s.
    static reform { Move.new("Reform", 1, 2.0, 0).special("reform").at(1.6).exposed(0, 2.0).anim("guard") }

    static hold(name, seconds) { Move.hold(seconds).rename(name) }
}

class WardenBrain {
    construct new() {
        _core = null
        _engine = null
        _shards = []            // [Combatant or null, Combatant or null]: left, right while split
        _broken = [false, false]
        _split = false
        _opened = false
        _last = null
        _repeats = 0
        _splitCombos = 0
        _crowded = 0
        _said = {}
    }

    core { _core }
    split { _split }

    // --- the one mind ----------------------------------------------------------------------

    chainFor(engine, fighter) {
        if (_core == null) attach_(engine, fighter)
        // Only the core decides - and only once every shard has finished its line too.
        if (fighter != _core) return null
        for (shard in _shards) {
            if (shard != null && shard.active && !shard.needsChain) return null
        }
        return _split ? pickSplit_() : pickMerged_()
    }

    attach_(engine, core) {
        _core = core
        _engine = engine
        engine.onSpecial("sunder", Fn.new { |battleEngine, fighter, move| sunder_() })
        engine.onSpecial("reform", Fn.new { |battleEngine, fighter, move| reform_() })
        engine.onSpecial("wardenFist", Fn.new { |battleEngine, fighter, move| say_("fist", "It's stuck after the punch! Hit it now!") })
        engine.onDefeat(Fn.new { |battleEngine, fighter| defeated_(fighter) })
    }

    bothArms { !_broken[0] && !_broken[1] }
    canSunder { !_broken[0] || !_broken[1] }

    pickMerged_() {
        var combo = null
        // "A fighter next to it for 2 stops in a row" brings the Earthshaker.
        if (_engine.adjacentEnemies(_core).count > 0) {
            _crowded = _crowded + 1
        } else {
            _crowded = 0
        }
        if (!_opened) {
            _opened = true
            combo = "Awakening"
        } else if (_crowded >= 2 && bothArms && _last != "Earthshaker") {
            combo = "Earthshaker"
        } else {
            var follow = null
            if (_last == "Awakening") follow = ["Fist and Wall", 0.6]
            if (_last == "Fist and Wall" || _last == "Earthshaker") follow = ["Sunder", 0.5]
            if (_last == "Reform") follow = ["Fist and Wall", 0.6]
            if (follow != null && allowed_(follow[0]) && RNG.float() < follow[1]) {
                combo = follow[0]
            } else {
                combo = weighted_([["Fist and Wall", 40], ["Earthshaker", 35], ["Sunder", 25]])
            }
        }
        if (combo == null) combo = "Fist and Wall"
        remember_(combo)
        _engine.combo(_core, combo)

        var moves = WardenMoves
        if (combo == "Awakening") {
            return [entry_(moves.hold("Settle", 2.0), null), entry_(moves.lumber, "toward"), entry_(moves.stoneFist, null)]
        }
        if (combo == "Earthshaker") {
            say_("slam", "The floor's glowing! Get off the glowy bit!")
            return [entry_(moves.lumber, "toward"), entry_(moves.groundSlam, null), entry_(moves.hold("Settle", 0.8), null)]
        }
        if (combo == "Sunder") {
            say_("sunder", "It's splitting into three! Watch all of them!")
            return [entry_(moves.stoneFist, null), entry_(moves.sunder, null)]
        }
        say_("bastion", "Don't hit it while it's got its arms up! It's like Dad's swing-post, but grumpier.")
        return [entry_(moves.bastion, null), entry_(moves.stoneFist, null)]
    }

    allowed_(combo) {
        if (combo == "Earthshaker") return bothArms
        if (combo == "Sunder") return canSunder
        return true
    }

    weighted_(options) {
        var total = 0
        var open = []
        for (option in options) {
            // Never the same combo three times running.
            if (!allowed_(option[0]) || (option[0] == _last && _repeats >= 2)) continue
            open.add(option)
            total = total + option[1]
        }
        if (open.count == 0) return null
        var roll = RNG.float() * total
        for (option in open) {
            roll = roll - option[1]
            if (roll <= 0) return option[0]
        }
        return open[-1][0]
    }

    remember_(combo) {
        _repeats = combo == _last ? _repeats + 1 : 1
        _last = combo
    }

    // Split: Three Stones first, at most one Crossfire, then always Reform.
    pickSplit_() {
        var combo = "Reform"
        if (_splitCombos == 0) {
            combo = "Three Stones"
        } else if (_splitCombos == 1 && _last == "Three Stones" && RNG.float() < 0.6) {
            combo = "Crossfire"
        }
        _splitCombos = _splitCombos + 1
        remember_(combo)
        _engine.combo(_core, combo)

        var moves = WardenMoves
        var heart = null
        if (combo == "Three Stones") {
            heart = [entry_(moves.heartGuard, null), entry_(moves.heartPulse, null), entry_(moves.hold("Rattle", 0.8), null)]
        } else if (combo == "Crossfire") {
            heart = [entry_(moves.heartPulse, null), entry_(moves.hold("Rattle", 1.2), null), entry_(moves.heartGuard, null)]
        } else {
            say_("reform", "It's pulling itself back together, and the middle's wide open!")
            heart = [entry_(moves.heartPulse, null), entry_(moves.reform, null)]
        }
        // The side shards are mirrored and share one line.
        for (shard in _shards) {
            if (shard == null || !shard.active) continue
            var line = null
            if (combo == "Three Stones") {
                line = [entry_(moves.shardRoll, "toward"), entry_(moves.shardSlam, null), entry_(moves.shardSlam, null)]
            } else if (combo == "Crossfire") {
                line = [entry_(moves.shardSlam, null), entry_(moves.shardRoll, "toward"), entry_(moves.shardSlam, null)]
            } else {
                line = [entry_(moves.shardSlam, null), entry_(moves.shardRoll, "toward"), entry_(moves.gather, gatherCell_)]
            }
            _engine.commit(shard, line)
        }
        return heart
    }

    entry_(move, dest) { ChainEntry.new(move, dest) }

    // Where Gather rolls: the free neighbouring cell nearest the Heart.
    gatherCell_ {
        return Fn.new { |engine, fighter|
            var best = null
            var bestDistance = (fighter.lane - _core.lane).abs + (fighter.row - _core.row).abs
            for (offset in Grid.offsets("ortho", fighter.facing)) {
                var lane = fighter.lane + offset[0]
                var row = fighter.row + offset[1]
                if (!engine.isFree(lane, row, fighter)) continue
                var distance = (lane - _core.lane).abs + (row - _core.row).abs
                if (distance < bestDistance) {
                    best = [lane, row]
                    bestDistance = distance
                }
            }
            return best
        }
    }

    // --- Sunder and Reform -----------------------------------------------------------------

    // The statue cracks into three across its row: the Heart stays where it stood, a shard lands in each other lane
    // (a broken arm doesn't come back). Anyone standing there is pushed back a cell.
    sunder_() {
        _split = true
        _splitCombos = 0
        _core.name = "Heart"
        var lanes = []
        for (lane in 0..._engine.lanes) {
            if (lane != _core.lane) lanes.add(lane)
        }
        _shards = [null, null]
        for (i in 0...2) {
            if (_broken[i] || i >= lanes.count) continue
            var lane = lanes[i]
            var inWay = _engine.occupant(lane, _core.row)
            if (inWay != null && !shoveClear_(inWay)) continue
            var shard = Warden.shardDef(i == 0 ? "Left Shard" : "Right Shard").makeCombatant(null)
            shard.traits["pool"] = _core
            shard.brain = this
            _engine.spawn(shard, _core.side, lane, _core.row)
            _shards[i] = shard
        }
        _engine.note(_core, "Sunder")
    }

    // Moves whoever stands where a shard lands: back a cell toward their own side, else any free neighbour.
    shoveClear_(who) {
        var back = -_engine.forward(who.side)
        if (_engine.push(who, 0, back)) return true
        for (offset in Grid.offsets("any", 1)) {
            if (_engine.push(who, offset[0], offset[1])) return true
        }
        return false
    }

    // The shards fly back into the Heart's cell and lock in: one statue again.
    reform_() {
        for (shard in _shards) {
            if (shard != null && shard.active) {
                shard.clearMove()
                shard.removed = true
                _engine.emit_("remove", shard, null)
            }
        }
        _shards = [null, null]
        _split = false
        _core.name = "The Warden"
        _engine.note(_core, "Reform")
    }

    defeated_(fighter) {
        for (i in 0...2) {
            if (i < _shards.count && _shards[i] == fighter) {
                _broken[i] = true
                _engine.note(fighter, "Crumbled")
                say_("crumble", "It crumbled! That arm's not coming back!")
            }
        }
    }

    // Kira's call-outs, each the first time it happens (without her it reads as a plain tip).
    say_(key, text) {
        if (_said.containsKey(key)) return
        _said[key] = true
        var kira = _engine.fighters.any { |fighter| fighter.side != _core.side && fighter.name == "Kira" && fighter.active }
        _engine.say(kira ? "Kira: " + text : text)
    }
}

class Warden {
    // The statue: 14 HP in one pool, Granite Skin, no stamina, Iron-bodied (weak to Storm).
    static def {
        var def = BattleCritter.fromSheet("The Warden", MutantBattleSprite).stats(POOL_HP, 0, 10, 0, 0, 0)
        def.trait("noStamina", true).trait("immovable", true).trait("granite", GRANITE).trait("bodyElement", "Iron")
        return def.trait("boss", true).tint(STONE_TINT).brain(Fn.new { WardenBrain.new() })
    }

    // A side shard: 4 HP of its own (and everything it takes also comes off the pool).
    static shardDef(name) {
        var def = BattleCritter.fromSheet(name, MutantBattleSprite).stats(SHARD_HP, 0, 10, 0, 0, 0)
        def.trait("noStamina", true).trait("immovable", true).trait("granite", GRANITE).trait("bodyElement", "Iron")
        return def.trait("boss", true).tint(SHARD_TINT)
    }
}
