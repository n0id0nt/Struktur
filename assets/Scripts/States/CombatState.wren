// states/CombatState.wren
// Phase 1 of the Interrupt Combat Roadmap - "a complete, boring, winnable fight". Strictly
// alternating turns, one "Attack" action, flat damage, no timing. Entered from ExperimentState when
// the player engages a critter (forced by an aggressive one, chosen via the prompt for a passive
// one - see ExperimentState.checkCombatTriggers()).
//
// Presentation is a full-screen dim overlay over the frozen overworld: the field is paused with
// Time.setTimeScale(0) on enter and restored on exit (this state's own pacing runs off
// Time.unscaledTime so it keeps ticking through that freeze). Everything downstream in the roadmap -
// the visible timeline, the interrupt formula, stamina - replaces the turn loop in this file; the
// Combatant/Stats/HealthBar pieces it leans on are meant to survive.
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
import "Combat/HealthBar" for HealthBar

var INTRO_TIME   = 0.8   // seconds the "you face off..." beat holds before the menu arms (also lets
                         // the Interact keypress that started the fight fully release first)
var MESSAGE_TIME = 0.9   // seconds each battle-log line holds before the turn advances
var OVER_TIME    = 1.6   // seconds the final result holds before returning to the overworld

var PLAYER_MAX_HP  = 30
var PLAYER_ATTACK  = 6

class CombatState is BaseState {
    construct new() {
        super()
        name = "CombatState"
        _root = null
        _phase = "intro"
        _timerEnd = 0
        _nextStep = null
    }

    enter(stateManager, params) {
        super.enter(stateManager, params)

        _opponentEntity = params["opponent"]
        var enemyScript = Script.getInstance(_opponentEntity)
        if (!enemyScript) {
            // The critter vanished between engaging and this frame - nothing to fight.
            System.print("[CombatState] opponent %(_opponentEntity) has no script, aborting")
            stateManager.clearCurrentState()
            return
        }

        _player = Combatant.new("You", PLAYER_MAX_HP, PLAYER_ATTACK, null)
        _enemy = Combatant.new(enemyScript.name, enemyScript.combatMaxHp, enemyScript.combatAttack, _opponentEntity)

        buildUI()

        _phase = "intro"
        _timerEnd = Time.unscaledTime + INTRO_TIME
        setMessage("You face off against the %(_enemy.name)!")
        refreshBars()

        // Freeze the overworld last, once everything above has succeeded - an error while building
        // the UI then can't strand the game paused.
        Time.setTimeScale(0)
    }

    buildUI() {
        var gw = Application.gameWidth
        var gh = Application.gameHeight
        var font = Font.load("Fonts/medieval_sharp/MedievalSharp-Bold.ttf", 60)

        _root = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(gw, gh), Vec2.new(0, 0))
        _root.setBackgroundColor(Vec4.new(0, 0, 0, 165))
        _root.setBorderColor(BLANK)
        _root.setZIndex(-1)   // over anything the overworld left on screen
        UIManager.addUIElement(_root)

        // Enemy: name + bar, top centre.
        var enemyName = UILabel.new(Vec2.new(0, 46), Vec2.new(0.5, 0), _enemy.name, 34.0)
        enemyName.setFont(font)
        enemyName.setTextColor(WHITE)
        enemyName.setAlignment(TextAlignment.CENTER)
        enemyName.setBoundingBoxToText()
        enemyName.setAnchorPoint(Vec2.new(0.5, 0))
        _root.addChild(enemyName)
        _enemyBar = HealthBar.new(_root, (gw / 2) - 170, 92, 340, 22)

        // Player: name + bar, bottom left.
        var playerName = UILabel.new(Vec2.new(48, gh - 168), Vec2.new(0, 0), _player.name, 28.0)
        playerName.setFont(font)
        playerName.setTextColor(WHITE)
        playerName.setBoundingBoxToText()
        playerName.setAnchorPoint(Vec2.new(0, 0))
        _root.addChild(playerName)
        _playerBar = HealthBar.new(_root, 48, gh - 134, 280, 20)

