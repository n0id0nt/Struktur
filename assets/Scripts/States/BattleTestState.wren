// States/BattleTestState.wren
// A sandbox for trying moves and stats in real battles. A setup screen (UI/BattleTest/BattleSetupUI.wren) picks up
// to three fighters per side from Combat/Config/BattleTestRoster.wren - the Training Dummy included - and the moves
// each one knows (cycling through Combat/Config/Moves.catalog); Start runs the real CombatState in the Battle_Arena
// with those sides, then comes back here with the result. You command your side exactly as in the game; the other
// side is the normal EnemyAI. Every fighter starts fresh at full health, nothing is saved, and the overworld isn't
// touched. While a battle runs, a live readout (UI/BattleTest/BattleReadoutUI.wren) shows every fighter's exact HP /
// stamina / charge and the Arbiter's latest rolls (the console's [Arbiter] lines have the full breakdowns).
// Opened with F6 from the overworld (States/ExperimentState.wren); Back returns to the game.
import "app" for Time
import "gameObject" for GameObject
import "gameObjectComponents" for World
import "States/BaseState" for BaseState
import "States/StateManager" for StateManager
import "States/CombatState" for CombatState
import "Combat/CombatOutcome" for CombatOutcome
import "Combat/Config/Moves" for Moves
import "Combat/Config/BattleTestRoster" for BattleTestRoster
import "Combat/Arbiter" for Arbiter, RECENT_ROLLS_KEPT
import "UI/BattleTest/BattleSetupUI" for BattleSetupUI, SLOTS_PER_SIDE, MOVES_PER_FIGHTER
import "UI/BattleTest/BattleReadoutUI" for BattleReadoutUI

var WORLD_FILE_PATH = "Levels/SevenGods.ldtk"
var BATTLE_ARENA_LEVEL_NAME = "Battle_Arena"
var NONE = -1   // an empty fighter slot / move slot

// One row of the setup: which roster fighter (or NONE) and which catalog move fills each move slot (or NONE).
class TestSlot {
    construct new(fighter, moves) {
        _fighter = fighter
        _moves = moves
    }

    fighter { _fighter }
    fighter=(value) { _fighter = value }
    moves { _moves }
    moves=(value) { _moves = value }
}

class BattleTestState is BaseState {
    construct new() {
        super()
        name = "BattleTestState"
        _stateManager = null
        _view = null
        _world = null
        _fighters = null
        _catalog = null
        _party = null
        _enemies = null
        _inBattle = false
        _readout = null
        _partyCombatants = []
        _enemyCombatants = []

        _manager = StateManager.new()
        _manager.insertState("CombatState", CombatState)
    }

    enter(stateManager, params) {
        super.enter(stateManager, params)
        _stateManager = stateManager
        Time.setTimeScale(1)
        _fighters = BattleTestRoster.fighters
        _catalog = Moves.catalog
        _inBattle = false

        // The world root the arena level (and every battle entity) hangs off - no overworld levels are loaded.
        _world = World.createWorldEntity(WORLD_FILE_PATH)

        // Default matchup: you vs the Training Dummy.
        _party = [slotFor_(fighterIndex_("You")), TestSlot.new(NONE, []), TestSlot.new(NONE, [])]
        _enemies = [slotFor_(fighterIndex_("Training Dummy")), TestSlot.new(NONE, []), TestSlot.new(NONE, [])]

        _view = BattleSetupUI.new()
        wireSide_(_view.partyRows, _party)
        wireSide_(_view.enemyRows, _enemies)
        _view.startButton.setOnClick { |sender, mousePos| startBattle_() }
        _view.backButton.setOnClick { |sender, mousePos| back_() }
        _view.setResult("")
        refresh_()
    }

    update(stateManager) {
        if (!_inBattle) {
            return
        }
        _manager.update()
        if (_manager.currentState != null) {
            refreshReadout_()
        } else {
            // The fight has closed (OverState) - or CombatState refused to start it.
            _inBattle = false
            closeReadout_()
            _view.show(true)
            _view.setResult(resultText_(CombatOutcome.take()))
        }
    }

