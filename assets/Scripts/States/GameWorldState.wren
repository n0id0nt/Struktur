// states/GameWorldState.wren
// Main gameplay state - loads level, creates entities, manages gameplay sub-states
// This state owns the game world and delegates to sub-states for different gameplay modes

import "gameObject" for GameObject
import "gameObjectComponents" for LocalTransform, WorldTransform, World, Level, Script, Sprite, Camera
import "math" for Vec2, Vec3, Vec4
import "resourceManager" for Font, Texture, Music
import "ui" for UIManager, UILabel
import "input" for Input

import "States/BaseState" for BaseState
import "States/StateManager" for StateManager
import "States/PlayState" for PlayState
import "States/InventoryState" for InventoryState
import "States/InteractState" for InteractState
import "States/GameOverState" for GameOverState

import "Colors" for WHITE
import "Inventory" for Inventory

var TILE_TEXTURE = "Tiles/cavesofgallet_tiles.png"
var PLAYER_TEXTURE = "Tiles/PlayerGrowthSprites.png"
var WORLD_FILE_PATH = "Levels/MemoryPalace.ldtk"
var Loops = 0

class GameWorldState is BaseState {
    construct new() {
        super()
        name = "GameWorldState"
        
        _interactLabel = null
        _loopCountLabel = null
        _worldEntity = null //TODO Create an entity constant for invalid entity or null entity
        _stateManager = StateManager.new()

        _stateManager.insertState("PlayState", PlayState)
        _stateManager.insertState("InventoryState", InventoryState)
        _stateManager.insertState("InteractState", InteractState)
        _stateManager.insertState("GameOverState", GameOverState)
        _gameMusic = null
    }

    getNorthRoom() {
        if (Inventory.contains("Love Letter")) {
            return "Garden"
        }
        return "Library"
    }

    getEastRoom() {
        if (Inventory.contains("Hammer")) {
            return "Workshop"
        }
        return "Kitchen"
    }

    getSouthRoom() {
        if (Inventory.contains("Star Chart")) {
            return "Observatory"
        }
        return "Bedroom"
    }

    getWestRoom() {
        if (Inventory.contains("Ornate Key")) {
            return "Vault"
        }
        return "Treasury"
    }

    getCourtyard() {
        if (Inventory.contains("Red Crystal Key") && Inventory.contains("Green Crystal Key") && Inventory.contains("Yellow Crystal Key") && Inventory.contains("Blue Crystal Key")) {
            return "Courtyard_Complete"
        }
        return "Courtyard"
    }

    calculateRoomListToLoad(worldEntity) {
        // NorthRoom (0-2)
        var northRoom = World.getLevelIndex(worldEntity, getNorthRoom())
        // EastRoom (3-5)
        var eastRoom = World.getLevelIndex(worldEntity, getEastRoom())
        // SouthRoom (6-8)
        var southRoom = World.getLevelIndex(worldEntity, getSouthRoom())
        // WestRoom (9-11)
        var westRoom = World.getLevelIndex(worldEntity, getWestRoom())
        // Courtyard (12)
        var courtyard = World.getLevelIndex(worldEntity, getCourtyard())
        return [northRoom, eastRoom, westRoom, southRoom, courtyard]
    }
    
