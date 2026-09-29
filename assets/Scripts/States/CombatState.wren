// states/CombatState.wren
// Phase 5 of the Interrupt Combat Roadmap - Archetypes & the RPS Triangle. The player takes on the
// pack of critters they walked into - 1 vs up to 3, each side carrying an archetype
// (Combat/Config/Archetype.wren) that supplies its move set. The player's battle profile is in
// Combat/Config/BattlePlayer.wren (and its Combatant persists on the Player script, so HP / stamina
// carry between fights); each enemy's is in Combat/Config/BattleCritter.wren, looked up by species
// name. Every body charges its own timeline at once, so turn order is a genuine scramble, and
// offensive moves pick a target.
//
// The triangle these numbers are tuned toward (see Combat/Config/Moves.wren - "expect churn"):
//   Speed beats Power (cheap 2u moves interrupt-lock a wind-up and sustain the lock)
//   Power beats Control (one landed Crush is >half a mage's HP; Control burns out holding the lock)
//   Control beats Speed (Slow wrecks the rhythm, Mend out-heals the chip)
//
// Presentation cuts to the Battle_Arena level (see Combat/BattleStage.wren), which deactivates the
// overworld player and every other loaded level entity for the duration (GameObject.setInactive) -
// that already stops their simulation (physics bodies are disabled too, see
// GameObjectManager::UpdateActiveStates), so nothing here needs to freeze the global time scale.
// pacing / Timeline / regen still run off Time.unscaled* (a convention kept from before this
// class stopped freezing time - harmless now, since scaled and unscaled agree). All the
// turn-flow/input-polling logic lives directly here; UI/Combat/CombatUI.wren is the pure view this
// owns and refreshes.
import "States/BaseState" for BaseState
import "input" for Input
import "app" for Time
import "gameObject" for GameObject
import "gameObjectComponents" for Script, World, WorldTransform, Level
import "math" for Vec3
import "random" for Random

import "Combat/Config/BattleCritter" for BattleCritter
import "Combat/Config/Moves" for Moves
import "Combat/Timeline" for Timeline
import "Combat/Disruption" for Disruption
import "Combat/BattleStage" for BattleStage
import "UI/Combat/CombatUI" for CombatUI

var INTRO_TIME   = 0.9   // fallback (fight-in-place) beat before the menu arms
var ENTER_TIME   = 0.65  // arena sweep + battler slide-in
var STRIKE_TIME  = 0.35  // curve-driven lunge toward the target (BattleStage.updateStrike)
var RETURN_TIME  = 0.25  // ease back to home afterwards (BattleStage.updateReturn)
var MESSAGE_TIME = 0.9
var OVER_TIME    = 1.7
var NAV_ARM_TIME = 0.14   // input lockout after a menu transition, so the prior keypress can't bleed
var STAMINA_REGEN = 2     // stamina per time unit, every combatant, while charging
// Chance an AI-controlled defender answers "yes" to its own parry prompt (see resolveMove) - a
// plain coin flip for now, tunable like every other combat constant.
var ENEMY_PARRY_CHANCE = 0.5

var RNG = Random.new()

class CombatState is BaseState {
    construct new() {
        super()
        name = "CombatState"
        _view = null
        _phase = "intro"
        _timerEnd = 0
        _navArmedAt = 0
        _nextStep = null
        _timeline = null
        _enemies = []
        _staggered = []
        _pendingMove = null
        // Log text + continuation stashed while the "attacking"/"returning" animation phases play
        // out, then handed to showLog() once they finish - see resolveMove/update().
        _pendingLogText = null
        _pendingLogNext = null
        _targetIndex = 0
        _targetAxisHeld = false
        _playerTarget = null
        // _player is NOT owned here - it's Player.combatant, which persists on the Player script.
        _player = null
        _enemyDefs = []   // BattleCritter per enemy, parallel to _enemies - visuals + numbers
        _stage = null     // BattleStage while an authored arena is in use; null = fight in place
        // "parryPrompt" phase state (a parryable move about to land on a human defender - see
        // resolveMove/finishResolve_) - who's attacking whom with what, and when the prompt appeared
        // (so it can time out - see update()).
        _parryActor = null
        _parryTarget = null
        _parryMove = null
        _parryPromptStart = 0
    }

