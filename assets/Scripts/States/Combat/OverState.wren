// States/Combat/OverState.wren
// The fight is decided (won, lost or fled): the closing line sits on screen with the menu hidden,
// then the whole CombatState is cleared from its parent manager, which tears the fight down.
// Entered with params text.
import "app" for Time
import "States/Combat/CombatSubState" for CombatSubState

var OVER_TIME = 1.7

class OverState is CombatSubState {
    construct new() {
        super()
        name = "OverState"
        _startTime = 0
    }

    begin(params) {
        var s = session
        s.view.setMessage(params["text"])
        s.view.showMoveMenu(false)
        _startTime = Time.unscaledTime
    }

    update(stateManager) {
        if (Time.unscaledTime - _startTime >= OVER_TIME) {
            session.outer.clearCurrentState()
        }
    }
}
