// Dialogue/CloseUp.wren
// The close-up view: a conversation showing something up close - a page of Kira's Big Book of Facts, a carving, a
// memory - as a large picture above the dialogue box (UI/Dialogue/CloseUpUI.wren). The "closeUp" / "closeUpEnd"
// dialogue commands ask for it here when their node is reached; States/InteractState.wren takes the request as it
// shows that node, and the picture stays up until "closeUpEnd" or the end of the conversation.
//
// Pictures live in assets/CloseUps, drawn at CloseUp.size (pixel art, shown at a whole-number scale).

// The size close-up pictures are drawn at: shown at 2x above the dialogue box.
var SIZE = [320, 180]

class CloseUp {
    static size { SIZE }

    // Show `path` (e.g. "CloseUps/kira_book_wolf_fact.png").
    static show(path) { __request = path }

    // Put it away.
    static hide() { __request = false }

    // What the last command asked for: a path to show, false to hide, or null for no change - and forgets it.
    static take() {
        var request = __request
        __request = null
        return request
    }
}