    enter(stateManager, params) {
        super.enter(stateManager, params)

        _enemies = []
        _enemyDefs = []
        for (entity in params["opponents"]) {
            var s = Script.getInstance(entity)
            if (s == null) {
                continue
            }
            var def = BattleCritter.forName(s.name)
            _enemyDefs.add(def)
            _enemies.add(def.makeCombatant(entity))
        }

        // The player's Combatant lives on the Player script (persistent HP / stamina across fights).
        var playerScript = params["player"] == null ? null : Script.getInstance(params["player"])
        if (playerScript == null || _enemies.count == 0) {
            System.print("[CombatState] no player or no valid opponents, aborting")
            stateManager.clearCurrentState()
            return
        }
        _player = playerScript.combatant

        _timeline = Timeline.new()
        for (e in _enemies) {
            _timeline.add(e)
        }
        _timeline.add(_player)

        var roster = _enemies.map { |e| "%(e.name)(%(e.stats.hp))" }.join(", ")
        System.print("[CombatState] You(%(_player.stats.hp)/%(_player.stats.maxHp)) vs %(roster)")

        var enemyNames = _enemies.map { |e| e.name }.toList
        var moveLabels = _player.moves.map { |m| m.menuLabel }.toList
        _view = CombatUI.new(enemyNames, _player.name, moveLabels)

        var i = 0
        for (button in _view.moveButtons) {
            var chosen = _player.moves[i]
            button.setOnClick { |s, m|
                if (_phase == "choosing" && armed()) {
                    commitPlayerMove(chosen)
                }
            }
            i = i + 1
        }
        _view.fleeButton.setOnClick { |s, m|
            if (_phase == "choosing" && armed()) {
                flee()
            }
        }

        var foe = _enemies.count == 1 ? "the %(_enemies[0].name)" : "%(_enemies.count) foes"
        _view.setMessage("You're set upon by %(foe)!")
        refreshViews()

        // Cut to the battle arena (Combat/BattleStage.wren) - the authored battle level
        // (ExperimentState.BATTLE_ARENA_LEVEL_NAME) if one was found, centred on its bounds rather
        // than its top-left corner (that's where World/LDtk places a level's WorldTransform);
        // otherwise a fixed offscreen patch of world, or an authored "BattleAnchor" room if one
        // exists. Needs the world root to parent battle entities under; without it (an unexpected
        // caller) fall back to fighting in place.
        if (params["world"] != null) {
            var battleLevelIndex = params["battleLevelIndex"]
            var battleLevelEntity = null
            var anchorPosition = null
            if (battleLevelIndex != null) {
                battleLevelEntity = World.getLoadedLevelEntity(params["world"], battleLevelIndex)
                if (battleLevelEntity == null) {
                    battleLevelEntity = World.loadLevelEntities(params["world"], battleLevelIndex)
                }
                GameObject.setActive(battleLevelEntity)
                var levelPos = WorldTransform.getPosition(battleLevelEntity)
                var level = Level.get(battleLevelEntity)
                anchorPosition = Vec3.new(levelPos.x + level.width / 2, levelPos.y + level.height / 2, 0)
            }
            if (anchorPosition == null) {
                var anchorEntity = firstEntity_("BattleAnchor")
                if (anchorEntity != null) {
                    anchorPosition = WorldTransform.getPosition(anchorEntity)
                }
            }
            _stage = BattleStage.new(anchorPosition, params["player"], params["world"],
                                     _player, _enemyDefs, _enemies, battleLevelEntity)
            _view.setVisible(false)
            _phase = "entering"
            _timerEnd = Time.unscaledTime + ENTER_TIME
        } else {
            _phase = "intro"
            _timerEnd = Time.unscaledTime + INTRO_TIME
        }
    }

    firstEntity_(identifier) {
        var list = GameObject.getAllWithIdentifier(identifier)
        return list.count > 0 ? list[0] : null
    }

