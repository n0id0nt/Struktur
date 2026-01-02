// states/InteractState.wren
// Main gameplay state - handles player movement, interaction checks
// This is the default state when playing the game

import "States/BaseState" for BaseState

class InteractState is BaseState {
    construct new() {
        super()
        _name = "InteractState"
    }
    
    enter() {
        super.enter()
        
        System.print("Entering play mode")
    }
    
    update() {

    }
    
    exit() {
        super.exit()
        
        System.print("Exiting play mode")
    }
}
