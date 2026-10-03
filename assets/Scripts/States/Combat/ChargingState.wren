// States/Combat/ChargingState.wren
// Every committed move's bar fills on the shared Timeline (Combat/Timeline.wren) and stamina
// regenerates; the moment a bar is full the move is handed to CombatResolver.resolveMove, which
// moves on to ParryPromptState, AttackingState or MessageState. No input here - the player has
// already committed (or had their move cancelled and will be sent back to ChoosingState later).
import "app" for Time
import "States/Combat/CombatSubState" for CombatSubState

class ChargingState is CombatSubState {
    construct new() {
        super()
        name = "ChargingState"
    }

    begin(params) {
        var s = session
        s.view.showMoveMenu(false)
        s.clearFlashes()
        s.view.setMessage("")
        s.refreshViews()
    }

    update(stateManager) {
        var s = session
        var units = s.timeline.tick(Time.unscaledDelta)
        s.regenStamina(units)
        s.refreshViews()
        var ready = s.timeline.nextReady()
        if (ready != null) {
            s.resolver.resolveMove(ready)
        }
    }
}
