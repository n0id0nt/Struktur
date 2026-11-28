// states/PlayState.wren
// Main gameplay state - handles player movement, interaction checks
// This is the default state when playing the game

import "States/BaseState" for BaseState
import "debug" for Debug

class PlayState is BaseState {
    construct new() {
        super()
        name = "PlayState"
    }
    
    enter(stateManager, params) {
        super.enter(stateManager, params)
        
        Debug.info("Entering play mode")
    }
    
    update(stateManager) {

    }
    
    exit() {
        super.exit()
        
        Debug.info("Exiting play mode")
    }
}
