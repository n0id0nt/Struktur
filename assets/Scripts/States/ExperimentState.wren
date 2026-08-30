// states/ExperimentState.wren
// Main gameplay state - loads level, creates entities, manages gameplay sub-states
// This state owns the game world and delegates to sub-states for different gameplay modes

import "gameObject" for GameObject
import "gameObjectComponents" for LocalTransform, WorldTransform, World, Level, Script, Sprite, Camera
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
        WorldTransform.setPosition(playerEntity, Vec3.new(600.0, 50.0, 0.0))
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
