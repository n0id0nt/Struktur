// states/ExperimentState.wren
// Main gameplay state - loads level, creates entities, manages gameplay sub-states
// This state owns the game world and delegates to sub-states for different gameplay modes

import "gameObject" for GameObject
import "gameObjectComponents" for LocalTransform, WorldTransform, World, Level, Script, Sprite, Camera, ParticleEmitter
import "math" for Vec2, Vec3, Vec4
import "resourceManager" for Font, Texture, Music
import "ui" for UIManager, UILabel
import "input" for Input
import "flags" for FlagManager
import "app" for Time, Application

import "States/BaseState" for BaseState
import "States/StateManager" for StateManager
import "States/PlayState" for PlayState
import "States/InventoryState" for InventoryState
import "States/InteractState" for InteractState
import "States/GameOverState" for GameOverState
import "States/CombatState" for CombatState

import "GameObjects/Critter" for Critter

import "Colors" for WHITE

var WORLD_FILE_PATH = "Levels/SevenGods.ldtk"
// Combat proximity ranges, in metres (converted to world pixels at use via Application.pixelsPerMeter,
// default 64). An aggressive critter (Chinlin) within FORCE range forces combat outright rather than
// just offering it via the interact prompt - meant to feel like being caught, not spotted. When a
// fight starts, every other critter within GROUP range OF THE PLAYER is dragged in too, nearest
// first, up to MAX_COMBAT_GROUP total (the closest 3 when more than that are in range).
var FORCE_COMBAT_METERS = 1.0
var GROUP_COMBAT_METERS = 3.0
var MAX_COMBAT_GROUP = 3

class ExperimentState is BaseState {
    construct new() {
        super()
        name = "ExperimentState"
        
        _worldEntity = null //TODO Create an entity constant for invalid entity or null entity
        _particleEntity = null
        _stateManager = StateManager.new()

        _stateManager.insertState("PlayState", PlayState)
        _stateManager.insertState("InventoryState", InventoryState)
        _stateManager.insertState("InteractState", InteractState)
        _stateManager.insertState("GameOverState", GameOverState)
        _stateManager.insertState("CombatState", CombatState)
        _gameMusic = null
        _interactLabel = null
        // Grace window after a fight ends (see update()) - long enough to run clear of an aggressive
        // critter before its proximity check can re-trigger combat. Covers every outcome: after a
        // loss or a flee the enemy is still alive and still next to you, and without this you'd be
        // stuck re-fighting it in place.
        _combatCooldownEndTime = 0
    }

    enter(stateManager, params) {
        super.enter(stateManager, params)
        
        System.print("Loading game world...")

        _gameMusic = Music.load("Sounds/gameMusic.wav")
        if (_gameMusic) {
            _gameMusic.setLooping(true)
            _gameMusic.play()
        }

        var worldEntity = World.createWorldEntity(WORLD_FILE_PATH)
        _worldEntity = worldEntity

        for (i in 0...World.getLevelsCount(worldEntity)) {
            var level = World.loadLevelEntities(worldEntity, i)
            GameObject.setParent(level, worldEntity)
        }

        var playerEntity = GameObject.create("Player", worldEntity)
        Script.createArg(playerEntity, "Player", {"Name": "Player"})
        WorldTransform.setPosition(playerEntity, Vec3.new(600.0, 300.0, 0.0))

        // "Fight" prompt for non-aggressive critters - same UILabel-above-target pattern
        // GameWorldState uses for its own "Interact" prompt.
        var font = Font.load("Fonts/medieval_sharp/MedievalSharp-Bold.ttf", 60)
        _interactLabel = UILabel.new(Vec2.new(0, 0), Vec2.new(0, 0), "Fight!", 16.0)
        _interactLabel.setVisible(false)
        _interactLabel.setFont(font)
        _interactLabel.setTextColor(WHITE)
        _interactLabel.setAnchorPoint(Vec2.new(0.5, 0.5))
        _interactLabel.setBoundingBoxToText()
        UIManager.addUIElement(_interactLabel)
        font.unload()

        spawnParticleDemo(worldEntity)

        // A few wandering critters to show off the per-entity FSMs. The chinlin also chases the
        // player when it gets close - see GameObjects/Chicken.wren and GameObjects/Chinlin.wren.
        spawnCritter("Chicken", Vec3.new(520.0, 400.0, 0.0), worldEntity)
        spawnCritter("Chicken", Vec3.new(690.0, 300.0, 0.0), worldEntity)
        spawnCritter("Chinlin", Vec3.new(600.0, 450.0, 0.0), worldEntity)
    }

