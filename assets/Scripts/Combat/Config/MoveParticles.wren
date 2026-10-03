// Combat/Config/MoveParticles.wren
// Per-move impact-effect presets for the persistent ParticleEmitter every Combat/Battler.wren
// carries (Battler.playImpact). trigger(emitter, id) reconfigures that ALREADY-EXISTING emitter -
// swaps its texture/color/velocity/etc. and fires a fresh burst via emitter.emit() - rather than
// creating and destroying a one-shot emitter/entity per hit.
//
// Melee moves reuse Sprites/sword_slash.png, Control moves reuse Sprites/magic.png (the same
// sprite ExperimentState's particle demo already uses) - both are known-good textures.
import "math" for Vec2, Vec4
import "resourceManager" for Texture

// Sprites/sword_slash.png is a 6x6 sheet of individual tick/spark cells, not one single image -
// every sword_slash-backed config below must pass these so the emitter renders one cell (frame 0)
// instead of the whole sheet squashed into a single sprite.
var SLASH_SHEET_COLUMNS = 6
var SLASH_SHEET_ROWS = 6

class MoveParticles {
    static trigger(emitter, id) {
        if (id == "slashSpark") {
            configure_(emitter, "Sprites/sword_slash.png", SLASH_SHEET_COLUMNS, SLASH_SHEET_ROWS,
                      10, 0.2, 0.3, 4,
                      Vec2.new(-90, -90), Vec2.new(90, 90), 0.35, 0.05,
                      Vec4.new(255, 255, 200, 255), Vec4.new(255, 255, 200, 0), false)
        } else if (id == "flurrySparks") {
            configure_(emitter, "Sprites/sword_slash.png", SLASH_SHEET_COLUMNS, SLASH_SHEET_ROWS,
                      18, 0.15, 0.25, 3,
                      Vec2.new(-120, -120), Vec2.new(120, 120), 0.25, 0.03,
                      Vec4.new(200, 220, 255, 255), Vec4.new(200, 220, 255, 0), false)
        } else if (id == "cleaveBurst") {
            configure_(emitter, "Sprites/sword_slash.png", SLASH_SHEET_COLUMNS, SLASH_SHEET_ROWS,
                      14, 0.3, 0.4, 6,
                      Vec2.new(-140, -140), Vec2.new(140, 140), 0.5, 0.08,
                      Vec4.new(255, 150, 60, 255), Vec4.new(255, 150, 60, 0), false)
        } else if (id == "crushShock") {
            configure_(emitter, "Sprites/sword_slash.png", SLASH_SHEET_COLUMNS, SLASH_SHEET_ROWS,
                      20, 0.35, 0.45, 8,
                      Vec2.new(-180, -180), Vec2.new(180, 180), 0.65, 0.1,
                      Vec4.new(255, 70, 60, 255), Vec4.new(255, 70, 60, 0), true)
        } else if (id == "onslaughtBlast") {
            configure_(emitter, "Sprites/sword_slash.png", SLASH_SHEET_COLUMNS, SLASH_SHEET_ROWS,
                      28, 0.4, 0.6, 10,
                      Vec2.new(-220, -220), Vec2.new(220, 220), 0.8, 0.15,
                      Vec4.new(200, 40, 180, 255), Vec4.new(200, 40, 180, 0), true)
        } else if (id == "boltSpark") {
            configure_(emitter, "Sprites/magic.png", 1, 1,
                      10, 0.18, 0.28, 4,
                      Vec2.new(-100, -160), Vec2.new(100, -40), 0.3, 0.05,
                      Vec4.new(80, 220, 255, 255), Vec4.new(80, 220, 255, 0), true)
        } else if (id == "slowMist") {
            configure_(emitter, "Sprites/magic.png", 1, 1,
                      8, 0.5, 0.7, 10,
                      Vec2.new(-30, -30), Vec2.new(30, 30), 0.5, 0.2,
                      Vec4.new(150, 110, 200, 220), Vec4.new(150, 110, 200, 0), true)
        } else {
            // Unrecognised / null id (non-offensive moves never call this - see
            // States/Combat/CombatResolver.wren) - no effect.
            return
        }
        emitter.emit()
    }

    static configure_(emitter, texturePath, columns, rows, burstCount, lifetimeMin, lifetimeMax,
                      spawnRadius, velocityMin, velocityMax, startScale, endScale, startColor,
                      endColor, additive) {
        var texture = Texture.load(texturePath)
        emitter.texture = texture
        texture.unload()

        // The emitter is persistent and reused across moves with different sheet layouts, so these
        // must be set every trigger - never assume the previous move's columns/rows still apply.
        emitter.columns = columns
        emitter.rows = rows

        emitter.burstCount = burstCount
        emitter.maxParticles = burstCount
        emitter.spawnRadius = spawnRadius
        emitter.lifetimeMin = lifetimeMin
        emitter.lifetimeMax = lifetimeMax
        emitter.velocityMin = velocityMin
        emitter.velocityMax = velocityMax
        emitter.acceleration = Vec2.new(0, 0)
        emitter.startScale = startScale
        emitter.endScale = endScale
        emitter.startColor = startColor
        emitter.endColor = endColor
        emitter.rotationSpeedMin = -6.0
        emitter.rotationSpeedMax = 6.0
        emitter.additive = additive
    }
}
