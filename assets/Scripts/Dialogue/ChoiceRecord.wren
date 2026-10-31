// Dialogue/ChoiceRecord.wren
// A one-slot mailbox from the dialogue system to States/InteractState.wren, like Party/DuelRequest.wren: after a
// choice is made the choice list collapses to just the option picked and stays up as a record of it
// (UI/Dialogue/ChoiceListUI.collapseTo). The "closeChoice" dialogue command (Dialogue/RegisteredFunctions/
// Commands.wren) asks for that record to be cleared; InteractState takes the request when it shows the node.
class ChoiceRecord {
    static requestClose() {
        __close = true
    }

    // True (once) if a node asked for the choice record to be cleared since the last call.
    static takeCloseRequest() {
        var requested = __close == true
        __close = false
        return requested
    }
}
