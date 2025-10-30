// states/GameWorldState.wren
// Main gameplay state - loads level, creates entities, manages gameplay sub-states
// This state owns the game world and delegates to sub-states for different gameplay modes

import "core/BaseState" for BaseState
import "core/StateManager" for StateManager

class GameWorldState is BaseState {
    construct new(game) {
        super(game)
        _name = "GameWorldState"
        
        // Sub-state manager for gameplay states (Play, Inventory, Interact, etc.)
        _gameplayStateManager = null
        
        // World entity (root of level hierarchy)
        _worldEntity = null
    }
    
    enter() {
        super.enter()
        
        System.print("Loading game world...")
        
        // TODO: Load LDTK level
        // _worldEntity = Level.Load(game, "MemoryPalace")
        
        // TODO: Create player
        // var playerEntity = Entity.Create("Player", _worldEntity)
        // Transform.SetPosition(playerEntity, 864, 32, 0)
        
        // TODO: Load level entities from LDTK
        // var rooms = this.calculateRoomList()
        // for (roomIndex in rooms) {
        //     Level.LoadLevelEntities(game, _worldEntity, roomIndex)
        // }
        
        // Create sub-state manager for gameplay states
        _gameplayStateManager = StateManager.new(game)
        
        // Start in PlayState
        _gameplayStateManager.changeState("PlayState")
        
        System.print("Game world loaded")
    }
    
    update(dt) {
        // Delegate to gameplay sub-state
        if (_gameplayStateManager != null) {
            _gameplayStateManager.update(dt)
        }
    }
    
    render() {
        // Delegate to gameplay sub-state
        if (_gameplayStateManager != null) {
            _gameplayStateManager.render()
        }
    }
    
    exit() {
        super.exit()
        
        System.print("Unloading game world...")
        
        // Exit sub-state manager
        if (_gameplayStateManager != null && _gameplayStateManager.currentState != null) {
            _gameplayStateManager.currentState.exit()
        }
        
        // TODO: Destroy all entities
        // if (_worldEntity != null) {
        //     Entity.Destroy(_worldEntity)
        // }
        
        // TODO: Unload level
        
        System.print("Game world unloaded")
    }
    
    // Helper methods for level loading (based on your GameWorldState.h)
    calculateRoomList() {
        // TODO: Implement room calculation based on inventory
        // var inventory = Context.GetInventory()
        // Check for items, return list of room indices
        return [0, 1, 2, 3, 4] // Placeholder
    }
    
    // Getters
    worldEntity { _worldEntity }
    gameplayStateManager { _gameplayStateManager }
}
