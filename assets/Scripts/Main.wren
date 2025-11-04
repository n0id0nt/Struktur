// Main game controller
// Owns the state manager and provides game-wide services
// Updated every frame by C++

import "States/StateManager" for StateManager
import "States/GameWorldState" for GameWorldState
import "States/MainMenuState" for MainMenuState

class Game {
    construct new() {
        System.print("Creating Game instance...")
        
        // Create state manager
        _stateManager = StateManager.new(this)
        
        System.print("Game instance created")

        // TODO Define the window, spash screen Text/Image and window name game icon ect
        
        // Initial states
        _stateManager.insertState("GameWorld", GameWorldState)
        _stateManager.insertState("MainMenu", MainMenuState)

        _stateManager.changeState("GameWorld")
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
    
    // Getters
    stateManager { _stateManager }
    
    quit() {
        System.print("Quit requested")
        // C++ binding could handle this
        // For now, just log
    }
}
