// core/StateManager.wren
// Manages state stack and transitions
// Delegates update to active state

import "debug" for Profile
import "serialisation" for Json, MapUtil

class StateManager {
    construct new() {
        _currentState = null
        _stateFactory = {}
        _currentStateParams = {}

        System.print("[StateManager] created")
    }
    
    // Update active state
    update() {
        if (_currentState) {
            Profile.begin("Update: " + _currentState.name)
            _currentState.update(this)
            Profile.end()
        }
    }
    
    // Render active state
    render() {
        if (_currentState) {
            Profile.begin("Render: " + _currentState.name)
            _currentState.render()
            Profile.end()
        }
    }
    
    // Send event to active state
    onEvent(type, data) {
        if (_currentState) {
            Profile.begin("On Event: " + _currentState.name)
            _currentState.onEvent(type, data)
            Profile.end()
        }
    }
    
    // Create state instance from name
    changeState(stateName) {
        changeState(stateName, {})
    }

    changeState(stateName, params) {
        if (_currentState) {
            Profile.begin("Exit State: " + _currentState.name)
            _currentState.exit()
            Profile.end()
        }
        _currentState = _stateFactory[stateName]
        if (_currentState) {
            _currentStateParams = StateManager.summariseParams(params)
            Profile.begin("Enter State: " + _currentState.name)
            _currentState.enter(this, params)
            Profile.end()
        } else {
            _currentStateParams = {}
            System.print("[StateManager] No state with name %(stateName) exists.")
        }
    }

    // clears current state
    clearCurrentState() {
        if (_currentState) {
            Profile.begin("Exit State: " + _currentState.name)
            _currentState.exit()
            Profile.end()
        }
        _currentState = null
        _currentStateParams = {}
    }

    insertState(stateName, stateConstructor) {
        Profile.begin("Inserting State: " + stateName)
        _stateFactory[stateName] = stateConstructor.new()
        Profile.end()
    }

    // Reduces an arbitrary params map to something Json.stringify can always handle - primitive values
    // (String/Num/Bool/null) pass through as-is, anything else (e.g. InteractState's real entity handle) becomes
    // a placeholder like "<Entity>". Needed because activeStateStackJson() below reports params as JSON, and
    // Json.stringify cannot serialise arbitrary foreign objects - summarising up front means that call can never
    // fail regardless of what a state is entered with.
    static summariseParams(params) {
        var summary = {}
        for (key in params.keys) {
            var value = params[key]
            if (value is String || value is Num || value is Bool || value == null) {
                summary[key] = value
            } else {
                summary[key] = "<%(value.type.name)>"
            }
        }
        return summary
    }

    // Flat list (outermost to innermost) of every currently-active state in this manager's chain, including any
    // nested sub-state-managers a state opts into via BaseState.subStateManager (see GameWorldState for the one
    // existing example) - used by the state debug editor window. Returns JSON so a single C++ call/parse covers
    // the whole chain instead of walking the Wren object graph one handle-call at a time.
    activeStateStackJson() {
        var stack = []
        var manager = this
        while (manager && manager.currentState) {
            stack.add({
                "name": manager.currentState.name,
                "params": manager.currentStateParams
            })
            manager = manager.currentState.subStateManager
        }
        // Json.stringify can't serialise a raw Map (it errors) - MapUtil.mapToPairs converts every Map in the
        // structure (including the nested "params" map per level) to the sentinel-tagged pairs form Json.
        // stringify actually understands, matching the same pattern Inventory.wren's own serialise() uses.
        return Json.stringify(MapUtil.mapToPairs(stack))
    }

    // Same walk as activeStateStackJson(), but returns the live state instances themselves (outermost to
    // innermost) instead of a JSON summary - lets the debug window hold a real WrenHandle* per depth for
    // exported-field inspection, which JSON can't carry.
    activeStateInstances() {
        var stack = []
        var manager = this
        while (manager && manager.currentState) {
            stack.add(manager.currentState)
            manager = manager.currentState.subStateManager
        }
        return stack
    }

    // Every state name this manager can changeState() to - for the state-debug window's "force transition"
    // dropdown.
    registeredStateNames() { _stateFactory.keys.toList }

    // Getters
    currentState { _currentState }
    currentStateParams { _currentStateParams }
    stateFactory { _stateFactory }

    // Setters
    stateFactory=(value) { _stateFactory = value }
}
