import "gameObjectComponents" for Sprite
import "math" for Vec2, Vec3, Vec4
import "resourceManager" for Texture

var WHITE = Vec4.new(255, 255, 255, 255)
var BLACK = Vec4.new(0, 0, 0, 255)

class ItemData {
    construct new(spriteIndex) {
        _spriteIndex = spriteIndex
    }

    getSpriteIndex() {
        return _spriteIndex
    }
}

class Item {
    construct new(entity, args) {
        _entity = entity
        _name = args["Name"]
        _returnable = args["Returnable"]
        _spriteDataMap = {
            "Love Letter": ItemData.new(10),
            "Hammer": ItemData.new(9),
            "Star Chart": ItemData.new(15),
            "Ornate Key": ItemData.new(11),
            "Ancient Tome": ItemData.new(1),
            "Rose": ItemData.new(14),
            "Fresh Bread": ItemData.new(2),
            "Tool Box": ItemData.new(16),
            "Personal Diary": ItemData.new(12),
            "Telescope": ItemData.new(4),
            "Gold Coins": ItemData.new(3),
            "Ancient Seal": ItemData.new(0),
            "Red Pedestal": ItemData.new(6),
            "Green Pedestal": ItemData.new(7),
            "Blue Pedestal": ItemData.new(8),
            "Yellow Pedestal": ItemData.new(9),
        }
        _itemsWithNoImage = [
            "Yellow Pedestal Inactive",
            "Blue Pedestal Inactive",
            "Red Pedestal Inactive",
            "Green Pedestal Inactive",
            "Yellow Pedestal Active",
            "Blue Pedestal Active",
            "Red Pedestal Active",
            "Green Pedestal Active",
            "Safe",
            "Memory Palace",
        ]
    }
    
    // Called after C++ has created base components
    // Script configures/initializes component values
    start() {
        var texture = Texture.load("assets/Tiles/Items.png")
        if (!_itemsWithNoImage.contains(_name)) {
            var interactionId = _name
            var color = WHITE
            var suffix = " Return"
            if (interactionId.endsWith(suffix)) {
                color = BLACK
                interactionId = interactionId[0...interactionId.count - suffix.count]
            }
            var itemData = _spriteDataMap[interactionId]
            Sprite.create(_entity, texture, color, Vec2.new(32, 48), 9, 2, false, itemData.getSpriteIndex(), 1)
        }
        texture.unload()
    }
    
    update() {

    }
    
    onDestroy() {
    }
    
    onEvent(event) {
        
    }
}
