// States/Combat/CombatSession.wren
// Everything the phases of one fight share: the Combatants and Timeline, the CombatUI view, the
// BattleStage (null when fighting in place), per-fight scratch (current target, staggered flashes,
// the input-arming lockout), and the transition helper every phase uses to hand off to the next.
// Built by States/CombatState.wren at the start of a fight and passed to each phase in
// params["session"] (see CombatSubState) - phases hold no fight data of their own beyond their own
// timers. The move-resolution rules live in CombatResolver (session.resolver).
import "app" for Time
import "random" for Random
import "Combat/Timeline" for Timeline
import "States/Combat/CombatResolver" for CombatResolver

var NAV_ARM_TIME  = 0.14   // input lockout after a menu transition, so the prior keypress can't bleed
var STAMINA_REGEN = 2      // stamina per time unit, every combatant, while charging

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

    // The enemy the player's committed move is aimed at (set when they commit, read when it resolves).
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

    // --- input arming -------------------------------------------------------------

    arm() {
        _navArmedAt = Time.unscaledTime + NAV_ARM_TIME
    }

    armed() {
        return Time.unscaledTime >= _navArmedAt
    }

    // --- player / enemy commitment ------------------------------------------------

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
        _timeline.commit(_player, move)
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
                _timeline.commit(e, enemyPickMove(e))
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

    // --- helpers ---------------------------------------------------------------------

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
