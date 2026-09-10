// states/CombatState.wren
// Phase 5 of the Interrupt Combat Roadmap - Archetypes & the RPS Triangle. The player takes on the
// pack of critters they walked into - 1 vs up to 3, each side carrying an archetype
// (Combat/Archetype.wren) that supplies its move set. The player's battle profile is in
// Combat/BattlePlayer.wren (and its Combatant persists on the Player script, so HP / stamina carry
// between fights); each enemy's is in Combat/BattleCritter.wren, looked up by species name. Every
// body charges its own timeline at once, so turn order is a genuine scramble, and offensive moves
// pick a target. Everything Phases 2-4 established (charge order, interrupts, stamina) carries through.
//
// The triangle these numbers are tuned toward (see Combat/Moves.wren - "expect churn"):
//   Speed beats Power (cheap 2u moves interrupt-lock a wind-up and sustain the lock)
//   Power beats Control (one landed Crush is >half a mage's HP; Control burns out holding the lock)
//   Control beats Speed (Slow wrecks the rhythm, Mend out-heals the chip)
//
// Presentation is a dim overlay over the frozen field (Time.setTimeScale(0)); pacing / Timeline /
// regen all run off Time.unscaled* so they tick through the freeze.
import "States/BaseState" for BaseState
import "input" for Input
import "app" for Application, Time
import "math" for Vec2, Vec4
import "ui" for UIManager, UILabel, UIPanel, TextAlignment
import "resourceManager" for Font
import "gameObject" for GameObject
import "gameObjectComponents" for Script, WorldTransform
import "random" for Random
import "Colors" for WHITE, BLACK, BLANK, LIGHTGRAY

import "Combat/BattleCritter" for BattleCritter
import "Combat/Timeline" for Timeline
import "Combat/Disruption" for Disruption
import "Combat/CombatantView" for CombatantView
import "Combat/BattleStage" for BattleStage

var INTRO_TIME   = 0.9   // fallback (fight-in-place) beat before the menu arms
var ENTER_TIME   = 0.65  // arena sweep + battler slide-in
var MESSAGE_TIME = 0.9
var OVER_TIME    = 1.7
var NAV_ARM_TIME = 0.14   // input lockout after a menu transition, so the prior keypress can't bleed
var STAMINA_REGEN = 2     // stamina per time unit, every combatant, while charging

var RNG = Random.new()

class CombatState is BaseState {
    construct new() {
        super()
        name = "CombatState"
        _root = null
        _phase = "intro"
        _timerEnd = 0
        _navArmedAt = 0
        _nextStep = null
        _timeline = null
        _enemies = []
        _staggered = []
        _pendingMove = null
        _targetIndex = 0
        _targetAxisHeld = false
        _playerTarget = null
        // UI handles - rebuilt in buildUI() each fight, but this state is a reused singleton (see
        // StateManager.insertState), so they must start null and be nulled again on exit():
        // removeUIElement(_root) disposes the foreign objects these wrap, and a stale
        // _playerView.refresh() on the next fight's enter() is a use-after-free.
        // _player is NOT owned here - it's Player.combatant, which persists on the Player script.
        _player = null
        _playerView = null
        _enemyViews = []
        _enemyDefs = []   // BattleCritter per enemy, parallel to _enemies - visuals + numbers
        _moveMenu = null
        _moveButtons = []
        _messageLabel = null
        _stage = null   // BattleStage while an authored arena is in use; null = fight in place
    }

    enter(stateManager, params) {
        super.enter(stateManager, params)

        // Drop last fight's UI wrappers before building this one's - their foreign objects were
        // disposed with the old _root in exit(); touching them again (refreshViews at the end of
        // this method walks _playerView) reads freed memory. _player is re-fetched below.
        _player = null
        _playerView = null
        _enemyViews = []

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

        buildUI()

        var foe = _enemies.count == 1 ? "the %(_enemies[0].name)" : "%(_enemies.count) foes"
        setMessage("You're set upon by %(foe)!")
        refreshViews()

        // Stage the fight in the authored arena if it exists (see Combat/BattleStage.wren's level
        // contract); otherwise fall back to fighting in place under the dim overlay.
        var anchor = firstEntity_("BattleAnchor")
        if (anchor != null) {
            var camStart = WorldTransform.getPosition(params["player"])
            _stage = BattleStage.new(anchor, camStart, params["player"], _player, _enemyDefs, _enemies)
            _root.setVisible(false)
            _phase = "entering"
            _timerEnd = Time.unscaledTime + ENTER_TIME
        } else {
            _phase = "intro"
            _timerEnd = Time.unscaledTime + INTRO_TIME
        }

        Time.setTimeScale(0)
    }