    fixedUpdate(stateManager) {
        if (_inBattle) {
            _manager.fixedUpdate()
        }
    }

    render() {
        if (_inBattle) {
            _manager.render()
        }
    }

    onEvent(type, data) {
        super.onEvent(type, data)
        if (_inBattle && _manager.currentState != null) {
            _manager.currentState.onEvent(type, data)
        }
    }

    exit() {
        super.exit()
        if (_manager.currentState != null) {
            _manager.clearCurrentState()
        }
        CombatOutcome.take()
        _inBattle = false
        closeReadout_()
        Time.setTimeScale(1)
        if (_view != null) {
            _view.teardown()
            _view = null
        }
        if (_world != null && GameObject.isValid(_world)) {
            GameObject.destroy(_world)
        }
        _world = null
        _stateManager = null
    }

    // --- setup ------------------------------------------------------------------------------

    fighterIndex_(label) {
        for (i in 0..._fighters.count) {
            if (_fighters[i][0] == label) {
                return i
            }
        }
        return 0
    }

    // A slot holding roster fighter `index` with its default moves.
    slotFor_(index) { TestSlot.new(index, defaultMoves_(index)) }

    // The fighter's own kit as catalog indices, padded with NONE to MOVES_PER_FIGHTER.
    defaultMoves_(index) {
        var moves = []
        for (move in _fighters[index][1].kit) {
            if (moves.count < MOVES_PER_FIGHTER) {
                moves.add(catalogIndex_(move.name))
            }
        }
        while (moves.count < MOVES_PER_FIGHTER) {
            moves.add(NONE)
        }
        return moves
    }

    catalogIndex_(name) {
        for (i in 0..._catalog.count) {
            if (_catalog[i].name == name) {
                return i
            }
        }
        return NONE
    }

    // Each row's buttons cycle its slot (the first slot of a side can't be emptied - a side needs a fighter).
    wireSide_(rows, slots) {
        for (r in 0...rows.count) {
            var slotIndex = r
            rows[r].fighterButton.setOnClick { |sender, mousePos| cycleFighter_(slots, slotIndex) }
            for (m in 0...rows[r].moveButtons.count) {
                var moveIndex = m
                rows[r].moveButtons[m].setOnClick { |sender, mousePos| cycleMove_(slots[slotIndex], moveIndex) }
            }
        }
    }

    cycleFighter_(slots, slotIndex) {
        if (_inBattle) {
            return
        }
        var slot = slots[slotIndex]
        var next = slot.fighter + 1
        if (next >= _fighters.count) {
            next = slotIndex == 0 ? 0 : NONE
        }
        slot.fighter = next
        slot.moves = next == NONE ? [] : defaultMoves_(next)
        refresh_()
    }

    cycleMove_(slot, moveIndex) {
        if (_inBattle || slot.fighter == NONE) {
            return
        }
        var next = slot.moves[moveIndex] + 1
        slot.moves[moveIndex] = next >= _catalog.count ? NONE : next
        refresh_()
    }

    refresh_() {
        refreshSide_(_view.partyRows, _party)
        refreshSide_(_view.enemyRows, _enemies)
    }

    refreshSide_(rows, slots) {
        for (r in 0...rows.count) {
            var slot = slots[r]
            var row = rows[r]
            if (slot.fighter == NONE) {
                row.fighterButton.setText("(empty)")
                for (b in row.moveButtons) {
                    b.setText("")
                }
                row.setStats("")
                continue
            }
            var def = _fighters[slot.fighter][1]
            row.fighterButton.setText(_fighters[slot.fighter][0])
            for (m in 0...row.moveButtons.count) {
                var i = slot.moves[m]
                row.moveButtons[m].setText(i == NONE ? "-" : "%(_catalog[i].name)  %(_catalog[i].timeCost)u")
            }
            row.setStats(BattleTestRoster.statLine(def))
        }
    }

