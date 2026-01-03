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
    }
    
    update(stateManager) {

    }
    
    exit() {
        super.exit()
        
        System.print("Exiting play mode")
    }
}
