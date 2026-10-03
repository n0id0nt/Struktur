// States/Combat/OverState.wren
// The fight is decided (won, lost or fled): the closing line sits on screen with the menu hidden, then the whole
// CombatState is cleared from its parent manager, which tears the fight down (and restores normal game time).
// Entered with params text.
import "app" for Time
import "States/Combat/CombatSubState" for CombatSubState

var OVER_TIME = 1.7   // game seconds

class OverState is CombatSubState {
    construct new() {
        super()
        name = "OverState"
        _startTime = 0
    }

    begin(params) {
        var s = session
        s.resumeTime()
        s.view.setMessage(params["text"])
        s.view.showMoveMenu(false)
        _startTime = Time.scaledTime
    }

    update(stateManager) {
        if (Time.scaledTime - _startTime >= OVER_TIME) {
            session.outer.clearCurrentState()
        }
    }
}
