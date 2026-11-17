// States/BattleTestState.wren
// A sandbox for trying the battle system. A setup screen (UI/BattleTest/BattleSetupUI.wren) picks up to three
// fighters per side from Combat/Config/BattleTestRoster.wren and, for every humanoid, what they carry and know:
// main hand, off-hand and armour type, gear quality, a weapon and an armour element, two custom effects on each
// (rank I-III), their stats tier and their six moves at tier I-III (only moves the equipment allows are offered).
// Monsters and props (the Training Dummy, the wolves, the Warden boss...) bring their own natural weapons and combo
// menus. A scenario can replace the opposition with a scripted fight: Thomas's lessons, the first wolves, the Warden.
//
// Start runs the real CombatState in the arena with those sides, then comes back here with the result. You command
// your side exactly as in the game; the other side is AI (its combo menu, or the planner for a humanoid without one).
// Every fighter starts fresh at full health, nothing is saved, and the overworld isn't touched.
// Opened with F6 from the overworld (States/GameWorldState.wren); Back returns to the game.
import "app" for Time
import "input" for Input
import "gameObject" for GameObject
import "gameObjectComponents" for World
import "States/BaseState" for BaseState
import "States/StateManager" for StateManager
import "States/CombatState" for CombatState
import "Combat/CombatOutcome" for CombatOutcome
import "Combat/Stats" for Stats
import "Combat/Move" for Move
import "Combat/Gear" for GearItem, Loadout, Elements
import "Combat/Encounter" for Encounter
import "Combat/EnemyAI" for PlannerBrain
import "Combat/MoveBook" for MoveBook
import "Combat/Config/Moves" for Moves
import "Combat/Config/GearTypes" for GearTypes
import "Combat/Config/BattleCritter" for BattleCritter
import "Combat/Config/BattleTestRoster" for BattleTestRoster
import "UI/BattleTest/BattleSetupUI" for BattleSetupUI, SLOTS_PER_SIDE, MOVE_SLOTS

var WORLD_FILE_PATH = "Levels/SevenGods.ldtk"
var BATTLE_ARENA_LEVEL_NAME = "GrassBattleStage"   // an arena level in SevenGods.ldtk
var NONE = -1
var SCENARIOS = ["Free battle", "Thomas's lessons", "The first wolves", "The Warden"]
var STAT_TIERS = ["Own", "Early", "Mid", "Late"]
var TIER_NAMES = ["", "I", "II", "III"]

// One fighter slot of the setup: which roster fighter (or NONE) and everything picked for it. Indices point into
// GearTypes.mainHands / offHands / armours, Elements.all and Elements.effects; NONE = nothing.
class TestSlot {
    construct new() {
        _data = {
            "fighter": NONE, "stats": 0, "main": NONE, "off": NONE, "armour": NONE, "quality": 0, "tier": 1,
            "welem": NONE, "aelem": NONE
        }
        for (key in ["wfx0", "wfx1", "afx0", "afx1"]) {
            _data[key] = NONE
            _data[key + "r"] = 1
        }
        _moves = []     // move ids, MOVE_SLOTS of them (null = empty)
        for (i in 0...MOVE_SLOTS) {
            _moves.add(null)
        }
    }

    [key] { _data[key] }
    [key]=(value) { _data[key] = value }
    moves { _moves }
    empty { _data["fighter"] == NONE }
}

class BattleTestState is BaseState {
    construct new() {
        super()
        name = "BattleTestState"
        _stateManager = null
        _view = null
        _world = null
        _inBattle = false
        _manager = StateManager.new()
        _manager.insertState("CombatState", CombatState)
    }

    enter(stateManager, params) {
        super.enter(stateManager, params)
        _stateManager = stateManager
        Time.setTimeScale(1)
        _fighters = BattleTestRoster.fighters
        _mains = GearTypes.mainHands
        _offs = GearTypes.offHands
        _armours = GearTypes.armours
        _inBattle = false
        _selected = 0
        _focusKind = null
        _focusId = null
        _tabHeld = false
        _scenario = 0
        _read = 0

        // The world root the arena level (and every battle entity) hangs off - no overworld levels are loaded.
        _world = World.createWorldEntity(WORLD_FILE_PATH)

        // Default matchup: you vs the Training Dummy.
        _slots = []
        for (i in 0...(SLOTS_PER_SIDE * 2)) {
            _slots.add(TestSlot.new())
        }
        setFighter_(_slots[0], fighterIndex_("You"))
        setFighter_(_slots[SLOTS_PER_SIDE], fighterIndex_("Training Dummy"))

        _view = BattleSetupUI.new()
        _view.onFocus = Fn.new { |kind, id| focused_(kind, id) }
        for (i in 0..._view.slotButtons.count) {
            var index = i
            _view.slotButtons[i].setOnClick { |sender, mousePos|
                _selected = index
                cycle_("fighter", 1)
            }
        }
        for (id in _view.pickerIds) {
            var pickerId = id
            _view.picker(id).setOnClick { |sender, mousePos| cycle_(pickerId, 1) }
        }
        _view.startButton.setOnClick { |sender, mousePos| startBattle_() }
        _view.backButton.setOnClick { |sender, mousePos| back_() }
        _view.setResult("")
        refresh_()
    }

