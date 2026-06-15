import "Inventory" for Inventory

class Room {
    construct new(entity, args) {
        _entity = entity
        _name = args["Name"]
        _transformItem = args["TransformItem"]
        _regularRoom = args["RegularRoom"]
        _transformedRoom = args["TransformedRoom"]
        
        System.print("Player constructed for: %(_name)")
    }
    
    name { _name }
    transformItem { _transformItem }
    regularRoom { _regularRoom }
    transformedRoom { _transformedRoom }

    transformRoom(isTransformed) {
        if (isTransformed) {
            _regularRoom.setInactive()
            _transformedRoom.setActive()
        } else {
            _transformedRoom.setInactive()
            _regularRoom.setActive()
        }
    }

    start() {
        var hasTransformItem = Inventory.contains(_transformItem)
        transformRoom(hasTransformItem)
    }
    
    update() {

    }
    
    onDestroy() {
    }
    
    onEvent(type, data) {       
        if (type == "pickupEntity") {
            var item = params["name"]
            if (item == _transformItem) {
                transformRoom(true)
            }
        } else if (type == "putdownEntity") {
            var item = params["name"]
            if (item == _transformItem) {
                transformRoom(false)
            }
        }
    }
}