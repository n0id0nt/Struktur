// GameObjects/Door.wren
// A one-tile (32 px) door that blocks the way until all of its Signals channels are on at once (e.g. every
// PressurePlate wired to it is pressed - see Signals.wren), then opens for good. The opening plays as an impact
// moment (ImpactEvents.wren / States/ImpactState.wren): the camera pans over, the door opens with a little shake,
// and the camera pans back. Open, it stops colliding and its sprite is hidden. Two doors on the same channels fill
// a two-tile doorway and share one moment.
// Art: Tiles/Wall Decor/Doors.png. Spawned by States/ExperimentState.wren.
import "gameObjectComponents" for Sprite, PhysicsBody, RenderLayer, WorldTransform
import "renderer" for FlipBit
import "math" for Vec2
import "resourceManager" for Texture
import "physics" for BodyDefinition, PhysicsBoxShape, BodyType, CollisionLayers
import "Colors" for WHITE, BLANK
import "Signals" for Signals
import "ImpactEvents" for ImpactEvents

// Half the door's size in metres (32 px at 32 px/m).
var HALF_SIZE = 0.5

class Door {
    // args: "Name", "Channels" - the Signals channels that must all be on to open it, comma-separated in one string
    // (script args only carry numbers, bools and strings, not lists); optional "OpenMessage" - a Localization key
    // captioning the opening.
    construct new(entity, args) {
        _entity = entity
        _name = args["Name"]
        _channelKey = args["Channels"]
        _channels = _channelKey == null ? [] : _channelKey.split(",").map {|c| c.trim() }.toList
        _openMessage = args["OpenMessage"]
        _open = false
        _requested = false   // waiting for its impact moment to play
    }

    name { _name }
    open { _open }

    start() {
        var texture = Texture.load("Tiles/Wall Decor/Doors.png")
        Sprite.create(_entity, texture, WHITE, Vec2.new(16, 16), 1, 1, FlipBit.NONE, 0, RenderLayer.ENTITIES, 0)
        texture.unload()

        var body = PhysicsBody.create(_entity, BodyDefinition.new(BodyType.STATIC_BODY),
                                      PhysicsBoxShape.new(HALF_SIZE, HALF_SIZE))
        body.setCollisionFilter(CollisionLayers.getLayer("Wall"), CollisionLayers.ALL)
    }

    update() {
        if (_open || _requested || !Signals.allOn(_channels)) {
            return
        }
        // Keyed by its channels, so the other half of a double door joins the same moment.
        _requested = true
        var position = WorldTransform.getPosition(_entity)
        ImpactEvents.request(_channelKey, Vec2.new(position.x, position.y), Fn.new { open_() }, _openMessage)
    }

    // Looked up each time - component handles can go stale.
    open_() {
        _open = true
        Sprite.get(_entity).color = BLANK
        // An empty mask collides with nothing.
        PhysicsBody.get(_entity).setCollisionFilter(CollisionLayers.getLayer("Wall"), 0)
        // Done with its plates: they go inactive and let go of anyone holding them down.
        for (channel in _channels) {
            Signals.retire(channel)
        }
    }

    onDestroy() {}

    onEvent(type, data) {}

    isInteractable() { false }

    getEntity() { _entity }
}
