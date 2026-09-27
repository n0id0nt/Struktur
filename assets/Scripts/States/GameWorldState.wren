// states/GameWorldState.wren
// Main gameplay state - loads level, creates entities, manages gameplay sub-states. This state owns
// the game world and delegates to sub-states for different gameplay modes. Owns the world/room
// setup, the nested sub-StateManager (PlayState/InventoryState/InteractState/GameOverState), the
// win-condition check, and the UI/Gameplay/InteractPromptUI view.
//
// Note: this state is not on the live gameplay path today - Main.wren starts "Experiment"
// (States/ExperimentState.wren) instead - but is kept consistent with the rest of the reorg.
import "gameObject" for GameObject
import "gameObjectComponents" for WorldTransform, World, Script
import "math" for Vec3
import "resourceManager" for Music
import "input" for Input
import "flags" for FlagManager

import "States/BaseState" for BaseState
import "States/StateManager" for StateManager
import "States/PlayState" for PlayState
import "States/InventoryState" for InventoryState
import "States/InteractState" for InteractState
import "States/GameOverState" for GameOverState
import "UI/Gameplay/InteractPromptUI" for InteractPromptUI

var WORLD_FILE_PATH = "Levels/MemoryPalace.ldtk"

class GameWorldState is BaseState {
    construct new() {
        super()
        name = "GameWorldState"

        _view = null
        _worldEntity = null //TODO Create an entity constant for invalid entity or null entity
        _stateManager = StateManager.new()

        _stateManager.insertState("PlayState", PlayState)
        _stateManager.insertState("InventoryState", InventoryState)
        _stateManager.insertState("InteractState", InteractState)
        _stateManager.insertState("GameOverState", GameOverState)
        _gameMusic = null
    }

    createRoom(worldEntity, roomName, regularRoomName, transformedRoomName, transformItem) {
        var roomEntity = GameObject.create(roomName, worldEntity)

        var regularRoomIndex = World.getLevelIndex(worldEntity, regularRoomName)
        var transformedRoomIndex = World.getLevelIndex(worldEntity, transformedRoomName)
        var regularRoom = World.loadLevelEntities(worldEntity, regularRoomIndex)
        var transformedRoom = World.loadLevelEntities(worldEntity, transformedRoomIndex)
        WorldTransform.setPosition(regularRoom, Vec3.new(0.0, 0.0, 0.0))
        WorldTransform.setPosition(transformedRoom, Vec3.new(0.0, 0.0, 0.0))
        GameObject.setParent(regularRoom, roomEntity)
        GameObject.setParent(transformedRoom, roomEntity)

        Script.createArg(roomEntity, "Room", {"Name": roomName, "TransformItem": transformItem,
            "RegularRoom": regularRoom, "TransformedRoom": transformedRoom})

        return roomEntity
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

        var northRoom = createRoom(worldEntity, "NorthRoom", "Library", "Garden", "Love Letter")
        WorldTransform.setPosition(northRoom, Vec3.new(400.0, 0.0, 0.0))

        var eastRoom = createRoom(worldEntity, "EastRoom", "Kitchen", "Workshop", "Hammer")
        WorldTransform.setPosition(eastRoom, Vec3.new(800.0, 400.0, 0.0))

        var southRoom = createRoom(worldEntity, "SouthRoom", "Bedroom", "Observatory", "Star Chart")
        WorldTransform.setPosition(southRoom, Vec3.new(400.0, 800.0, 0.0))

        var westRoom = createRoom(worldEntity, "WestRoom", "Treasury", "Vault", "Love Letter")
        WorldTransform.setPosition(westRoom, Vec3.new(0.0, 400.0, 0.0))

        var courtyardIndex = World.getLevelIndex(worldEntity, "Courtyard")
        var courtyard = World.loadLevelEntities(worldEntity, courtyardIndex)
        WorldTransform.setPosition(courtyard, Vec3.new(400.0, 400.0, 0.0))

        var playerEntity = GameObject.create("Player", worldEntity)
        Script.createArg(playerEntity, "Player", {"Name": "Player"})
        WorldTransform.setPosition(playerEntity, Vec3.new(600.0, 50.0, 0.0))

        _view = InteractPromptUI.new("Interact")

        //_stateManager.changeState("PlayState")
        _stateManager.changeState("InventoryState")

        System.print("Game world loaded")
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

        var inputInteract = Input.isInputJustReleased("Interact")
        var inventoryInteract = Input.isInputJustReleased("Inventory")

        // TODO this should be an event.
        var playerEntities = GameObject.getAllWithIdentifier("Player")
        if (inventoryInteract) {
            _view.hide()
            //TODO also pause the game time to pause the players animation
            // just forcing player to idle for now
            for (entity in playerEntities) {
                var script = Script.getInstance(entity)
                script.playerForceStop()
            }
            _stateManager.changeState("InventoryState")
            if (_gameMusic) {
                _gameMusic.stop()
            }
            return
        }

        for (entity in playerEntities) {
            var script = Script.getInstance(entity)
            if (!script) {
                continue
            }
            var interactEntity = script.getInteractEntity()

            if (interactEntity) {
                _view.showAbove(WorldTransform.getPosition(interactEntity))
                if (inputInteract) {
                    _view.hide()
                    script.playerForceStop()
                    // Change state to interact state
                    _stateManager.changeState("InteractState", {"interactingEntity": interactEntity})
                    if (_gameMusic) {
                        _gameMusic.stop()
                    }
                    return
                }
            } else {
                _view.hide()
            }

            // check player at bottom of screen
            var playerPosition = WorldTransform.getPosition(entity)
            if (playerPosition.y > 1755.0) {
                if (FlagManager.getFlag("red_pedestal_active") && FlagManager.getFlag("green_pedestal_active") &&
                    FlagManager.getFlag("yellow_pedestal_active") && FlagManager.getFlag("blue_pedestal_active")) {
                    script.playerForceStop()
                    _stateManager.changeState("GameOverState")
                    if (_gameMusic) {
                        _gameMusic.stop()
                    }
                    return
                }

                // reset the current state
                stateManager.changeState("GameWorld")
                return
            }
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

        if (_view != null) {
            _view.teardown()
            _view = null
        }
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
