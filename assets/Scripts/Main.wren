// Main game controller
// Owns the state manager and provides game-wide services
// Updated every frame by C++

import "States/StateManager" for StateManager

class Game {
    construct new() {
        System.print("Creating Game instance...")
        
        // Create state manager
        _stateManager = StateManager.new(this)
        
        System.print("Game instance created")
        
        // Load initial state
        this.changeState("GameState")
    }
    
    // Called by C++ every frame
    update(dt) {      
        _stateManager.update(dt)
    }
    
    // Called by C++ for rendering
    render() {
        _stateManager.render()
    }
    
    // State management convenience methods
    changeState(stateName) {
        _stateManager.changeState(stateName)
    }
    
    pushState(stateName) {
        _stateManager.pushState(stateName)
    }
    
    popState() {
        _stateManager.popState()
    }
    
    // Getters
    stateManager { _stateManager }
    time { _time }
    
    quit() {
        System.print("Quit requested")
        // C++ binding could handle this
        // For now, just log
    }
}
