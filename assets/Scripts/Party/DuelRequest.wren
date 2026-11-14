// Party/DuelRequest.wren
// A one-slot mailbox from the dialogue system to the world: the "startDuel" / "startBattle" dialogue commands (Dialogue/
// RegisteredFunctions/Commands.wren) can't start a fight themselves - they run in the middle of InteractState - so they
// leave the challenger's name here, and States/GameWorldState.wren picks it up when that dialogue has ended and starts
// the fight.
//   lethal false - a duel (startDuel): non-lethal, losing leaves everyone on 1 HP (Thomas, the constructs).
//   lethal true  - a story battle (startBattle): losing is a real defeat - game over, back to the last rest spot
//                  (the Wardens).
class DuelRequest {
    static request(npcName) { DuelRequest.request(npcName, false) }

    static request(npcName, lethal) {
        __challenger = npcName
        __lethal = lethal
    }

    // Returns the pending fight as [challenger name, lethal] (or null) and clears it.
    static take() {
        if (__challenger == null) {
            return null
        }
        var pending = [__challenger, __lethal == true]
        __challenger = null
        __lethal = null
        return pending
    }
}
