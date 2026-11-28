// World/Warps.wren
// A pending warp from one room to another - a DoorWorp (GameObjects/DoorWorp.wren) the player stepped on asks for one
// here, and the gameplay state (States/GameWorldState.wren) takes it on its next update and moves the party. Kept
// apart from both so the trigger doesn't need the state and the state doesn't poll every warp.
class Warps {
    // targetIndex: the room to go to; fromIndex: the room the warp is in; destination: the iid of the DoorWorp to
    // arrive at (null: whichever one in the target room leads back to fromIndex). The first request in a frame wins.
    static request(targetIndex, fromIndex, destination) {
        if (__pending == null) {
            __pending = [targetIndex, fromIndex, destination]
        }
    }

    // [targetIndex, fromIndex, destination], or null - and clears it.
    static take() {
        var pending = __pending
        __pending = null
        return pending
    }

    static clear() {
        __pending = null
    }
}
