// World/ImpactEvents.wren
// A queue of "impact" moments for the gameplay state to play out (States/ImpactState.wren): the camera pans to a
// spot in the world, something happens there (the actions run) with a little screen shake, and the camera pans
// back. Requested by world objects - a Door whose pressure plates are all down (GameObjects/Door.wren) - and
// taken by States/ExperimentState.wren once nothing else (dialogue, a fight) is running. A plain module so every
// script shares one queue, like World/Signals.wren.
import "math" for Vec2

class ImpactEvent {
    construct new(key) {
        _key = key
        _positions = []
        _actions = []
        _message = null
    }

    key { _key }
    actions { _actions }
    // Localization key of a caption explaining what happens, shown at the top of the screen (or null).
    message { _message }

    add(position, action, message) {
        _positions.add(position)
        if (action != null) {
            _actions.add(action)
        }
        if (_message == null) {
            _message = message
        }
    }

    // The spot the camera looks at: the middle of every position requested under this key (both halves of a
    // double door, say).
    target {
        var x = 0
        var y = 0
        for (p in _positions) {
            x = x + p.x
            y = y + p.y
        }
        return Vec2.new(x / _positions.count, y / _positions.count)
    }

    // Runs every action - called once the camera has arrived.
    trigger() {
        for (action in _actions) {
            action.call()
        }
    }
}

class ImpactEvents {
    // Queues an event at position (Vec2/Vec3), running action (a Fn, or null) when it plays, with an optional
    // caption (message: a Localization key, or null). Requests sharing a key that haven't played yet merge into one
    // event, so two doors on the same plates make one moment.
    static request(key, position, action) { ImpactEvents.request(key, position, action, null) }

    static request(key, position, action, message) {
        if (__pending == null) {
            __pending = []
        }
        var event = null
        for (pending in __pending) {
            if (pending.key == key) {
                event = pending
            }
        }
        if (event == null) {
            event = ImpactEvent.new(key)
            __pending.add(event)
        }
        event.add(position, action, message)
    }

    // The oldest queued event (removed from the queue), or null.
    static take() {
        if (__pending == null || __pending.count == 0) {
            return null
        }
        return __pending.removeAt(0)
    }

    static clear() {
        __pending = null
    }
}
