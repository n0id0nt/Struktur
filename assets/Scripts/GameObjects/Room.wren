import "gameObject" for GameObject

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
            GameObject.setInactive(_regularRoom)
            GameObject.setActive(_transformedRoom)
        } else {
            GameObject.setInactive(_transformedRoom)
            GameObject.setActive(_regularRoom)
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
            var item = data["name"]
            if (item == _transformItem) {
                transformRoom(true)
            }
        } else if (type == "putdownEntity") {
            var item = data["name"]
            if (item == _transformItem) {
                transformRoom(false)
            }
        }
    }
}