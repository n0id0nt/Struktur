// states/CombatState.wren
// Phase 2 of the Interrupt Combat Roadmap - Visible Timelines. Turn order comes from charge, not
// seat order: every combatant charges toward a committed Move, and whoever's bar fills first acts.
// Still linear - no interrupts yet (Phase 3). The player picks from a short move list with different
// charge costs, so ordering actually varies with their choices; the enemy has a single move.
//
// Entered from ExperimentState when the player engages a critter (forced by an aggressive one,
// chosen via the prompt for a passive one - see ExperimentState.checkCombatTriggers()).
// Presentation is a full-screen dim overlay over the frozen field (Time.setTimeScale(0)); this
// state's own pacing and the Timeline run off Time.unscaledDelta / unscaledTime, so they keep
// ticking through that freeze.
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
import "Combat/Move" for Move
import "Combat/Timeline" for Timeline
import "Combat/HealthBar" for HealthBar
import "Combat/ChargeBar" for ChargeBar

var INTRO_TIME   = 0.8   // beat before the menu arms (also lets the Interact keypress fully release)
var MESSAGE_TIME = 0.9   // each battle-log line holds this long before the turn advances
var OVER_TIME    = 1.6   // final result holds this long before returning to the overworld

var PLAYER_MAX_HP = 30

class CombatState is BaseState {
    construct new() {
        super()
        name = "CombatState"
        _root = null
        _phase = "intro"
        _timerEnd = 0
        _nextStep = null
        _timeline = null
    }

    // The player's move list. Slower moves are more damage per time unit, but you eat more enemy
    // hits while you charge them - that trade sharpens in Phase 3 when charge time also sets how
    // hard you can be interrupted.
    playerMoves() {
        return [
            Move.new("Jab", 2, 3),
            Move.new("Strike", 4, 7),
            Move.new("Heavy Blow", 6, 13)
        ]
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

        _player = Combatant.new("You", PLAYER_MAX_HP, playerMoves(), null)
        var enemyMove = Move.new("Attack", enemyScript.combatMoveCost, enemyScript.combatAttack)
        _enemy = Combatant.new(enemyScript.name, enemyScript.combatMaxHp, [enemyMove], _opponentEntity)

        _timeline = Timeline.new()
        _timeline.add(_player)
        _timeline.add(_enemy)

        buildUI()

        _phase = "intro"
        _timerEnd = Time.unscaledTime + INTRO_TIME
        setMessage("You face off against the %(_enemy.name)!")
        refreshBars()
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

        // Enemy: name, HP bar, charge bar + committed-move readout, top centre.
        var ex = (gw / 2) - 170
        var enemyName = simpleLabel(_enemy.name, Vec2.new(0, 42), Vec2.new(0.5, 0), 34.0, font)
        enemyName.setAlignment(TextAlignment.CENTER)
        enemyName.setAnchorPoint(Vec2.new(0.5, 0))
        _root.addChild(enemyName)
        _enemyHpBar = HealthBar.new(_root, ex, 90, 340, 20)
        _enemyChargeBar = ChargeBar.new(_root, ex, 114, 340, 10)
        _enemyMoveLabel = simpleLabel("", Vec2.new(ex + 350, 106), Vec2.new(0, 0), 18.0, font)
        _root.addChild(_enemyMoveLabel)

        // Player: name, HP bar, charge bar + committed-move readout, bottom left.
        var px = 48
        var py = gh - 150
        var playerName = simpleLabel(_player.name, Vec2.new(px, py - 34), Vec2.new(0, 0), 28.0, font)
        _root.addChild(playerName)
        _playerHpBar = HealthBar.new(_root, px, py, 280, 18)
        _playerChargeBar = ChargeBar.new(_root, px, py + 22, 280, 10)
        _playerMoveLabel = simpleLabel("", Vec2.new(px + 290, py + 14), Vec2.new(0, 0), 18.0, font)
        _root.addChild(_playerMoveLabel)

        // Battle log / prompt, centre.
        _messageLabel = simpleLabel("", Vec2.new(0, 0), Vec2.new(0.5, 0.34), 30.0, font)
        _messageLabel.setAlignment(TextAlignment.CENTER)
        _messageLabel.setAnchorPoint(Vec2.new(0.5, 0.5))
        _root.addChild(_messageLabel)

        // Move menu, bottom right - one container so it can be shown/hidden wholesale.
        _menu = UIPanel.new(Vec2.new(-32, -32), Vec2.new(1, 1), Vec2.new(280, 244), Vec2.new(0, 0))
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
        var h = 46
        var button = UIPanel.new(Vec2.new(0, index * (h + 8)), Vec2.new(0, 0), Vec2.new(260, h), Vec2.new(0, 0))
        button.setBackgroundColor(LIGHTGRAY)
        button.setBorderColor(WHITE)
        button.setBorderWidth(2)
        button.setFocusable(true)
        _menu.addChild(button)

        var label = UILabel.new(Vec2.new(0, 0), Vec2.new(0.5, 0.5), text, 22.0)
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
            _timeline.tick(Time.unscaledDelta)
            refreshChargeBars()
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

    // Player needs to pick - menu open, Timeline paused (planning time). The enemy commits here too
    // so both start charging together the moment the player chooses.
    beginChoosing() {
        _phase = "choosing"
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
        setMessage("")
        refreshTimeline()
    }

    resolveMove(actor) {
        var move = _timeline.committedMove(actor)
        var isPlayer = actor == _player
        var target = isPlayer ? _enemy : _player
        var dealt = actor.use(move, target)
        _timeline.clear(actor)
        refreshBars()
        refreshTimeline()

        var line = "%(_enemy.name) lands %(move.name) - %(dealt) damage!"
        if (isPlayer) {
            line = "You land %(move.name) - %(dealt) damage!"
        }
        showLog(line, Fn.new {
            if (!_enemy.alive) {
                win()
            } else if (!_player.alive) {
                lose()
            } else if (isPlayer) {
                beginChoosing()
            } else {
                // Enemy re-commits its one move and both resume charging (the player's own charge
                // was untouched - they didn't act).
                _timeline.commit(_enemy, _enemy.moves[0])
                _phase = "charging"
                setMessage("")
                refreshTimeline()
            }
        })
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

    // Show a line, then run `next` (a Fn) once it has held for MESSAGE_TIME.
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

    refreshBars() {
        _playerHpBar.setFraction(_player.stats.fraction)
        _enemyHpBar.setFraction(_enemy.stats.fraction)
    }

    // Bars only - cheap, called every frame while charging.
    refreshChargeBars() {
        _playerChargeBar.setFraction(_timeline.fraction(_player))
        _enemyChargeBar.setFraction(_timeline.fraction(_enemy))
    }

    // Bars + the committed-move readouts - called only when a move is committed or cleared.
    refreshTimeline() {
        refreshChargeBars()
        setLabel(_playerMoveLabel, moveReadout(_timeline.committedMove(_player)))
        setLabel(_enemyMoveLabel, moveReadout(_timeline.committedMove(_enemy)))
    }

    setLabel(label, text) {
        label.setText(text)
        label.setBoundingBoxToText()
    }

    moveReadout(move) {
        return move == null ? "" : move.name
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
