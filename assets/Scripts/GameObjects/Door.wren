// GameObjects/Door.wren
// A one-tile (32 px) door that blocks the way until it opens, then stays open for good: it stops colliding, its
// leaves swing open (the sheet tools/door_open_sheet.py builds from the closed door), and a saved flag (openedFlag)
// keeps it open in later sessions. Two doors with the same name fill
// a two-tile doorway and open together.
// Every door can be talked to while it's closed, like a sign: its dialogue (found by its name -
// Dialogue/DialogueEntryPoints.wren) decides what happens. A plain door's dialogue opens it; a locked one's checks for
// a key and, if the player has it, takes the key and opens it - both with the "openDoor" dialogue command
// (Dialogue/RegisteredFunctions/Commands.wren). A door wired to pressure plates just hints at them.
// A door with Signals channels also opens by itself once they are all on at once (e.g. every PressurePlate wired to
// it is pressed - see World/Signals.wren). That opening plays as an impact moment (World/ImpactEvents.wren /
// States/ImpactState.wren): the camera pans over, the door opens with a little shake, and the camera pans back.
// Art: Tiles/Wall Decor/DoorsOpening.png - one row of OPEN_FRAMES frames, closed first, fully open last.
// Spawned by States/ExperimentState.wren.
import "gameObjectComponents" for Sprite, SpriteAnimation, PhysicsBody, RenderLayer, WorldTransform
import "animation" for SpriteAnimationDefinition
import "flags" for FlagManager
import "renderer" for FlipBit
import "math" for Vec2
import "resourceManager" for Texture
import "physics" for BodyDefinition, PhysicsBoxShape, BodyType, CollisionLayers
import "Util/Colors" for WHITE
import "World/Signals" for Signals
import "World/ImpactEvents" for ImpactEvents

// Half the door's size in metres (32 px at 32 px/m).
var HALF_SIZE = 0.5
// Frames in DoorsOpening.png (tools/door_open_sheet.py --frames), and how long the swing takes.
var OPEN_FRAMES = 8
var OPEN_TIME = 0.6

class Door {
    // args: "Name"; optional "Channels" - the Signals channels that must all be on to open it, comma-separated in one
    // string (script args only carry numbers, bools and strings, not lists); optional "OpenMessage" - a Localization
    // key captioning that opening.
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

    // The saved flag set once the door with this name has been opened.
    static openedFlag(name) { "%(name) Opened" }

    start() {
        var texture = Texture.load("Tiles/Wall Decor/DoorsOpening.png")
        Sprite.create(_entity, texture, WHITE, Vec2.new(16, 16), OPEN_FRAMES, 1, FlipBit.NONE, 0,
                      RenderLayer.ENTITIES, 0)
        texture.unload()
        var anim = SpriteAnimation.create(_entity)
        // endFrame is exclusive - SpriteAnimationDefinition.new(start, end, seconds, loop). A clip that doesn't loop
        // holds its last frame, so "Open" ends and stays fully open.
        anim.addAnimation("Closed", SpriteAnimationDefinition.new(0, 1, 1, false))
        anim.addAnimation("Open", SpriteAnimationDefinition.new(0, OPEN_FRAMES, OPEN_TIME, false))
        SpriteAnimation.setCurrentAnimation(_entity, "Closed")

        var body = PhysicsBody.create(_entity, BodyDefinition.new(BodyType.STATIC_BODY),
                                      PhysicsBoxShape.new(HALF_SIZE, HALF_SIZE))
        body.setCollisionFilter(CollisionLayers.getLayer("Wall"), CollisionLayers.ALL)

        // Opened in an earlier session (or before its room was streamed back in) - come back open, without the moment.
        if (FlagManager.getFlag(Door.openedFlag(_name))) {
            open_(false)
        }
    }

    update() {
        if (_open || _requested || !Signals.allOn(_channels)) {
            return
        }
        // Keyed by its channels, so the other half of a double door joins the same moment.
        _requested = true
        var position = WorldTransform.getPosition(_entity)
        ImpactEvents.request(_channelKey, Vec2.new(position.x, position.y), Fn.new { open_(true) }, _openMessage)
    }

    // Opens it for good right now - the "openDoor" dialogue command.
    openNow() {
        if (!_open) {
            open_(true)
        }
    }

    // swing: play the leaves swinging open, or (false) just show it already open. Free to walk through straight away.
    open_(swing) {
        _open = true
        if (swing) {
            SpriteAnimation.forcePlayAnimation(_entity, "Open")
        } else {
            SpriteAnimation.playAnimationAt(_entity, "Open", 1)
        }
        // Looked up each time - component handles can go stale.
        // An empty mask collides with nothing.
        PhysicsBody.get(_entity).setCollisionFilter(CollisionLayers.getLayer("Wall"), 0)
        // Done with its plates: they go inactive and let go of anyone holding them down.
        for (channel in _channels) {
            Signals.retire(channel)
        }
        FlagManager.setFlag(Door.openedFlag(_name), true)
        FlagManager.save("flags.sav")
    }

    onDestroy() {}

    onEvent(type, data) {}

    isInteractable() { !_open }

    // Only talked to when the player is facing it (Player.getInteractEntity).
    requiresFacing() { true }

    getEntity() { _entity }
}
