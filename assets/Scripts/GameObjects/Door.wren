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
//   PressurePad - its Keys are the PressurePad entities that must all be held down at once (each plate's channel is
//                 its own iid), so it opens by itself like the Channels door above;
//   Key         - its Keys are Item entities; talking to it with every one of those items in the inventory uses
//                 them up and opens it;
//   Dialogue    - talking to it opens it (or runs its dialogue, if it has a Name with a DialogueEntryPoints entry).
// Art: the keyhole block from Backgrounds/Tilesets/TilesetDungeon.png (16 px, drawn at 2x to fill the 32 px doorway).
// There is no opening animation in the current art - an opened door simply disappears.
// Placed in the level (LDtk) - World::Level spawns it with its fields as args.
import "gameObjectComponents" for Sprite, PhysicsBody, RenderLayer, WorldTransform, LocalTransform
import "flags" for FlagManager
import "renderer" for FlipBit
import "math" for Vec2, Vec3
import "resourceManager" for Texture
import "physics" for BodyDefinition, PhysicsBoxShape, BodyType, CollisionLayers
import "Util/Colors" for WHITE, BLANK
import "World/Signals" for Signals
import "World/ImpactEvents" for ImpactEvents
import "Inventory/Inventory" for Inventory
import "Localization/DisplayNames" for DisplayNames
import "localization" for Localization

// Half the door's size in metres (32 px at 32 px/m).
var HALF_SIZE = 0.5
// TilesetDungeon.png is a 12 x 4 grid of 16 px tiles; the closed door is its first tile (the keyhole block).
var SHEET_COLUMNS = 12
var SHEET_ROWS = 4
var CLOSED_SPRITE_INDEX = 0
// The art is 16 px and the doorway 32 px.
var ART_SCALE = 2

class Door {
    // args (all optional): "Name" - its dialogue key and saved-flag name (a level door without one uses its LDtk
    // "Iid"); "Channels" - the Signals channels that must all be on to open it, comma-separated in one string (script
    // args only carry numbers, bools and strings, not lists); "OpenMessage" - a Localization key captioning that
    // opening; "DoorType" / "Keys" / "KeysNames" - a level door's opening rule (see the header): Keys is the
    // referenced entities' iids and KeysNames their Name fields (World::Level fills that in), both comma-separated.
    construct new(entity, args) {
        _entity = entity
        _name = args["Name"] != null && args["Name"] != "" ? args["Name"] : args["Iid"]
        _doorType = args["DoorType"]
        _openMessage = args["OpenMessage"]
        _channelKey = args["Channels"]
        _requiredItems = []
        if (_doorType == "PressurePad") {
            _channelKey = args["Keys"]
        } else if (_doorType == "Key") {
            _requiredItems = Door.splitList_(args["KeysNames"])
        }
        _channels = Door.splitList_(_channelKey)
        _open = false
        _requested = false   // waiting for its impact moment to play
    }

    static splitList_(text) {
        if (text == null || text == "") return []
        return text.split(",").map {|c| c.trim() }.where {|c| c != "" }.toList
    }

    name { _name }
    open { _open }

    // The saved flag set once the door with this name has been opened.
    static openedFlag(name) { "%(name) Opened" }

    start() {
        var texture = Texture.load("Backgrounds/Tilesets/TilesetDungeon.png")
        Sprite.create(_entity, texture, WHITE, Vec2.new(8, 8), SHEET_COLUMNS, SHEET_ROWS, FlipBit.NONE,
                      CLOSED_SPRITE_INDEX, RenderLayer.ENTITIES, 0)
        texture.unload()
        LocalTransform.setScale(_entity, Vec3.new(ART_SCALE, ART_SCALE, 1))

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

    // swing: opened just now (true) or already open from an earlier session (false) - both look the same with the
    // current art, which has no opening animation. Free to walk through straight away.
    open_(swing) {
        _open = true
        var sprite = Sprite.get(_entity)
        if (sprite) {
            sprite.color = BLANK
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

    // Talked to with no dialogue of its own (States/InteractState.wren): open it if its rule allows. Returns the line
    // to show, or null.
    interact() {
        if (_open) return null
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
        if (_channels.count > 0) {
            return Localization.get("interact.door.sealed")
        }
        open_(true)
        return null
    }

    onDestroy() {}

    onEvent(type, data) {}

    isInteractable() { !_open }

    // Only talked to when the player is facing it (Player.getInteractEntity).
    requiresFacing() { true }

    getEntity() { _entity }
}
