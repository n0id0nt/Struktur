// States/Combat/TargetingState.wren
// The player picked an offensive move with more than one enemy standing: left/right (UIDir) steps
// the target cursor between living enemies, UIAccept commits the move on the highlighted one,
// UICancel backs out to ChoosingState. The move being targeted is CombatSession.pendingMove.
import "input" for Input
import "States/Combat/CombatSubState" for CombatSubState

class TargetingState is CombatSubState {
    construct new() {
        super()
        name = "TargetingState"
        _axisHeld = false
    }

    begin(params) {
        var s = session
        _axisHeld = false
        s.arm()
        s.view.showMoveMenu(false)
        s.view.setMessage("Choose a target.  <  >")
        s.refreshViews()
    }

    update(stateManager) {
        var s = session
        if (!s.armed()) {
            return
        }
        var ax = Input.getInputAxis2("UIDir").x
        if (ax.abs < 0.3) {
            _axisHeld = false
        } else if (!_axisHeld) {
            _axisHeld = true
            s.targetIndex = s.stepTarget(s.targetIndex, ax > 0 ? 1 : -1)
            s.refreshViews()
        }
        if (Input.isInputJustReleased("UIAccept")) {
            s.commitPlayerMoveOn(s.pendingMove, s.enemies[s.targetIndex])
        } else if (Input.isInputJustReleased("UICancel")) {
            s.cancelPendingMove()
            s.goTo("ChoosingState")
        }
    }
}
