// GameObjects/Door.wren
// A one-tile (32 px) door that blocks the way until it opens, then stays open for good: it stops colliding, the
// block vanishes, and a saved flag (openedFlag) keeps it open in later sessions. Two doors with the same name fill
// a two-tile doorway and open together.
// Every door can be talked to while it's closed, like a sign: its dialogue (found by its name -
// Dialogue/DialogueEntryPoints.wren) decides what happens. A plain door's dialogue opens it; a locked one's checks for
// a key and, if the player has it, takes the key and opens it - both with the "openDoor" dialogue command
// (Dialogue/RegisteredFunctions/Commands.wren). A door wired to pressure plates just hints at them.
// A door with Signals channels also opens by itself once they are all on at once (e.g. every PressurePlate wired to
// it is pressed - see World/Signals.wren). That opening plays as an impact moment (World/ImpactEvents.wren /
// States/ImpactState.wren): the camera pans over, the door opens with a little shake, and the camera pans back.
// Doors placed in the level (LDtk "Door" entity) say how they open with DoorType instead:
//   PressurePad - its KeyRefs are the PressurePad entities that must all be held down at once (each plate's channel
//                 is its own iid), so it opens by itself like the Channels door above;
//   Key         - its KeyRefs are Item entities; talking to it with every one of those items in the inventory uses
//                 them up and opens it;
//   Dialogue    - talking to it opens it (or runs its dialogue, if it has a Name with a DialogueEntryPoints entry);
//   Barrier     - the other way round: open until a scene drops it (the "closeDoor" dialogue command - the wall that
//                 splits the party in the shrine), shut until "openDoor" lifts it again. Its state is the saved flag
//                 closedFlag, so it comes back the same way after a reload or with its room streamed out;
//   Hidden      - a doorway that passes for the dark beyond the room's walls until Soul Sight is on (World/SoulSight.wren):
//                 then it glows with the soul's light and can be talked to, which opens it (or runs its dialogue -
//                 the Guide apologising to the first one). It listens for the soulSight event; it never opens unseen.
// Any level door can also list story events in KeyIds (World/StoryEvents.wren - e.g. "Defeat RunicConstuct1", fired by
// the "triggerEvent" dialogue command): it stays sealed until every one has happened (and its plates, if any, are
// held), then opens by itself. Doors sharing the same KeyIds open together in one moment - a wide doorway.
// Art: the keyhole block from Backgrounds/Tilesets/TilesetDungeon.png (16 px, drawn at 2x to fill the 32 px doorway).
// There is no opening animation in the current art - an opened door simply disappears.
// Placed in the level (LDtk) - World::Level spawns it with its fields as args.
import "gameObjectComponents" for Sprite, PhysicsBody, RenderLayer, WorldTransform, LocalTransform
import "flags" for FlagManager
import "renderer" for FlipBit
import "math" for Vec2, Vec3, Vec4
import "resourceManager" for Texture
import "physics" for BodyDefinition, PhysicsBoxShape, BodyType, CollisionLayers
import "Util/Colors" for WHITE, BLANK
import "World/Signals" for Signals
import "World/ImpactEvents" for ImpactEvents
import "World/StoryEvents" for StoryEvents
import "Inventory/Inventory" for Inventory
import "Localization/DisplayNames" for DisplayNames
import "localization" for Localization
import "gameObject" for GameObject
import "app" for Time
import "World/SoulSight" for SoulSight
import "Events/GameEvents" for GameEvents
import "Util/SoulFx" for SoulFx

// Half the door's size in metres (32 px at 32 px/m).
var HALF_SIZE = 0.5
// TilesetDungeon.png is a 12 x 4 grid of 16 px tiles; the closed door is its first tile (the keyhole block).
var SHEET_COLUMNS = 12
var SHEET_ROWS = 4
var CLOSED_SPRITE_INDEX = 0
// The art is 16 px and the doorway 32 px.
var ART_SCALE = 2
// A Hidden door unseen: the keyhole block tinted to the black past the room's walls.
var HIDDEN_TINT = Vec4.new(0, 0, 0, 255)

