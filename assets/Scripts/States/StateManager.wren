// core/StateManager.wren
// Manages state stack and transitions
// Handles push/pop/change operations
// Delegates update to active state

class StateManager {
    construct new() {
        _currentState = null
        _stateFactory = {}

        System.print("StateManager created")
    }
    
    // Update active state
    update(dt) {
        if (_currentState) {
            _currentState.update(dt)
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
        // TODO add some error handling here for when there is no state of given name
        _currentState = _stateFactory[stateName].new() // TODO Decide where I want to call new() here or where the state factory is created
        // TODO Assert that there is a new state here
        if (_currentState) {
            _currentState.enter()
        }
    }

    insertState(stateName, stateConstructor) {
        _stateFactory[stateName] = stateConstructor
    }
    
    // Getters
    currentState { _currentState }
    stateFactory { _stateFactory }

    // Setters
    stateFactory=(value) { _stateFactory = value }
}
