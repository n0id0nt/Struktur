// States/Combat/EnteringState.wren
// The arena entrance: the camera eases down onto the formation while every battler runs in place and slides in
// from its wing (Combat/BattleStage.intro / playEntrance), with the combat UI hidden. Only used when a
// BattleStage exists - a fight in place (no arena) starts in IntroState instead. Hands off to ChoosingState.
import "app" for Time
import "States/Combat/CombatSubState" for CombatSubState

var ENTER_TIME = 0.65   // arena sweep + battler slide-in (game seconds)

class EnteringState is CombatSubState {
    construct new() {
        super()
        name = "EnteringState"
        _startTime = 0
    }

    begin(params) {
        var s = session
        s.resumeTime()
        s.view.setVisible(false)
        s.stage.playEntrance()
        _startTime = Time.scaledTime
    }

    update(stateManager) {
        var s = session
        var elapsed = Time.scaledTime - _startTime
        s.stage.intro(elapsed / ENTER_TIME)
        if (elapsed >= ENTER_TIME) {
            s.stage.settle()
            s.view.setVisible(true)
            s.goTo("ChoosingState")
        }
    }
}
