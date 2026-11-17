// States/CombatState.wren
// A fight in the animation-timed battle system: a shared timeline of fixed-length moves on a 3 x 4 arena. Every
// combatant commits to a chain of moves (at least two, about 4 seconds) at a time; the clock only stops when one of
// the party's chains runs out and its next one has to be picked. The rules live in Combat/BattleEngine.wren (and
// Combat/Rules.wren for gear, elements and effects); this state is the shell around them - it builds the two sides,
// the arena (Combat/BattleStage.wren) and the screen (UI/Combat/CombatUI.wren), and runs the fight through phases:
//
//   entering   the arena entrance (camera eases down, everyone runs onto their marks)
//   prompt     a tutorial line is up (Combat/Encounter.wren) - Accept for the next
//   select     the clock is stopped: a party member builds a chain - pick moves (the six loadout moves, Hold, an
//              item) until the chain bar fills, pick a cell for a movement move, Undo with Cancel, then Start
//   run        the clock runs: chains play, hits land, the stage and the screen follow the engine
//   fleeing    the party runs off        over   the closing line, then the fight closes
//
// The party side is every standing member of Party/Party.wren (all human controlled - the player's Combatant lives
// on its Player script, an NPC member's on its party record, so HP carries between fights). The opposition is the
// overworld entities that were engaged (their battle defs by name - Combat/Config/BattleCritter.wren), the stages
// of a scripted Encounter (Thomas's lessons, the Warden), or - from the battle test - ready-made Combatants.
//
// Presentation cuts to the arena level (see Combat/BattleStage.wren), which deactivates the overworld player and
// every other loaded level for the duration.
import "States/BaseState" for BaseState
import "app" for Time
import "input" for Input
import "ui" for UIManager
import "gameObject" for GameObject
import "gameObjectComponents" for Script, World, WorldTransform, Level
import "math" for Math, Vec2, Vec3, Vec4
import "reflect" for Reflect

import "Combat/Move" for Move
import "Combat/Chain" for ChainBuilder
import "Combat/BattleEngine" for BattleEngine
import "Combat/BattleStage" for BattleStage
import "Combat/CombatOutcome" for CombatOutcome
import "Combat/Defeated" for Defeated
import "Combat/Encounter" for Encounter
import "Combat/MoveBook" for MoveBook
import "Combat/Config/Moves" for Moves
import "Combat/Config/BattleItems" for BattleItems
import "Combat/Config/BattleCritter" for BattleCritter
import "Combat/Config/BattlePlayer" for BattlePlayer
import "Party/Party" for Party
import "GameObjects/Critter" for Critter
import "UI/Combat/CombatUI" for CombatUI
import "UI/Combat/TimelineUI" for TimelineRow
import "Inventory/Inventory" for Inventory
import "Inventory/ItemDatabase" for ItemDatabase, ItemCategory
import "Localization/DisplayNames" for DisplayNames
import "debug" for Debug

var ENTER_TIME = 0.65       // arena sweep + battler slide-in
var OVER_TIME = 1.8         // the closing line
var FLEE_TIME = 0.7
var STAGE_PAUSE = 0.9       // between the stages of an encounter
var NAV_ARM_TIME = 0.14     // input lockout after a menu change, so the press that caused it can't bleed through
var LOADOUT_SLOTS = 6
var SLOT_HOLD = 6
var SLOT_ITEM = 7

var COLOR_DAMAGE = Vec4.new(255, 235, 150, 255)
var COLOR_HURT = Vec4.new(255, 110, 100, 255)
var COLOR_BLOCK = Vec4.new(150, 200, 255, 255)
var COLOR_GOOD = Vec4.new(140, 255, 170, 255)
var COLOR_NOTE = Vec4.new(255, 190, 90, 255)
var COLOR_TELL = Vec4.new(215, 210, 200, 255)

var HINT_MOVES = "[input=UIAccept] Add move    [input=UICancel] Undo"
var HINT_SUB = "[input=UIAccept] Choose    [input=UICancel] Back"
var HINT_PICK = "[input=UIDir] Choose a cell    [input=UIAccept] Confirm    [input=UICancel] Back"
var HINT_CONFIRM = "[input=UIAccept] Choose    [input=UICancel] Undo last"

class CombatState is BaseState {
    construct new() {
        super()
        name = "CombatState"
        _view = null
        _stage = null
        _engine = null
        _phase = null
    }

    // The running fight's engine (null between fights) - for debug overlays.
    engine { _engine }
    // The current phase, whose chain is being built and the builder - for debug tools and tests.
    phase { _phase }
    chooser { _chooser }
    builder { _builder }

