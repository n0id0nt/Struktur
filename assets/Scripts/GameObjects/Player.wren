import "game" for Transform

var INTERACTABLE_DISTANCE = 64.0

class Player {
    construct new(entity, name) {
        _entity = entity
        _name = name
        _timeAccumulator = 0
        _initialized = false
        _facing = Vec2.new()
        _speed = Vec2.new()
        
        System.print("NPCBehavior constructed for: %(_name)")
    }
    
    // Called after C++ has created base components
    // Script configures/initializes component values
    create(entity) {
        System.print("NPCBehavior.Create() called for: %(_name)")
        
        // Get the entity's position
        var pos = Transform.getPosition(_entity)
        if (pos != null) {
            System.print("Initial position: %(pos.x), %(pos.y), %(pos.y)")
        }
        
        _initialized = true
    }
    
    update(dt) {
        if (!_initialized) return
        
        _timeAccumulator = _timeAccumulator + dt
        
        // Example: Simple idle animation behavior
        if (_timeAccumulator > 2.0) {
            // Get current position
            var pos = Transform.getPosition(_entity)
            if (pos != null) {
                // Move slightly (example behavior)
                var newY = pos.y + 0.1
                System.print("New position: %(pos.x), %(pos.y), %(pos.z)")
                //Transform.setPosition(_entity, pos[0], newY, pos[2])
            }
            
            _timeAccumulator = 0
        }
    }
    
    onDestroy() {
        System.print("NPCBehavior.OnDestroy() called for: %(_name)")
    }
    
    onEvent(event) {
        var eventType = event["type"]
        
        if (eventType == "CollisionBegin") {
            var other = event["otherEntity"]
            System.print("NPC %(_name) collided with entity: %(other)")
        } else if (eventType == "MessageReceived") {
            System.print("NPC %(_name) received message")
        }
    }

    getPlayerAnimation(animationType) {
        if (_facing.x > 0.01 || _facing.x < -0.01) {
            return "side%(animationType)Animation"
        }
        if (_facing.y > 0.01) {
            return "up%(animationType)Animation"
        }
        return "down%(animationType)Animation"
    }

    playerForceStop() {
        var velocity = Vec2.new()
        PhysicsSystem.setLinearVelocity(_entity, velocity)
        var animation = getPlayerAnimation("Idle")
        AnimationSystem.playAnimation(_entity, animation)
    }

    playerControl(dir) {
        // set player facing
        if (dir.length() > 0.001) {
            dir = dir.normalize()
            _facing = dir
        }

        var velocity = dir * _speed
        PhysicsSystem.setLinearVelocity(_entity, velocity)

        if (dir.length() > 0.001) {
            var animation = getPlayerAnimation("Run")
            AnimationSystem.playAnimation(_entity, animation)

            if (dir.x > 0) {
                SpriteSystem.setFlipped(_entity, false)
            } else if (dir.x < 0) {
                SpriteSystem.setFlipped(_entity, true)
            }
        } else {
            var animation = getPlayerAnimation("Idle")
            AnimationSystem.playAnimation(_entity, animation)
        }
    }

    canInteract() {
        var closestDistance = Math.infinity
        var playerWorldPosition = TransformSystem.getPosition(_entity)
        var closestEntity = null

        TransformSystem.setTransform()
        GameObject.forEachWithComponent(["WorldTransform", "WrenScript"]) { |entity|
            if (!ScriptSystem.call(entity, "isInteractable")) {
                return
            }
            var interactableWorldPosition = TransformSystem.getPosition(entity)
            var distance = Vec3.distance(interactableWorldPosition, playerWorldPosition)
            if (distance < closestDistance) {
                closestDistance = distance
                closestEntity = interactableEntity
            }
        }
        if (closestDistance < INTERACTABLE_DISTANCE) {
            return closestEntity
        }
        return null
    }
}