    update(stateManager) {
        if (!_inBattle) {
            // Q / E (UITab) step the focused picker back / forward.
            var tab = Input.getInputAxis("UITab")
            if (tab.abs < 0.3) {
                _tabHeld = false
            } else if (!_tabHeld) {
                _tabHeld = true
                if (_focusKind == "picker") {
                    cycle_(_focusId, tab > 0 ? 1 : -1)
                } else if (_focusKind == "slot") {
                    cycle_("fighter", tab > 0 ? 1 : -1)
                }
            }
            return
        }
        _manager.update()
        if (_manager.currentState == null) {
            // The fight has closed - or CombatState refused to start it.
            _inBattle = false
            _view.show(true)
            _view.setResult(resultText_(CombatOutcome.take()))
        }
    }

    fixedUpdate(stateManager) {
        if (_inBattle) _manager.fixedUpdate()
    }

    render() {
        if (_inBattle) _manager.render()
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

    subStateManager { _manager }

    // --- the slots ----------------------------------------------------------------------------

    fighterIndex_(label) {
        for (i in 0..._fighters.count) {
            if (_fighters[i][0] == label) return i
        }
        return 0
    }

    isPartySlot_(index) { index < SLOTS_PER_SIDE }
    humanoid_(slot) { !slot.empty && _fighters[slot["fighter"]][2] }
    defOf_(slot) { _fighters[slot["fighter"]][1] }

    static indexOfId_(list, id) {
        for (i in 0...list.count) {
            if (list[i].id == id) return i
        }
        return NONE
    }

    // Puts roster fighter `index` (or NONE) in the slot with its own gear and moves.
    setFighter_(slot, index) {
        slot["fighter"] = index
        for (key in ["main", "off", "armour", "welem", "aelem", "wfx0", "wfx1", "afx0", "afx1"]) {
            slot[key] = NONE
        }
        slot["stats"] = 0
        slot["quality"] = 0
        slot["tier"] = 1
        for (i in 0...MOVE_SLOTS) {
            slot.moves[i] = null
        }
        if (index == NONE || !_fighters[index][2]) return
        var def = _fighters[index][1]
        var load = def.loadout
        if (load.main != null) slot["main"] = BattleTestState.indexOfId_(_mains, load.main.type.id)
        if (load.off != null) slot["off"] = BattleTestState.indexOfId_(_offs, load.off.type.id)
        if (load.armour != null) slot["armour"] = BattleTestState.indexOfId_(_armours, load.armour.type.id)
        var names = def.kitNames
        for (i in 0...MOVE_SLOTS) {
            if (i < names.count) slot.moves[i] = Moves.byName(names[i]).id
        }
        fillMoves_(slot)
    }

    // The loadout the slot's pickers describe.
    loadoutOf_(slot) {
        var quality = slot["quality"]
        var main = slot["main"] == NONE ? null : GearItem.new(_mains[slot["main"]], quality, element_(slot["welem"]), effects_(slot, "wfx"))
        var off = slot["off"] == NONE ? null : GearItem.new(_offs[slot["off"]], quality, null, null)
        var armour = slot["armour"] == NONE ? null : GearItem.new(_armours[slot["armour"]], quality, element_(slot["aelem"]), effects_(slot, "afx"))
        return Loadout.new(main, off, armour)
    }

    element_(index) { index == NONE ? null : Elements.all[index] }

    effects_(slot, prefix) {
        var list = []
        for (i in 0...2) {
            var key = "%(prefix)%(i)"
            var effect = slot[key]
            if (effect == NONE) continue
            var name = Elements.effects[effect]
            // The same effect twice on one item counts once, at the higher rank.
            var known = false
            for (entry in list) {
                if (entry[0] == name) {
                    known = true
                    if (slot[key + "r"] > entry[1]) entry[1] = slot[key + "r"]
                }
            }
            if (!known) list.add([name, slot[key + "r"]])
        }
        return list
    }

    // The move ids the slot's equipment allows at its move tier, in catalog order.
    allowedMoves_(slot) {
        var load = loadoutOf_(slot)
        var out = []
        for (id in Moves.ids) {
            if (Moves.minTier(id) > slot["tier"]) continue
            if (load.allows(Moves.get(id, slot["tier"]))) out.add(id)
        }
        return out
    }

    // After the gear changes: moves it no longer allows drop out, and the empty slots fill up - Step first, then
    // what the weapon itself unlocks, then the rest.
    fillMoves_(slot) {
        var allowed = allowedMoves_(slot)
        var moves = slot.moves
        for (i in 0...MOVE_SLOTS) {
            if (moves[i] != null && !allowed.contains(moves[i])) moves[i] = null
        }
        var wanted = []
        if (allowed.contains("Step")) wanted.add("Step")
        for (id in allowed) {
            if (Moves.get(id, slot["tier"]).weapons != null) wanted.add(id)
        }
        for (id in allowed) {
            if (!wanted.contains(id)) wanted.add(id)
        }
        // Only top up a loadout that has gone short - a deliberately small one (the starter kit) stays small.
        var have = moves.where { |m| m != null }.count
        if (have >= 3) return
        for (id in wanted) {
            if (have >= MOVE_SLOTS) break
            if (moves.contains(id)) continue
            for (i in 0...MOVE_SLOTS) {
                if (moves[i] == null) {
                    moves[i] = id
                    have = have + 1
                    break
                }
            }
        }
    }

    // --- pickers ------------------------------------------------------------------------------

    focused_(kind, id) {
        _focusKind = kind
        _focusId = id
        if (kind == "slot" && id != _selected) {
            _selected = id
            refresh_()
        }
    }

    // Steps picker `id` of the selected slot by `dir` (+1 / -1).
    cycle_(id, dir) {
        if (_inBattle) return
        if (id == "scenario") {
            _scenario = BattleTestState.wrap_(_scenario + dir, 0, SCENARIOS.count - 1)
            refresh_()
            return
        }
        if (id == "read") {
            _read = BattleTestState.wrap_(_read + dir, 0, 2)
            refresh_()
            return
        }
        var slot = _slots[_selected]
        if (id == "fighter") {
            cycleFighter_(slot, dir)
            refresh_()
            return
        }
        if (!humanoid_(slot)) return
        if (id == "stats") {
            slot[id] = BattleTestState.wrap_(slot[id] + dir, 0, STAT_TIERS.count - 1)
        } else if (id == "main") {
            slot[id] = BattleTestState.wrap_(slot[id] + dir, NONE, _mains.count - 1)
            if (slot[id] != NONE && _mains[slot[id]].twoHanded) slot["off"] = NONE
            fillMoves_(slot)
        } else if (id == "off") {
            if (slot["main"] != NONE && _mains[slot["main"]].twoHanded) return
            slot[id] = BattleTestState.wrap_(slot[id] + dir, NONE, _offs.count - 1)
            fillMoves_(slot)
        } else if (id == "armour") {
            slot[id] = BattleTestState.wrap_(slot[id] + dir, NONE, _armours.count - 1)
        } else if (id == "quality") {
            slot[id] = BattleTestState.wrap_(slot[id] + dir, 0, 2)
        } else if (id == "tier") {
            slot[id] = BattleTestState.wrap_(slot[id] + dir, 1, 3)
            fillMoves_(slot)
        } else if (id == "welem" || id == "aelem") {
            slot[id] = BattleTestState.wrap_(slot[id] + dir, NONE, Elements.all.count - 1)
        } else if (id.endsWith("r")) {
            slot[id] = BattleTestState.wrap_(slot[id] + dir, 1, 3)
        } else if (id.startsWith("wfx") || id.startsWith("afx")) {
            slot[id] = BattleTestState.wrap_(slot[id] + dir, NONE, Elements.effects.count - 1)
        } else if (id.startsWith("move")) {
            cycleMove_(slot, Num.fromString(id[4..-1]), dir)
        }
        refresh_()
    }

    static wrap_(value, low, high) {
        if (value > high) return low
        if (value < low) return high
        return value
    }

    // Your side only takes humanoids (and its first slot can't be emptied - a side needs a fighter); the other side
    // takes anyone.
    cycleFighter_(slot, dir) {
        var party = isPartySlot_(_selected)
        var canEmpty = _selected != 0 && _selected != SLOTS_PER_SIDE
        var index = slot["fighter"]
        for (step in 0..._fighters.count + 1) {
            index = BattleTestState.wrap_(index + dir, NONE, _fighters.count - 1)
            if (index == NONE) {
                if (canEmpty) break
                continue
            }
            if (!party || _fighters[index][2]) break
        }
        setFighter_(slot, index)
    }

    // A move slot steps through the moves the equipment allows that aren't already in the loadout (and "empty").
    cycleMove_(slot, moveIndex, dir) {
        var allowed = allowedMoves_(slot)
        var options = [null]
        for (id in allowed) {
            if (id == slot.moves[moveIndex] || !slot.moves.contains(id)) options.add(id)
        }
        var at = options.indexOf(slot.moves[moveIndex])
        if (at < 0) at = 0
        slot.moves[moveIndex] = options[BattleTestState.wrap_(at + dir, 0, options.count - 1)]
    }

    // --- showing it -----------------------------------------------------------------------------

    refresh_() {
        for (i in 0..._slots.count) {
            var slot = _slots[i]
            _view.setSlotText(i, slot.empty ? "(empty)" : _fighters[slot["fighter"]][0])
        }
        _view.setSelected(_selected)
        _view.setPickerText("scenario", "Scenario: %(SCENARIOS[_scenario])")
        _view.setPickerText("read", "Your Read: %(_read)")

        var slot = _slots[_selected]
        var side = isPartySlot_(_selected) ? "Your side" : "Opponent"
        var number = _selected % SLOTS_PER_SIDE + 1
        if (slot.empty) {
            _view.setTitle("%(side) %(number): empty")
            for (id in _view.pickerIds) {
                if (id != "scenario" && id != "read") _view.setPickerText(id, id == "fighter" ? "(empty)" : "-")
            }
            _view.setInfo(["Pick a fighter for this slot."])
            return
        }
        var label = _fighters[slot["fighter"]][0]
        _view.setTitle("%(side) %(number): %(label)")
        _view.setPickerText("fighter", label)
        if (!humanoid_(slot)) {
            for (id in _view.pickerIds) {
                if (id != "scenario" && id != "read" && id != "fighter") _view.setPickerText(id, "-")
            }
            var def = defOf_(slot)
            var kind = def.passive ? "A prop: it never acts on its own." : "A monster: its own natural weapon and combo menu."
            _view.setInfo([kind, "HP %(def.maxHp)"])
            return
        }
        var statsName = STAT_TIERS[slot["stats"]]
        var qualityName = Elements.qualityNames[slot["quality"]]
        _view.setPickerText("stats", "Stats: %(statsName)")
        _view.setPickerText("main", "Main: " + (slot["main"] == NONE ? "Unarmed" : _mains[slot["main"]].name))
        _view.setPickerText("off", "Off: " + (slot["off"] == NONE ? "-" : _offs[slot["off"]].name))
        _view.setPickerText("armour", "Armour: " + (slot["armour"] == NONE ? "None" : _armours[slot["armour"]].name))
        _view.setPickerText("quality", "Gear quality: %(qualityName)")
        _view.setPickerText("tier", "Move tier: " + TIER_NAMES[slot["tier"]])
        _view.setPickerText("welem", "Weapon: " + (slot["welem"] == NONE ? "no element" : Elements.all[slot["welem"]]))
        _view.setPickerText("aelem", "Armour: " + (slot["aelem"] == NONE ? "no element" : Elements.all[slot["aelem"]]))
        var kinds = [["wfx", "W"], ["afx", "A"]]
        for (kind in kinds) {
            for (i in 0...2) {
                var key = "%(kind[0])%(i)"
                var effect = slot[key]
                _view.setPickerText(key, kind[1] + " effect: " + (effect == NONE ? "-" : Elements.effects[effect]))
                _view.setPickerText(key + "r", effect == NONE ? "-" : TIER_NAMES[slot[key + "r"]])
            }
        }
        for (i in 0...MOVE_SLOTS) {
            var id = slot.moves[i]
            var text = "-"
            if (id != null) {
                var move = Moves.get(id, slot["tier"])
                var seconds = Move.fmt(move.completion)
                text = "%(move.name)  %(seconds)s"
            }
            _view.setPickerText("move%(i)", text)
        }
        _view.setInfo(summary_(slot))
    }

    // What the slot's gear adds up to: tax, the weapon x armour pairing, Resonance.
    summary_(slot) {
        var load = loadoutOf_(slot)
        var tax = load.tax
        var total = (tax[0] + tax[1]).round
        var line1 = "Tax per move: %(total) (weapons %(tax[0].round) + armour %(tax[1].round))   Armour defence %(load.armourDefence)   Stagger resistance %((load.staggerRes * 100).round) pct"
        var pairing = load.pairing
        var line2 = "Elements: " + (pairing == null ? "no weapon x armour pairing" : pairing)
        var resonant = []
        for (effect in Elements.effects) {
            if (load.capstone(effect)) {
                resonant.add(effect + " capstone")
            } else if (load.weaponRank(effect) > 0 && load.armourRank(effect) > 0) {
                resonant.add(effect + " (+1 rank)")
            }
        }
        var line3 = "Resonance: " + (resonant.count == 0 ? "none" : resonant.join(", "))
        return [line1, line2, line3]
    }

    // --- running a battle -----------------------------------------------------------------------

    startBattle_() {
        if (_inBattle) return
        var used = {}
        var party = []
        var enemies = []
        for (i in 0..._slots.count) {
            var slot = _slots[i]
            if (slot.empty) continue
            var isParty = isPartySlot_(i)
            var combatant = build_(slot, used)
            if (isParty) {
                combatant.read = _read
                party.add(combatant)
            } else {
                combatant.brain = brainFor_(slot)
                enemies.add(combatant)
            }
        }
        var encounter = null
        if (_scenario == 1) encounter = Encounter.thomasLessons
        if (_scenario == 3) encounter = Encounter.warden
        if (_scenario == 2) {
            // Two starving wolves, with the lesson's call-outs.
            enemies = []
            for (i in 0...2) {
                var def = BattleCritter.starvingThornwolf
                var wolf = def.makeCombatant(null, i == 0 ? "Starving Thornwolf" : "Thornwolf 2", null, null)
                wolf.brain = def.makeBrain()
                enemies.add(wolf)
            }
            encounter = Encounter.forEnemies(["Starving Thornwolf"], party.count)
        }

        _inBattle = true
        _view.show(false)
        _view.setResult("")
        _manager.changeState("CombatState", {
            "partyCombatants": party,
            "enemyCombatants": enemies,
            "encounter": encounter,
            "player": null,
            "world": _world,
            "testItems": true,
            "battleLevelIndex": World.getLevelIndex(_world, BATTLE_ARENA_LEVEL_NAME)
        })
    }

    // A fresh Combatant for a filled slot. used: label -> count across both sides, so a second Chicken is
    // "Chicken 2".
    build_(slot, used) {
        var label = _fighters[slot["fighter"]][0]
        var def = defOf_(slot)
        used[label] = used.containsKey(label) ? used[label] + 1 : 1
        var name = used[label] == 1 ? label : "%(label) %(used[label])"
        if (label.endsWith(" (boss)")) name = def.name
        if (!humanoid_(slot)) return def.makeCombatant(null, name, null, null)
        var moves = []
        for (id in slot.moves) {
            if (id != null) moves.add(Moves.get(id, slot["tier"]))
        }
        var combatant = def.makeCombatant(null, name, loadoutOf_(slot), moves)
        if (slot["stats"] > 0) combatant.stats = Stats.tier(slot["stats"] - 1)
        return combatant
    }

    // The other side's AI: the def's own (a monster's or a scripted humanoid's combo menu, a boss's mind), or - for a
    // humanoid without one - the planner, which fights with whatever gear and moves were picked here.
    brainFor_(slot) {
        var def = defOf_(slot)
        if (!humanoid_(slot) || def.hasMenu) return def.makeBrain()
        return PlannerBrain.new()
    }

    resultText_(outcome) {
        if (outcome == null) return "The battle didn't start."
        var text = "Last battle: "
        if (outcome.won) {
            text = text + "won"
        } else if (outcome.lost) {
            text = text + "lost"
        } else {
            text = text + "fled"
        }
        if (outcome.fallen.count > 0) {
            var names = outcome.fallen.join(", ")
            text = "%(text)  -  defeated: %(names)"
        }
        return text
    }

    back_() {
        if (_inBattle) return
        _stateManager.changeState("GameWorld")
    }
}