    // params: "opponents" (overworld entities to fight) and the Party, as GameWorldState sends - or, from the battle
    // test (States/BattleTestState.wren), explicit sides: "partyCombatants" / "enemyCombatants" (ready-made
    // Combatants, the opposition's with their brains set), with "player" null. Optional: "encounter" (a
    // Combat/Encounter to run instead of / around the opponents), "challenger" (the name a dialogue fight was
    // started against - picks its Encounter), "duel", "canFlee", "testItems" (hand out the test items).
    enter(stateManager, params) {
        super.enter(stateManager, params)
        _outer = stateManager
        _params = params
        _duel = params["duel"] == true
        _canFlee = params["canFlee"] == true
        _testItems = params["testItems"] == true
        _flags = {}
        _marks = {}
        _prompts = []
        _afterPrompt = null
        _chooser = null
        _builder = null
        _mode = null
        _axisHeld = false
        _armedAt = 0
        _lastSlot = 0
        _itemsQueued = {}

        _party = []
        _enemies = []
        _encounter = params["encounter"]
        _stageIndex = 0
        if (params["partyCombatants"] != null) {
            _party = params["partyCombatants"]
            _enemies = params["enemyCombatants"]
        } else {
            gatherSides_(params)
        }
        // A scripted fight takes its opposition from its first stage (when that stage names one).
        if (_encounter != null && _encounter.stages[0].enemies.count > 0) {
            _enemies = []
        }
        if (_party.count == 0 || (_enemies.count == 0 && _encounter == null)) {
            Debug.info("[CombatState] no party or no valid opponents, aborting")
            stateManager.clearCurrentState()
            return
        }

        _engine = BattleEngine.new(3, 4)
        _engine.deploy(_party, 0)
        if (_enemies.count > 0) {
            _engine.deploy(_enemies, 1)
        } else {
            addStageEnemies_(_encounter.stages[0])
        }
        var roster = _engine.side(1).map { |e| e.name }.join(", ")
        var partyRoster = _party.map { |c| c.name }.join(", ")
        Debug.info("[CombatState] %(partyRoster) vs %(roster)")

        // Room on screen for whoever may join the opposition mid-fight (a boss's shards).
        var enemySlots = _engine.side(1).count
        if (_engine.side(1).any { |e| e.has("boss") }) enemySlots = enemySlots + 2
        _view = CombatUI.new(_party.count, enemySlots.max(1))
        _view.setFleeAvailable(_canFlee)
        _view.onSlotClick = Fn.new { |index| slotClicked_(index) }
        _view.onSlotFocus = Fn.new { |index| slotFocused_(index) }
        _view.fleeButton.setOnClick { |s, m| flee_() }

        buildStage_(params)
        refreshViews_()

        if (_stage != null) {
            _view.setVisible(false)
            _stage.playEntrance()
            setPhase_("entering")
        } else {
            startStage_()
        }
    }

    // The overworld fight: the opponents' entities and the whole (standing) party.
    gatherSides_(params) {
        var names = []
        if (params["opponents"] != null) {
            for (entity in params["opponents"]) {
                var s = Script.getInstance(entity)
                if (s == null) continue
                var def = BattleCritter.forName(s.name)
                var combatant = enemyCombatant_(s, def, entity)
                combatant.brain = def.makeBrain()
                _enemies.add(combatant)
                names.add(def.name)
            }
        }
        // The human-controlled side is the whole party, leader first: each member's persistent Combatant.
        for (member in Party.members) {
            var combatant = Party.combatantOf(member)
            // A downed NPC member (knocked out earlier, not rested since) sits the fight out entirely.
            if (combatant == null || member.downed) continue
            combatant.brain = null
            if (member.isPlayer) {
                BattlePlayer.refresh(combatant)
            } else {
                combatant.moves = npcKit_(combatant)
            }
            _party.add(combatant)
        }
        if (_encounter == null) {
            _encounter = params["challenger"] != null ? Encounter.forChallenger(params["challenger"]) : null
            if (_encounter == null) _encounter = Encounter.forEnemies(names, _party.count)
        }
    }

    // A party NPC fights with their def's moves - less Brace until Thomas has taught it (first arc, lesson 2).
    npcKit_(combatant) {
        var def = combatant.def
        if (def == null) return combatant.moves
        return def.kit.where { |move| move.id != "Brace" || MoveBook.isUnlocked("Brace") }.toList
    }

    // The Combatant an overworld opponent fights as. A critter keeps its own across fights (Critter.combatant), so
    // the HP it lost in a fight you fled or lost is still missing next time - until its level unloads and
    // re-creates it. Anything else (an NPC duel challenger) fights fresh every time.
    enemyCombatant_(script, def, entity) {
        if (!(script is Critter)) {
            return def.makeCombatant(entity)
        }
        if (script.combatant == null) {
            script.combatant = def.makeCombatant(entity)
        }
        var combatant = script.combatant
        combatant.stats.gainStamina(combatant.stats.maxStamina)
        return combatant
    }

