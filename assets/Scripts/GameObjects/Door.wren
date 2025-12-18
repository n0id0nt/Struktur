import "game" for Transform

class Door {
    construct new(entity, name) {
        _entity = entity
        _name = name
        _timeAccumulator = 0
        _initialized = false
        
    }
    

    create(entity) {        

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
}
