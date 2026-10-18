// Party/DuelRequest.wren
// A one-slot mailbox from the dialogue system to the world: the "startDuel" dialogue command (Dialogue/
// RegisteredFunctions/Commands.wren) can't start a fight itself - it runs in the middle of InteractState - so it
// leaves the challenger's name here, and States/ExperimentState.wren picks it up when that dialogue has ended
// and starts the battle.
class DuelRequest {
    static request(npcName) {
        __challenger = npcName
    }

    // Returns the pending challenger's name (or null) and clears it.
    static take() {
        var name = __challenger
        __challenger = null
        return name
    }
}
