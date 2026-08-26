// Main game controller
// Owns the state manager and provides game-wide services
// Updated every frame by C++

import "app" for Application
import "input" for Input
import "localization" for Localization
import "flags" for FlagManager
import "fileSystem" for FileSystem

import "States/StateManager" for StateManager
import "States/GameWorldState" for GameWorldState
import "States/MainMenuState" for MainMenuState
import "States/SettingsState" for SettingsState
import "Inventory" for Inventory
import "Dialogue/DialogueLoader" for DialogueLoader

class Game {
    // Called before the game window is created and will initialise the project settings.
    // Don't load or set any files that are in the write directory here
    construct new() {
        // TODO Define the window, spash screen Text/Image and window name game icon ect
        var windowWidth = 1280
        var windowHeight = 720
        Application.setWindowSize(windowWidth, windowHeight)
        Application.setApplicationName("Memory Palace")
        //Application.setIsFullScreen(true)

        var gameObjectsScripts = [
            "Door",
            "Item",
            "NPC",
            "Player",
            "Room",
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
        FileSystem.seedFromDefaults("Settings/InputBindings/InputConfig.json", "InputConfig.json")
        Input.loadInputBindings("Settings/InputBindings/InputConfig.json")
        Localization.loadManifest("Localization/languages.json")
        FlagManager.load("flags.sav")
        Inventory.load("inventory.sav")

        DialogueLoader.loadAllDialogue()
        // Initial states
        _stateManager.insertState("GameWorld", GameWorldState)
        _stateManager.insertState("MainMenu", MainMenuState)
        _stateManager.insertState("Settings", SettingsState)

        _stateManager.changeState("MainMenu")
    }
    
    // Called by C++ every frame
    update() {
        _stateManager.update()
    }

    // Called by C++ at a fixed cadence (GameData::timeStep, see GameLoop's accumulator loop in Game.cpp),
    // independent of render framerate - use this instead of update() for gameplay logic that should stay in
    // step with physics.
    fixedUpdate() {
        _stateManager.fixedUpdate()
    }

    // Called by C++ for rendering
    render() {
        _stateManager.render()
    }

    // called after the update loop for each event that frame
    onEvent(type, data) {
        _stateManager.onEvent(type, data)
    }
    
    quit() {
        System.print("Quit requested")
        // C++ binding could handle this
        // For now, just log
    }

    // Lets C++ reach the root StateManager (see WrenStateManager::GetActiveStateStackJson/
    // GetRegisteredStateNames/TriggerStateChange) for the state-debug editor window.
    stateManager { _stateManager }
}