    firstEntity_(identifier) {
        var list = GameObject.getAllWithIdentifier(identifier)
        return list.count > 0 ? list[0] : null
    }

    buildUI() {
        var gw = Application.gameWidth
        var gh = Application.gameHeight
        var font = Font.load("Fonts/medieval_sharp/MedievalSharp-Bold.ttf", 60)

        _root = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(gw, gh), Vec2.new(0, 0))
        _root.setBackgroundColor(Vec4.new(0, 0, 0, 175))
        _root.setBorderColor(BLANK)
        _root.setZIndex(-1)
        UIManager.addUIElement(_root)

        // Enemy stacks across the top.
        _enemyViews = []
        var n = _enemies.count
        var slotW = 300
        var gap = 44
        var startX = (gw - (n * slotW + (n - 1) * gap)) / 2
        for (i in 0...n) {
            var x = startX + i * (slotW + gap)
            _enemyViews.add(CombatantView.new(_root, _enemies[i], x, 40, slotW, true, font))
        }

        // Battle log / prompt, centre.
        _messageLabel = simpleLabel("", Vec2.new(0, 0), Vec2.new(0.5, 0.4), 30.0, font)
        _messageLabel.setAlignment(TextAlignment.CENTER)
        _messageLabel.setAnchorPoint(Vec2.new(0.5, 0.5))
        _root.addChild(_messageLabel)

        // Player stack, bottom-left.
        _playerView = CombatantView.new(_root, _player, 56, gh - 150, 320, true, font)

        // Move menu, bottom-right - one button per move in the player's kit, plus Flee.
        _moveMenu = menuPanel(Vec2.new(320, 300))
        _moveButtons = []
        var i = 0
        for (move in _player.moves) {
            var chosen = move
            var b = makeButton(_moveMenu, chosen.menuLabel, i, font)
            b.setOnClick { |s, m|
                if (_phase == "choosing" && armed()) {
                    commitPlayerMove(chosen)
                }
            }
            _moveButtons.add(b)
            i = i + 1
        }
        var fleeBtn = makeButton(_moveMenu, "Flee", i, font)
        fleeBtn.setOnClick { |s, m|
            if (_phase == "choosing" && armed()) {
                flee()
            }
        }
        _moveMenu.setVisible(false)

