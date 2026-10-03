// States/Combat/ChoosingState.wren
// The player's move menu is up and nothing is charging: the player picks a move (the menu buttons,
// wired once per fight in States/CombatState.wren, call CombatSession.commitPlayerMove) or flees
// with UICancel. Entering this phase also lets every enemy that needs one pick its next move.
// Leads to TargetingState (offensive move against a group) or ChargingState; fleeing ends the
// fight via CombatResolver.flee.
import "input" for Input
import "States/Combat/CombatSubState" for CombatSubState

class ChoosingState is CombatSubState {
    construct new() {
        super()
        name = "ChoosingState"
    }

    begin(params) {
        var s = session
        s.arm()
        s.clearFlashes()
        s.commitEnemyMoves()
        s.view.setMessage("Choose your move.")
        s.view.showMoveMenu(true)
        s.view.focusFirstMoveButton()
        s.refreshViews()
    }

    update(stateManager) {
        if (session.armed() && Input.isInputJustReleased("UICancel")) {
            session.resolver.flee()
        }
    }
}
