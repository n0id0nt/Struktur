// States/Combat/CombatSubState.wren
// Base class for every phase of a fight (EnteringState, ChoosingState, ChargingState, ...). States/
// CombatState.wren owns a nested StateManager and registers one of these per phase; every
// transition goes through CombatSession.goTo(), which always hands the shared CombatSession over in
// params["session"] - this base stores it and calls begin(params) so a phase only has to override
// begin() (run once on entry) and update() (run every frame), never touch enter()/exit() plumbing.
// Each phase instance is reused across entries (StateManager.insertState builds it once), so
// begin() must reset any per-entry field it owns, and exit() drops the session reference so a
// finished fight's views/stage aren't kept alive.
import "States/BaseState" for BaseState

class CombatSubState is BaseState {
    construct new() {
        super()
    }

    session { _session }

    enter(stateManager, params) {
        super.enter(stateManager, params)
        _session = params["session"]
        begin(params)
    }

    exit() {
        super.exit()
        _session = null
    }

    // Override: per-entry setup. params is whatever the previous phase passed to goTo() (plus
    // "session", already stored).
    begin(params) {}
}
