// States/Combat/CombatSession.wren
// Everything the phases of one fight share: the Combatants and Timeline, the CombatUI view, the
// BattleStage (null when fighting in place), per-fight scratch (current target, staggered flashes,
// parry offers, the input-arming lockout), game-time control and the transition helper every phase uses
// to hand off to the next. Built by States/CombatState.wren at the start of a fight and passed to each
// phase in params["session"] (see CombatSubState) - phases hold no fight data of their own beyond their
// own timers. The move-resolution rules live in CombatResolver (session.resolver).
//
// Battles run on game time: the Timeline ticks on Time.scaledDelta and every animation is anchored to
// the Timeline, so pausing (menus) or slowing (the parry prompt) time via Time.setTimeScale stops or
// slows the whole fight. Only input arming stays on real time so menus still respond while paused.
import "app" for Time
import "random" for Random
import "Combat/Timeline" for Timeline, SECONDS_PER_TIME_UNIT
import "States/Combat/CombatResolver" for CombatResolver

var NAV_ARM_TIME  = 0.14   // input lockout after a menu transition, so the prior keypress can't bleed
var STAMINA_REGEN = 2      // stamina per time unit, every combatant, while charging

// Game-time scale while the parry prompt is up - the attack keeps crawling toward impact so the player
// has to decide quickly. Menus pause time outright (scale 0).
var PARRY_TIME_SCALE = 0.25
// Even if an attack is already at impact when its prompt opens, the player gets at least this many game
// seconds to answer.
var PARRY_MIN_WINDOW = 0.3
// Fraction of an attack's duration the parry window can never exceed (so a long window setting can't open
// the prompt before the attack has visibly started).
var PARRY_MAX_LEAD = 0.8
// Chance an AI-controlled defender parries an incoming attack - a plain coin flip for now, tunable like
// every other combat constant.
var ENEMY_PARRY_CHANCE = 0.5

var RNG = Random.new()

class CombatSession {
    // outer: the StateManager CombatState itself lives in (OverState clears it to end the fight).
    // manager: CombatState's nested StateManager holding the phases. player: Player.combatant (it
    // persists on the Player script). enemies / enemyDefs: parallel lists, one Combatant + one
    // BattleCritter per opponent.
    construct new(outer, manager, player, enemies, enemyDefs, view, stage) {
        _outer = outer
        _manager = manager
        _player = player
        _enemies = enemies
        _enemyDefs = enemyDefs
        _view = view
        _stage = stage

        _timeline = Timeline.new()
        for (e in _enemies) {
            _timeline.add(e)
        }
        _timeline.add(_player)

        _staggered = []
        _playerTarget = null
        _pendingMove = null
        _targetIndex = 0
        _navArmedAt = 0
        _resolver = CombatResolver.new(this)
    }

    outer { _outer }
    player { _player }
    enemies { _enemies }
    enemyDefs { _enemyDefs }
    view { _view }
    stage { _stage }
    timeline { _timeline }
    resolver { _resolver }

    // The enemy the player's committed move is aimed at (set when they commit).
    playerTarget { _playerTarget }

    // The move waiting on a target pick while TargetingState is up.
    pendingMove { _pendingMove }

    // Index into enemies of the TargetingState cursor.
    targetIndex { _targetIndex }
    targetIndex=(value) { _targetIndex = value }

    // --- transitions -------------------------------------------------------------

    // Hands off to the phase registered under `name`, passing the session along (every phase reads
    // it from params["session"], see CombatSubState). The caller must return straight afterwards -
    // the phase being left is already exited by the time this returns.
    goTo(name) { goTo(name, {}) }

    goTo(name, params) {
        params["session"] = this
        _manager.changeState(name, params)
    }

    // True while the phase registered under `name` is the active one - lets UI click callbacks
    // (wired once per fight, not per phase) ignore clicks that arrive in the wrong phase.
    isIn(name) {
        return _manager.currentState != null && _manager.currentState.name == name
    }

    // --- game time ---------------------------------------------------------------------