class Door {
    // args (all optional): "Name" - its dialogue key and saved-flag name (a level door without one uses its LDtk
    // "Iid"); "Channels" - the Signals channels that must all be on to open it, comma-separated in one string (script
    // args only carry numbers, bools and strings, not lists); "OpenMessage" - a Localization key captioning that
    // opening; "DoorType" / "KeyRefs" / "KeyRefsNames" / "KeyIds" - a level door's opening rule (see the header):
    // KeyRefs is the referenced entities' iids and KeyRefsNames their Name fields (World::Level fills that in), KeyIds
    // the story events it waits for - all comma-separated. ("Keys" / "KeysNames" - the older name of KeyRefs.)
    construct new(entity, args) {
        _entity = entity
        _name = args["Name"] != null && args["Name"] != "" ? args["Name"] : args["Iid"]
        _doorType = args["DoorType"]
        _openMessage = args["OpenMessage"]
        _channelKey = args["Channels"]
        _requiredItems = []
        var keyRefs = args["KeyRefs"] != null ? args["KeyRefs"] : args["Keys"]
        var keyRefNames = args["KeyRefsNames"] != null ? args["KeyRefsNames"] : args["KeysNames"]
        if (_doorType == "PressurePad") {
            _channelKey = keyRefs
        } else if (_doorType == "Key") {
            _requiredItems = Door.splitList_(keyRefNames)
        }
        _channels = Door.splitList_(_channelKey)
        _events = Door.splitList_(args["KeyIds"])
        // What the opening moment is keyed by, so both halves of a wide doorway share one.
        _momentKey = _channels.count > 0 ? _channelKey : _events.join(",")
        _open = false
        _requested = false   // waiting for its impact moment to play
    }

    static splitList_(text) {
        if (text == null || text == "") return []
        return text.split(",").map {|part| part.trim() }.where {|part| part != "" }.toList
    }

    name { _name }
    open { _open }

    // The saved flag set once the door with this name has been opened.
    static openedFlag(name) { "%(name) Opened" }

    // The saved flag a Barrier is down while set.
    static closedFlag(name) { "%(name) Closed" }

    isBarrier { _doorType == "Barrier" }
    isHidden { _doorType == "Hidden" }

    start() {
        var texture = Texture.load("Backgrounds/Tilesets/TilesetDungeon.png")
        Sprite.create(_entity, texture, WHITE, Vec2.new(8, 8), SHEET_COLUMNS, SHEET_ROWS, FlipBit.NONE,
                      CLOSED_SPRITE_INDEX, RenderLayer.ENTITIES, 0)
        texture.unload()
        LocalTransform.setScale(_entity, Vec3.new(ART_SCALE, ART_SCALE, 1))

        var body = PhysicsBody.create(_entity, BodyDefinition.new(BodyType.STATIC_BODY),
                                      PhysicsBoxShape.new(HALF_SIZE, HALF_SIZE))
        body.setCollisionFilter(CollisionLayers.getLayer("Wall"), CollisionLayers.ALL)

        if (isBarrier) {
            if (!FlagManager.getFlag(Door.closedFlag(_name))) setPassable_(true)
            return
        }
        if (isHidden) {
            _seen = false
            _shimmer = null
            Sprite.get(_entity).color = HIDDEN_TINT
        }
        // Opened in an earlier session (or before its room was streamed back in) - come back open, without the moment.
        if (FlagManager.getFlag(Door.openedFlag(_name))) {
            open_(false)
        } else if (_channels.count == 0 && _events.count > 0 && StoryEvents.allHappened(_events)) {
            // Its story events happened while its room wasn't loaded - it's simply open now.
            open_(false)
        }
        if (isHidden && !_open && SoulSight.active) reveal_(true)
    }

    // A Hidden door: Soul Sight came on (glow, and it can be talked to) or went off (back to the dark).
    reveal_(seen) {
        _seen = seen
        if (_open) return
        if (seen) {
            // Its sparks sit on a child, so they go when it's destroyed; the block itself glows (update).
            if (_shimmer == null) {
                _shimmer = SoulFx.lightEmitter(_entity, 0)
                SoulFx.setLit(_shimmer, 6)
            }
        } else {
            dropShimmer_()
            Sprite.get(_entity).color = HIDDEN_TINT
        }
    }

