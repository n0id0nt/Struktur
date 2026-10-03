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
// This class is just the shell around a fight: it builds the Combatants, the UI/Combat/CombatUI
// view and (when an arena exists) the Combat/BattleStage, bundles them into a States/Combat/
// CombatSession, and runs a nested StateManager over one sub-state per phase of the fight, each its
// own class under States/Combat/:
//   EnteringState  - arena entrance animation (or IntroState, the fight-in-place fallback beat)
//   ChoosingState  - player's move menu (time PAUSED)     TargetingState - pick which enemy to hit (PAUSED)
//   ChargingState  - timelines fill, attack animations play, hits land
//   ParryPromptState - "Parry?" while an attack crawls to impact (time SLOWED)
//   ReturningState - attacker eases home   MessageState - battle-log line
//   FleeState      - the player runs off    OverState    - closing line, then done
// Phases hand off to each other through CombatSession.goTo(); the move-resolution rules live in
// States/Combat/CombatResolver.wren.
//
// Battles are animation driven (Combat/Config/ActionAnimations.wren): a move's attack animation is its charge,
// and everything runs on game time - menus pause it and the parry prompt slows it with Time.setTimeScale, which
// pauses/slows every animation, the Timeline and the particles together. exit() always restores normal time.
//
// Presentation cuts to the Battle_Arena level (see Combat/BattleStage.wren), which deactivates the
// overworld player and every other loaded level entity for the duration (GameObject.setInactive), so
// the paused/slowed time scale only ever affects the fight itself.
import "States/BaseState" for BaseState
import "States/StateManager" for StateManager
import "app" for Time
import "gameObject" for GameObject
import "gameObjectComponents" for Script, World, WorldTransform, Level
import "math" for Vec3

import "Combat/Config/BattleCritter" for BattleCritter
import "Combat/BattleStage" for BattleStage
import "UI/Combat/CombatUI" for CombatUI
import "States/Combat/CombatSession" for CombatSession
import "States/Combat/EnteringState" for EnteringState
import "States/Combat/IntroState" for IntroState
import "States/Combat/ChoosingState" for ChoosingState
import "States/Combat/TargetingState" for TargetingState
import "States/Combat/ChargingState" for ChargingState
import "States/Combat/ParryPromptState" for ParryPromptState
import "States/Combat/FleeState" for FleeState
import "States/Combat/ReturningState" for ReturningState
import "States/Combat/MessageState" for MessageState
import "States/Combat/OverState" for OverState

class CombatState is BaseState {
    construct new() {
        super()
        name = "CombatState"
        _view = null
        _stage = null     // BattleStage while an authored arena is in use; null = fight in place
        _session = null

        _manager = StateManager.new()
        _manager.insertState("EnteringState", EnteringState)
        _manager.insertState("IntroState", IntroState)
        _manager.insertState("ChoosingState", ChoosingState)
        _manager.insertState("TargetingState", TargetingState)
        _manager.insertState("ChargingState", ChargingState)
        _manager.insertState("ParryPromptState", ParryPromptState)
        _manager.insertState("FleeState", FleeState)
        _manager.insertState("ReturningState", ReturningState)
        _manager.insertState("MessageState", MessageState)
        _manager.insertState("OverState", OverState)
    }

    enter(stateManager, params) {
        super.enter(stateManager, params)

        var enemies = []
        var enemyDefs = []
        for (entity in params["opponents"]) {
            var s = Script.getInstance(entity)
            if (s == null) {
                continue
            }
            var def = BattleCritter.forName(s.name)
            enemyDefs.add(def)
            enemies.add(def.makeCombatant(entity))
        }

        // The player's Combatant lives on the Player script (persistent HP / stamina across fights).
        var playerScript = params["player"] == null ? null : Script.getInstance(params["player"])
        if (playerScript == null || enemies.count == 0) {
            System.print("[CombatState] no player or no valid opponents, aborting")
            stateManager.clearCurrentState()
            return
        }
        var player = playerScript.combatant

        var roster = enemies.map { |e| "%(e.name)(%(e.stats.hp))" }.join(", ")
        System.print("[CombatState] You(%(player.stats.hp)/%(player.stats.maxHp)) vs %(roster)")

        var enemyNames = enemies.map { |e| e.name }.toList
        var moveLabels = player.moves.map { |m| m.menuLabel }.toList
        _view = CombatUI.new(enemyNames, player.name, moveLabels)

        // Cut to the battle arena (Combat/BattleStage.wren) - the authored battle level
        // (ExperimentState.BATTLE_ARENA_LEVEL_NAME) if one was found, centred on its bounds rather
        // than its top-left corner (that's where World/LDtk places a level's WorldTransform);
        // otherwise a fixed offscreen patch of world, or an authored "BattleAnchor" room if one
        // exists. Needs the world root to parent battle entities under; without it (an unexpected
        // caller) fall back to fighting in place.
        _stage = null
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
                                     player, enemyDefs, enemies, battleLevelEntity)
        }

        var session = CombatSession.new(stateManager, _manager, player, enemies, enemyDefs, _view, _stage)
        _session = session

        // The move-menu buttons are wired once per fight (a button's click handler can't be
        // replaced), so each click re-checks it landed in the right phase.
        var i = 0
        for (button in _view.moveButtons) {
            var chosen = player.moves[i]
            button.setOnClick { |s, m|
                if (session.isIn("ChoosingState") && session.armed()) {
                    session.commitPlayerMove(chosen)
                }
            }
            i = i + 1
        }
        _view.fleeButton.setOnClick { |s, m|
            if (session.isIn("ChoosingState") && session.armed()) {
                session.resolver.flee()
            }
        }

        var foe = enemies.count == 1 ? "the %(enemies[0].name)" : "%(enemies.count) foes"
        _view.setMessage("You're set upon by %(foe)!")
        session.refreshViews()

        session.goTo(_stage != null ? "EnteringState" : "IntroState")
    }

    firstEntity_(identifier) {
        var list = GameObject.getAllWithIdentifier(identifier)
        return list.count > 0 ? list[0] : null
    }

    update(stateManager) {
        _manager.update()
        // Every frame, whatever the phase: anchor each battler's animation to the Timeline (see
        // CombatSession.animateBattlers). _session is null if the phase just ended the whole fight.
        if (_session != null) {
            _session.animateBattlers()
        }
    }

    fixedUpdate(stateManager) {
        _manager.fixedUpdate()
    }

    render() {
        _manager.render()
    }

    onEvent(type, data) {
        super.onEvent(type, data)
        _manager.onEvent(type, data)
    }

    exit() {
        super.exit()
        // Let the active phase exit first, then fold the arena away (reactivates the overworld
        // player, puts survivors back) so the world is coherent again before the UI teardown.
        _manager.clearCurrentState()
        // Menus pause time and the parry prompt slows it - whatever phase the fight ended in, the world
        // gets normal time back.
        Time.setTimeScale(1)
        if (_stage != null) {
            _stage.teardown()
            _stage = null
        }
        if (_view != null) {
            _view.teardown()
            _view = null
        }
        _session = null
    }

    // Visible to the state-debug window's stack view (and StateManager's own traversal) - the active
    // phase of the fight.
    subStateManager { _manager }
}
