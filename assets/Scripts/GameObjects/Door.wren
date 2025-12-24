import "physics" for BodyDefinition, PhysicsCircleShape
import "gameObjectComponents" for PhysicsBody

class Door {
    construct new(entity, args) {
        _entity = entity
        _name = args["Name"]
    }
    
    start() {        
        var bodyDef = BodyDefinition.createStaticBody()
        var doorShape = PhysicsCircleShape.new(0.5)
        var physicsBody = PhysicsBody.create(_entity, bodyDef, doorShape)
        physicsBody.syncToPhysics = true
    }
    
    update() {

    }
    
    onDestroy() {
    }
    
    onEvent(event) {

    }

    isInteractable() {
        return true
    }

    getEntity() {
        return _entity
    }

    getInteractId() {
        return "Entrance Door"
    }
}
