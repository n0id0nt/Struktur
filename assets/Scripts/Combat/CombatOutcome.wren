// Combat/CombatOutcome.wren
// A one-slot mailbox from a finished fight to the world, like Party/DuelRequest.wren: CombatResolver.endFight
// records how the fight ended, and States/ExperimentState.wren takes it once the CombatState has closed (the
// overworld can't be told directly - the fight ends from inside its own nested state machine).
//   result    - "won" / "lost" / "fled"
//   duel      - true for a challenge fight (ExperimentState.startDuel) - a lost duel is never a death
//   fallen    - names of the opponents who died in it (their overworld entities are already removed)
//   recruited - names of the opponents who joined the party instead (CombatResolver.win)
class CombatOutcome {
    construct new(result, duel, fallen, recruited) {
        _result = result
        _duel = duel
        _fallen = fallen
        _recruited = recruited
    }

    result { _result }
    duel { _duel }
    fallen { _fallen }
    recruited { _recruited }

    won { _result == "won" }
    lost { _result == "lost" }
    fled { _result == "fled" }

    toString { "%(_result)%(_duel ? " (duel)" : "") fallen [%(_fallen.join(", "))] recruited [%(_recruited.join(", "))]" }

    static record(outcome) {
        __pending = outcome
    }

    // Returns the last fight's outcome (or null) and clears it.
    static take() {
        var outcome = __pending
        __pending = null
        return outcome
    }
}
