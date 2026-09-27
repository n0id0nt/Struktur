// states/MainMenuState.wren
// Main menu state - first state when game starts. Wires the Start/Settings/Quit buttons to state
// transitions and drives the rich-text showcase's reveal timing. UI/MainMenu/MainMenuUI.wren is the
// pure view this owns and refreshes.
import "app" for Time
import "States/BaseState" for BaseState
import "UI/MainMenu/MainMenuUI" for MainMenuUI

// Reveal speed for the rich-text showcase (seconds per glyph) - matches InteractState's own dialogue
// scroll pacing convention (Time.scaledTime driving a glyph count), just via
// MainMenuUI.revealGlyphs/UIRichLabel.setVisibleGlyphCount instead of substring truncation.
var RICH_TEXT_REVEAL_SPEED = 0.03

class MainMenuState is BaseState {
    construct new() {
        super()
        name = "MainMenuState"
        _stateManager = null
        _view = null
        _richTextStartTime = 0
        _richTextGlyphCount = 0
    }

    enter(stateManager, params) {
        super.enter(stateManager, params)
        _stateManager = stateManager
        System.print("Entering Main Menu")

        _view = MainMenuUI.new()
        _view.startButton.setOnClick { |sender, mousePos| startGame() }
        _view.settingsButton.setOnClick { |sender, mousePos| openSettings() }
        _view.quitButton.setOnClick { |sender, mousePos| quit() }

        _richTextGlyphCount = _view.richTextGlyphCount
        _richTextStartTime = Time.scaledTime
    }

    update(stateManager) {
        // Rich-text showcase reveal (see MainMenuUI.showcaseRichText_/RICH_TEXT_REVEAL_SPEED) - same
        // elapsed-time-driven glyph count InteractState's dialogue scroll uses, just driving
        // revealGlyphs instead of slicing a raw string.
        if (_view != null && _richTextGlyphCount > 0) {
            var revealed = ((Time.scaledTime - _richTextStartTime) / RICH_TEXT_REVEAL_SPEED).floor
            if (revealed > _richTextGlyphCount) {
                revealed = _richTextGlyphCount
            }
            _view.revealGlyphs(revealed)
        }
    }

    exit() {
        super.exit()
        System.print("Exiting Main Menu")
        if (_view != null) {
            _view.teardown()
            _view = null
        }
    }

    startGame() {
        System.print("Starting game...")
        _stateManager.changeState("GameWorld")
    }

    openSettings() {
        _stateManager.changeState("Settings")
    }

    quit() {
        System.print("Quit requested (not yet wired to the application)")
    }
}
