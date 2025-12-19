import "game" for BodyDefinition, PhysicsCircleShape, PhysicsBody

class Door {
    construct new(entity, name) {
        _entity = entity
        _name = name
    }
    
    create(entity) {        
        var bodyDef = BodyDefinition.createStaticBody()
        var doorShape = PhysicsCircleShape.new(0.5)
        var physicsBody = PhysicsBody.create(_entity, bodyDef, doorShape)
        physicsBody.syncToPhysics = true
    }
    
    update(dt) {

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
