import "gameObjectComponents" for Sprite
import "math" for Vec2, Vec3, Vec4
import "resourceManager" for Texture
import "Colors" for BLACK, WHITE, BLANK
import "Inventory" for Inventory
import "flags" for FlagManager

var SHADOW_COLOR = Vec4.copy(BLACK)
SHADOW_COLOR.w = 100

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
        _interactable = true
        _entity = entity
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
        _name = args["Name"]
        _isShadow = false
        _disabled = false

        if (_returnable) {
            if (Inventory.contains(name)) {
                _isShadow = true
            }
        } else {
            if (FlagManager.getFlag(name)) {
                _disabled = true
            }
        }

    }
    
    name { _name }

    returnable { _returnable }
    
    // Called after C++ has created base components
    // Script configures/initializes component values
    start() {
        var texture = Texture.load("Tiles/Items.png")
        if (!_itemsWithNoImage.contains(_name)) {
            var interactionId = _name
            var color = WHITE
            if (_isShadow) {
                color = SHADOW_COLOR
            }
            var itemData = _spriteDataMap[interactionId]
            Sprite.create(_entity, texture, color, Vec2.new(32, 48), 9, 2, false, itemData.getSpriteIndex(), 1)
        }
        texture.unload()
        if (_disabled) {
            disableItem()
        }
    }
    
    update() {

    }
    
    onDestroy() {
    }
    
    onEvent(event) {
        
    }

    isInteractable() {
        return _interactable
    }

    getEntity() {
        return _entity
    }

    getInteractId() {
        return "Entrance Door"
    }

    setIsShadow(isShadow) {
        _isShadow = isShadow
        var color = WHITE
        if (_isShadow) {
            color = SHADOW_COLOR
        }
        System.print("adding item %(_entity) to inventory")
        var sprite = Sprite.get(_entity)
        if (sprite) {
            sprite.color = color
        }
    }

    disableItem() {
        _disabled = true
        _interactable = false
        var sprite = Sprite.get(_entity)
        if (sprite) {
            sprite.color = BLANK
        }
    }
}
