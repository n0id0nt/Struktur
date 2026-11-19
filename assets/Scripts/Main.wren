// Main game controller
// Owns the state manager and provides game-wide services
// Updated every frame by C++

import "app" for Application, Time
import "input" for Input
import "ui" for InputGlyphs
import "localization" for Localization
import "flags" for FlagManager
import "fileSystem" for FileSystem
import "physics" for CollisionLayers

import "States/StateManager" for StateManager
import "States/GameWorldState" for GameWorldState
import "States/MainMenuState" for MainMenuState
import "States/SettingsState" for SettingsState
import "States/BattleTestState" for BattleTestState
import "Inventory/Inventory" for Inventory
import "Party/Party" for Party
import "Combat/MoveBook" for MoveBook
import "Inventory/Equipment" for Equipment
import "Combat/Config/BattlePlayer" for BattlePlayer
import "Combat/Defeated" for Defeated
import "World/MapLog" for MapLog
import "Dialogue/DialogueLoader" for DialogueLoader
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
        Application.setPixelsPerMeter(32)
        var gameObjectsScripts = [
            "Chicken",
            "Chinlin",
            "Door",
            "DoorWorp",
            "Item",
            "NPC",
            "PartyMember",
            "PressurePlate",
            "PressurePad",
            "Sign",
            "SoulFragment",
            "Player",
            "Room",
            "Interactable",
            "RestSpot",
            "Spawn",
            "Marker",
            "Path",
            "WayPoint",
            "Trigger",
        ]

        for (gameObjectsScript in gameObjectsScripts) {
            Application.registerComponentScript("GameObjects/%(gameObjectsScript)", gameObjectsScript)
        }
        // The LDtk "Critter" entity is a spawner for the creature its Enemy field names - its own folder, since
        // GameObjects/Critter.wren is the creatures' shared base class (see GameObjects/Level/Critter.wren).
        Application.registerComponentScript("GameObjects/Level/Critter", "Critter")

        // Create state manager
        _stateManager = StateManager.new()
        Inventory.init()
        MoveBook.init(BattlePlayer.starterKit)
        Equipment.init()
        Party.init()
        Defeated.init()
        MapLog.init()
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
        FlagManager.load("flags.sav")
        Inventory.load("inventory.sav")
        // Before the moves: what the weapon allows decides how an old save's empty loadout slots get filled.
        Equipment.load()
        MoveBook.load("moves.sav")
        Party.load("party.sav")
        Defeated.load("defeated.sav")
        MapLog.load("map.sav")

        DialogueLoader.loadAllDialogue()

        // Collision layers. "Wall" is registered first so it takes bit 0x0001, which is the category
        // the tilemap collision bodies are already built with (TileMapCollisionBodyGenerator uses a
        // default b2Filter). "Actor" is the player + critters: they set their category to Actor and
        // their mask to Wall only, so they still hit level geometry (and doors/NPCs, which share the
        // wall bit) but pass straight through each other. Registration is idempotent, so re-running
        // start() on a debug restart is harmless.
        CollisionLayers.registerLayer("Wall")
        CollisionLayers.registerLayer("Actor")
        // Sensors (pressure plates) sit on Trigger and look for Actor; only the player and party members add
        // Trigger to their own mask, so critters never press them.
        CollisionLayers.registerLayer("Trigger")

        // Initial states
        _stateManager.insertState("GameWorld", GameWorldState)
        _stateManager.insertState("MainMenu", MainMenuState)
        _stateManager.insertState("Settings", SettingsState)
        // The battle sandbox - F6 from the overworld (GameWorldState) opens it.
        _stateManager.insertState("BattleTest", BattleTestState)

        // The gameplay state (Arc 1 - Levels/SevenGods.ldtk). MainMenu's Start also leads here.
        _stateManager.changeState("GameWorld")
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
