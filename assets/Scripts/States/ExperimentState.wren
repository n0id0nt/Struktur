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

import "States/BaseState" for BaseState
import "States/StateManager" for StateManager
import "States/PlayState" for PlayState
import "States/InventoryState" for InventoryState
import "States/InteractState" for InteractState
import "States/GameOverState" for GameOverState

import "Colors" for WHITE

var WORLD_FILE_PATH = "Levels/SevenGods.ldtk"

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
        _gameMusic = null
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
            _stateManager.update()
            return
        }
        if (_gameMusic && !_gameMusic.isPlaying()) {
            _gameMusic.play()
        }
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
