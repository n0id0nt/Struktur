// core/BaseState.wren
// Base class for all game states
// Provides default implementations and common interface
// All states should inherit from this

class BaseState {
    construct new(game) {
        _game = game
        _name = "BaseState"
    }
    
    // Called when state becomes active
    enter() {
        System.print("%(name) entered")
    }
    
    // Called when state becomes inactive
    exit() {
        System.print("%(name) exited")
    }
    
    // Called every frame while active
    update(dt) {
        // Override in derived states
    }
    
    // Optional: Called for rendering (if separate from update)
    render() {
        // Override in derived states
    }
    
    // Optional: Called when returning to this state from a popped state
    resume() {
        System.print("%(name) resumed")
        // Default behavior: just log
        // Override if you need custom resume logic
    }
    
    // Optional: Called when events are sent to the state
    onEvent(event) {
        // Override in derived states
    }
    
    // Convenience methods for state transitions
    changeState(stateName) {
        _game.changeState(stateName)
    }
    
    pushState(stateName) {
        _game.pushState(stateName)
    }
    
    popState() {
        _game.popState()
    }
    
    // Getters
    game { _game }
    name { _name }
    name=(value) { _name = value }
}
