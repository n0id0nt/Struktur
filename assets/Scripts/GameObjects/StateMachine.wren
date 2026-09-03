// GameObjects/StateMachine.wren
// Minimal per-entity finite state machine, used by the critter behaviour scripts (Chicken, Chinlin).
//
// A state is just a bag of three callbacks, any of which may be null:
//   enter  - Fn.new { ... }   runs once when the state becomes active
//   update - Fn.new { ... }   runs every tick while the state is active
//   exit   - Fn.new { ... }   runs once when the state is left
//
// The callbacks are normally closures created inside the owning object's methods, so they can call
// that object's methods directly - Wren captures `this` into a closure defined in a method (see
// third_party/wren/test/language/this/closure.wren).
//
// Timing (e.g. "wander for 1-5 seconds") is left to the owner - drive it off Time.scaledTime rather
// than an accumulated delta, since fixedUpdate() can run more than once per rendered frame.
class StateMachine {
    construct new() {
        _states = {}
        _current = null
        _currentName = null
    }

    // Register a state. Pass null for any callback you don't need.
    define(name, enter, update, exit) {
        _states[name] = { "enter": enter, "update": update, "exit": exit }
    }

    has(name) { _states.containsKey(name) }
    name { _currentName }

    change(name) {
        if (!_states.containsKey(name)) {
            System.print("[StateMachine] no state named '%(name)'")
            return
        }
        if (_current && _current["exit"]) { _current["exit"].call() }
        _current = _states[name]
        _currentName = name
        if (_current["enter"]) { _current["enter"].call() }
    }

    update() {
        if (_current && _current["update"]) { _current["update"].call() }
    }
}
