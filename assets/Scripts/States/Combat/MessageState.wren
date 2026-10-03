// States/Combat/MessageState.wren
// A move's battle-log line sits on screen for a beat. Entered with params text; once it's been up
// long enough CombatResolver.afterMove decides what follows (win / lose / ChoosingState /
// ChargingState).
import "app" for Time
import "States/Combat/CombatSubState" for CombatSubState

var MESSAGE_TIME = 0.9

class MessageState is CombatSubState {
    construct new() {
        super()
        name = "MessageState"
        _startTime = 0
    }

    begin(params) {
        session.view.setMessage(params["text"])
        _startTime = Time.unscaledTime
    }

    update(stateManager) {
        if (Time.unscaledTime - _startTime >= MESSAGE_TIME) {
            session.resolver.afterMove()
        }
    }
}
