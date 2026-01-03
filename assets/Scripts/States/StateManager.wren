// core/StateManager.wren
// Manages state stack and transitions
// Delegates update to active state

class StateManager {
    construct new() {
        _currentState = null
        _stateFactory = {}

        System.print("[StateManager] created")
    }
    
    // Update active state
    update() {
        if (_currentState) {
            _currentState.update(this)
        }
    }
    
    // Render active state
    render() {
        if (_currentState) {
            _currentState.render()
        }
    }
    
    // Send event to active state
    sendEvent(event) {
        if (_currentState) {
            _currentState.onEvent(event)
        }
    }
    
    // Create state instance from name
    changeState(stateName) {
        if (_currentState) {
            _currentState.exit()
        }
        _currentState = _stateFactory[stateName]
        if (_currentState) {
            _currentState.enter()
        } else {
            System.print("[StateManager] No state with name %(stateName) exists.")
        }
    }

    // clears current state
    clearCurrentState() {
        if (_currentState) {
            _currentState.exit()
        }
        _currentState = null
    }

    insertState(stateName, stateConstructor) {
        _stateFactory[stateName] = stateConstructor.new()
    }
    
    // Getters
    currentState { _currentState }
    stateFactory { _stateFactory }

    // Setters
    stateFactory=(value) { _stateFactory = value }
}
