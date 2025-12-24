// Main game controller
// Owns the state manager and provides game-wide services
// Updated every frame by C++

import "app" for Application

import "States/StateManager" for StateManager
import "States/GameWorldState" for GameWorldState
import "States/MainMenuState" for MainMenuState

class Game {
    // Called before the game window is created and will initial the project settings.
    construct new() {        
        
        // TODO Define the window, spash screen Text/Image and window name game icon ect
        var windowWidth = 1280
        var windowHeight = 720
        Application.setWindowSize(windowWidth, windowHeight)
        Application.setApplicationName("Memory Palace")

        var gameObjectsScripts = [
            "Door",
            "Item",
            "NPC",
            "Player",
        ]

        for (gameObjectsScript in gameObjectsScripts) {
            Application.registerComponentScript("GameObjects/%(gameObjectsScript)", gameObjectsScript)
        }

        // Create state manager
        _stateManager = StateManager.new()
    }

    // Called after the all the systems are initialised 
    start() {
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
    
    quit() {
        System.print("Quit requested")
        // C++ binding could handle this
        // For now, just log
    }
}
