// states/GameWorldState.wren
// Main gameplay state - loads level, creates entities, manages gameplay sub-states
// This state owns the game world and delegates to sub-states for different gameplay modes

import "States/BaseState" for BaseState
import "States/StateManager" for StateManager

var TILE_TEXTURE = "assets/Tiles/cavesofgallet_tiles.png"
var PLAYER_TEXTURE = "assets/Tiles/PlayerGrowthSprites.png"
var WORLD_FILE_PATH = "assets/Levels/MemoryPalace.ldtk"
var WHITE = vec4.new(0,0,0,255)

class GameWorldState is BaseState {
    construct new() {
        super()
        _name = "GameWorldState"
        
        _interactLabel = null
        _loopCountLabel = null
        _worldEntity = null //TODO Create an entity constant for invalid entity or null entity
        _stateManager = StateManager.new()
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

        GameData.Loops = GameData.Loops + 1 // increment the game loop count
        
        var font = RebsourceManager.GetFontResource("assets/Fonts/medieval_sharp/MedievalSharp-Bold.ttf_60")

        var worldEntity = Level.createWorldEntity(WORLD_FILE_PATH)
        _worldEntity = worldEntity

        var roomList = calculateRoomListToLoad(worldEntity)

        var northRoom = Level.loadLevelEntities(worldEntity, roomList[0])
        Transform.setPosition(northRoom, vec3.new(576.0, 0.0, 0.0))
        var northRoomSpriteEntity = GameObjectManger.create("northRoomSprite", northRoom)
        Transform.setLocalPosition(northRoomSpriteEntity, vec3.new(0.0, 0.0, 0.0))
        var northRoomKey = getNorthRoom()
        var northRoomSpriteTexture = ResourceManager.GetTexture("assets/Tiles/%(northRoomKey).png")
        SpriteComponent.create(northRoomSpriteEntity, northRoomSpriteTexture, WHITE, vec2.new(0, 0), 1, 1, false, 0, 0)
        
        var eastRoom = Level.loadLevelEntities(worldEntity, roomList[1])
        Transform.setPosition(eastRoom, vec3.new(1152.0, 576.0, 0.0))
        var eastRoomSpriteEntity = GameObjectManger.create("eastRoomSprite", eastRoom)
        Transform.setLocalPosition(eastRoomSpriteEntity, vec3.new(0.0, 0.0, 0.0))
        var eastRoomKey = getEastRoom()
        var eastRoomSpriteTexture = ResourceManager.GetTexture("assets/Tiles/%(eastRoomKey).png")
        SpriteComponent.create(eastRoomSpriteEntity, eastRoomSpriteTexture, WHITE, vec2.new(0, 0), 1, 1, false, 0, 0)
        
        var westRoom = Level.loadLevelEntities(worldEntity, roomList[2])
        Transform.setPosition(westRoom, vec3.new(0.0, 576.0, 0.0))
        var westRoomSpriteEntity = GameObjectManger.create("westRoomSprite", westRoom)
        Transform.setLocalPosition(westRoomSpriteEntity, vec3.new(0.0, 0.0, 0.0))
        var westRoomKey = getWestRoom()
        var westRoomSpriteTexture = ResourceManager.GetTexture("assets/Tiles/%(westRoomKey).png")
        SpriteComponent.create(westRoomSpriteEntity, westRoomSpriteTexture, WHITE, vec2.new(0, 0), 1, 1, false, 0, 0)
        
        var southRoom = Level.loadLevelEntities(worldEntity, roomList[3])
        Transform.setPosition(southRoom, vec3.new(576.0, 1152.0, 0.0))
        var southRoomSpriteEntity = GameObjectManger.create("southRoomSprite", southRoom)
        Transform.setLocalPosition(southRoomSpriteEntity, vec3.new(0.0, 0.0, 0.0))
        var southRoomKey = getSouthRoom()
        var southRoomSpriteTexture = ResourceManager.GetTexture("assets/Tiles/%(southRoomKey).png")
        SpriteComponent.create(southRoomSpriteEntity, southRoomSpriteTexture, WHITE, vec2.new(0, 0), 1, 1, false, 0, 0)
        
        var courtyard = Level.loadLevelEntities(worldEntity, roomList[4])
        Transform.setPosition(courtyard, vec3.new(576.0, 576.0, 0.0))
        var courtyardSpriteEntity = GameObjectManger.create("courtyardSprite", courtyard)
        Transform.setLocalPosition(courtyardSpriteEntity, vec3.new(0.0, 0.0, 0.0))
        var courtyardSpriteTexture = ResourceManager.GetTexture("assets/Tiles/Courtyard.png")
        SpriteComponent.create(courtyardSpriteEntity, courtyardSpriteTexture, WHITE, vec2.new(0, 0), 1, 1, false, 0, 0)
        
        var northRoomDupe = Level.loadLevelEntities(worldEntity, roomList[0])
        Transform.setPosition(northRoomDupe, vec3.new(576.0, 1728.0, 0.0))
        var northRoomDupeSpriteEntity = GameObjectManger.create("northRoomDupeSprite", northRoomDupe)
        Transform.setLocalPosition(northRoomDupeSpriteEntity, vec3.new(0.0, 0.0, 0.0))
        SpriteComponent.create(northRoomDupeSpriteEntity, northRoomSpriteTexture, WHITE, vec2.new(0, 0), 1, 1, false, 0, 0)

        var playerEntity = GameObjectManger.createWithScript("Player", worldEntity, "GameObjects/Player")
        Transform.setPosition(playerEntity, vec3.new(864.0, 32.0, 0.0))

        var lockedDoorEntity = GameObjectManger.createWithScript("Entrance Door", worldEntity, "GameObjects/Door")
        Transform.setPosition(lockedDoorEntity, vec3.new(864.0, 0.0, 0.0))
        
        // Create the UI for the level.
        _interactLabel = UIManager.CreateUILabel(context, vec2.new(0, 0), vec2.new(0, 0), "Interact", 16.0)
        _interactLabel.SetVisible(false)
        _interactLabel.SetFont(font)
        _interactLabel.SetTextColor(WHITE) // Change this when the background is created.
        _interactLabel.SetAnchorPoint(vec2.new(0.5, 0.5))

        var loops = GameData.Loops
        _loopCountLabel = UIManager.CreateUILabel(context, vec2.new(20, 20), vec2.new(0, 0), "Loops: %(loops)", 30.0)
        _loopCountLabel.SetFont(font)
        _loopCountLabel.SetTextColor(WHITE) // Change this when the background is created.
        _loopCountLabel.SetVisible(true)

        _stateManager.changeState("PlayState")
        
        System.print("Game world loaded")
    }
    