    update(stateManager) {
        if (_phase == "entering") {
            _stage.intro((Time.unscaledTime - (_timerEnd - ENTER_TIME)) / ENTER_TIME)
            if (Time.unscaledTime >= _timerEnd) {
                _stage.settle()
                _view.setVisible(true)
                beginChoosing()
            }
        } else if (_phase == "intro") {
            if (Time.unscaledTime >= _timerEnd) {
                beginChoosing()
            }
        } else if (_phase == "choosing") {
            if (armed() && Input.isInputJustReleased("UICancel")) {
                flee()
            }
        } else if (_phase == "targeting") {
            updateTargeting()
        } else if (_phase == "charging") {
            var units = _timeline.tick(Time.unscaledDelta)
            regenStamina(units)
            refreshViews()
            var ready = _timeline.nextReady()
            if (ready != null) {
                resolveMove(ready)
            }
        } else if (_phase == "parryPrompt") {
            // The prompt itself (UI/Combat/ParryPromptUI.wren, wired in resolveMove) answers via
            // onParryAnswer_ - this just enforces the reply deadline if the player never clicks.
            if (Time.unscaledTime - _parryPromptStart >= _parryMove.parryPromptTime) {
                onParryAnswer_(false)
            }
        } else if (_phase == "attacking") {
            var t = (Time.unscaledTime - (_timerEnd - STRIKE_TIME)) / STRIKE_TIME
            if (t > 1) {
                t = 1
            }
            _stage.updateStrike(t)
            if (Time.unscaledTime >= _timerEnd) {
                _phase = "returning"
                _timerEnd = Time.unscaledTime + RETURN_TIME
            }
        } else if (_phase == "returning") {
            var t = (Time.unscaledTime - (_timerEnd - RETURN_TIME)) / RETURN_TIME
            if (t > 1) {
                t = 1
            }
            _stage.updateReturn(t)
            if (Time.unscaledTime >= _timerEnd) {
                showLog(_pendingLogText, _pendingLogNext)
                _pendingLogText = null
                _pendingLogNext = null
            }
        } else if (_phase == "message") {
            if (Time.unscaledTime >= _timerEnd) {
                var step = _nextStep
                _nextStep = null
                step.call()
            }
        } else if (_phase == "over") {
            if (Time.unscaledTime >= _timerEnd) {
                stateManager.clearCurrentState()
            }
        }
    }

    updateTargeting() {
        if (!armed()) {
            return
        }
        var ax = Input.getInputAxis2("UIDir").x
        if (ax.abs < 0.3) {
            _targetAxisHeld = false
        } else if (!_targetAxisHeld) {
            _targetAxisHeld = true
            _targetIndex = stepTarget(_targetIndex, ax > 0 ? 1 : -1)
            refreshViews()
        }
        if (Input.isInputJustReleased("UIAccept")) {
            commitPlayerMoveOn(_pendingMove, _enemies[_targetIndex])
        } else if (Input.isInputJustReleased("UICancel")) {
            _pendingMove = null
            _phase = "choosing"
            arm()
            _view.showMoveMenu(true)
            _view.focusFirstMoveButton()
            refreshViews()
        }
    }

    // --- turn flow -------------------------------------------------------------

    beginChoosing() {
        _phase = "choosing"
        arm()
        clearFlashes()
        commitEnemyMoves()
        _view.setMessage("Choose your move.")
        _view.showMoveMenu(true)
        _view.focusFirstMoveButton()
        refreshViews()
    }

    commitPlayerMove(move) {
        if (move.offensive && livingEnemies().count > 1) {
            _pendingMove = move
            _targetIndex = firstLivingEnemy()
            _phase = "targeting"
            arm()
            _view.showMoveMenu(false)
            _view.setMessage("Choose a target.  <  >")
            refreshViews()
            return
        }
        var target = move.offensive ? _enemies[firstLivingEnemy()] : _player
        commitPlayerMoveOn(move, target)
    }

