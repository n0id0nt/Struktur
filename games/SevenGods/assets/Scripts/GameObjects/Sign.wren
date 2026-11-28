// GameObjects/Sign.wren
// A signpost: a static, interactable prop that shows a short message through the dialogue system
// (States/InteractState.wren, via Dialogue/DialogueEntryPoints.wren's "<Name>" -> node id table).
// Placeholder sprite for now - the small paper note (Items/Other/Letter.png); swap PLACEHOLDER_SPRITE below
// once there is real signpost art. Placed in the level (LDtk "Sign" entity); an optional "Sprite" field swaps the art.
import "gameObjectComponents" for Sprite, PhysicsBody, RenderLayer
import "renderer" for FlipBit
import "math" for Vec2
import "resourceManager" for Texture
import "physics" for BodyDefinition, PhysicsCircleShape, BodyType
import "Util/Colors" for WHITE
import "localization" for Localization
import "World/StoryGates" for StoryGates

var PLACEHOLDER_SPRITE = "Items/Other/Letter.png"

class Sign {
    // args: "Name" (also the dialogue key), optional "Sprite" (an image path to show instead - e.g. the rest spot).
    // A sign placed in the level (LDtk "Sign" entity) has "MessageKey" instead: a Localization key (en.json) for the
    // one line it shows - or, if a DialogueEntryPoints entry has that name, the conversation it starts.
    construct new(entity, args) {
        _entity = entity
        _messageKey = args["MessageKey"]
        _name = args["Name"] != null ? args["Name"] : _messageKey
        _spritePath = args["Sprite"] != null && args["Sprite"] != "" ? args["Sprite"] : PLACEHOLDER_SPRITE
        // Optional story window (ShowFromStage / HideFromStage / HideIfFlag LDtk fields) - World/StoryGates.wren.
        _gate = StoryGates.fromArgs(args)
    }

    // Also the key DialogueEntryPoints.get() looks the sign's dialogue up by.
    name { _name }

    // Called after C++ has created base components.
    start() {
        var texture = Texture.load(_spritePath)
        Sprite.create(_entity, texture, WHITE, Vec2.new(texture.width / 2, texture.height * 3 / 4), 1, 1, FlipBit.NONE, 0,
                      RenderLayer.ENTITIES, 0)
        texture.unload()

        // Solid, so the player walks around it rather than through it.
        var bodyDef = BodyDefinition.new(BodyType.STATIC_BODY)
        PhysicsBody.create(_entity, bodyDef, PhysicsCircleShape.new(0.2))

        StoryGates.register(_entity, _gate)
    }

    update() {}

    onDestroy() {
        StoryGates.unregister(_entity)
    }

    onEvent(type, data) {}

    // Read with no dialogue of its own (States/InteractState.wren): its message line.
    interact() {
        if (_messageKey == null || _messageKey == "") return null
        return Localization.get(_messageKey)
    }

    isInteractable() {
        return StoryGates.isVisible(_entity)
    }

    // Only talked to / read when the player is facing it (Player.getInteractEntity).
    requiresFacing() { true }

    getEntity() {
        return _entity
    }
}
