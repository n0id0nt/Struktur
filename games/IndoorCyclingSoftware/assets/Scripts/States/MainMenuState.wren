// states/MainMenuState.wren
// Main menu state. Wires the ride, Settings and Quit buttons to state
// transitions and drives the rich-text showcase's reveal timing. UI/MainMenu/MainMenuUI.wren is the
// pure view this owns and refreshes.
import "app" for Time
import "States/BaseState" for BaseState
import "ride" for Ride
import "UI/MainMenu/MainMenuUI" for MainMenuUI
import "debug" for Debug

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
        Debug.info("Entering Main Menu")

        _view = MainMenuUI.new()
        _pendingState = null
        _view.freeRideButton.setOnClick { |sender, mousePos|
            _pendingState = ["Ride", {"mode": "freeRide"}]
        }
        _view.workoutsButton.setOnClick { |sender, mousePos|
            _pendingState = ["WorkoutBrowser", {}]
        }
        _view.rideSettingsButton.setOnClick { |sender, mousePos|
            _pendingState = ["RideSettings", {}]
        }
        _view.settingsButton.setOnClick { |sender, mousePos| openSettings() }
        _view.quitButton.setOnClick { |sender, mousePos| quit() }

        _richTextGlyphCount = _view.richTextGlyphCount
        _richTextStartTime = Time.scaledTime
    }

    update(stateManager) {
        // Ride screens are opened from update(), not from inside the click (the UI's input pass).
        if (_pendingState != null) {
            var target = _pendingState
            _pendingState = null
            _stateManager.changeState(target[0], target[1])
            return
        }
        // A workout given on the command line opens straight in the workout browser (once).
        if (!_checkedCommandLine) {
            _checkedCommandLine = true
            var path = Ride.commandLineWorkout()
            if (path != null) {
                _stateManager.changeState("WorkoutBrowser", {"preselect": path})
                return
            }
        }
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
        Debug.info("Exiting Main Menu")
        if (_view != null) {
            _view.teardown()
            _view = null
        }
    }

    openSettings() {
        _stateManager.changeState("Settings")
    }

    quit() {
        Debug.info("Quit requested (not yet wired to the application)")
    }
}