    // A menu is up (move choice, target pick): game time stops - every animation, the Timeline and the
    // particles freeze where they are.
    pauseTime() {
        Time.setTimeScale(0)
    }

    // The parry prompt is up: game time crawls.
    slowTime() {
        Time.setTimeScale(PARRY_TIME_SCALE)
    }

    resumeTime() {
        Time.setTimeScale(1)
    }

    // --- input arming -------------------------------------------------------------

    // Real time on purpose (not game time): menus must respond while game time is paused.
    arm() {
        _navArmedAt = Time.unscaledTime + NAV_ARM_TIME
    }

    armed() {
        return Time.unscaledTime >= _navArmedAt
    }

    // --- committing moves ----------------------------------------------------------

    // The Timeline's commit/clear plus the matching animation change: a committed combatant plays its
    // move's clip (retimed to the move's charge), a cleared one goes back to idle. Everything in a fight
    // that changes a combatant's move goes through these two.
    commitMove(combatant, move) {
        _timeline.commit(combatant, move)
        if (_stage != null) {
            _stage.beginCharge(combatant, move, _timeline.costOf(combatant))
        }
    }

    clearMove(combatant) {
        _timeline.clear(combatant)
        if (_stage != null) {
            _stage.endCharge(combatant)
        }
    }

    // Shoves a mid-charge combatant's bar backward (a stagger). No animation call needed: every battler's
    // clip is re-anchored to the Timeline fraction each frame (animateBattlers), so it jumps back with it.
    interruptMove(combatant, units) {
        _timeline.interrupt(combatant, units)
    }

    // The player picked `move` from the menu - offensive moves against a group first need a target
    // (TargetingState), everything else commits straight away.
    commitPlayerMove(move) {
        if (move.offensive && livingEnemies().count > 1) {
            _pendingMove = move
            _targetIndex = firstLivingEnemy()
            goTo("TargetingState")
            return
        }
        var target = move.offensive ? _enemies[firstLivingEnemy()] : _player
        commitPlayerMoveOn(move, target)
    }

    commitPlayerMoveOn(move, target) {
        _playerTarget = target
        _pendingMove = null
        commitMove(_player, move)
        goTo("ChargingState")
    }

    // TargetingState was cancelled - drop the half-chosen move.
    cancelPendingMove() {
        _pendingMove = null
    }

    // Every enemy that still needs a move picks one (target is always the player).
    commitEnemyMoves() {
        for (e in _enemies) {
            if (e.alive && _timeline.needsMove(e)) {
                commitMove(e, enemyPickMove(e))
            }
        }
    }

    // Simple AI: heal when hurt, otherwise a random offensive move biased toward the cheaper ones
    // (pick two, keep the shorter).
    enemyPickMove(enemy) {
        if (enemy.stats.fraction < 0.4) {
            for (m in enemy.moves) {
                if (m.healAmount > 0) {
                    return m
                }
            }
        }
        var offensive = []
        for (m in enemy.moves) {
            if (m.offensive) {
                offensive.add(m)
            }
        }
        if (offensive.count == 0) {
            return enemy.moves[0]
        }
        var a = offensive[RNG.int(offensive.count)]
        var b = offensive[RNG.int(offensive.count)]
        return a.timeCost <= b.timeCost ? a : b
    }

    // Who `actor`'s committed move is aimed at: the player's chosen enemy (or the first one still
    // standing if it fell), or the player for any enemy. null when there's nobody left to hit.
    targetOf(actor) {
        var isPlayer = actor == _player
        var target = isPlayer ? _playerTarget : _player
        if (target == null || !target.alive) {
            target = isPlayer ? firstLivingEnemyCombatant() : _player
        }
        return target
    }

    // --- parrying ---------------------------------------------------------------------

