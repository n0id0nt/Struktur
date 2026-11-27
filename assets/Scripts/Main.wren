// Main game controller
// Owns the state manager and provides game-wide services
// Updated every frame by C++

import "app" for Application, Time
import "input" for Input
import "ui" for InputGlyphs
import "localization" for Localization
import "fileSystem" for FileSystem

import "States/StateManager" for StateManager
import "States/MainMenuState" for MainMenuState
import "States/DevicePairingState" for DevicePairingState
import "States/RiderSelectState" for RiderSelectState
import "States/RiderEditState" for RiderEditState
import "States/RideState" for RideState
import "States/WorkoutBrowserState" for WorkoutBrowserState
import "States/RideSettingsState" for RideSettingsState
import "Rider/RiderProfiles" for RiderProfiles
import "States/SettingsState" for SettingsState
import "debug" for Debug

class Game {
    // Called before the game window is created and will initialise the project settings.
    // Don't load or set any files that are in the write directory here
    construct new() {
        // TODO Define the window, spash screen Text/Image and window name game icon ect
        var windowWidth = 1280
        var windowHeight = 720
        Application.setWindowSize(windowWidth, windowHeight)
        Application.setApplicationName("Seven Gods")
        //Application.setIsFullScreen(true)
        // The 2D game on this branch's history (Seven Gods: levels, combat, dialogue) isn't loaded here - its
        // engine pipeline isn't compiled on this branch (docs/RideRefactorPlan.md).

        // Create state manager
        _stateManager = StateManager.new()
    }

    // Called after the all the systems are initialised
    start() {
        // Safety net: ensures a fresh run always starts at normal time scale, regardless of whatever
        // a debug restart (editor Stop/Play) might have left it at.
        Time.setTimeScale(1)

        FileSystem.seedFromDefaults("Settings/InputBindings/InputConfig.json", "InputConfig.json")
        Input.loadInputBindings("Settings/InputBindings/InputConfig.json")
        // Button art for input hints (UIInputHint / [input=] rich text) - see UIKit.hint.
        InputGlyphs.load("Settings/InputBindings/InputGlyphs.json")
        Localization.loadManifest("Localization/languages.json")
        RiderProfiles.load()

        // Initial states
        // First screen: pair a trainer / power meter / heart rate strap, then on to the main menu.
        _stateManager.insertState("DevicePairing", DevicePairingState)
        // Then "Who's Riding?": pick or create a rider profile (name, weight, height, units).
        _stateManager.insertState("RiderSelect", RiderSelectState)
        _stateManager.insertState("RiderEdit", RiderEditState)
        // Indoor-cycling screens, opened from the main menu.
        _stateManager.insertState("Ride", RideState)
        _stateManager.insertState("WorkoutBrowser", WorkoutBrowserState)
        _stateManager.insertState("RideSettings", RideSettingsState)
        _stateManager.insertState("MainMenu", MainMenuState)
        _stateManager.insertState("Settings", SettingsState)
        // Device pairing -> rider select -> main menu.
        // Set STRUKTUR_FITNESS_SIMULATE=1 to pair with simulated devices when no trainer is to hand.
        _stateManager.changeState("DevicePairing")
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
        Debug.info("Quit requested")
        // C++ binding could handle this
        // For now, just log
    }

    // Lets C++ reach the root StateManager (see WrenStateManager::GetActiveStateStackJson/
    // GetRegisteredStateNames/TriggerStateChange) for the state-debug editor window.
    stateManager { _stateManager }
}
