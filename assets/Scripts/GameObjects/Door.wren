import "physics" for BodyDefinition, PhysicsSquareShape
import "gameObjectComponents" for PhysicsBody

class Door {
    construct new(entity, args) {
        _entity = entity
        _name = args["Name"]
    }
    
    name { _name }
    
    start() {        
        var bodyDef = BodyDefinition.createStaticBody()
        var doorShape = PhysicsSquareShape.new(1.5, 0.5)
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