    // name doubles as the class name (both scripts are registered under their own name in Main.wren)
    // and as the entity identifier.
    spawnCritter(className, position, worldEntity) {
        var entity = GameObject.create(className, worldEntity)
        Script.createArg(entity, className, {"Name": className})
        WorldTransform.setPosition(entity, position)
    }

    // Demonstrates Component::ParticleEmitter: a looping "magic fountain" that sprays glowing motes
    // upward (screen -y) and lets a constant downward acceleration arc them back down. The emitter
    // simulates at this entity's world position, so it just needs a Transform (added by
    // GameObject.create) - no Sprite required.
    spawnParticleDemo(worldEntity) {
        _particleEntity = GameObject.create("ParticleDemo", worldEntity)
        WorldTransform.setPosition(_particleEntity, Vec3.new(600.0, 220.0, 0.0))

        // Sprites/magic.png is a single 60x60 frame - leave columns/rows at their 1x1 default so each
        // particle draws the whole texture. The emitter keeps its own reference to the texture, so the
        // script's handle can be released immediately afterwards (same pattern as Sprite.create).
        var texture = Texture.load("Sprites/magic.png")
        var emitter = ParticleEmitter.create(_particleEntity, texture)
        texture.unload()

        emitter.looping      = true
        emitter.emissionRate = 45.0                       // motes per second
        emitter.maxParticles = 250                        // pool capacity
        emitter.spawnRadius  = 8.0                        // spawn within a small disc, not a single point

        emitter.lifetimeMin  = 0.7
        emitter.lifetimeMax  = 1.4

        emitter.velocityMin  = Vec2.new(-70.0, -230.0)    // up-and-out (screen space: -y is up)
        emitter.velocityMax  = Vec2.new(70.0, -120.0)
        emitter.acceleration = Vec2.new(0.0, 260.0)       // gravity pulls them back down into an arc

        emitter.startColor   = Vec4.new(120.0, 180.0, 255.0, 255.0)  // bright blue
        emitter.endColor     = Vec4.new(255.0, 120.0, 220.0, 0.0)    // fade out to pink
        emitter.startScale   = 0.6
        emitter.endScale     = 0.05                       // shrink to nothing over the lifetime

        emitter.rotationSpeedMin = -6.0                   // radians/sec - lazy tumble
        emitter.rotationSpeedMax = 6.0

        emitter.additive     = true                       // glow - additive blend, no depth sort needed

        // One-shot puff on spawn, on top of the continuous stream, so the fountain "pops" into life.
        emitter.burstCount   = 40
    }
    
    // Mirrors update(stateManager)'s own substate delegation below - GameWorldState is the one real
    // nested-subStateManager case today, so this doubles as the proof that fixed-update delegation works through
    // a substate, not just at the leaf level.
    fixedUpdate(stateManager) {
        if (_stateManager.currentState) {
            _stateManager.fixedUpdate()
        }
    }

    update(stateManager) {
        // if substate return out here
        if (_stateManager.currentState) {
            var wasCombat = _stateManager.currentState.name == "CombatState"
            _stateManager.update()
            if (wasCombat && !_stateManager.currentState) {
                _combatCooldownEndTime = Time.scaledTime + 2.0
            }
            return
        }
        if (_gameMusic && !_gameMusic.isPlaying()) {
            _gameMusic.play()
        }

        if (Time.scaledTime >= _combatCooldownEndTime) {
            checkCombatTriggers()
        }
    }