    dropShimmer_() {
        if (_shimmer != null && GameObject.isValid(_shimmer)) GameObject.destroy(_shimmer)
        _shimmer = null
    }

    // True once everything it waits for is done: its plates held (if it has any) and its story events happened (if it
    // lists any). A door that waits for neither never opens by itself.
    ready_() {
        if (_channels.count == 0 && _events.count == 0) return false
        if (_channels.count > 0 && !Signals.allOn(_channels)) return false
        return StoryEvents.allHappened(_events)
    }

    update() {
        // A seen Hidden door breathes: its block glows in the soul's colour, swelling and fading.
        if (_shimmer != null) {
            var glow = 0.75 + 0.25 * (Time.scaledTime * 3).sin
            var tint = SoulFx.tint
            Sprite.get(_entity).color = Vec4.new((tint.x * glow).round, (tint.y * glow).round, (tint.z * glow).round, 255)
        }
        if (_open || _requested || !ready_()) {
            return
        }
        // Keyed by its channels / events, so the other half of a double door joins the same moment.
        _requested = true
        var position = WorldTransform.getPosition(_entity)
        ImpactEvents.request(_momentKey, Vec2.new(position.x, position.y), Fn.new { open_(true) }, _openMessage)
    }

    // Opens it for good right now - the "openDoor" dialogue command. A Barrier lifts (the command clears its flag).
    openNow() {
        if (isBarrier) {
            setPassable_(true)
        } else if (!_open) {
            open_(true)
        }
    }

    // Drops a Barrier - the "closeDoor" dialogue command (which saves its flag). Other doors never close.
    closeNow() {
        if (isBarrier) setPassable_(false)
    }

    // Shows / hides the block and turns its collision off / on. Looked up each time - component handles can go stale.
    setPassable_(passable) {
        _open = passable
        var sprite = Sprite.get(_entity)
        if (sprite) {
            sprite.color = passable ? BLANK : WHITE
        }
        // An empty mask collides with nothing.
        PhysicsBody.get(_entity).setCollisionFilter(CollisionLayers.getLayer("Wall"), passable ? 0 : CollisionLayers.ALL)
    }

    // swing: opened just now (true) or already open from an earlier session (false) - both look the same with the
    // current art, which has no opening animation. Free to walk through straight away.
    open_(swing) {
        setPassable_(true)
        dropShimmer_()
        // Done with its plates: they go inactive and let go of anyone holding them down.
        for (channel in _channels) {
            Signals.retire(channel)
        }
        FlagManager.setFlag(Door.openedFlag(_name), true)
        FlagManager.save("flags.sav")
    }

    // Talked to with no dialogue of its own (States/InteractState.wren): open it if its rule allows. Returns the line
    // to show, or null.
    interact() {
        if (_open) return null
        if (isBarrier) return Localization.get("interact.door.sealed")
        if (isHidden) {
            open_(true)
            return null
        }
        if (_doorType == "Key") {
            var missing = _requiredItems.where {|item| !Inventory.contains(item) }.toList
            if (missing.count > 0 || _requiredItems.count == 0) {
                var needed = missing.map {|item| DisplayNames.itemDisplay(item) }.join(", ")
                if (needed == "") return Localization.get("interact.door.locked")
                return Localization.get("interact.door.locked_needs").replace("{items}", needed)
            }
            for (item in _requiredItems) {
                Inventory.removeItem(item)
            }
            Inventory.save("inventory.sav")
            open_(true)
            return Localization.get("interact.door.unlocked")
        }
        if (_channels.count > 0 || _events.count > 0) {
            return Localization.get("interact.door.sealed")
        }
        open_(true)
        return null
    }

    onDestroy() {}

    onEvent(type, data) {
        if (isHidden && type == GameEvents.soulSightType) reveal_(SoulSight.fromEvent(data))
    }

    // An unseen Hidden door is just the dark - nothing to talk to.
    isInteractable() { !_open && (!isHidden || _seen) }

    // Only talked to when the player is facing it (Player.getInteractEntity).
    requiresFacing() { true }

    getEntity() { _entity }
}
