// states/GameOverState.wren
// The game-over screen, two ways:
//   - no params: the finished-game screen ("Game Complete" - States/GameWorldState.wren), no button.
//   - params title / continueText / onContinue (a Fn): the "You fell" screen after the party is wiped
//     (States/ExperimentState.gameOver_). Game time is paused while it's up; the Continue button runs
//     onContinue (the respawn) and closes the screen.
// It owns the UI/GameOver/GameOverUI view's lifecycle.
import "app" for Time
import "States/BaseState" for BaseState
import "UI/GameOver/GameOverUI" for GameOverUI

// Real seconds the Continue button ignores clicks for after the screen opens, so the key that finished the
// fight can't skip straight past it.
var ARM_TIME = 0.4

class GameOverState is BaseState {
    construct new() {
        super()
        name = "GameOverState"
        _view = null
        _stateManager = null
        _onContinue = null
        _armedAt = 0
        _paused = false
    }

    enter(stateManager, params) {
        super.enter(stateManager, params)
        _stateManager = stateManager
        _onContinue = params["onContinue"]
        if (_onContinue == null) {
            _view = GameOverUI.new()
            return
        }
        _view = GameOverUI.new(params["title"], params["continueText"])
        _armedAt = Time.unscaledTime + ARM_TIME
        Time.setTimeScale(0)
        _paused = true
        _view.button.setOnClick { |sender, mousePos| continue_() }
    }

    continue_() {
        if (_onContinue == null || Time.unscaledTime < _armedAt) {
            return
        }
        var onContinue = _onContinue
        _onContinue = null
        Time.setTimeScale(1)
        _paused = false
        onContinue.call()
        _stateManager.clearCurrentState()
    }

    update(stateManager) {
    }

    exit() {
        super.exit()
        if (_paused) {
            Time.setTimeScale(1)
            _paused = false
        }
        if (_view != null) {
            _view.teardown()
            _view = null
        }
        _onContinue = null
        _stateManager = null
    }
}
