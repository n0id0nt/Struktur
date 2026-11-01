// GameObjects/Sign.wren
// A signpost: a static, interactable prop that shows a short message through the dialogue system
// (States/InteractState.wren, via Dialogue/DialogueEntryPoints.wren's "<Name>" -> node id table).
// Placeholder sprite for now - the small paper note from Tiles/Items.png (index 13); swap the texture /
// index below once there is real signpost art. Spawned by States/ExperimentState.wren.
import "gameObjectComponents" for Sprite, PhysicsBody, RenderLayer
import "renderer" for FlipBit
import "math" for Vec2
import "resourceManager" for Texture
import "physics" for BodyDefinition, PhysicsCircleShape, BodyType
import "Colors" for WHITE

var PLACEHOLDER_SPRITE_INDEX = 13

class Sign {
    // args: "Name" (also the dialogue key), optional "SpriteIndex" (a Tiles/Items.png frame - e.g. the rest spot).
    construct new(entity, args) {
        _entity = entity
        _name = args["Name"]
        _spriteIndex = args["SpriteIndex"] != null ? args["SpriteIndex"] : PLACEHOLDER_SPRITE_INDEX
    }

    // Also the key DialogueEntryPoints.get() looks the sign's dialogue up by.
    name { _name }

    // Called after C++ has created base components.
    start() {
        var texture = Texture.load("Tiles/Items.png")
        Sprite.create(_entity, texture, WHITE, Vec2.new(32, 48), 9, 2, FlipBit.NONE, _spriteIndex, RenderLayer.ENTITIES, 0)
        texture.unload()

        // Solid, so the player walks around it rather than through it.
        var bodyDef = BodyDefinition.new(BodyType.STATIC_BODY)
        PhysicsBody.create(_entity, bodyDef, PhysicsCircleShape.new(0.2))
    }

    update() {}

    onDestroy() {}

    onEvent(type, data) {}

    isInteractable() {
        return true
    }

    // Only talked to / read when the player is facing it (Player.getInteractEntity).
    requiresFacing() { true }

    getEntity() {
        return _entity
    }
}
