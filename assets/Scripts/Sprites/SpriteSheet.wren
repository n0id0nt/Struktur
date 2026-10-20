// Sprites/SpriteSheet.wren - creates a sprite and its animations from the data in a generated
// Sprites/<Character>Sprite.wren file (written by 3dModelsToSprites/tools/pack_sheets.py, copied here by
// build_sprites.bat). Hand-written; the <Character>Sprite.wren files next to it are generated.
import "gameObjectComponents" for Sprite, SpriteAnimation, RenderLayer
import "renderer" for FlipBit
import "math" for Vec2
import "resourceManager" for Texture
import "animation" for SpriteAnimationDefinition
import "Colors" for WHITE

class SpriteSheet {
    // Adds the sprite and its animations to an entity, on the entities layer.
    static create(entity, data) { SpriteSheet.create(entity, data, RenderLayer.ENTITIES) }

    // Same, on the given render layer. Returns the entity's SpriteAnimation.
    static create(entity, data, layer) {
        var texture = Texture.load(data["texture"])
        var origin = data["origin"]
        Sprite.create(entity, texture, WHITE, Vec2.new(origin[0], origin[1]), data["columns"], data["rows"], FlipBit.NONE, 0, layer, 0)
        texture.unload()

        var spriteAnimation = SpriteAnimation.create(entity)
        for (anim in data["animations"]) {
            // anim = [name, first frame, end frame (not included), seconds, loops]
            spriteAnimation.addAnimation(anim[0], SpriteAnimationDefinition.new(anim[1], anim[2], anim[3], anim[4]))
        }
        SpriteAnimation.setCurrentAnimation(entity, data["defaultAnimation"])
        return spriteAnimation
    }

    // Names of every animation in the data, for menus or debugging.
    static animationNames(data) { data["animations"].map {|anim| anim[0] }.toList }
}
