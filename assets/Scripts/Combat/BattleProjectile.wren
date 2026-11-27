// Combat/BattleProjectile.wren
// A shot crossing the battle grid (Combat/BattleStage.wren): when a ranged move starts - one aimed "first" (the
// nearest foe in the lane: bows, the crossbow, throwing knives, spells) - a projectile leaves the shooter and flies
// down the lane to the cell its move hits, arriving at the move's impact time, then disappears. Purely visual: the hit
// itself is the engine's (Combat/BattleEngine.wren).
import "gameObject" for GameObject
import "gameObjectComponents" for WorldTransform, Sprite, RenderLayer
import "math" for Vec2, Vec3
import "resourceManager" for Texture
import "renderer" for FlipBit
import "Util/Colors" for WHITE

// The art a move fires, by its weapon tags / scaling (all drawn pointing east).
var ARROW = "FX/Projectile/Arrow.png"
var KNIFE = "FX/Projectile/Kunai.png"
var SPELL = "FX/Projectile/EnergyBall.png"   // 4 frames of 16 x 16
var CHEST_HEIGHT = 12                        // px above the feet the shot flies at
var DRAW_ORDER = 1000                        // over the battlers

class BattleProjectile {
    // Whether `move` fires something visible.
    static fires(move) { move != null && move.isAttack && move.aim == "first" }

    // from / to: world positions of the shooter's and the target cell's feet; seconds: how long until the hit.
    construct new(move, parent, from, to, seconds) {
        _from = Vec3.new(from.x, from.y - CHEST_HEIGHT, 0)
        _to = Vec3.new(to.x, to.y - CHEST_HEIGHT, 0)
        _seconds = seconds.max(0.05)
        _age = 0
        _entity = GameObject.create("BattleProjectile", parent)
        var path = BattleProjectile.artFor_(move)
        var texture = Texture.load(path)
        var columns = path == SPELL ? 4 : 1
        var size = Vec2.new(texture.width / columns, texture.height)
        var flip = _to.x < _from.x ? FlipBit.HORIZONTAL : FlipBit.NONE
        Sprite.create(_entity, texture, WHITE, Vec2.new(size.x / 2, size.y / 2), columns, 1, flip, 0, RenderLayer.ENTITIES, DRAW_ORDER)
        texture.unload()
        WorldTransform.setPosition(_entity, _from)
    }

    static artFor_(move) {
        if (move.scaling == "mana") return SPELL
        var tags = move.weapons
        if (tags != null && tags.contains("dagger")) return KNIFE
        return ARROW
    }

    // Advances it; false once it has arrived (and is gone).
    update(dt) {
        if (_entity == null) return false
        _age = _age + dt
        var t = (_age / _seconds).min(1)
        WorldTransform.setPosition(_entity, Vec3.new(_from.x + (_to.x - _from.x) * t, _from.y + (_to.y - _from.y) * t, 0))
        if (t >= 1) {
            teardown()
            return false
        }
        return true
    }

    teardown() {
        if (_entity != null && GameObject.isValid(_entity)) GameObject.destroy(_entity)
        _entity = null
    }
}
