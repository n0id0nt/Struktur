// States/Combat/ChargingState.wren
// Game time runs at full speed: every committed move's bar fills on the shared Timeline (Combat/Timeline.wren),
// which is also its attack animation (each battler's clip is anchored to the bar - see
// CombatSession.animateBattlers), and stamina regenerates. When an attack's parry window opens, an attack aimed
// at the player hands over to ParryPromptState; the moment a bar is full the move lands via
// CombatResolver.resolveMove and the phases move on to ReturningState / MessageState. No input here - the player
// has already committed (or had their move cancelled and will be sent back to ChoosingState later).
import "States/Combat/CombatSubState" for CombatSubState

class ChargingState is CombatSubState {
    construct new() {
        super()
        name = "ChargingState"
    }

    begin(params) {
        var s = session
        s.resumeTime()
        s.view.showMoveMenu(false)
        s.clearFlashes()
        s.view.setMessage("")
        s.refreshViews()
    }

    update(stateManager) {
        var s = session
        s.advanceCharge()

        var attacker = s.nextParryOffer()
        if (attacker != null) {
            s.goTo("ParryPromptState", {"actor": attacker})
            return
        }

        var ready = s.timeline.nextReady()
        if (ready != null) {
            s.resolver.resolveMove(ready)
        }
    }
}
