// states/MainMenuState.wren
// Main menu state - first state when game starts
// Handles menu navigation and transitions to game

import "core/BaseState" for BaseState

class MainMenuState is BaseState {
    construct new(game) {
        super(game)
        _name = "MainMenuState"
    }
    
    enter() {
        super.enter()
        
        System.print("Main Menu - TODO: Create UI")
        // TODO: Create menu UI
        // UI.CreatePanel(...)
        // UI.CreateButton("Start Game", ...)
        // UI.CreateButton("Settings", ...)
        // UI.CreateButton("Quit", ...)
    }
    
    update(dt) {
        // TODO: Handle input
        // if (Input.justPressed("Confirm")) {
        //     this.startGame()
        // }
        
        // Temporary: Auto-start game for testing
        // Remove this once you have proper menu
        // this.changeState("GameWorldState")
    }
    
    exit() {
        super.exit()
        
        System.print("Main Menu - TODO: Destroy UI")
        // TODO: Destroy menu UI
    }
    
    startGame() {
        System.print("Starting game...")
        this.changeState("GameWorldState")
    }
    
    openSettings() {
        System.print("Opening settings...")
        this.pushState("SettingsState")
    }
    
    quit() {
        System.print("Quitting game...")
        game.quit()
    }
}
