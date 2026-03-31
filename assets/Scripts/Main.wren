// Main game controller
// Owns the state manager and provides game-wide services
// Updated every frame by C++

import "app" for Application
import "input" for Input

import "States/StateManager" for StateManager
import "States/GameWorldState" for GameWorldState
import "States/MainMenuState" for MainMenuState
import "Inventory" for Inventory
import "Dialogue/DialogueLoader" for DialogueLoader

class Game {
    // Called before the game window is created and will initial the project settings.
    construct new() {        
        
        // TODO Define the window, spash screen Text/Image and window name game icon ect
        var windowWidth = 1280
        var windowHeight = 720
        Application.setWindowSize(windowWidth, windowHeight)
        Application.setApplicationName("Memory Palace")

        Input.loadInputBindings("Settings/InputBindings/InputConfig.json")

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
        Inventory.init()
    }

    // Called after the all the systems are initialised 
    start() {
        DialogueLoader.loadAllDialogue()
        // Initial states
        _stateManager.insertState("GameWorld", GameWorldState)
        _stateManager.insertState("MainMenu", MainMenuState)

        _stateManager.changeState("GameWorld")
    }
    
    // Called by C++ every frame
    update() {      
        _stateManager.update()
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
