// core/StateManager.wren
// Manages state stack and transitions
// Delegates update to active state

import "debug" for Profile

class StateManager {
    construct new() {
        _currentState = null
        _stateFactory = {}

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
    sendEvent(type, data) {
        if (_currentState) {
            Profile.begin("Send Event: " + _currentState.name)
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
            Profile.begin("Enter State: " + _currentState.name)
            _currentState.enter(this, params)
            Profile.end()
        } else {
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
    }

    insertState(stateName, stateConstructor) {
        Profile.begin("Inserting State: " + stateName)
        _stateFactory[stateName] = stateConstructor.new()
        Profile.end()
    }
    
    // Getters
    currentState { _currentState }
    stateFactory { _stateFactory }

    // Setters
    stateFactory=(value) { _stateFactory = value }
}
