// States/Combat/EnteringState.wren
// The arena entrance: the camera eases down onto the formation while every battler slides in from
// its wing (Combat/BattleStage.intro), with the combat UI hidden. Only used when a BattleStage
// exists - a fight in place (no arena) starts in IntroState instead. Hands off to ChoosingState.
import "app" for Time
import "States/Combat/CombatSubState" for CombatSubState

var ENTER_TIME = 0.65   // arena sweep + battler slide-in

class EnteringState is CombatSubState {
    construct new() {
        super()
        name = "EnteringState"
        _startTime = 0
    }

    begin(params) {
        session.view.setVisible(false)
        _startTime = Time.unscaledTime
    }

    update(stateManager) {
        var s = session
        var elapsed = Time.unscaledTime - _startTime
        s.stage.intro(elapsed / ENTER_TIME)
        if (elapsed >= ENTER_TIME) {
            s.stage.settle()
            s.view.setVisible(true)
            s.goTo("ChoosingState")
        }
    }
}
