// States/Combat/ChoosingState.wren
// The move menu is up for the party member whose turn it is (CombatSession.chooser - every party member is human
// controlled, so the human picks for each in turn): game time is PAUSED (every committed combatant holds its
// wind-up pose) while they pick a move (the menu buttons, wired once per fight in States/CombatState.wren, call
// CombatSession.commitChoice) or flee with UICancel. Committing a move comes back here for the next member who
// needs one. Entering this phase also lets every enemy that
// needs one pick its next move. Leads to TargetingState (offensive move against a group) or ChargingState;
// fleeing ends the fight via CombatResolver.flee.
import "input" for Input
import "States/Combat/CombatSubState" for CombatSubState

class ChoosingState is CombatSubState {
    construct new() {
        super()
        name = "ChoosingState"
    }

    begin(params) {
        var s = session
        var member = s.chooser
        if (member == null) {
            // Everyone already has a move (the whole party was committed by an earlier pass).
            s.goTo("ChargingState")
            return
        }
        s.pauseTime()
        s.arm()
        s.clearFlashes()
        s.commitEnemyMoves()

        var labels = []
        for (move in member.moves) {
            labels.add(move.menuLabel)
        }
        s.view.setMoveLabels(labels)
        s.view.setMessage(member == s.player ? "Choose your move." : "Choose %(member.name)'s move.")
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
