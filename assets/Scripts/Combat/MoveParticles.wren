// Combat/MoveParticles.wren
// Per-move impact bursts (Combat/BattleStage.wren's spawnImpact_). spawn(entity, id) wires up a
// one-shot ParticleEmitter on an already-positioned entity and returns how long (seconds) the
// caller should wait before destroying it - there's no engine-side auto-destroy for particle
// emitters (only ParticleEmitter.aliveCount is queryable), so lifetime-based cleanup is the
// caller's job (see BattleStage's _activeEffects/pruneEffects_).
//
// Melee moves reuse Sprites/sword_slash.png, Control moves reuse Sprites/magic.png (the same
// sprite ExperimentState's particle demo already uses) - both are known-good textures, which
// matters because ParticleEmitter.create aborts the fiber on a bad texture path.
import "gameObjectComponents" for ParticleEmitter
import "math" for Vec2, Vec4
import "resourceManager" for Texture

class MoveParticles {
    // one-shot burst: looping = false means emissionRate is never consumed (only the burst fires),
    // so it's left at its default.
    static spawn(entity, id) {
        if (id == "slashSpark") {
            return burst_(entity, "Sprites/sword_slash.png", 10, 0.2, 0.3, 4,
                         Vec2.new(-90, -90), Vec2.new(90, 90), 0.35, 0.05,
                         Vec4.new(255, 255, 200, 255), Vec4.new(255, 255, 200, 0), false)
        }
        if (id == "flurrySparks") {
            return burst_(entity, "Sprites/sword_slash.png", 18, 0.15, 0.25, 3,
                         Vec2.new(-120, -120), Vec2.new(120, 120), 0.25, 0.03,
                         Vec4.new(200, 220, 255, 255), Vec4.new(200, 220, 255, 0), false)
        }
        if (id == "cleaveBurst") {
            return burst_(entity, "Sprites/sword_slash.png", 14, 0.3, 0.4, 6,
                         Vec2.new(-140, -140), Vec2.new(140, 140), 0.5, 0.08,
                         Vec4.new(255, 150, 60, 255), Vec4.new(255, 150, 60, 0), false)
        }
        if (id == "crushShock") {
            return burst_(entity, "Sprites/sword_slash.png", 20, 0.35, 0.45, 8,
                         Vec2.new(-180, -180), Vec2.new(180, 180), 0.65, 0.1,
                         Vec4.new(255, 70, 60, 255), Vec4.new(255, 70, 60, 0), true)
        }
        if (id == "onslaughtBlast") {
            return burst_(entity, "Sprites/sword_slash.png", 28, 0.4, 0.6, 10,
                         Vec2.new(-220, -220), Vec2.new(220, 220), 0.8, 0.15,
                         Vec4.new(200, 40, 180, 255), Vec4.new(200, 40, 180, 0), true)
        }
        if (id == "boltSpark") {
            return burst_(entity, "Sprites/magic.png", 10, 0.18, 0.28, 4,
                         Vec2.new(-100, -160), Vec2.new(100, -40), 0.3, 0.05,
                         Vec4.new(80, 220, 255, 255), Vec4.new(80, 220, 255, 0), true)
        }
        if (id == "slowMist") {
            return burst_(entity, "Sprites/magic.png", 8, 0.5, 0.7, 10,
                         Vec2.new(-30, -30), Vec2.new(30, 30), 0.5, 0.2,
                         Vec4.new(150, 110, 200, 220), Vec4.new(150, 110, 200, 0), true)
        }
        // Unrecognised id - no visual effect, but still return a lifetime so callers that don't
        // special-case null keep working.
        return 0
    }

    static burst_(entity, texturePath, burstCount, lifetimeMin, lifetimeMax, spawnRadius,
                  velocityMin, velocityMax, startScale, endScale, startColor, endColor, additive) {
        var texture = Texture.load(texturePath)
        var emitter = ParticleEmitter.create(entity, texture)
        texture.unload()

        emitter.looping = false
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

        return lifetimeMax
    }
}