    // Aggressive critters (Chinlin) force the player straight into CombatState once they close the
    // distance - no prompt, no choice. Non-aggressive critters (Chicken) just become another
    // interactable, reusing the exact scan/prompt every Item/NPC/Door already uses
    // (Player.getInteractEntity(), see GameWorldState for the original of this pattern) - the player
    // has to press Interact to start that fight.
    checkCombatTriggers() {
        var playerEntities = GameObject.getAllWithIdentifier("Player")
        for (playerEntity in playerEntities) {
            var playerScript = Script.getInstance(playerEntity)
            if (!playerScript) {
                continue
            }
            var playerPosition = WorldTransform.getPosition(playerEntity)
            var forceRange = FORCE_COMBAT_METERS * Application.pixelsPerMeter

            for (critter in Critter.all) {
                if (!critter.aggressive) {
                    continue
                }
                var critterPosition = critter.position
                if (critterPosition && Vec3.distance(playerPosition, critterPosition) <= forceRange) {
                    _interactLabel.setVisible(false)
                    enterCombat(critter.entity, playerEntity)
                    return
                }
            }

            var interactEntity = playerScript.getInteractEntity()
            if (interactEntity && Script.getInstance(interactEntity) is Critter) {
                _interactLabel.setVisible(true)
                var interactWorldPosition = WorldTransform.getPosition(interactEntity)
                var screenInteractPosition = Camera.worldPosToScreenPos(interactWorldPosition) + Vec2.new(0, -32)
                _interactLabel.setPosition(screenInteractPosition, Vec2.new(0, 0))
                if (Input.isInputJustReleased("Interact")) {
                    _interactLabel.setVisible(false)
                    enterCombat(interactEntity, playerEntity)
                    return
                }
            } else {
                _interactLabel.setVisible(false)
            }
        }
    }

    enterCombat(primaryEntity, playerEntity) {
        for (entity in GameObject.getAllWithIdentifier("Player")) {
            var script = Script.getInstance(entity)
            if (script) {
                script.playerForceStop()
            }
        }
        if (_gameMusic) {
            _gameMusic.stop()
        }
        _stateManager.changeState("CombatState", {
            "opponents": combatGroup(primaryEntity, playerEntity),
            "player": playerEntity,
            "world": _worldEntity
        })
    }

    // The critter you engaged, plus every other critter within GROUP_COMBAT_METERS of the player -
    // nearest first, capped at MAX_COMBAT_GROUP (so when more than that are in range you fight the
    // closest ones). primaryEntity is always in, even if it's somehow outside the group range.
    combatGroup(primaryEntity, playerEntity) {
        var group = [primaryEntity]
        var playerPosition = WorldTransform.getPosition(playerEntity)
        if (playerPosition == null) {
            return group
        }
        var groupRange = GROUP_COMBAT_METERS * Application.pixelsPerMeter

        // Gather candidates as [distance, entity], then sort by distance so the cap keeps the closest.
        var candidates = []
        for (critter in Critter.all) {
            if (critter.entity == primaryEntity) {
                continue
            }
            var p = critter.position
            if (p == null) {
                continue
            }
            var d = Vec3.distance(playerPosition, p)
            if (d <= groupRange) {
                candidates.add([d, critter.entity])
            }
        }
        candidates.sort { |a, b| a[0] < b[0] }

        for (candidate in candidates) {
            if (group.count >= MAX_COMBAT_GROUP) {
                break
            }
            group.add(candidate[1])
        }
        return group
    }

    render() {
        // Delegate to gameplay sub-state
        if (_stateManager.currentState) {
            _stateManager.render()
        }
    }
    
    exit() {
        super.exit()
        
        System.print("Unloading game world...")
        
        // Exit sub-state manager
        if (_stateManager && _stateManager.currentState) {
            _stateManager.currentState.exit()
        }
        
        // Child of _worldEntity, so destroying the world tears the emitter down with it.
        _particleEntity = null
        GameObject.destroy(_worldEntity)

        UIManager.removeUIElement(_interactLabel)
        _interactLabel = null
        _gameMusic.stop()
        _gameMusic.unload()
        _gameMusic = null

        System.print("Game world unloaded")
    }
    
    onEvent(type, data) {
        super.onEvent(type, data)
        if (_stateManager && _stateManager.currentState) {
            _stateManager.currentState.onEvent(type, data)
        }
    }
    
    // Getters
    worldEntity { _worldEntity }
    subStateManager { _stateManager }
}
