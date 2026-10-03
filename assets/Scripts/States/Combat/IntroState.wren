// States/Combat/IntroState.wren
// The fight-in-place fallback beat (no BattleStage, so no arena entrance to play) - just a short
// pause on the "you're set upon" message before the move menu arms. Hands off to ChoosingState.
import "app" for Time
import "States/Combat/CombatSubState" for CombatSubState

var INTRO_TIME = 0.9   // fallback (fight-in-place) beat before the menu arms

class IntroState is CombatSubState {
    construct new() {
        super()
        name = "IntroState"
        _startTime = 0
    }

    begin(params) {
        _startTime = Time.unscaledTime
    }

    update(stateManager) {
        if (Time.unscaledTime - _startTime >= INTRO_TIME) {
            session.goTo("ChoosingState")
        }
    }
}
