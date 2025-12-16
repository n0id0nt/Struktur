// states/GameWorldState.wren
// Main gameplay state - loads level, creates entities, manages gameplay sub-states
// This state owns the game world and delegates to sub-states for different gameplay modes

import "game" for Transform, Vec2, Vec3, Vec4, Inventory, World, Level, ResourceManager, GameObject, Script, Sprite, UIManager, Input, Camera

import "States/BaseState" for BaseState
import "States/StateManager" for StateManager

var TILE_TEXTURE = "assets/Tiles/cavesofgallet_tiles.png"
var PLAYER_TEXTURE = "assets/Tiles/PlayerGrowthSprites.png"
var WORLD_FILE_PATH = "assets/Levels/MemoryPalace.ldtk"
var WHITE = Vec4.new(255, 255, 255, 255)
var Loops = 0

class GameWorldState is BaseState {
    construct new() {
        super()
        _name = "GameWorldState"
        
        _interactLabel = null
        _loopCountLabel = null
        _worldEntity = null //TODO Create an entity constant for invalid entity or null entity
        _stateManager = StateManager.new()

        _stateManager.insertState("PlayState", BaseState)
        _stateManager.insertState("inventoryState", BaseState)
        _stateManager.insertState("interactState", BaseState)
        _stateManager.insertState("gameOverState", BaseState)
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
    
    enter() {
        super.enter()
        
        System.print("Loading game world...")

        Loops = Loops + 1 // increment the game loop count
        
        var font = ResourceManager.getFontResource("assets/Fonts/medieval_sharp/MedievalSharp-Bold.ttf_60")

        var worldEntity = Level.createWorldEntity(WORLD_FILE_PATH)
        _worldEntity = worldEntity

        var roomList = calculateRoomListToLoad(worldEntity)

        var northRoom = Level.loadLevelEntities(worldEntity, roomList[0])
        Transform.setPosition(northRoom, Vec3.new(576.0, 0.0, 0.0))
        var northRoomSpriteEntity = GameObject.create("northRoomSprite", northRoom)
        Transform.setLocalPosition(northRoomSpriteEntity, Vec3.new(0.0, 0.0, 0.0))
        var northRoomKey = getNorthRoom()
        var northRoomSpriteTexture = ResourceManager.getTextureResource("assets/Tiles/%(northRoomKey).png")
        Sprite.create(northRoomSpriteEntity, northRoomSpriteTexture, WHITE, Vec2.new(0, 0), 1, 1, false, 0, 0)
        
        var eastRoom = Level.loadLevelEntities(worldEntity, roomList[1])
        Transform.setPosition(eastRoom, Vec3.new(1152.0, 576.0, 0.0))
        var eastRoomSpriteEntity = GameObject.create("eastRoomSprite", eastRoom)
        Transform.setLocalPosition(eastRoomSpriteEntity, Vec3.new(0.0, 0.0, 0.0))
        var eastRoomKey = getEastRoom()
        var eastRoomSpriteTexture = ResourceManager.getTextureResource("assets/Tiles/%(eastRoomKey).png")
        Sprite.create(eastRoomSpriteEntity, eastRoomSpriteTexture, WHITE, Vec2.new(0, 0), 1, 1, false, 0, 0)
        
        var westRoom = Level.loadLevelEntities(worldEntity, roomList[2])
        Transform.setPosition(westRoom, Vec3.new(0.0, 576.0, 0.0))
        var westRoomSpriteEntity = GameObject.create("westRoomSprite", westRoom)
        Transform.setLocalPosition(westRoomSpriteEntity, Vec3.new(0.0, 0.0, 0.0))
        var westRoomKey = getWestRoom()
        var westRoomSpriteTexture = ResourceManager.getTextureResource("assets/Tiles/%(westRoomKey).png")
        Sprite.create(westRoomSpriteEntity, westRoomSpriteTexture, WHITE, Vec2.new(0, 0), 1, 1, false, 0, 0)
        
        var southRoom = Level.loadLevelEntities(worldEntity, roomList[3])
        Transform.setPosition(southRoom, Vec3.new(576.0, 1152.0, 0.0))
        var southRoomSpriteEntity = GameObject.create("southRoomSprite", southRoom)
        Transform.setLocalPosition(southRoomSpriteEntity, Vec3.new(0.0, 0.0, 0.0))
        var southRoomKey = getSouthRoom()
        var southRoomSpriteTexture = ResourceManager.getTextureResource("assets/Tiles/%(southRoomKey).png")
        Sprite.create(southRoomSpriteEntity, southRoomSpriteTexture, WHITE, Vec2.new(0, 0), 1, 1, false, 0, 0)
        
        var courtyard = Level.loadLevelEntities(worldEntity, roomList[4])
        Transform.setPosition(courtyard, Vec3.new(576.0, 576.0, 0.0))
        var courtyardSpriteEntity = GameObject.create("courtyardSprite", courtyard)
        Transform.setLocalPosition(courtyardSpriteEntity, Vec3.new(0.0, 0.0, 0.0))
        var courtyardSpriteTexture = ResourceManager.getTextureResource("assets/Tiles/Courtyard.png")
        Sprite.create(courtyardSpriteEntity, courtyardSpriteTexture, WHITE, Vec2.new(0, 0), 1, 1, false, 0, 0)
        
        var northRoomDupe = Level.loadLevelEntities(worldEntity, roomList[0])
        Transform.setPosition(northRoomDupe, Vec3.new(576.0, 1728.0, 0.0))
        var northRoomDupeSpriteEntity = GameObject.create("northRoomDupeSprite", northRoomDupe)
        Transform.setLocalPosition(northRoomDupeSpriteEntity, Vec3.new(0.0, 0.0, 0.0))
        Sprite.create(northRoomDupeSpriteEntity, northRoomSpriteTexture, WHITE, Vec2.new(0, 0), 1, 1, false, 0, 0)

        var playerEntity = GameObject.create("Player", worldEntity)
        Script.createArg(playerEntity, "Assets/Scripts/GameObjects/Player.wren", "Player", "Test Player Name")
        Transform.setPosition(playerEntity, Vec3.new(864.0, 32.0, 0.0))

        var lockedDoorEntity = GameObject.create("Entrance Door", worldEntity)
        Script.createArg(lockedDoorEntity, "Assets/Scripts/GameObjects/Door.wren", "Door", "Test Door Name")
        Transform.setPosition(lockedDoorEntity, Vec3.new(864.0, 0.0, 0.0))
        
        // Create the UI for the level.
        _interactLabel = UIManager.createUILabel(Vec2.new(0, 0), Vec2.new(0, 0), "Interact", 16.0)
        _interactLabel.setVisible(false)
        _interactLabel.setFont(font)
        _interactLabel.setTextColor(WHITE) // Change this when the background is created.
        _interactLabel.setAnchorPoint(Vec2.new(0.5, 0.5))

        var loops = Loops
        _loopCountLabel = UIManager.createUILabel(Vec2.new(20, 20), Vec2.new(0, 0), "Loops: %(loops)", 30.0)
        _loopCountLabel.setFont(font)
        _loopCountLabel.setTextColor(WHITE) // Change this when the background is created.
        _loopCountLabel.setVisible(true)

        _stateManager.changeState("PlayState")
        
        System.print("Game world loaded")
    }
    
    update(dt) {
        // if substate return out here
        if (_stateManager.currentState) {
            _stateManager.update(dt)
            return
        }

        var inputDir = Input.getInputAxis2("Move")
        var inputInteract = Input.isInputJustReleased("Interact")
        var inventoryInteract = Input.isInputJustReleased("Inventory")

        System.print("Updated")
        System.print("Input Dir: %(inputDir)")

        // TODO this should be an event.
        var playerEntities = GameObject.getAllWithIdentifier("Player")
        if (inventoryInteract) {
            _interactLabel.SetVisible(false)
            //TODO also pause the game time to pause the players animation
            // just forcing player to idle for now
            for (entity in playerEntities) {
                Script.call(entity, "playerForceStop")
            }
            _stateManager.changeState("inventoryState")
            return
        }

        for (entity in playerEntities) {
            Script.callArg(entity, "playerControl", [inputDir.x, inputDir.y])

            var interactEntity = Script.call(entity, "getInteractEntity")

            if (interactEntity) {
                _interactLabel.setVisible(true)
                
                var interactWorldPosition = Transform.getPosition(interactEntity)
                var screenInteractPosition = Camera.worldPosToScreenPos(interactWorldPosition) + Vec2.new(0, -32)
                _interactLabel.setPosition(screenInteractPosition, Vec2.new(0, 0))
                if (inputInteract) {
                    _interactLabel.setVisible(false)
                    Script.call(entity, "playerForceStop")
                    // Change state to interact state
                    _stateManager.ChangeState("interactState")
                    return
                }
            } else {
                _interactLabel.setVisible(false)
            }

            // check player at bottom of screen
            var playerPosition = Transform.getPosition(entity)
            if (playerPosition.y > 1755.0) {
                if (Inventory.contains("Red Pedestal Active") && Inventory.contains("Green Pedestal Active") && Inventory.contains("Yellow Pedestal Active") && Inventory.contains("Blue Pedestal Active")) {
                    Script.call(entity, "playerForceStop")
                    _stateManager.ChangeState("gameOverState")
                    return
                } 

                // reset the current state
                //_stateManager.ChangeState("gameWorldState")
                exit()
                enter()
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
        if (_stateManager != null && _stateManager.currentState != null) {
            _stateManager.currentState.exit()
        }
        
        GameObject.destroy(_worldEntity)
        
        UIManager.removeUILabel(_interactLabel)
        UIManager.removeUILabel(_loopCountLabel)

        System.print("Game world unloaded")
    }
    
    // Getters
    worldEntity { _worldEntity }
    gameplayStateManager { _stateManager }
}
