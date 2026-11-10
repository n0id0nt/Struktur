import "gameObjectComponents" for Sprite, RenderLayer
import "renderer" for FlipBit
import "math" for Vec2, Vec3, Vec4
import "resourceManager" for Texture
import "Util/Colors" for BLACK, WHITE, BLANK
import "Inventory/Inventory" for Inventory
import "flags" for FlagManager
import "debug" for Debug
import "Inventory/ItemDatabase" for ItemDatabase
import "Localization/DisplayNames" for DisplayNames
import "localization" for Localization

var SHADOW_COLOR = Vec4.copy(BLACK)
SHADOW_COLOR.w = 100

// What a pickup with no icon yet shows.
var PLACEHOLDER_SPRITE = "Items/Object/Bag.png"

class Item {
    // World-only art for the pedestals (they are never inventory items, so they have no ItemDatabase icon).
    static pedestalArt {
        if (__pedestalArt == null) {
            __pedestalArt = {
                "Red Pedestal": "Items/Resource/GemRed.png",
                "Green Pedestal": "Items/Resource/GemGreen.png",
                "Blue Pedestal": "Items/Resource/GemPurple.png",
                "Yellow Pedestal": "Items/Resource/GemYellow.png",
            }
        }
        return __pedestalArt
    }

    // The image a pickup shows in the world: its inventory icon, else a generic bag for items with no art yet.
    static spritePath(name) {
        var path = Item.pedestalArt[name]
        if (path == null) path = ItemDatabase.iconPath(name)
        return path != null ? path : PLACEHOLDER_SPRITE
    }

    // Items the world shows no sprite for.
    static namesWithNoImage {
        if (__namesWithNoImage == null) {
            __namesWithNoImage = [
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
        return __namesWithNoImage
    }

    static createSprite(entity, name, color) {
        if (Item.namesWithNoImage.contains(name)) {
            return
        }
        var texture = Texture.load(Item.spritePath(name))
        // One small image per item - pivot at the horizontal centre, three quarters down, so it sits on its tile.
        Sprite.create(entity, texture, color, Vec2.new(texture.width / 2, texture.height * 3 / 4), 1, 1, FlipBit.NONE, 0, RenderLayer.ENTITIES, 1)
        texture.unload()
    }

    // Level viewer (editor): the sprite as placed, ignoring inventory/flag state. Items with no image get none, so the
    // viewer shows a placeholder.
    static preview(entity, args) {
        Item.createSprite(entity, Item.nameFrom_(args), WHITE)
    }

    // The item id from its args - "Name" (an ItemDatabase / dialogue item id).
    static nameFrom_(args) {
        var name = args["Name"]
        if (name == null || name == "") {
            Debug.warning("Item %(args["Iid"]) has no Name - give it one in LDtk")
            return "Unknown Item"
        }
        return name
    }

    // args: "Name" (the item id), optional "Returnable", and "Iid" when placed in the level (LDtk "Item" entity).
    construct new(entity, args) {
        _interactable = true
        _entity = entity
        _returnable = args["Returnable"] == true
        _name = Item.nameFrom_(args)
        // A level item remembers being picked up by its own iid, so two pickups of the same item are separate.
        _pickedFlag = args["Iid"] != null ? "%(args["Iid"]) Picked" : null
        _isShadow = false
        _disabled = false

        if (_returnable) {
            if (Inventory.contains(name)) {
                _isShadow = true
            }
        } else {
            if (FlagManager.getFlag(name) || (_pickedFlag != null && FlagManager.getFlag(_pickedFlag))) {
                _disabled = true
            }
        }

    }

    // Picked up with no dialogue of its own (States/InteractState.wren): into the inventory, gone from the world (or,
    // returnable, left as a shadow to put back). Returns the line to show.
    interact() {
        if (_disabled || _isShadow) return null
        Inventory.addItem(_name)
        Inventory.save("inventory.sav")
        if (_returnable) {
            setIsShadow(true)
        } else {
            FlagManager.setFlag(_pickedFlag != null ? _pickedFlag : _name, true)
            FlagManager.save("flags.sav")
            disableItem()
        }
        return Localization.get("interact.item.picked_up").replace("{item}", DisplayNames.itemDisplay(_name))
    }
    
    name { _name }

    returnable { _returnable }
    
    // Called after C++ has created base components
    // Script configures/initializes component values
    start() {
        Item.createSprite(_entity, _name, _isShadow ? SHADOW_COLOR : WHITE)
        if (_disabled) {
            disableItem()
        }
    }
    
    update() {

    }
    
    onDestroy() {
    }
    
    onEvent(type, data) {
        
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
        Debug.info("adding item %(_entity) to inventory")
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
