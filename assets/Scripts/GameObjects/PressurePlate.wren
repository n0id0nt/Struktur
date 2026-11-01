// GameObjects/PressurePlate.wren
// A floor plate that's pressed while the player or a party member stands on it, turning its Signals channel on
// (see Signals.wren - a Door wired to that channel opens), and reporting who stands on it to PressurePlates.wren.
// Detection is a physics sensor: its body is on the "Trigger" layer, which only the player and party members
// include in their collision mask (Player.wren, PartyMember.wren), so critters walk over it without pressing it.
// The sprite is the first tile of Tiles/Floors/Floors.png when up and the second when pressed.
// Once whatever it opens is open for good (its channel retired - Signals.retire, done by Door.wren) the plate goes
// inactive: it stays down, greyed out, ignores whoever steps on it, and any party member left holding it down
// (Member.staying) comes back to follow the party. Spawned by States/ExperimentState.wren.
import "gameObjectComponents" for Sprite, PhysicsBody, RenderLayer
import "renderer" for FlipBit
import "math" for Vec2, Vec4
import "resourceManager" for Texture
import "physics" for BodyDefinition, PhysicsBoxShape, BodyType, CollisionLayers
import "Colors" for WHITE
import "Signals" for Signals
import "PressurePlates" for PressurePlates
import "Party" for Party

var UP_SPRITE_INDEX = 0
var PRESSED_SPRITE_INDEX = 1
// Floors.png is a 3 x 4 grid of 16 px tiles.
var SHEET_COLUMNS = 3
var SHEET_ROWS = 4
// Tint of an inactive plate (its door is open for good) - the pressed tile, greyed out.
var INACTIVE_TINT = Vec4.new(120, 120, 120, 255)
// Half the sensor's size in metres - a bit inside the 16 px tile, so a foot has to be on it rather than brushing
// an edge.
var SENSOR_HALF_SIZE = 0.2

class PressurePlate {
    // args: "Name", "Channel" (the Signals channel it drives).
    construct new(entity, args) {
        _entity = entity
        _name = args["Name"]
        _channel = args["Channel"]
        _occupants = {}   // entity id -> true, for everyone currently standing on it
        _inactive = false
    }

    name { _name }
    channel { _channel }
    pressed { _occupants.count > 0 }
    inactive { _inactive }

    start() {
        var texture = Texture.load("Tiles/Floors/Floors.png")
        // On the floor: above the level's ground tiles (orderInLayer <= 0), below every entity.
        Sprite.create(_entity, texture, WHITE, Vec2.new(8, 8), SHEET_COLUMNS, SHEET_ROWS, FlipBit.NONE,
                      UP_SPRITE_INDEX, RenderLayer.BACKGROUND_MID, 1)
        texture.unload()

        var body = PhysicsBody.create(_entity, BodyDefinition.new(BodyType.STATIC_BODY),
                                      PhysicsBoxShape.new(SENSOR_HALF_SIZE, SENSOR_HALF_SIZE))
        body.isSensor = true
        body.setCollisionFilter(CollisionLayers.getLayer("Trigger"), CollisionLayers.getLayer("Actor"))
    }

    update() {
        if (!_inactive && Signals.isRetired(_channel)) {
            deactivate_()
        }
    }

    // Its door is open for good: stay down and greyed out, stop reporting who's on it, and send anyone who was
    // told to hold it down back to the party.
    deactivate_() {
        _inactive = true
        var sprite = Sprite.get(_entity)
        sprite.index = PRESSED_SPRITE_INDEX
        sprite.color = INACTIVE_TINT
        for (other in _occupants.keys) {
            PressurePlates.track(other, -1)
            var member = Party.memberForEntity(other)
            if (member != null && member.staying) {
                Party.setStaying(member.name, false)
            }
        }
        _occupants = {}
        Signals.set(_channel, _entity, false)
    }

    onDestroy() {
        Signals.set(_channel, _entity, false)
        for (other in _occupants.keys) {
            PressurePlates.track(other, -1)
        }
    }

    // Contact events go to every script, so only the ones involving this plate count.
    onEvent(type, data) {
        if (_inactive || (type != "collisionEnter" && type != "collisionExit")) {
            return
        }
        var other = null
        if (data["entityA"] == _entity) {
            other = data["entityB"]
        } else if (data["entityB"] == _entity) {
            other = data["entityA"]
        }
        if (other == null) {
            return
        }

        var wasPressed = pressed
        if (type == "collisionEnter") {
            if (!_occupants.containsKey(other)) {
                _occupants[other] = true
                PressurePlates.track(other, 1)
            }
        } else if (_occupants.containsKey(other)) {
            _occupants.remove(other)
            PressurePlates.track(other, -1)
        }
        if (pressed != wasPressed) {
            Sprite.get(_entity).index = pressed ? PRESSED_SPRITE_INDEX : UP_SPRITE_INDEX
            Signals.set(_channel, _entity, pressed)
        }
    }

    isInteractable() { false }

    getEntity() { _entity }
}