    update(dt) {
        // if substate return out here
        if (_stateManager.currentState) {
            _stateManager.update()
            return
        }

        var transformSystem = SystemManager.GetTransformSystem()
        var animationSystem = SystemManager.GetAnimationSystem()

        var inputDir = Input.GetInputAxis2("Move")
        var inputInteract = Input.IsInputJustReleased("Interact")
        var inventoryInteract = Input.IsInputJustReleased("Inventory")

        var view = registry.view<Component::Player>()
        if (inventoryInteract) {
            _interactLabel.SetVisible(false)
            //TODO also pause the game time to pause the players animation
            // just forcing player to idle for now
            for (auto& entity : view)
            {
                Player.PlayerForceStop(entity)
            }
            var inventoryState = std::make_unique<InventoryState>()
            _stateManager.ChangeState(inventoryState)
            return
        }

        for (auto& entity : view) {
            Player.PlayerControl(entity, inputDir)

            var canInteract = Player.CanInteract(entity)

            if (canInteract != null)
            {
                auto& interactWorldTransform = Registry.GetWorldTransform(canInteract)
                m_interactLabel->SetVisible(true)
                
                glm::vec2 screenInteractPosition = camera.WorldPosToScreenPos(interactWorldTransform.position) + vec2.new(0, -32)
                m_interactLabel->SetPosition(screenInteractPosition, vec2.new(0, 0))
                if (inputInteract)
                {
                    m_interactLabel->SetVisible(false)
                    Struktur::Player::PlayerForceStop(context, entity)
                    // Change state to interact state
                    std::unique_ptr<InteractState> interactState = std::make_unique<InteractState>(canInteract)
                    m_stateManager.ChangeState(context, std::move(interactState))
                    return
                }
            } else {
                m_interactLabel->SetVisible(false)
            }

            // check player at bottom of screen
            auto& playerPosition = registry.get<Component::WorldTransform>(entity).position
            if (playerPosition.y > 1755.0f) {
                Inventory& inventory = context.GetInventory()
                if ((std::find(inventory.begin(), inventory.end(), "Red Pedestal Active") != inventory.end())
                    && (std::find(inventory.begin(), inventory.end(), "Green Pedestal Active") != inventory.end())
                    && (std::find(inventory.begin(), inventory.end(), "Yellow Pedestal Active") != inventory.end())
                    && (std::find(inventory.begin(), inventory.end(), "Blue Pedestal Active") != inventory.end())) {
                    Struktur::Player::PlayerForceStop(context, entity)
                    std::unique_ptr<GameOverState> gameOverState = std::make_unique<GameOverState>()
                    m_stateManager.ChangeState(context, std::move(gameOverState))
                    return
                } 

                // reset the current state
                std::unique_ptr<GamePlay::GameWorldState> gameWorldState = std::make_unique<GamePlay::GameWorldState>()
                stateManager.ChangeState(context, std::move(gameWorldState))
                return
            }

            auto& playerSprite = registry.get<Component::Sprite>(entity)
            playerSprite.renderPriority = (int)playerPosition.y
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
        
        // TODO: Destroy all entities
        // if (_worldEntity != null) {
        //     Entity.Destroy(_worldEntity)
        // }
        
        // TODO: Unload level
        
        System.print("Game world unloaded")
    }
    
    // Getters
    worldEntity { _worldEntity }
    gameplayStateManager { _stateManager }
}
