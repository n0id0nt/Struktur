// core/StateManager.wren
// Manages state stack and transitions
// Handles push/pop/change operations
// Delegates update to active state

class StateManager {
    construct new(game) {
        _game = game
        _stateStack = []
        
        System.print("StateManager created")
    }
    
    // Change to a new state (replaces current)
    changeState(stateName) {
        System.print("Changing state to: %(stateName)")
        
        // Exit current state
        if (_stateStack.count > 0) {
            var currentState = _stateStack[-1]
            currentState.exit()
        }
        
        // Clear stack
        _stateStack.clear()
        
        // Create and enter new state
        var newState = this.createState(stateName)
        if (newState != null) {
            _stateStack.add(newState)
            newState.enter()
        } else {
            System.print("ERROR: Failed to create state: %(stateName)")
        }
    }
    
    // Push a new state on top (pauses current)
    pushState(stateName) {
        System.print("Pushing state: %(stateName)")
        
        // Create new state
        var newState = this.createState(stateName)
        if (newState != null) {
            _stateStack.add(newState)
            newState.enter()
        } else {
            System.print("ERROR: Failed to create state: %(stateName)")
        }
    }
    
    // Pop current state (return to previous)
    popState() {
        if (_stateStack.count <= 1) {
            System.print("WARNING: Cannot pop last state")
            return
        }
        
        System.print("Popping state")
        
        // Exit current state
        var currentState = _stateStack[-1]
        currentState.exit()
        
        // Remove from stack
        _stateStack.removeAt(-1)
        
        // Re-enter previous state if it exists
        if (_stateStack.count > 0) {
            var previousState = _stateStack[-1]
            // Optional: call resume() instead of enter() for returning states
            if (previousState.respondsTo("resume(_)")) {
                previousState.resume()
            }
        }
    }
    
    // Update active state
    update(dt) {
        if (_stateStack.count > 0) {
            var currentState = _stateStack[-1]
            currentState.update(dt)
        }
    }
    
    // Render active state
    render() {
        if (_stateStack.count > 0) {
            var currentState = _stateStack[-1]
            if (currentState.respondsTo("render()")) {
                currentState.render()
            }
        }
    }
    
    // Send event to active state
    sendEvent(event) {
        if (_stateStack.count > 0) {
            var currentState = _stateStack[-1]
            if (currentState.respondsTo("onEvent(_)")) {
                currentState.onEvent(event)
            }
        }
    }
    
    // Create state instance from name
    createState(stateName) {
        // Import the state class dynamically
        // This is a placeholder - actual implementation depends on your state files
        
        // For now, we'll use a simple factory pattern
        // You'll need to import all your states and add them here
        
        if (stateName == "MainMenuState") {
            // Would need: import "states/MainMenuState" for MainMenuState
            // return MainMenuState.new(_game)
            System.print("TODO: Import and create MainMenuState")
            return null
        } else if (stateName == "GameWorldState") {
            // Would need: import "states/GameWorldState" for GameWorldState
            // return GameWorldState.new(_game)
            System.print("TODO: Import and create GameWorldState")
            return null
        }
        
        System.print("ERROR: Unknown state: %(stateName)")
        return null
    }
    
    // Getters
    currentState { _stateStack.count > 0 ? _stateStack[-1] : null }
    stateStack { _stateStack }
    game { _game }
}
