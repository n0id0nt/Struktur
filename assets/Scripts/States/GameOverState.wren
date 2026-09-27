// states/GameOverState.wren
// The game-over screen. Trivial by nature - no clicks, no computed text, no per-frame behaviour -
// it just owns the UI/GameOver/GameOverUI view's lifecycle.
import "States/BaseState" for BaseState
import "UI/GameOver/GameOverUI" for GameOverUI

class GameOverState is BaseState {
    construct new() {
        super()
        name = "GameOverState"
        _view = null
    }

    enter(stateManager, params) {
        super.enter(stateManager, params)
        _view = GameOverUI.new()
    }

    update(stateManager) {
    }

    exit() {
        super.exit()
        if (_view != null) {
            _view.teardown()
            _view = null
        }
    }
}
