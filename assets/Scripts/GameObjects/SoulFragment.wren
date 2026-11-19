// GameObjects/SoulFragment.wren
// The soul fragment coming loose from the shrine's Heart (Scene 9): a pulsing sparkle of blue light trailing sparks
// (Util/SoulFx.wren), that drifts down from the crystal, wavers, then slides into the
// player's chest and is gone in a puff of sparks. Spawned by the "soulFragment" dialogue command
// (Dialogue/RegisteredFunctions/Commands.wren), which holds the conversation for as long as it takes.
// args: "FromX" / "FromY" - where it comes loose (px); "Seconds" - how long until it reaches the player.
import "gameObject" for GameObject
import "gameObjectComponents" for Sprite, WorldTransform, LocalTransform, RenderLayer
import "resourceManager" for Texture
import "renderer" for FlipBit
import "math" for Vec2, Vec3, Vec4
import "app" for Time
import "Util/SoulFx" for SoulFx

var SPARK_SHEET = "FX/Particle/Spark.png"
var SPARK_COLUMNS = 7
var SPARKLE_CELL = 3     // the big four-pointed sparkle in the spark sheet
var SCALE = 2
var CHEST_HEIGHT = 12    // px above the player's feet it sinks in at
var DRIFT_SHARE = 0.45   // the first part of the trip: a slow, wavering fall straight down
var DRIFT_DROP = 14      // px it falls in that part
var FADE_SECONDS = 1.2   // after it's in: how long the last sparks get before the entity goes

class SoulFragment {
    construct new(entity, args) {
        _entity = entity
        _from = Vec2.new(args["FromX"], args["FromY"])
        _seconds = args["Seconds"] != null ? args["Seconds"] : 2.5
        _startedAt = null
        _arrivedAt = null
    }

    start() {
        var texture = Texture.load(SPARK_SHEET)
        Sprite.create(_entity, texture, SoulFx.tint, Vec2.new(5, 4), SPARK_COLUMNS, 1, FlipBit.NONE, SPARKLE_CELL,
                      RenderLayer.FOREGROUND, 0)
        texture.unload()
        LocalTransform.setScale(_entity, Vec3.new(SCALE, SCALE, 1))
        _light = SoulFx.lightEmitter(_entity, 0)
        SoulFx.setLit(_light, 30)
        WorldTransform.setPosition(_entity, Vec3.new(_from.x, _from.y, 0))
        _startedAt = Time.scaledTime
    }

    update() {
        var now = Time.scaledTime
        if (_arrivedAt != null) {
            if (now - _arrivedAt >= FADE_SECONDS) GameObject.destroy(_entity)
            return
        }
        var progress = ((now - _startedAt) / _seconds).min(1)
        var players = GameObject.getAllWithIdentifier("Player")
        var chest = _from
        if (players.count > 0) {
            var feet = WorldTransform.getPosition(players[0])
            chest = Vec2.new(feet.x, feet.y - CHEST_HEIGHT)
        }
        var dropped = Vec2.new(_from.x, _from.y + DRIFT_DROP)
        var position = null
        if (progress < DRIFT_SHARE) {
            // Falling slowly, swaying side to side as if it isn't sure yet.
            var share = progress / DRIFT_SHARE
            position = Vec2.new(_from.x + 4 * (now * 3).sin, _from.y + DRIFT_DROP * share)
        } else {
            // Then it makes up its mind: easing in towards the chest.
            var share = (progress - DRIFT_SHARE) / (1 - DRIFT_SHARE)
            var eased = share * share * (3 - 2 * share)
            position = Vec2.new(dropped.x + (chest.x - dropped.x) * eased, dropped.y + (chest.y - dropped.y) * eased)
        }
        WorldTransform.setPosition(_entity, Vec3.new(position.x, position.y, 0))
        // It brightens and swells a little as it closes in.
        var pulse = 1 + 0.15 * (now * 8).sin
        LocalTransform.setScale(_entity, Vec3.new(SCALE * pulse, SCALE * pulse, 1))
        if (progress >= 1) arrive_()
    }

    // In: the mote goes out, a last puff of sparks, and the entity goes once they've faded.
    arrive_() {
        _arrivedAt = Time.scaledTime
        var sprite = Sprite.get(_entity)
        if (sprite != null) sprite.color = Vec4.new(0, 0, 0, 0)
        SoulFx.setLit(_light, 0)
        SoulFx.burst(_light, 16)
    }

    onDestroy() {}

    onEvent(type, data) {}

    isInteractable() { false }

    getEntity() { _entity }
}