    // --- running a battle ---------------------------------------------------------------------

    startBattle_() {
        if (_inBattle) {
            return
        }
        var used = {}
        var partyCombatants = []
        var partyDefs = []
        var enemyCombatants = []
        var enemyDefs = []
        build_(_party, partyCombatants, partyDefs, used)
        build_(_enemies, enemyCombatants, enemyDefs, used)

        _partyCombatants = partyCombatants
        _enemyCombatants = enemyCombatants
        Arbiter.clearRolls()

        _inBattle = true
        _view.show(false)
        _view.setResult("")
        _readout = BattleReadoutUI.new(partyCombatants.count + enemyCombatants.count, RECENT_ROLLS_KEPT)
        _manager.changeState("CombatState", {
            "partyCombatants": partyCombatants,
            "partyDefs": partyDefs,
            "enemyCombatants": enemyCombatants,
            "enemyDefs": enemyDefs,
            "player": null,
            "world": _world,
            "battleLevelIndex": World.getLevelIndex(_world, BATTLE_ARENA_LEVEL_NAME)
        })
    }

    // Fresh Combatants for one side's filled slots. A fighter given no moves at all just Braces. used: label ->
    // count across both sides, so a second Chicken is "Chicken 2".
    build_(slots, combatants, defs, used) {
        for (slot in slots) {
            if (slot.fighter == NONE) {
                continue
            }
            var label = _fighters[slot.fighter][0]
            var def = _fighters[slot.fighter][1]
            var moves = []
            for (i in slot.moves) {
                if (i != NONE) {
                    moves.add(_catalog[i])
                }
            }
            if (moves.count == 0) {
                moves.add(Moves.brace)
            }
            used[label] = used.containsKey(label) ? used[label] + 1 : 1
            var name = used[label] == 1 ? label : "%(label) %(used[label])"
            combatants.add(BattleTestRoster.combatant(def, name, moves))
            defs.add(def)
        }
    }

    // --- live readout ------------------------------------------------------------------------

    refreshReadout_() {
        var session = _manager.currentState.session
        if (_readout == null || session == null) {
            return
        }
        var lines = []
        for (c in _partyCombatants) {
            lines.add([fighterLine_(c, session.timeline), true])
        }
        for (c in _enemyCombatants) {
            lines.add([fighterLine_(c, session.timeline), false])
        }
        _readout.setFighters(lines)
        _readout.setRolls(Arbiter.recentRolls)
    }

    // "Chinlin  HP 112/150  SP 31/55 EXH  > Crush 40pct (6.4u)" - or "DOWN".
    fighterLine_(c, timeline) {
        var s = c.stats
        var line = "%(c.name)  HP %(s.hp.round)/%(s.maxHp)  SP %(s.stamina.floor)/%(s.maxStamina)"
        if (!c.alive) {
            return "%(line)  DOWN"
        }
        if (s.exhausted) {
            line = "%(line) EXH"
        }
        var move = timeline.committedMove(c)
        if (move != null) {
            line = "%(line)  > %(move.name) %((timeline.fraction(c) * 100).floor)pct (%(Arbiter.fmt_(timeline.costOf(c)))u)"
        }
        return line
    }

    closeReadout_() {
        if (_readout != null) {
            _readout.teardown()
            _readout = null
        }
    }

    resultText_(outcome) {
        if (outcome == null) {
            return "The battle didn't start."
        }
        var text = "Last battle: "
        if (outcome.won) {
            text = text + "won"
        } else if (outcome.lost) {
            text = text + "lost"
        } else {
            text = text + "fled"
        }
        if (outcome.fallen.count > 0) {
            text = "%(text)  -  defeated: %(outcome.fallen.join(", "))"
        }
        return text
    }

    back_() {
        if (_inBattle) {
            return
        }
        _stateManager.changeState("Experiment")
    }
}
