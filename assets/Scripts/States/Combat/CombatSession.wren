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
import "Combat/Arbiter" for Arbiter
import "Combat/EnemyAI" for EnemyAI
import "Combat/Timeline" for Timeline, SECONDS_PER_TIME_UNIT
import "States/Combat/CombatResolver" for CombatResolver

var NAV_ARM_TIME  = 0.14   // input lockout after a menu transition, so the prior keypress can't bleed

// Game-time scale while the parry prompt is up - the attack keeps crawling toward impact so the player
// has to decide quickly. Menus pause time outright (scale 0).
var PARRY_TIME_SCALE = 0.25
// Even if an attack is already at impact when its prompt opens, the player gets at least this many game
// seconds to answer.
var PARRY_MIN_WINDOW = 0.3
// Fraction of an attack's duration the parry window can never exceed (so a long window setting can't open
// the prompt before the attack has visibly started).
var PARRY_MAX_LEAD = 0.8

class CombatSession {
    // outer: the StateManager CombatState itself lives in (OverState clears it to end the fight).
    // manager: CombatState's nested StateManager holding the phases. party: the human-controlled side - every
    // party member's persistent Combatant, leader (the first player) first (see Party.wren). enemies /
    // enemyDefs: parallel lists, one Combatant + one BattleCritter per opponent.
    construct new(outer, manager, party, enemies, enemyDefs, view, stage) {
        _outer = outer
        _manager = manager
        _party = party
        _player = party[0]
        _enemies = enemies
        _enemyDefs = enemyDefs
        _view = view
        _stage = stage

        _timeline = Timeline.new()
        for (e in _enemies) {
            _timeline.add(e)
        }
        for (member in _party) {
            _timeline.add(member)
        }

        _staggered = []
        _pendingMove = null
        _targetIndex = 0
        _navArmedAt = 0
        _parryAttacker = null
        _duel = false
        _resolver = CombatResolver.new(this)
    }

    outer { _outer }

    // The party leader (the first player) - the one the log calls "you".
    player { _player }

    // Everyone on the human-controlled side, leader first.
    party { _party }
    enemies { _enemies }
    enemyDefs { _enemyDefs }
    view { _view }
    stage { _stage }
    timeline { _timeline }
    resolver { _resolver }

    // True for a challenge fight (States/ExperimentState.startDuel) - recorded in Combat/CombatOutcome so a lost
    // duel isn't treated as a death. Set by States/CombatState.wren from params["duel"].
    duel { _duel }
    duel=(value) { _duel = value }

    // The move waiting on a target pick while TargetingState is up.
    pendingMove { _pendingMove }

    // Index into enemies of the TargetingState cursor.
    targetIndex { _targetIndex }
    targetIndex=(value) { _targetIndex = value }

    // The attacker whose move ParryPromptState is asking the player about (null otherwise) - its stack in the
    // combat UI is flagged with the ">>" marker so the player can see whose attack they are about to parry.
    parryAttacker { _parryAttacker }
    parryAttacker=(value) { _parryAttacker = value }

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
    commitMove(combatant, move) { commitMove(combatant, move, null) }