        font.unload()
    }

    menuPanel(size) {
        var p = UIPanel.new(Vec2.new(-30, -30), Vec2.new(1, 1), size, Vec2.new(0, 0))
        p.setAnchorPoint(Vec2.new(1, 1))
        p.setBackgroundColor(BLANK)
        p.setBorderColor(BLANK)
        _root.addChild(p)
        return p
    }

    simpleLabel(text, absPos, relPos, size, font) {
        var lbl = UILabel.new(absPos, relPos, text, size)
        lbl.setFont(font)
        lbl.setTextColor(WHITE)
        lbl.setBoundingBoxToText()
        return lbl
    }

    makeButton(menu, text, index, font) {
        var h = 44
        var button = UIPanel.new(Vec2.new(0, index * (h + 8)), Vec2.new(0, 0), Vec2.new(304, h), Vec2.new(0, 0))
        button.setBackgroundColor(LIGHTGRAY)
        button.setBorderColor(WHITE)
        button.setBorderWidth(2)
        button.setFocusable(true)
        menu.addChild(button)

        var label = UILabel.new(Vec2.new(0, 0), Vec2.new(0.5, 0.5), text, 19.0)
        label.setFont(font)
        label.setTextColor(BLACK)
        label.setAlignment(TextAlignment.CENTER)
        label.setBoundingBoxToText()
        label.setAnchorPoint(Vec2.new(0.5, 0.5))
        label.setZIndex(10)
        button.addChild(label)

        button.setOnFocus { |s| button.setBackgroundColor(WHITE) }
        button.setOnLoseFocus { |s| button.setBackgroundColor(LIGHTGRAY) }
        return button
    }

    update(stateManager) {
        if (_phase == "entering") {
            _stage.intro((Time.unscaledTime - (_timerEnd - ENTER_TIME)) / ENTER_TIME)
            if (Time.unscaledTime >= _timerEnd) {
                _stage.settle()
                _root.setVisible(true)
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
            _moveMenu.setVisible(true)
            UIManager.setFocus(_moveButtons[0])
            refreshViews()
        }
    }

    // --- turn flow -------------------------------------------------------------

    beginChoosing() {
        _phase = "choosing"
        arm()
        clearFlashes()
        commitEnemyMoves()
        setMessage("Choose your move.")
        _moveMenu.setVisible(true)
        UIManager.setFocus(_moveButtons[0])
        refreshViews()
    }

    commitPlayerMove(move) {
        if (move.offensive && livingEnemies().count > 1) {
            _pendingMove = move
            _targetIndex = firstLivingEnemy()
            _phase = "targeting"
            arm()
            _moveMenu.setVisible(false)
            setMessage("Choose a target.  <  >")
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
        _moveMenu.setVisible(false)
        _phase = "charging"
        clearFlashes()
        setMessage("")
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

    resolveMove(actor) {
        var move = _timeline.committedMove(actor)
        var isPlayer = actor == _player
        var target = isPlayer ? _playerTarget : _player
        if (target == null || !target.alive) {
            target = isPlayer ? firstLivingEnemyCombatant() : _player
        }

        var dealt = 0
        var staggerUnits = 0
        if (target != null) {
            dealt = actor.use(move, target)
            if (move.offensive && _timeline.isCharging(target)) {
                staggerUnits = Disruption.delay(move, _timeline.fraction(target))
                _timeline.interrupt(target, staggerUnits)
                _staggered.add(target)
            }
        } else {
            actor.use(move, actor)   // heal / self-buff with nothing to hit
        }

        _timeline.clear(actor)
        refreshViews()

        if (_stage != null) {
            _stage.strike(actor, move.offensive ? target : null)
        }

        showLog(resolveLine(isPlayer, actor, move, dealt, target, staggerUnits), Fn.new {
            if (_stage != null) {
                _stage.rest()
            }
            if (livingEnemies().count == 0) {
                win()
            } else if (!_player.alive) {
                lose()
            } else if (isPlayer) {
                beginChoosing()
            } else {
                commitEnemyMoves()
                _phase = "charging"
                clearFlashes()
                setMessage("")
                refreshViews()
            }
        })
    }

    resolveLine(isPlayer, actor, move, dealt, target, staggerUnits) {
        var who = isPlayer ? "You" : actor.name
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
        setMessage(text)
        _phase = "message"
        _nextStep = next
        _timerEnd = Time.unscaledTime + MESSAGE_TIME
    }

    endFight(text) {
        setMessage(text)
        _phase = "over"
        _timerEnd = Time.unscaledTime + OVER_TIME
        _moveMenu.setVisible(false)
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

    setMessage(text) {
        _messageLabel.setText(text)
        _messageLabel.setBoundingBoxToText()
    }

    readout(combatant) {
        var m = _timeline.committedMove(combatant)
        if (m == null) {
            return ""
        }
        return combatant.stats.exhausted ? "%(m.name)  (slow)" : m.name
    }

    refreshViews() {
        if (_root == null) {
            return
        }
        if (_playerView != null) {
            _playerView.refresh(_timeline.fraction(_player), readout(_player), _staggered.contains(_player))
        }
        var i = 0
        for (v in _enemyViews) {
            var c = _enemies[i]
            v.refresh(_timeline.fraction(c), readout(c), _staggered.contains(c))
            v.selected = (_phase == "targeting" && i == _targetIndex)
            i = i + 1
        }
    }

    exit() {
        Time.setTimeScale(1)
        super.exit()
        // Fold the arena away first (reactivates the overworld player, puts survivors back) so the
        // world is coherent again before the UI teardown.
        if (_stage != null) {
            _stage.teardown()
            _stage = null
        }
        if (_root) {
            UIManager.removeUIElement(_root)
            _root = null
        }
        // _root's subtree is gone now - drop every wrapper that pointed into it so nothing touches a
        // disposed foreign object before the next fight rebuilds them.
        _playerView = null
        _enemyViews = []
        _moveMenu = null
        _moveButtons = []
        _messageLabel = null
        _timeline = null
        _player = null   // just the reference - the Combatant itself lives on the Player script
        _enemies = []
        _enemyDefs = []
        _nextStep = null
        _pendingMove = null
        _playerTarget = null
    }

    opponents { _enemies }
}
