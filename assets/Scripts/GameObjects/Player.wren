import "game" for Entity, Transform

class Player {
    construct new(entity, name) {
        _entity = entity
        _name = name
        _timeAccumulator = 0
        _initialized = false
        
        System.print("NPCBehavior constructed for: %(_name)")
    }
    
    // Called after C++ has created base components
    // Script configures/initializes component values
    Create(entity) {
        System.print("NPCBehavior.Create() called for: %(_name)")
        
        // Get the entity's position
        var pos = Transform.getPosition(_entity)
        if (pos != null) {
            System.print("Initial position: %(pos[0]), %(pos[1]), %(pos[2])")
        }
        
        _initialized = true
    }
    
    Update(dt) {
        if (!_initialized) return
        
        _timeAccumulator = _timeAccumulator + dt
        
        // Example: Simple idle animation behavior
        if (_timeAccumulator > 2.0) {
            // Get current position
            var pos = Transform.getPosition(_entity)
            if (pos != null) {
                // Move slightly (example behavior)
                var newY = pos[1] + 0.1
                System.print("New position: %(pos[0]), %(pos[1]), %(pos[2])")
                //Transform.setPosition(_entity, pos[0], newY, pos[2])
            }
            
            _timeAccumulator = 0
        }
    }
    
    OnDestroy() {
        System.print("NPCBehavior.OnDestroy() called for: %(_name)")
    }
    
    OnEvent(event) {
        var eventType = event["type"]
        
        if (eventType == "CollisionBegin") {
            var other = event["otherEntity"]
            System.print("NPC %(_name) collided with entity: %(other)")
        } else if (eventType == "MessageReceived") {
            System.print("NPC %(_name) received message")
        }
    }
}