        // Battle log / prompt, centre.
        _messageLabel = UILabel.new(Vec2.new(0, 0), Vec2.new(0.5, 0.36), "", 30.0)
        _messageLabel.setFont(font)
        _messageLabel.setTextColor(WHITE)
        _messageLabel.setAlignment(TextAlignment.CENTER)
        _messageLabel.setAnchorPoint(Vec2.new(0.5, 0.5))
        _root.addChild(_messageLabel)

        // Actions, bottom right. Not focused until the intro beat ends (see update()).
        _attackButton = makeButton("Attack", 0, font)
        _attackButton.setOnClick { |sender, mousePos|
            if (_phase == "choosing") {
                playerAttack()
            }
        }
        _fleeButton = makeButton("Flee", 1, font)
        _fleeButton.setOnClick { |sender, mousePos|
            if (_phase == "choosing") {
                flee()
            }
        }

        font.unload()
    }

    // Panel + centred label, focusable, with a focus highlight - the same shape MainMenuState builds
    // its menu buttons from. index 0 sits above index 1, anchored to the screen's bottom-right.
    makeButton(text, index, font) {
        var button = UIPanel.new(Vec2.new(-40, -150 + index * 66), Vec2.new(1, 1), Vec2.new(200, 54), Vec2.new(0, 0))
        button.setAnchorPoint(Vec2.new(1, 1))
        button.setBackgroundColor(LIGHTGRAY)
        button.setBorderColor(WHITE)
        button.setBorderWidth(2)
        button.setFocusable(true)
        _root.addChild(button)

        var label = UILabel.new(Vec2.new(0, 0), Vec2.new(0.5, 0.5), text, 24.0)
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
        } else if (_phase == "message") {
            if (Time.unscaledTime >= _timerEnd) {
                var step = _nextStep
                _nextStep = null
                step.call()
            }
        } else if (_phase == "choosing") {
            // UICancel is a shortcut for the Flee button.
            if (Input.isInputJustReleased("UICancel")) {
                flee()
            }
        } else if (_phase == "over") {
            if (Time.unscaledTime >= _timerEnd) {
                stateManager.clearCurrentState()
            }
        }
    }

    beginChoosing() {
        _phase = "choosing"
        setMessage("What will you do?")
        UIManager.setFocus(_attackButton)
    }

    playerAttack() {
        var dealt = _player.attack(_enemy)
        refreshBars()
        showLog("You hit %(_enemy.name) for %(dealt)!", Fn.new {
            if (!_enemy.alive) {
                win()
            } else {
                enemyTurn()
            }
        })
    }

    enemyTurn() {
        var dealt = _enemy.attack(_player)
        refreshBars()
        showLog("%(_enemy.name) hits you for %(dealt)!", Fn.new {
            if (!_player.alive) {
                lose()
            } else {
                beginChoosing()
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

    // Show a line, then run `next` (a Fn) once it has held for MESSAGE_TIME. The Attack/Flee buttons
    // stay focused (and lit) through the message, but their handlers no-op unless _phase ==
    // "choosing", so a stray press does nothing - cheaper and safer than deregistering them
    // (clearFocusElements() would wipe the whole focus registry, not just re-arm).
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
    }

    setMessage(text) {
        _messageLabel.setText(text)
        _messageLabel.setBoundingBoxToText()
    }

    refreshBars() {
        _playerBar.setFraction(_player.stats.fraction)
        _enemyBar.setFraction(_enemy.stats.fraction)
    }

    exit() {
        // Restore first, unconditionally - a half-built UI must never leave the field frozen.
        Time.setTimeScale(1)
        super.exit()

        if (_root) {
            // Recurses the whole subtree, disposing each node - that unregisters the buttons from
            // the focus navigator and clears any focus/hover pointing at them (UIManager::RemoveElement).
            UIManager.removeUIElement(_root)
            _root = null
        }
        _opponentEntity = null
        _player = null
        _enemy = null
        _nextStep = null
    }

    // Getter
    opponent { _opponentEntity }
}
