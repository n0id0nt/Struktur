// states/CombatState.wren
// Phase 4 of the Interrupt Combat Roadmap - Stamina & Move Tiers. On top of Phase 2's charge-order
// turns and Phase 3's interrupts: every move now costs stamina, stamina trickles back while you
// wind up, and dropping below 20% adds a time unit to everything you commit (exhaustion). Guard
// buys stamina back at the cost of a turn. So the move list is a real economy, not just a timing
// gamble - spam your Tier-3 hammer and you tire out and slow down; pace yourself and you don't.
//
// Entered from ExperimentState when the player engages a critter. Presentation is a full-screen dim
// overlay over the frozen field (Time.setTimeScale(0)); this state's pacing, the Timeline, and
// stamina regen all run off Time.unscaledDelta / unscaledTime so they tick through that freeze.
import "States/BaseState" for BaseState
import "input" for Input
import "app" for Application, Time
import "math" for Vec2, Vec4
import "ui" for UIManager, UILabel, UIPanel, TextAlignment
import "resourceManager" for Font
import "gameObject" for GameObject
import "gameObjectComponents" for Script
import "Colors" for WHITE, BLACK, BLANK, LIGHTGRAY

import "Combat/Combatant" for Combatant
import "Combat/Moves" for Moves
import "Combat/Timeline" for Timeline
import "Combat/Disruption" for Disruption
import "Combat/HealthBar" for HealthBar
import "Combat/ChargeBar" for ChargeBar
import "Combat/StaminaBar" for StaminaBar

var INTRO_TIME   = 0.8   // beat before the menu arms (also lets the Interact keypress fully release)
var MESSAGE_TIME = 0.9   // each battle-log line holds this long before the turn advances
var OVER_TIME    = 1.6   // final result holds this long before returning to the overworld

var PLAYER_MAX_HP      = 120
var PLAYER_MAX_STAMINA = 70
var STAMINA_REGEN      = 2   // stamina per time unit, applied to both combatants while charging

class CombatState is BaseState {
    construct new() {
        super()
        name = "CombatState"
        _root = null
        _phase = "intro"
        _timerEnd = 0
        _nextStep = null
        _timeline = null
        _playerStaggered = false
        _enemyStaggered = false
    }

    enter(stateManager, params) {
        super.enter(stateManager, params)

        _opponentEntity = params["opponent"]
        var enemyScript = Script.getInstance(_opponentEntity)
        if (!enemyScript) {
            System.print("[CombatState] opponent %(_opponentEntity) has no script, aborting")
            stateManager.clearCurrentState()
            return
        }

        _player = Combatant.new("You", PLAYER_MAX_HP, PLAYER_MAX_STAMINA, Moves.playerKit, null)
        _enemy = Combatant.new(enemyScript.name, enemyScript.combatMaxHp, enemyScript.combatStamina,
                               [Moves.critterAttack(enemyScript)], _opponentEntity)

        _timeline = Timeline.new()
        _timeline.add(_player)
        _timeline.add(_enemy)

        buildUI()

        _phase = "intro"
        _timerEnd = Time.unscaledTime + INTRO_TIME
        setMessage("You face off against the %(_enemy.name)!")
        refreshVitals()
        refreshTimeline()

        Time.setTimeScale(0)
    }

    buildUI() {
        var gw = Application.gameWidth
        var gh = Application.gameHeight
        var font = Font.load("Fonts/medieval_sharp/MedievalSharp-Bold.ttf", 60)

        _root = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(gw, gh), Vec2.new(0, 0))
        _root.setBackgroundColor(Vec4.new(0, 0, 0, 165))
        _root.setBorderColor(BLANK)
        _root.setZIndex(-1)
        UIManager.addUIElement(_root)

        // Enemy: name, HP / charge / stamina bars, committed-move readout - top centre.
        var ex = (gw / 2) - 170
        var enemyName = simpleLabel(_enemy.name, Vec2.new(0, 40), Vec2.new(0.5, 0), 34.0, font)
        enemyName.setAlignment(TextAlignment.CENTER)
        enemyName.setAnchorPoint(Vec2.new(0.5, 0))
        _root.addChild(enemyName)
        _enemyHpBar = HealthBar.new(_root, ex, 88, 340, 18)
        _enemyChargeBar = ChargeBar.new(_root, ex, 110, 340, 9)
        _enemyStaminaBar = StaminaBar.new(_root, ex, 123, 340, 6)
        _enemyMoveLabel = simpleLabel("", Vec2.new(ex + 350, 100), Vec2.new(0, 0), 18.0, font)
        _root.addChild(_enemyMoveLabel)