    // target: who the move is aimed at (a Combatant), or null for a move with no target (a stall).
    commitMove(combatant, move, target) {
        _timeline.commit(combatant, move)
        _timeline.setTarget(combatant, target)
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

    // The party member whose turn it is to pick a move: the first living member with nothing committed (null
    // when everyone has a move). The human-controlled side picks for each member in turn through the same menu.
    chooser {
        for (member in _party) {
            if (member.alive && _timeline.needsMove(member)) {
                return member
            }
        }
        return null
    }

    // The human picked `move` for the current chooser - offensive moves against a group first need a target
    // (TargetingState), everything else commits straight away.
    commitPlayerMove(move) {
        if (move.offensive && livingEnemies().count > 1) {
            _pendingMove = move
            _targetIndex = firstLivingEnemy()
            goTo("TargetingState")
            return
        }
        var target = move.offensive ? _enemies[firstLivingEnemy()] : chooser
        commitPlayerMoveOn(move, target)
    }

    // Menu button `index` was chosen for the current chooser (the buttons are wired once per fight, so they
    // call in here with their index and this looks the move up).
    commitChoice(index) {
        var member = chooser
        if (member != null && index < member.moves.count) {
            commitPlayerMove(member.moves[index])
        }
    }

    // Commits `move` on `target` for the current chooser, then on to the next member who needs a move - or,
    // once everyone has one, back to charging.
    commitPlayerMoveOn(move, target) {
        commitMove(chooser, move, target)
        _pendingMove = null
        goTo(chooser != null ? "ChoosingState" : "ChargingState")
    }

    // TargetingState was cancelled - drop the half-chosen move.
    cancelPendingMove() {
        _pendingMove = null
    }

    // Every enemy that still needs a move picks one and a target (Combat/EnemyAI.wren - self-targeted for a
    // heal / stamina move).
    commitEnemyMoves() {
        for (e in _enemies) {
            if (e.alive && _timeline.needsMove(e)) {
                var choice = EnemyAI.choose(e, livingParty(), _timeline)
                commitMove(e, choice[0], choice[1])
            }
        }
    }

    // True for a combatant on the human-controlled side.
    isParty(combatant) { _party.contains(combatant) }

    livingParty() {
        var out = []
        for (member in _party) {
            if (member.alive) {
                out.add(member)
            }
        }
        return out
    }

    // Who `actor`'s committed move is aimed at (stored on the Timeline when it was committed). If that target
    // has fallen since, the first living combatant on the other side takes its place; null when there is
    // nobody left to hit.
    targetOf(actor) {
        var target = _timeline.target(actor)
        if (target != null && target.alive) {
            return target
        }
        if (isParty(actor)) {
            return firstLivingEnemyCombatant()
        }
        var living = livingParty()
        return living.count == 0 ? null : living[0]
    }

    // --- parrying ---------------------------------------------------------------------

    // The parry window of an attack opens `move.parryPromptTime` game seconds before it lands (capped so
    // it can't open before the attack has visibly started). Returns the first attacker whose window has
    // just opened and who is aimed at the player - ChargingState then opens ParryPromptState for it - or
    // null. Attacks aimed at an AI-controlled enemy are settled right here instead: the defender flips its
    // EnemyAI decides whether it tries to parry and, if so, the attempt is arbitrated with an AI timing and its
    // parry pose plays.
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
            if (isParty(target)) {
                return c
            }
            if (EnemyAI.wantsParry(target, c, move, _timeline)) {
                answerParry(c, true, Arbiter.aiParryTiming())
            } else {
                answerParry(c, false, 0)
            }
        }
        return null
    }

    // The defender of `attacker`'s move answered (or an AI decided). A parry attempt is arbitrated now
    // (Combat/Arbiter.parry - timing: 0..1 through the window) so the parry pose plays at once; the result is
    // kept for the moment the move lands (CombatResolver applies it).
    answerParry(attacker, chosen, timing) {
        var defender = targetOf(attacker)
        if (!chosen || defender == null) {
            _timeline.setParryResult(attacker, null)
            return
        }
        _timeline.setParryResult(attacker, Arbiter.parry(defender, attacker, timing))
        if (_stage != null) {
            _stage.react(defender, "parry")
        }
    }

    // The ParryResult for `attacker`'s committed move, or null if its target didn't try to parry.
    parryResult(attacker) {
        return _timeline.parryResult(attacker)
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
            c.stats.gainStamina(Arbiter.staminaRegen(c, units))
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
        // The party member whose move is being picked is flagged with the ">>" marker.
        var turnOf = (isIn("ChoosingState") || isIn("TargetingState")) ? chooser : null
        var p = 0
        for (member in _party) {
            _view.refreshParty(p, member.alive, member.stats.fraction, _timeline.fraction(member),
                               member.stats.staminaFraction, readout(member), _staggered.contains(member),
                               member == turnOf)
            p = p + 1
        }
        var i = 0
        for (c in _enemies) {
            _view.refreshEnemy(i, c.alive, c.stats.fraction, _timeline.fraction(c), c.stats.staminaFraction,
                               readout(c), _staggered.contains(c),
                               (isIn("TargetingState") && i == _targetIndex) || c == _parryAttacker)
            i = i + 1
        }
    }
}