    addStageEnemies_(stage) {
        var loose = []
        for (spec in stage.enemies) {
            var def = spec[0]
            var combatant = def.makeCombatant(null)
            combatant.brain = def.makeBrain()
            _enemies.add(combatant)
            if (spec[1] == null) {
                loose.add(combatant)
            } else {
                _engine.add(combatant, 1, spec[1], spec[2])
            }
        }
        if (loose.count > 0) _engine.deploy(loose, 1)
    }

    // Cut to the battle arena (Combat/BattleStage.wren) - the authored battle level if one was found, centred on
    // its bounds; otherwise an authored "BattleAnchor", or a fixed offscreen patch of world.
    buildStage_(params) {
        _stage = null
        if (params["world"] == null) return
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
            var anchors = GameObject.getAllWithIdentifier("BattleAnchor")
            if (anchors.count > 0) anchorPosition = WorldTransform.getPosition(anchors[0])
        }
        _stage = BattleStage.new(anchorPosition, params["player"], params["world"], _engine, battleLevelEntity)
    }

    setPhase_(phase) {
        _phase = phase
        _phaseStart = Time.unscaledTime
    }

    arm_() { _armedAt = Time.unscaledTime + NAV_ARM_TIME }
    armed_ { Time.unscaledTime >= _armedAt }

    // --- the loop ---------------------------------------------------------------------------

    update(stateManager) {
        if (_engine == null) return
        _view.update()
        var elapsed = Time.unscaledTime - _phaseStart
        if (_phase == "entering") {
            _stage.intro(elapsed / ENTER_TIME)
            if (elapsed >= ENTER_TIME) {
                _stage.settle()
                _view.setVisible(true)
                startStage_()
            }
        } else if (_phase == "run") {
            run_()
        } else if (_phase == "select") {
            select_()
            idleFrame_()
        } else if (_phase == "prompt") {
            if (armed_ && Input.isInputJustPressed("UIAccept")) nextPrompt_()
            idleFrame_()
        } else if (_phase == "stageClear") {
            idleFrame_()
            if (elapsed >= STAGE_PAUSE) nextStage_()
        } else if (_phase == "fleeing") {
            if (_stage != null) _stage.runOff(Math.min(elapsed / FLEE_TIME, 1))
            if (elapsed >= FLEE_TIME) endFight_("You slipped away.", "fled")
        } else if (_phase == "over") {
            idleFrame_()
            if (elapsed >= OVER_TIME) _outer.clearCurrentState()
        }
    }

    // A frame with the clock stopped: the stage holds its poses (idles still breathe) and the screen stays fresh.
    idleFrame_() {
        if (_stage != null) _stage.update(0, [])
        refreshViews_()
    }

    run_() {
        var dt = Time.scaledDelta
        _engine.update(dt)
        var events = _engine.takeEvents()
        for (event in events) {
            show_(event)
        }
        if (_stage != null) _stage.update(dt, events)
        refreshViews_()
        if (_engine == null) return
        if (_engine.result != null) {
            decided_(_engine.result)
        } else if (_engine.waiting.count > 0) {
            beginSelection_()
        }
    }

    // --- stages and prompts ------------------------------------------------------------------

    encounterStage_ { _encounter == null ? null : _encounter.stages[_stageIndex] }

    // The (first or next) stage is on the board: its start hook and intro lines, then the fight.
    startStage_() {
        var stage = encounterStage_
        if (stage != null) {
            if (stage.onStart != null) stage.onStart.call(this)
            for (line in stage.intro) {
                _prompts.add(line)
            }
        }
        _engine.think()
        beginSelection_()
    }

    // The stage's opposition is beaten and there is another: line the party up again against the next one.
    nextStage_() {
        _stageIndex = _stageIndex + 1
        _engine.nextStage()
        addStageEnemies_(encounterStage_)
        if (_stage != null) _stage.resync()
        startStage_()
    }

    showPrompts_(after) {
        _afterPrompt = after
        _view.showCommand(false)
        if (_stage != null) _stage.clearHighlights()
        setPhase_("prompt")
        nextPrompt_()
    }

    nextPrompt_() {
        if (_prompts.count == 0) {
            _view.hidePrompt()
            var after = _afterPrompt
            _afterPrompt = null
            if (after != null) after.call()
            return
        }
        var line = _prompts.removeAt(0)
        _view.showPrompt(line[0] == null ? "Tip" : line[0], line[1])
        arm_()
    }

    // --- what an Encounter's hooks can do (Combat/Encounter.wren) ------------------------------

    // True the first time it is asked about `key` in this fight.
    once(key) {
        if (_flags.containsKey(key)) return false
        _flags[key] = true
        return true
    }
    mark(key, value) { _marks[key] = value }
    marked(key) { _marks.containsKey(key) }
    markOf(key) { _marks[key] }
    inParty(name) { _party.any { |c| c.name == name && c.alive } }
    // A line on the banner - nothing stops.
    say(text) { _view.setMessage(text, 4.5) }
    // Lines for a prompt box at the next selection stop.
    prompt(lines) {
        for (line in lines) {
            _prompts.add(line)
        }
    }
    // The player learns a move (and anyone in the party whose own kit has it starts using it).
    teach(moveName) {
        // The battle test fights with its own picks and saves nothing.
        var sandbox = _params["partyCombatants"] != null
        if (!sandbox && MoveBook.unlock(moveName)) MoveBook.save("moves.sav")
        for (c in _party) {
            if (c.def == BattlePlayer) {
                if (!sandbox) {
                    c.moves = BattlePlayer.kit
                } else if (!c.moves.any { |m| m.id == moveName } && c.moves.count < LOADOUT_SLOTS) {
                    c.moves.add(Moves.byName(moveName))
                }
            } else if (c.def != null && c.def.kitNames.contains(moveName) && !c.moves.any { |m| m.id == moveName }) {
                c.moves.add(Moves.byName(moveName))
            }
        }
    }

    // --- selection ---------------------------------------------------------------------------

    beginSelection_() {
        if (_prompts.count > 0) {
            showPrompts_(Fn.new { beginSelection_() })
            return
        }
        var waiting = _engine.waiting
        if (waiting.count == 0) {
            _chooser = null
            _builder = null
            _view.showCommand(false)
            if (_stage != null) _stage.clearHighlights()
            setPhase_("run")
            return
        }
        _chooser = waiting[0]
        _builder = ChainBuilder.new(_engine, _chooser)
        _itemsQueued = {}
        _lastSlot = 0
        setPhase_("select")
        _view.showCommand(true)
        showMoves_()
    }

    select_() {
        if (!armed_) return
        if (_mode == "pick") {
            pickInput_()
            return
        }
        if (Input.isInputJustPressed("UICancel")) {
            if (_mode == "hold" || _mode == "items") {
                showMoves_()
            } else if (_builder.count > 0) {
                _builder.undo()
                showMoves_()
            }
        }
    }

    // The main grid: the six loadout moves, Hold, Item.
    showMoves_() {
        if (_builder.complete) {
            showConfirm_()
            return
        }
        _mode = "moves"
        arm_()
        var entries = []
        for (i in 0...LOADOUT_SLOTS) {
            var move = i < _chooser.moves.count ? _chooser.moves[i] : null
            if (move == null) {
                entries.add(["-", "", false])
            } else {
                var cost = _builder.costOf(move)
                var info = "%(Move.fmt(move.completion))s  %(cost)sp"
                // Long names (tier III heavies) give up the time - it is in the detail line and the timeline preview.
                if (move.name.count + info.count > 22) info = "%(cost)sp"
                var label = move.name
                if (label.count + info.count > 22) label = label[0...(21 - info.count)] + "."
                entries.add([label, info, _builder.blockedBy(move) == null])
            }
        }
        entries.add(["Hold", "rest", !_builder.lastIsHold])
        var items = itemList_()
        entries.add(["Item", items.count == 0 ? "none" : "x%(items.count)", items.count > 0])
        _view.setSlots(entries, _lastSlot)
        var whose = _chooser.name == "You" ? "Your chain" : "%(_chooser.name)'s chain"
        _view.setHeader(whose)
        var stage = encounterStage_
        _view.setHint(stage != null && stage.hint != null ? "%(HINT_MOVES)    %(stage.hint)" : HINT_MOVES)
        refreshChain_()
        slotFocused_(_lastSlot)
    }

    showHold_() {
        _mode = "hold"
        arm_()
        var entries = []
        var rate = _engine.rules.holdRecovery(_chooser)
        for (seconds in Moves.holdLengths) {
            entries.add(["Hold %(Move.fmt(seconds))s", "+%((rate * seconds).round)sp", true])
        }
        _view.setSlots(entries, 3)
        _view.setHeader("Hold - for how long?")
        _view.setHint(HINT_SUB)
        _view.setDetail("Stand and catch your breath: the only way stamina comes back. Never twice in a row.")
        if (_stage != null) _stage.setHighlights(null, [_builder.lane, _builder.row])
    }

    showItems_() {
        _mode = "items"
        arm_()
        _itemList = itemList_()
        var entries = []
        for (item in _itemList) {
            entries.add([item[1], "x%(item[2])", true])
        }
        _view.setSlots(entries, 0)
        _view.setHeader("Use which item?")
        _view.setHint(HINT_SUB)
        if (_stage != null) _stage.setHighlights(null, [_builder.lane, _builder.row])
    }

    // Full: Start it, or take the last move back.
    showConfirm_() {
        _mode = "confirm"
        arm_()
        _view.setSlots([["Start", "go", true], ["Undo last", "", true]], 0)
        _view.setHeader("Chain ready")
        _view.setHint(HINT_CONFIRM)
        _view.setDetail(_builder.entries.map { |e| e.move.name }.join(" > "))
        refreshChain_()
        if (_stage != null) _stage.setHighlights(null, [_builder.lane, _builder.row])
    }

    refreshChain_() {
        var b = _builder
        var text = "%(Move.fmt(b.time)) / %(Move.fmt(b.budget))s   SP %(b.stamina.floor)"
        _view.setChain(text, b.time / b.budget)
    }

    // The usable items: [[id, display name, count left], ...] - the party's consumables (less what this chain has
    // already queued), or the test items in the battle test.
    itemList_() {
        var out = []
        if (_testItems) {
            for (id in BattleItems.testItems) {
                out.add([id, id, 9])
            }
            return out
        }
        for (stack in Inventory.stacksIn(ItemCategory.CONSUMABLE)) {
            if (BattleItems.forItem(stack.id) == null) continue
            var queued = _itemsQueued.containsKey(stack.id) ? _itemsQueued[stack.id] : 0
            if (stack.count - queued > 0) out.add([stack.id, DisplayNames.itemDisplay(stack.id), stack.count - queued])
        }
        return out
    }

    slotClicked_(index) {
        if (_phase != "select" || !armed_ || _builder == null) return
        if (_mode == "moves") {
            _lastSlot = index
            if (index == SLOT_HOLD) {
                if (!_builder.lastIsHold) showHold_()
            } else if (index == SLOT_ITEM) {
                if (itemList_().count > 0) showItems_()
            } else if (index < _chooser.moves.count) {
                chooseMove_(_chooser.moves[index])
            }
        } else if (_mode == "hold") {
            _builder.add(Moves.hold(Moves.holdLengths[index]), null)
            showMoves_()
        } else if (_mode == "items") {
            var item = _itemList[index]
            var move = BattleItems.forItem(item[0])
            if (_builder.blockedBy(move) == null) {
                _builder.add(move, null)
                _itemsQueued[item[0]] = (_itemsQueued.containsKey(item[0]) ? _itemsQueued[item[0]] : 0) + 1
            }
            showMoves_()
        } else if (_mode == "confirm") {
            if (index == 0) {
                commit_()
            } else {
                _builder.undo()
                showMoves_()
            }
        }
    }

    chooseMove_(move) {
        var why = _builder.blockedBy(move)
        if (why != null) {
            _view.setDetail(CombatState.reason_(why))
            return
        }
        if (_builder.needsDestination(move)) {
            var cells = _builder.destinations(move)
            if (cells.count == 0) {
                _view.setDetail("Nowhere to go from there.")
                return
            }
            beginPick_(move, cells)
            return
        }
        _builder.add(move, null)
        showMoves_()
    }

    static reason_(why) {
        if (why == "hold") return "Hold can't follow Hold."
        if (why == "stamina") return "Not enough stamina - Hold first."
        if (why == "weapon") return "Can't use that with what you're holding."
        return "The chain is full."
    }

    // What the focused slot would do: a readout under the grid, and its cells on the arena - from where the chain
    // so far leaves the chooser.
    slotFocused_(index) {
        if (_phase != "select" || _builder == null) return
        var here = [_builder.lane, _builder.row]
        if (_mode != "moves") {
            if (_mode == "items" && _itemList != null && index < _itemList.count) {
                _view.setDetail(BattleItems.describe(_itemList[index][0]))
            }
            return
        }
        if (index == SLOT_HOLD) {
            _view.setDetail("Hold: wait 0.2 - 2.0s and recover stamina.")
            if (_stage != null) _stage.setHighlights(null, here)
            return
        }
        if (index == SLOT_ITEM) {
            _view.setDetail("Use an item - it takes time, and lands at its effect time.")
            if (_stage != null) _stage.setHighlights(null, here)
            return
        }
        if (index >= _chooser.moves.count) {
            _view.setDetail("")
            if (_stage != null) _stage.setHighlights(null, here)
            return
        }
        var move = _chooser.moves[index]
        var why = _builder.blockedBy(move)
        var text = describe_(move)
        if (why != null && why != "full") text = "%(text)  -  %(CombatState.reason_(why))"
        _view.setDetail(text)
        if (_stage != null) {
            var cells = []
            if (_builder.needsDestination(move)) {
                for (cell in _builder.destinations(move)) {
                    cells.add([cell[0], cell[1], "goto"])
                }
            }
            for (cell in _builder.cellsOf(move, null)) {
                cells.add([cell[0], cell[1], move.isHeal ? "heal" : "aim"])
            }
            _stage.setHighlights(cells, here)
        }
    }

    // "Attack  hits 0.5s  dmg 28  stagger 0.3s".
    describe_(move) {
        var parts = [move.typeLine]
        if (move.isMovement) parts.add("moves %(Move.fmt(move.moveTime))s")
        if (move.isAttack) {
            var weapon = _engine.rules.weaponFor(_chooser, move)
            var damage = ((move.damage + weapon[0]) * (1 + _engine.rules.scalingOf(_chooser, move) / 100)).round
            var stagger = ((move.stagger * weapon[1]) * 100).round / 100
            parts.add("hits %(Move.fmt(move.impact))s")
            parts.add("dmg %(damage)")
            parts.add("stagger %(stagger)s")
        }
        if (move.isHeal) {
            parts.add("heals %(_engine.rules.healAmount(_chooser, move, 1)) at %(Move.fmt(move.impact))s")
        }
        if (move.isDefend) {
            var w = move.window
            parts.add("guard %(Move.fmt(w[0]))-%(Move.fmt(w[2]))s (best %(Move.fmt(w[1]))s)")
            parts.add("def %(move.defence.round)")
        }
        if (move.isDodge) parts.add("dodge %(Move.fmt(move.dodge[0]))-%(Move.fmt(move.dodge[1]))s")
        return parts.join("  ")
    }

    // --- picking a destination ---------------------------------------------------------------

    beginPick_(move, cells) {
        _mode = "pick"
        arm_()
        _pickMove = move
        _pickCells = cells
        _pickIndex = 0
        // Start on the cell straight ahead when there is one.
        var ahead = _builder.row + _builder.facing
        for (i in 0...cells.count) {
            if (cells[i][0] == _builder.lane && cells[i][1] == ahead) _pickIndex = i
        }
        _axisHeld = true
        _view.setSlots([], null)
        UIManager.clearFocusElements()
        _view.setHeader("%(move.name) - to which cell?")
        _view.setDetail("")
        _view.setHint(HINT_PICK)
        paintPick_()
    }

    paintPick_() {
        if (_stage == null) return
        var cells = []
        for (cell in _pickCells) {
            cells.add([cell[0], cell[1], "goto"])
        }
        var dest = _pickCells[_pickIndex]
        for (cell in _builder.cellsOf(_pickMove, dest)) {
            cells.add([cell[0], cell[1], "aim"])
        }
        _stage.setHighlights(cells, dest)
    }

    pickInput_() {
        var axis = Input.getInputAxis2("UIDir")
        if (axis.x.abs < 0.3 && axis.y.abs < 0.3) {
            _axisHeld = false
        } else if (!_axisHeld) {
            _axisHeld = true
            // Rows run left to right, lanes top to bottom (the axis's up is +y).
            var dRow = axis.x.abs >= 0.3 ? axis.x.sign : 0
            var dLane = axis.y.abs >= 0.3 ? -axis.y.sign : 0
            var from = _pickCells[_pickIndex]
            var best = _pickIndex
            var bestScore = 0
            for (i in 0..._pickCells.count) {
                var cell = _pickCells[i]
                var along = (cell[1] - from[1]) * dRow + (cell[0] - from[0]) * dLane
                var across = ((cell[1] - from[1]) * dLane).abs + ((cell[0] - from[0]) * dRow).abs
                var score = along * 2 - across
                if (along > 0 && score > bestScore) {
                    best = i
                    bestScore = score
                }
            }
            if (best != _pickIndex) {
                _pickIndex = best
                paintPick_()
            }
        }
        if (Input.isInputJustPressed("UIAccept")) {
            _builder.add(_pickMove, _pickCells[_pickIndex])
            showMoves_()
        } else if (Input.isInputJustPressed("UICancel")) {
            showMoves_()
        }
    }

    // The chain is built: commit it, then the next party member who needs one - or let the clock run.
    commit_() {
        _engine.commit(_chooser, _builder.entries)
        beginSelection_()
    }

    flee_() {
        if (!_canFlee || _phase != "select") return
        _view.showCommand(false)
        if (_stage != null) _stage.clearHighlights()
        setPhase_("fleeing")
    }

    // --- showing the fight -----------------------------------------------------------------

    // One engine event: floating text over whoever it happened to, the banner, the encounter's hooks.
    show_(event) {
        var type = event["type"]
        var who = event["who"]
        var text = event["text"]
        if (type == "hit") {
            var amount = event["amount"]
            if (amount > 0) {
                pop_(who, "-%(amount)", event["blocked"] ? COLOR_BLOCK : (who.side == 0 ? COLOR_HURT : COLOR_DAMAGE))
            } else if (event["blocked"]) {
                pop_(who, "Blocked", COLOR_BLOCK)
            } else {
                pop_(who, "0", COLOR_TELL)
            }
        } else if (type == "dodge") {
            pop_(who, "Dodged", COLOR_GOOD)
        } else if (type == "heal") {
            var healed = event["amount"]
            pop_(who, "+%(healed)", COLOR_GOOD)
        } else if (type == "direct") {
            var lost = event["amount"]
            pop_(who, "-%(lost) %(text)", COLOR_NOTE)
        } else if (type == "note" || type == "status") {
            pop_(who, text, COLOR_NOTE)
        } else if (type == "combo") {
            pop_(who, text, COLOR_TELL)
        } else if (type == "falter") {
            pop_(who, "Falter", COLOR_HURT)
        } else if (type == "blocked") {
            pop_(who, "No room", COLOR_TELL)
        } else if (type == "say") {
            say(text)
        } else if (type == "item") {
            if (!_testItems && who.side == 0) {
                Inventory.removeItem(event["item"])
                Inventory.save("inventory.sav")
            }
        }
        var stage = encounterStage_
        if (stage != null && stage.onEvent != null) stage.onEvent.call(this, event)
    }

    pop_(who, text, color) {
        if (who == null || _stage == null) return
        var at = _stage.screenOf(who)
        _view.popup(Vec2.new(at.x, at.y - 96), text, color)
    }

    // Cards and the timeline, from the engine as it stands.
    refreshViews_() {
        if (_engine == null) return
        var cards = _view.partyCards
        for (i in 0...cards.count) {
            if (i < _party.count) {
                refreshCard_(cards[i], _party[i])
            } else {
                cards[i].setVisible(false)
            }
        }
        var foes = _engine.fighters.where { |f| f.side == 1 && !f.removed }.toList
        cards = _view.enemyCards
        for (i in 0...cards.count) {
            if (i < foes.count) {
                refreshCard_(cards[i], foes[i])
            } else {
                cards[i].setVisible(false)
            }
        }

        var read = 0
        for (c in _party) {
            if (c.read > read) read = c.read
        }
        var rows = []
        for (c in _party) {
            if (c.active) rows.add(timelineRow_(c, 2))
        }
        for (f in foes) {
            if (f.active) rows.add(timelineRow_(f, read))
        }
        _view.timeline.draw(rows)
    }

    refreshCard_(card, c) {
        var s = c.stats
        card.setVisible(true)
        card.refresh(c.name, c.alive, s.hp, s.maxHp, (c.has("noStamina") || s.maxStamina <= 0) ? 1 : s.staminaFraction, CombatState.statusText_(c))
        card.selected = c == _chooser && _phase == "select"
    }

    static statusText_(c) {
        var parts = []
        if (c.burn > 0) parts.add("Burn x%(c.burn)")
        if (c.frozen > 0) {
            parts.add("Frozen")
        } else if (c.chill > 0) {
            parts.add("Chill x%(c.chill)")
        }
        if (c.bleed > 0) parts.add("Bleed x%(c.bleed)")
        var charges = c.counter("charges")
        if (charges > 0) parts.add("Charge x%(charges)")
        if (c.shield >= 1) parts.add("Shield %(c.shield.round)")
        if (c.exposedNow) parts.add("Open!")
        return parts.join("  ")
    }

    // One combatant's row: a damage animation first if it is in one, the rest of its active move, then as much of
    // its chain as may be seen (visible: 0 the active move only, 1 the next one too, 2 everything). The chooser's
    // row previews the chain being built instead.
    timelineRow_(c, visible) {
        var row = TimelineRow.new(c.name, c.side == 0, c == _chooser && _phase == "select")
        var t = 0
        if (c == _chooser && _phase == "select" && _builder != null) {
            for (entry in _builder.entries) {
                t = row.addMove(entry.move, t, 0, false, true)
            }
            return row
        }
        if (c.stun > 0) t = row.addBlock(0, c.stun, "stagger", "")
        if (c.move == null) return row
        t = row.addMove(c.move, t, c.progress, c.impactDone, false)
        var last = visible >= 2 ? c.chain.count - 1 : c.chainIndex + visible
        var i = c.chainIndex + 1
        while (i <= last && i < c.chain.count) {
            t = row.addMove(c.chain[i].move, t, 0, false, false)
            i = i + 1
        }
        return row
    }

    // --- endings -------------------------------------------------------------------------------

    decided_(result) {
        _view.showCommand(false)
        if (_stage != null) _stage.clearHighlights()
        if (result == "won") {
            var stage = encounterStage_
            var more = _encounter != null && _stageIndex + 1 < _encounter.stages.count
            if (stage != null) {
                for (line in stage.outro) {
                    _prompts.add(line)
                }
            }
            if (more) {
                // Another stage: this one's closing lines, a beat, then the next lines up.
                var onward = Fn.new { setPhase_("stageClear") }
                if (_prompts.count > 0) {
                    showPrompts_(onward)
                } else {
                    onward.call()
                }
                return
            }
            var finish = Fn.new { win_() }
            if (_prompts.count > 0) {
                showPrompts_(finish)
            } else {
                finish.call()
            }
            return
        }
        // A duel is non-lethal (States/GameWorldState.onCombatEnded_ gets everyone back up) - so it's a yield.
        endFight_(_duel ? "You yield - the fight is lost." : "You were overwhelmed...", "lost")
    }

    // Every opponent is down: any who "join the party when defeated" (a recruitable NPC, like the Duelist - see
    // GameObjects/NPC.joinsPartyOnDefeat) are added to the party instead of just vanishing.
    win_() {
        var recruited = []
        for (e in _enemies) {
            if (e.entity != null && GameObject.isValid(e.entity)) {
                var script = Script.getInstance(e.entity)
                // Duck-typed (not `is NPC`): registered scripts live in their own module instance.
                if (script != null && Reflect.hasMethod(script, "joinsPartyOnDefeat()") && script.joinsPartyOnDefeat()) {
                    Party.join(script.name, WorldTransform.getPosition(e.entity))
                    recruited.add(script.name)
                }
            }
        }
        var line = _enemies.count == 1 ? "You defeated %(_enemies[0].name)!" : "The fight is won!"
        if (_encounter != null) line = "%(_encounter.name) - victory!"
        if (recruited.count > 0) {
            var names = recruited.join(", ")
            line = "%(line)  %(names) joins your party!"
        }
        endFight_(line, "won", recruited)
    }

    endFight_(text, result) { endFight_(text, result, []) }

    // Every ending funnels through here. result: "won" / "lost" / "fled"; recruited: names of the opponents who
    // joined the party. Removes the fallen from the overworld, leaves the outcome for the world
    // (Combat/CombatOutcome.wren - States/GameWorldState.wren takes it once the fight has closed), then the
    // closing line.
    endFight_(text, result, recruited) {
        // Stamina is a per-fight resource: everyone leaves at full stamina whatever the ending (HP carries over).
        for (combatant in _party + _enemies) {
            combatant.stats.gainStamina(combatant.stats.maxStamina)
            combatant.clearStatuses()
        }
        var fallen = removeFallen_(recruited)
        var outcome = CombatOutcome.new(result, _duel, fallen, recruited)
        Debug.info("[CombatState] fight over: %(outcome)")
        CombatOutcome.record(outcome)
        _view.showCommand(false)
        _view.hidePrompt()
        _view.setMessage(text)
        setPhase_("over")
    }

    // Whatever the ending, every opponent who died in this fight is removed from the overworld, and a level-placed
    // critter is recorded in Combat/Defeated.wren (by its LDtk iid) so the level doesn't re-create it. Returns the
    // names of the fallen. A beaten story character (a duel challenger, a story battle's foe) stays standing in the
    // world so their dialogue can carry on.
    removeFallen_(recruited) {
        var fallen = []
        for (e in _enemies) {
            if (e.alive) continue
            if (!recruited.contains(e.name)) fallen.add(e.name)
            if (e.entity != null && GameObject.isValid(e.entity)) {
                var script = Script.getInstance(e.entity)
                if (!(script is Critter) && !recruited.contains(e.name)) continue
                if (!recruited.contains(e.name) && script is Critter) {
                    Defeated.add(script.iid)
                }
                GameObject.destroy(e.entity)
            }
        }
        return fallen
    }

    exit() {
        super.exit()
        Time.setTimeScale(1)
        if (_stage != null) {
            _stage.teardown()
            _stage = null
        }
        if (_view != null) {
            _view.teardown()
            _view = null
        }
        _engine = null
        _chooser = null
        _builder = null
        _party = []
        _enemies = []
    }
}