        // Player: name, HP / charge / stamina bars, committed-move readout - bottom left.
        var px = 48
        var py = gh - 158
        var playerName = simpleLabel(_player.name, Vec2.new(px, py - 34), Vec2.new(0, 0), 28.0, font)
        _root.addChild(playerName)
        _playerHpBar = HealthBar.new(_root, px, py, 300, 18)
        _playerChargeBar = ChargeBar.new(_root, px, py + 22, 300, 9)
        _playerStaminaBar = StaminaBar.new(_root, px, py + 35, 300, 7)
        _playerMoveLabel = simpleLabel("", Vec2.new(px + 312, py + 12), Vec2.new(0, 0), 18.0, font)
        _root.addChild(_playerMoveLabel)

        // Battle log / prompt, centre.
        _messageLabel = simpleLabel("", Vec2.new(0, 0), Vec2.new(0.5, 0.32), 30.0, font)
        _messageLabel.setAlignment(TextAlignment.CENTER)
        _messageLabel.setAnchorPoint(Vec2.new(0.5, 0.5))
        _root.addChild(_messageLabel)

        // Move menu, bottom right - one container so it can be shown/hidden wholesale.
        _menu = UIPanel.new(Vec2.new(-30, -30), Vec2.new(1, 1), Vec2.new(304, 288), Vec2.new(0, 0))
        _menu.setAnchorPoint(Vec2.new(1, 1))
        _menu.setBackgroundColor(BLANK)
        _menu.setBorderColor(BLANK)
        _root.addChild(_menu)

        _moveButtons = []
        var i = 0
        for (move in _player.moves) {
            var chosen = move
            var btn = makeButton(chosen.menuLabel, i, font)
            btn.setOnClick { |sender, mousePos|
                if (_phase == "choosing") {
                    commitPlayerMove(chosen)
                }
            }
            _moveButtons.add(btn)
            i = i + 1
        }
        _fleeButton = makeButton("Flee", i, font)
        _fleeButton.setOnClick { |sender, mousePos|
            if (_phase == "choosing") {
                flee()
            }
        }
        _menu.setVisible(false)   // hidden through the intro beat; shown in beginChoosing()