    enter(stateManager, params) {
        super.enter(stateManager, params)
        
        System.print("Loading game world...")

        Loops = Loops + 1 // increment the game loop count
        
        _gameMusic = Music.load("Sounds/gameMusic.wav")
        if (_gameMusic) {
            _gameMusic.setLooping(true)
            _gameMusic.play()
        }

        var font = Font.load("Fonts/medieval_sharp/MedievalSharp-Bold.ttf", 60)

        var worldEntity = World.createWorldEntity(WORLD_FILE_PATH)
        _worldEntity = worldEntity

        var roomList = calculateRoomListToLoad(worldEntity)

        var northRoom = World.loadLevelEntities(worldEntity, roomList[0])
        WorldTransform.setPosition(northRoom, Vec3.new(576.0, 0.0, 0.0))
        var northRoomSpriteEntity = GameObject.create("northRoomSprite", northRoom)
        LocalTransform.setPosition(northRoomSpriteEntity, Vec3.new(0.0, 0.0, 0.0))
        var northRoomKey = getNorthRoom()
        var northRoomSpriteTexture = Texture.load("Tiles/%(northRoomKey).png")
        Sprite.create(northRoomSpriteEntity, northRoomSpriteTexture, WHITE, Vec2.new(0, 0), 1, 1, false, 0, 0)

        var eastRoom = World.loadLevelEntities(worldEntity, roomList[1])
        WorldTransform.setPosition(eastRoom, Vec3.new(1152.0, 576.0, 0.0))
        var eastRoomSpriteEntity = GameObject.create("eastRoomSprite", eastRoom)
        LocalTransform.setPosition(eastRoomSpriteEntity, Vec3.new(0.0, 0.0, 0.0))
        var eastRoomKey = getEastRoom()
        var eastRoomSpriteTexture = Texture.load("Tiles/%(eastRoomKey).png")
        Sprite.create(eastRoomSpriteEntity, eastRoomSpriteTexture, WHITE, Vec2.new(0, 0), 1, 1, false, 0, 0)
        eastRoomSpriteTexture.unload()

        var westRoom = World.loadLevelEntities(worldEntity, roomList[2])
        WorldTransform.setPosition(westRoom, Vec3.new(0.0, 576.0, 0.0))
        var westRoomSpriteEntity = GameObject.create("westRoomSprite", westRoom)
        LocalTransform.setPosition(westRoomSpriteEntity, Vec3.new(0.0, 0.0, 0.0))
        var westRoomKey = getWestRoom()
        var westRoomSpriteTexture = Texture.load("Tiles/%(westRoomKey).png")
        Sprite.create(westRoomSpriteEntity, westRoomSpriteTexture, WHITE, Vec2.new(0, 0), 1, 1, false, 0, 0)
        westRoomSpriteTexture.unload()

        var southRoom = World.loadLevelEntities(worldEntity, roomList[3])
        WorldTransform.setPosition(southRoom, Vec3.new(576.0, 1152.0, 0.0))
        var southRoomSpriteEntity = GameObject.create("southRoomSprite", southRoom)
        LocalTransform.setPosition(southRoomSpriteEntity, Vec3.new(0.0, 0.0, 0.0))
        var southRoomKey = getSouthRoom()
        var southRoomSpriteTexture = Texture.load("Tiles/%(southRoomKey).png")
        Sprite.create(southRoomSpriteEntity, southRoomSpriteTexture, WHITE, Vec2.new(0, 0), 1, 1, false, 0, 0)
        southRoomSpriteTexture.unload()

        var courtyard = World.loadLevelEntities(worldEntity, roomList[4])
        WorldTransform.setPosition(courtyard, Vec3.new(576.0, 576.0, 0.0))
        var courtyardSpriteEntity = GameObject.create("courtyardSprite", courtyard)
        LocalTransform.setPosition(courtyardSpriteEntity, Vec3.new(0.0, 0.0, 0.0))
        var courtyardSpriteTexture = Texture.load("Tiles/Courtyard.png")
        Sprite.create(courtyardSpriteEntity, courtyardSpriteTexture, WHITE, Vec2.new(0, 0), 1, 1, false, 0, 0)
        courtyardSpriteTexture.unload()

        var northRoomDupe = World.loadLevelEntities(worldEntity, roomList[0])
        WorldTransform.setPosition(northRoomDupe, Vec3.new(576.0, 1728.0, 0.0))
        var northRoomDupeSpriteEntity = GameObject.create("northRoomDupeSprite", northRoomDupe)
        LocalTransform.setPosition(northRoomDupeSpriteEntity, Vec3.new(0.0, 0.0, 0.0))
        Sprite.create(northRoomDupeSpriteEntity, northRoomSpriteTexture, WHITE, Vec2.new(0, 0), 1, 1, false, 0, 0)
        northRoomSpriteTexture.unload()

        var playerEntity = GameObject.create("Player", worldEntity)
        Script.createArg(playerEntity, "Player", ["Name"], ["Test Player Name"])
        WorldTransform.setPosition(playerEntity, Vec3.new(864.0, 32.0, 0.0))

        var lockedDoorEntity = GameObject.create("Entrance Door", worldEntity)
        Script.createArg(lockedDoorEntity, "Door", ["Name"], ["Test Door Name"])
        WorldTransform.setPosition(lockedDoorEntity, Vec3.new(864.0, 0.0, 0.0))
        
        // Create the UI for the level.
        _interactLabel = UILabel.new(Vec2.new(0, 0), Vec2.new(0, 0), "Interact", 16.0)
        _interactLabel.setVisible(false)
        _interactLabel.setFont(font)
        _interactLabel.setTextColor(WHITE) // Change this when the background is created.
        _interactLabel.setAnchorPoint(Vec2.new(0.5, 0.5))
        _interactLabel.setBoundingBoxToText()
        UIManager.addUIElement(_interactLabel)

        var loops = Loops
        _loopCountLabel = UILabel.new(Vec2.new(20, 20), Vec2.new(0, 0), "Loops: %(loops)", 30.0)
        _loopCountLabel.setFont(font)
        _loopCountLabel.setTextColor(WHITE) // Change this when the background is created.
        _loopCountLabel.setVisible(true)
        _loopCountLabel.setBoundingBoxToText()
        UIManager.addUIElement(_loopCountLabel)
        font.unload()
        //_stateManager.changeState("PlayState")
        
        System.print("Game world loaded")
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
        var inputDir = Input.getInputAxis2("Move")
        inputDir.y = inputDir.y * -1
        var inputInteract = Input.isInputJustReleased("Interact")
        var inventoryInteract = Input.isInputJustReleased("Inventory")

        // TODO this should be an event.
        var playerEntities = GameObject.getAllWithIdentifier("Player")
        if (inventoryInteract) {
            _interactLabel.setVisible(false)
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
            script.playerControl(inputDir)

            var interactEntity = script.getInteractEntity()

            if (interactEntity) {
                //System.print("Interact Entity")
                _interactLabel.setVisible(true)
                
                var interactWorldPosition = WorldTransform.getPosition(interactEntity)
                var screenInteractPosition = Camera.worldPosToScreenPos(interactWorldPosition) + Vec2.new(0, -32)
                _interactLabel.setPosition(screenInteractPosition, Vec2.new(0, 0))
                if (inputInteract) {
                    _interactLabel.setVisible(false)
                    script.playerForceStop()
                    // Change state to interact state
                    _stateManager.changeState("InteractState", {"interactingEntity": interactEntity})
                    if (_gameMusic) {
                        _gameMusic.stop()
                    }
                    return
                }
            } else {
                _interactLabel.setVisible(false)
            }

            // check player at bottom of screen
            var playerPosition = WorldTransform.getPosition(entity)
            if (playerPosition.y > 1755.0) {
                if (Inventory.contains("Red Pedestal Active") && Inventory.contains("Green Pedestal Active") && Inventory.contains("Yellow Pedestal Active") && Inventory.contains("Blue Pedestal Active")) {
                    script.playerForceStop()
                    _stateManager.changeState("gameOverState")
                    if (_gameMusic) {
                        _gameMusic.stop()
                    }
                    return
                } 

                // reset the current state
                stateManager.changeState("GameWorld")
                return
            }

            Sprite.setRenderPriority(entity, playerPosition.y)
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
        UIManager.removeUIElement(_loopCountLabel)
        _interactLabel = null
        _loopCountLabel = null
        _gameMusic.stop()
        _gameMusic.unload()
        _gameMusic = null

        System.print("Game world unloaded")
    }
    
    // Getters
    worldEntity { _worldEntity }
    gameplayStateManager { _stateManager }
}
