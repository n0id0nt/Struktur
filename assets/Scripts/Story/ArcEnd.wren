// Story/ArcEnd.wren
// A one-slot mailbox like Party/DuelRequest.wren: the "endArc" dialogue command can't change state from inside the
// conversation, so it leaves a request here and States/GameWorldState.wren shows the end-of-arc card once the
// dialogue has closed.
class ArcEnd {
    static request() {
        __requested = true
    }

    // True once if the arc's end was requested since the last call.
    static take() {
        var requested = __requested == true
        __requested = false
        return requested
    }
}
