// states/PlayState.wren
// Main gameplay state - handles player movement, interaction checks
// This is the default state when playing the game

import "States/BaseState" for BaseState

class PlayState is BaseState {
    construct new() {
        super()
        _name = "PlayState"
    }
    
    enter() {
        super.enter()
        
        System.print("Entering play mode")
        
        // Enable gameplay systems
        // Physics.SetEnabled(true)
        // AI.SetEnabled(true)
    }
    
    update(dt) {
        // TODO: Get input
        // var moveDir = Input.GetAxis2("Move")
        // var interactPressed = Input.JustPressed("Interact")
        // var inventoryPressed = Input.JustPressed("Inventory")
        
        // TODO: Handle inventory toggle
        // if (inventoryPressed) {
        //     System.print("Opening inventory")
        //     this.pushState("InventoryState")
        //     return
        // }
        
        // TODO: Find player entity
        // var player = Entity.FindByTag("Player")
        // if (player == null) return
        
        // TODO: Move player
        // Player.Move(player, moveDir)
        
        // TODO: Check for interaction
        // var interactable = Player.GetNearbyInteractable(player)
        // if (interactable != null) {
        //     // Show interact prompt
        //     if (interactPressed) {
        //         System.print("Interacting with entity")
        //         this.pushState("InteractState") // Pass interactable entity
        //         return
        //     }
        // }
        
        // TODO: Check win/lose conditions
        // if (Player.IsDead(player)) {
        //     this.changeState("GameOverState")
        // }
    }
    
    exit() {
        super.exit()
        
        System.print("Exiting play mode")
    }
    
    resume() {
        super.resume()
        
        System.print("Resuming play mode")
        // Called when returning from InventoryState or InteractState
    }
}