    commitPlayerMoveOn(move, target) {
        _playerTarget = target
        _pendingMove = null
        _timeline.commit(_player, move)
        _view.showMoveMenu(false)
        _phase = "charging"
        clearFlashes()
        _view.setMessage("")
        refreshViews()
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

    // Looks up the acting combatant's move/target and, for anything parryable landing on a living
    // target, offers that target a parry decision before resolving: a UI prompt for the player
    // (the "parryPrompt" phase - see onParryAnswer_), or an instant random yes/no for an AI-controlled
    // enemy. Everything else (non-offensive moves, no live target) resolves immediately as before.
    resolveMove(actor) {
        var move = _timeline.committedMove(actor)
        var isPlayer = actor == _player
        var target = isPlayer ? _playerTarget : _player
        if (target == null || !target.alive) {
            target = isPlayer ? firstLivingEnemyCombatant() : _player
        }

        if (move.offensive && move.parryable && target != null && target.alive) {
            if (target == _player) {
                _parryActor = actor
                _parryTarget = target
                _parryMove = move
                _parryPromptStart = Time.unscaledTime
                if (_stage != null) {
                    _stage.beginTelegraph(actor)
                }
                _view.showParryPrompt(Fn.new { onParryAnswer_(true) }, Fn.new { onParryAnswer_(false) })
                _phase = "parryPrompt"
                return
            }
            // AI-controlled defender - no UI, just an instant coin flip.
            finishResolve_(actor, move, target, isPlayer, RNG.float() < ENEMY_PARRY_CHANCE)
            return
        }

        finishResolve_(actor, move, target, isPlayer, false)
    }

    // UI/Combat/ParryPromptUI.wren's Yes/No callbacks (wired in resolveMove) and update()'s
    // "parryPrompt" timeout all funnel through here. Guarded against the phase already having moved
    // on, in case a button click and the timeout land the same frame.
    onParryAnswer_(chosen) {
        if (_phase != "parryPrompt") {
            return
        }
        _view.hideParryPrompt()
        finishResolve_(_parryActor, _parryMove, _parryTarget, false, chosen)
    }

    // The actual resolution (damage, stagger/parry-cancel, log, strike animation) - either called
    // straight from resolveMove (nothing to decide, or an AI's instant decision) or from
    // onParryAnswer_ once the player's prompt is answered/times out. parryChosen: true cancels all of
    // the move's damage (Combat/Combatant.wren) and costs the attacker a Combat/Config/Moves.stalled
    // beat; the defender's own in-flight move is also cancelled regardless of side, since choosing to
    // parry is a deliberate reaction to make.
    finishResolve_(actor, move, target, isPlayer, parryChosen) {
        var cancelFraction = parryChosen ? 1 : 0

        _timeline.clear(actor)

        var dealt = 0
        var staggerUnits = 0
        var defenderMoveCancelled = false
        if (target != null) {
            dealt = actor.use(move, target, cancelFraction)
            if (move.offensive && _timeline.isCharging(target)) {
                if (parryChosen) {
                    // Choosing to parry costs the defender their own in-flight move; they have to
                    // pick again instead of the usual partial stagger.
                    _timeline.clear(target)
                    defenderMoveCancelled = true
                } else {
                    staggerUnits = Disruption.delay(move, _timeline.fraction(target))
                    _timeline.interrupt(target, staggerUnits)
                    _staggered.add(target)
                }
            }
            if (parryChosen) {
                // A landed parry also punishes the attacker: instead of letting them pick a real move
                // again immediately, they sit out Combat/Config/Moves.stalled first.
                _timeline.commit(actor, Moves.stalled)
            }
        } else {
            actor.use(move, actor, 0)   // heal / self-buff with nothing to hit
        }

        refreshViews()

        var logText = resolveLine(isPlayer, actor, move, dealt, target, staggerUnits, cancelFraction,
                                  defenderMoveCancelled)
        var afterLog = Fn.new {
            if (_stage != null) {
                _stage.rest()
            }
            if (livingEnemies().count == 0) {
                win()
            } else if (!_player.alive) {
                lose()
            } else if (_timeline.needsMove(_player)) {
                // The player has no move in flight - either their own move just resolved cleanly, or
                // a parry (theirs or one landed on them) just cleared it - either way they choose
                // again. Covers every case without needing to special-case who just acted.
                beginChoosing()
            } else {
                commitEnemyMoves()
                _phase = "charging"
                clearFlashes()
                _view.setMessage("")
                refreshViews()
            }
        }

        if (move.name == "Stalled") {
            if (_stage != null) {
                _stage.showStalled(actor)
            }
            showLog(logText, afterLog)
        } else if (_stage != null && move.offensive && target != null) {
            // Offensive moves against a live target get the curve-driven lunge + impact particle
            // (Combat/BattleStage.wren); everything else (heals/buffs/no-target) keeps the old instant
            // animation-switch and goes straight to the message beat.
            _stage.beginStrike(actor, target, move)
            _pendingLogText = logText
            _pendingLogNext = afterLog
            _phase = "attacking"
            _timerEnd = Time.unscaledTime + STRIKE_TIME
        } else {
            if (_stage != null) {
                _stage.strike(actor, null)
            }
            showLog(logText, afterLog)
        }
    }

    resolveLine(isPlayer, actor, move, dealt, target, staggerUnits, cancelFraction, defenderMoveCancelled) {
        var who = isPlayer ? "You" : actor.name
        if (move.name == "Stalled") {
            return "%(who) %(isPlayer ? "recover" : "recovers") from being parried."
        }
        if (move.healAmount > 0) {
            return "%(who) mends. (+%(move.healAmount) HP)"
        }
        if (move.staminaRestore > 0 && move.damage == 0) {
            return "%(who) recovers. (+%(move.staminaRestore) stamina)"
        }
        var whom = "?"
        if (target != null) {
            whom = (target == _player) ? "you" : target.name
        }
        var line = "%(who) hit%(isPlayer ? "" : "s") %(whom) with %(move.name) - %(dealt) dmg."
        if (cancelFraction > 0) {
            line = "%(line)  Parried! No damage got through."
        }
        if (defenderMoveCancelled) {
            var whoDefended = target == _player ? "Your" : "%(target.name)'s"
            line = "%(line)  %(whoDefended) own move was cancelled!"
        }
        if (staggerUnits > 0) {
            line = "%(line)  Staggered (-%(staggerUnits.floor))!"
        }
        return line
    }

    win() {
        for (e in _enemies) {
            if (e.entity) {
                GameObject.destroy(e.entity)
            }
        }
        endFight(_enemies.count == 1 ? "You defeated the %(_enemies[0].name)!" : "The pack is beaten!")
    }

    lose() {
        endFight("You were overwhelmed...")
    }

    flee() {
        endFight("You slipped away.")
    }

    showLog(text, next) {
        _view.setMessage(text)
        _phase = "message"
        _nextStep = next
        _timerEnd = Time.unscaledTime + MESSAGE_TIME
    }

    endFight(text) {
        _view.setMessage(text)
        _phase = "over"
        _timerEnd = Time.unscaledTime + OVER_TIME
        _view.showMoveMenu(false)
    }

    // --- helpers -------------------------------------------------------------

    regenStamina(units) {
        for (c in _timeline.combatants) {
            c.stats.gainStamina(units * STAMINA_REGEN)
        }
    }

    clearFlashes() {
        _staggered = []
    }

    arm() {
        _navArmedAt = Time.unscaledTime + NAV_ARM_TIME
    }

    armed() {
        return Time.unscaledTime >= _navArmedAt
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
        if (_view == null) {
            return
        }
        _view.refreshPlayer(_player.alive, _player.stats.fraction, _timeline.fraction(_player),
                            _player.stats.staminaFraction, readout(_player), _staggered.contains(_player))
        var i = 0
        for (c in _enemies) {
            _view.refreshEnemy(i, c.alive, c.stats.fraction, _timeline.fraction(c), c.stats.staminaFraction,
                               readout(c), _staggered.contains(c), _phase == "targeting" && i == _targetIndex)
            i = i + 1
        }
    }

    exit() {
        super.exit()
        // Fold the arena away first (reactivates the overworld player, puts survivors back) so the
        // world is coherent again before the UI teardown.
        if (_stage != null) {
            _stage.teardown()
            _stage = null
        }
        if (_view != null) {
            _view.teardown()
            _view = null
        }
        _timeline = null
        _player = null   // just the reference - the Combatant itself lives on the Player script
        _enemies = []
        _enemyDefs = []
        _nextStep = null
        _pendingMove = null
        _pendingLogText = null
        _pendingLogNext = null
        _playerTarget = null
        _parryActor = null
        _parryTarget = null
        _parryMove = null
    }

    opponents { _enemies }
}
