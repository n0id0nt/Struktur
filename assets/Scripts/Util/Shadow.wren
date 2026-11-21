// Util/Shadow.wren
// A soft round shadow on the ground under a character in the world (Actors/Character/Shadow.png) - the player
// (GameObjects/Player.wren), NPCs (GameObjects/NPC.wren) and party followers (GameObjects/PartyMember.wren). A child
// entity, so it follows its owner and hides / goes with it (a story-gated NPC turned off turns its shadow off too).
import "gameObject" for GameObject
import "gameObjectComponents" for Sprite, LocalTransform, RenderLayer
import "math" for Vec2, Vec3, Vec4
import "resourceManager" for Texture
import "renderer" for FlipBit

var SHADOW_TEXTURE = "Actors/Character/Shadow.png"
var SHADOW_PIVOT = Vec2.new(6, 3.5)     // the image's centre (12 x 7 px) - centred on the owner's feet
var SHADOW_TINT = Vec4.new(255, 255, 255, 200)
// On the floor: above the level's ground tiles (orderInLayer <= 0) and pressure plates (1), under every entity.
var SHADOW_ORDER = 2

class Shadow {
    // Adds a shadow under `entity`. Returns the shadow's entity.
    static attach(entity) {
        var shadow = GameObject.create("Shadow", entity)
        LocalTransform.setPosition(shadow, Vec3.new(0, 0, 0))
        var texture = Texture.load(SHADOW_TEXTURE)
        Sprite.create(shadow, texture, SHADOW_TINT, SHADOW_PIVOT, 1, 1, FlipBit.NONE, 0, RenderLayer.BACKGROUND_MID,
                      SHADOW_ORDER)
        texture.unload()
        return shadow
    }
}
