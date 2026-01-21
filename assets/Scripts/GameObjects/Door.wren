import "physics" for BodyDefinition, PhysicsBoxShape, BodyType
import "gameObjectComponents" for PhysicsBody

class Door {
    construct new(entity, args) {
        _entity = entity
        _name = args["Name"]
    }
    
    name { _name }
    
    start() {        
        var bodyDef = BodyDefinition.new(BodyType.STATIC_BODY)
        var doorShape = PhysicsBoxShape.new(1, 0.25)
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