        font.unload()
    }

    simpleLabel(text, absPos, relPos, size, font) {
        var lbl = UILabel.new(absPos, relPos, text, size)
        lbl.setFont(font)
        lbl.setTextColor(WHITE)
        lbl.setBoundingBoxToText()
        return lbl
    }

    // Button `index` counts from the top of the menu stack; buttons are parented under _menu.
    makeButton(text, index, font) {
        var h = 44
        var button = UIPanel.new(Vec2.new(0, index * (h + 8)), Vec2.new(0, 0), Vec2.new(288, h), Vec2.new(0, 0))
        button.setBackgroundColor(LIGHTGRAY)
        button.setBorderColor(WHITE)
        button.setBorderWidth(2)
        button.setFocusable(true)
        _menu.addChild(button)

        var label = UILabel.new(Vec2.new(0, 0), Vec2.new(0.5, 0.5), text, 20.0)
        label.setFont(font)
        label.setTextColor(BLACK)
        label.setAlignment(TextAlignment.CENTER)
        label.setBoundingBoxToText()
        label.setAnchorPoint(Vec2.new(0.5, 0.5))
        label.setZIndex(10)
        button.addChild(label)

        button.setOnFocus { |sender| button.setBackgroundColor(WHITE) }
        button.setOnLoseFocus { |sender| button.setBackgroundColor(LIGHTGRAY) }
        return button
    }

    update(stateManager) {
        if (_phase == "intro") {
            if (Time.unscaledTime >= _timerEnd) {
                beginChoosing()
            }
        } else if (_phase == "charging") {
            var units = _timeline.tick(Time.unscaledDelta)
            regenStamina(units)
            refreshChargeBars()
            refreshVitals()
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

    regenStamina(units) {
        _player.stats.gainStamina(units * STAMINA_REGEN)
        _enemy.stats.gainStamina(units * STAMINA_REGEN)
    }

    // Player needs to pick - menu open, Timeline paused (planning time). The enemy commits here too
    // (Timeline.commit bakes in an exhaustion penalty if it's tired). An interrupted enemy keeps its
    // pushed-back charge - only a fired move gets re-committed.
    beginChoosing() {
        _phase = "choosing"
        clearStagger()
        setMessage("Choose your move.")
        _menu.setVisible(true)
        UIManager.setFocus(_moveButtons[0])
        if (_timeline.needsMove(_enemy)) {
            _timeline.commit(_enemy, _enemy.moves[0])
        }
        refreshTimeline()
    }

    commitPlayerMove(move) {
        _timeline.commit(_player, move)
        _menu.setVisible(false)
        _phase = "charging"
        clearStagger()
        setMessage("")
        refreshTimeline()
    }

    resumeCharging() {
        _timeline.commit(_enemy, _enemy.moves[0])
        _phase = "charging"
        clearStagger()
        setMessage("")
        refreshTimeline()
    }

    resolveMove(actor) {
        var move = _timeline.committedMove(actor)
        var isPlayer = actor == _player
        var target = isPlayer ? _enemy : _player
        var dealt = actor.use(move, target)

        // Interrupt: an offensive hit landing on a still-winding-up target shoves their charge back.
        var staggerUnits = 0
        if (move.damage > 0 && _timeline.isCharging(target)) {
            staggerUnits = Disruption.delay(move, _timeline.fraction(target))
            _timeline.interrupt(target, staggerUnits)
            if (isPlayer) {
                _enemyStaggered = true
            } else {
                _playerStaggered = true
            }
        }

        _timeline.clear(actor)
        refreshVitals()
        refreshTimeline()

        showLog(resolveLine(isPlayer, move, dealt, staggerUnits), Fn.new {
            if (!_enemy.alive) {
                win()
            } else if (!_player.alive) {
                lose()
            } else if (isPlayer) {
                beginChoosing()
            } else {
                resumeCharging()
            }
        })
    }

    resolveLine(isPlayer, move, dealt, staggerUnits) {
        if (move.damage == 0) {
            return "You brace yourself. (+%(move.staminaRestore) stamina)"
        }
        var attacker = isPlayer ? "You land" : "%(_enemy.name) lands"
        var line = "%(attacker) %(move.name) - %(dealt) dmg."
        if (staggerUnits > 0) {
            var victim = isPlayer ? "%(_enemy.name) is" : "You're"
            line = "%(line)  %(victim) staggered (-%(staggerUnits.floor))!"
        }
        return line
    }

    win() {
        if (_enemy.entity) {
            GameObject.destroy(_enemy.entity)
        }
        endFight("You defeated the %(_enemy.name)!")
    }

    lose() {
        endFight("The %(_enemy.name) got the better of you...")
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
        _menu.setVisible(false)
    }

    setMessage(text) {
        _messageLabel.setText(text)
        _messageLabel.setBoundingBoxToText()
    }

    clearStagger() {
        _playerStaggered = false
        _enemyStaggered = false
    }

    // HP + stamina bars - called after any damage / stamina change, and every frame while charging
    // (stamina regenerates continuously).
    refreshVitals() {
        _playerHpBar.setFraction(_player.stats.fraction)
        _enemyHpBar.setFraction(_enemy.stats.fraction)
        _playerStaminaBar.setFraction(_player.stats.staminaFraction)
        _enemyStaminaBar.setFraction(_enemy.stats.staminaFraction)
    }

    // Charge bars + stagger flashes - called every frame while charging.
    refreshChargeBars() {
        _playerChargeBar.setFraction(_timeline.fraction(_player))
        _playerChargeBar.setStaggered(_playerStaggered)
        _enemyChargeBar.setFraction(_timeline.fraction(_enemy))
        _enemyChargeBar.setStaggered(_enemyStaggered)
    }

    // Charge bars + the committed-move readouts - called when a move is committed, cleared, or
    // interrupted.
    refreshTimeline() {
        refreshChargeBars()
        setLabel(_playerMoveLabel, readout(_player))
        setLabel(_enemyMoveLabel, readout(_enemy))
    }

    readout(combatant) {
        var move = _timeline.committedMove(combatant)
        if (move == null) {
            return ""
        }
        return combatant.stats.exhausted ? "%(move.name)  (exhausted)" : move.name
    }

    setLabel(label, text) {
        label.setText(text)
        label.setBoundingBoxToText()
    }

    exit() {
        // Restore first, unconditionally - a half-built UI must never leave the field frozen.
        Time.setTimeScale(1)
        super.exit()

        if (_root) {
            UIManager.removeUIElement(_root)
            _root = null
        }
        _opponentEntity = null
        _player = null
        _enemy = null
        _timeline = null
        _nextStep = null
    }

    // Getter
    opponent { _opponentEntity }
}
