// core/BaseState.wren
// Base class for all game states
// Provides default implementations and common interface
// All states should inherit from this

class BaseState {
    construct new() {
        _name = "BaseState"
    }
    
    // Called when state becomes active
    enter(stateManager, params) {
        System.print("%(name) entered")
    }
    
    // Called when state becomes inactive
    exit() {
        System.print("%(name) exited")
    }
    
    // Called every frame while active
    update(stateManager) {
        // Override in derived states
    }

    // Called at a fixed cadence while active (see StateManager.fixedUpdate()) - independent of render framerate.
    // Override in derived states that need physics-synced gameplay logic; a state that owns its own nested
    // subStateManager (see below) should mirror update(stateManager)'s own delegation here too (see
    // GameWorldState).
    fixedUpdate(stateManager) {
        // Override in derived states
    }

    // Optional: Called for rendering (if separate from update)
    render() {
        // Override in derived states
    }
    
    // Optional: Called when events are sent to the state
    onEvent(type, data) {
        // Override in derived states
    }
    
    // Getters
    name { _name }
    name=(value) { _name = value }

    // Override in a state that owns its own nested StateManager for sub-states (see GameWorldState) - returning
    // it here is how a state opts into being visible in the state-debug window's stack view. null (default)
    // means "leaf state, nothing nested beneath it".
    subStateManager { null }
}