    // The parry window of an attack opens `move.parryPromptTime` game seconds before it lands (capped so
    // it can't open before the attack has visibly started). Returns the first attacker whose window has
    // just opened and who is aimed at the player - ChargingState then opens ParryPromptState for it - or
    // null. Attacks aimed at an AI-controlled enemy are settled right here instead: the defender flips its
    // coin and, if it parries, plays its parry reaction at once.
    nextParryOffer() {
        for (c in _timeline.combatants) {
            var move = _timeline.committedMove(c)
            if (move == null || !c.alive || !move.offensive || !move.parryable || _timeline.parryOffered(c)) {
                continue
            }
            var duration = _timeline.costOf(c) * SECONDS_PER_TIME_UNIT
            var lead = move.parryPromptTime
            if (lead > duration * PARRY_MAX_LEAD) {
                lead = duration * PARRY_MAX_LEAD
            }
            if (_timeline.fraction(c) < 1 - lead / duration) {
                continue
            }
            _timeline.markParryOffered(c)

            var target = targetOf(c)
            if (target == null || !target.alive) {
                continue
            }
            if (target == _player) {
                return c
            }
            answerParry(c, RNG.float() < ENEMY_PARRY_CHANCE)
        }
        return null
    }

    // The defender of `attacker`'s move answered (or an AI decided): remember it for the moment the
    // move lands, and if they chose to parry show it now.
    answerParry(attacker, chosen) {
        _timeline.setParryDecision(attacker, chosen)
        if (chosen && _stage != null) {
            _stage.react(targetOf(attacker), "parry")
        }
    }

    parryDecision(attacker) {
        return _timeline.parryDecision(attacker)
    }

    // --- helpers ---------------------------------------------------------------------

    // One frame of a fight that's running on game time: charge every committed move, regenerate stamina,
    // refresh the UI. (The battlers' animations follow the Timeline - see animateBattlers.)
    advanceCharge() {
        var units = _timeline.tick(Time.scaledDelta)
        regenStamina(units)
        refreshViews()
    }

    // Anchors every battler to the Timeline: its clip and lunge follow the fraction of the move it's
    // charging, reactions tick, idle ones idle. Called every frame by CombatState.update, whatever the
    // phase - so animation never depends on which phase happens to be running.
    animateBattlers() {
        if (_stage == null) {
            return
        }
        for (c in _timeline.combatants) {
            var fraction = _timeline.committedMove(c) == null ? 0 : _timeline.fraction(c)
            _stage.drive(c, fraction, targetOf(c))
        }
    }

    regenStamina(units) {
        for (c in _timeline.combatants) {
            c.stats.gainStamina(units * STAMINA_REGEN)
        }
    }

    markStaggered(combatant) {
        _staggered.add(combatant)
    }

    clearFlashes() {
        _staggered = []
    }

    livingEnemies() {
        var out = []
        for (e in _enemies) {
            if (e.alive) {
                out.add(e)
            }
        }
        return out
    }

    firstLivingEnemy() {
        for (i in 0..._enemies.count) {
            if (_enemies[i].alive) {
                return i
            }
        }
        return 0
    }

    firstLivingEnemyCombatant() {
        var living = livingEnemies()
        return living.count == 0 ? null : living[0]
    }

    stepTarget(from, dir) {
        var n = _enemies.count
        var i = from
        for (step in 1..n) {
            i = (i + dir + n) % n
            if (_enemies[i].alive) {
                return i
            }
        }
        return from
    }

    readout(combatant) {
        var m = _timeline.committedMove(combatant)
        if (m == null) {
            return ""
        }
        return combatant.stats.exhausted ? "%(m.name)  (slow)" : m.name
    }

    refreshViews() {
        _view.refreshPlayer(_player.alive, _player.stats.fraction, _timeline.fraction(_player),
                            _player.stats.staminaFraction, readout(_player), _staggered.contains(_player))
        var i = 0
        for (c in _enemies) {
            _view.refreshEnemy(i, c.alive, c.stats.fraction, _timeline.fraction(c), c.stats.staminaFraction,
                               readout(c), _staggered.contains(c), isIn("TargetingState") && i == _targetIndex)
            i = i + 1
        }
    }
}
