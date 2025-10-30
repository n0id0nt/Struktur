// Main game controller
// Owns the state manager and provides game-wide services
// Updated every frame by C++

import "States/StateManager" for StateManager

class Game {
    construct new() {
        System.print("Creating Game instance...")
        
        // Create state manager
        _stateManager = StateManager.new(this)
        
        // Game-wide data
        _isPaused = false
        _debugMode = false
        _time = 0
        
        System.print("Game instance created")
        
        // Load initial state
        this.changeState("GameState")
    }
    
    // Called by C++ every frame
    update(dt) {
        // Don't update if paused
        if (_isPaused) {
            return
        }
        
        // Update time
        _time = _time + dt
        
        // Update state manager (delegates to active state)
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
    isPaused { _isPaused }
    debugMode { _debugMode }
    time { _time }
    
    // Setters
    isPaused=(value) { _isPaused = value }
    setDebugMode(value) { _debugMode = value }
    
    // Game-wide utility methods
    pause() {
        System.print("Game paused")
        _isPaused = true
    }
    
    unpause() {
        System.print("Game unpaused")
        _isPaused = false
    }
    
    quit() {
        System.print("Quit requested")
        // C++ binding could handle this
        // For now, just log
    }
}
