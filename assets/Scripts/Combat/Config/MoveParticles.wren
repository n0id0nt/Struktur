// Combat/Config/MoveParticles.wren
// Per-move impact-effect presets for the persistent ParticleEmitter every Combat/Battler.wren
// carries (Battler.playImpact). trigger(emitter, id) looks the id up in PARTICLES, reconfigures that
// ALREADY-EXISTING emitter with it - swaps its texture/color/velocity/etc. - and fires a fresh burst via
// emitter.emit(), rather than creating and destroying a one-shot emitter/entity per hit.
//
// Melee moves use Sprites/sword_slash.png, Control moves Sprites/magic.png (the same sprite
// ExperimentState's particle demo uses) - both are known-good textures.
import "math" for Vec2, Vec4
import "resourceManager" for Texture

// Sprites/sword_slash.png is a 6x6 sheet of individual tick/spark cells, not one single image -
// every sword_slash-backed preset must pass these so the emitter renders one cell (frame 0)
// instead of the whole sheet squashed into a single sprite.
var SLASH_SHEET_COLUMNS = 6
var SLASH_SHEET_ROWS = 6

// particle id -> preset. Every key is a configure_ argument of the same name.
var PARTICLES = {
    "slashSpark": {
        "texturePath": "Sprites/sword_slash.png", "columns": SLASH_SHEET_COLUMNS, "rows": SLASH_SHEET_ROWS,
        "burstCount": 10, "lifetimeMin": 0.2, "lifetimeMax": 0.3, "spawnRadius": 4,
        "velocityMin": Vec2.new(-90, -90), "velocityMax": Vec2.new(90, 90), "startScale": 0.35, "endScale": 0.05,
        "startColor": Vec4.new(255, 255, 200, 255), "endColor": Vec4.new(255, 255, 200, 0), "additive": false
    },
    "flurrySparks": {
        "texturePath": "Sprites/sword_slash.png", "columns": SLASH_SHEET_COLUMNS, "rows": SLASH_SHEET_ROWS,
        "burstCount": 18, "lifetimeMin": 0.15, "lifetimeMax": 0.25, "spawnRadius": 3,
        "velocityMin": Vec2.new(-120, -120), "velocityMax": Vec2.new(120, 120), "startScale": 0.25, "endScale": 0.03,
        "startColor": Vec4.new(200, 220, 255, 255), "endColor": Vec4.new(200, 220, 255, 0), "additive": false
    },
    "cleaveBurst": {
        "texturePath": "Sprites/sword_slash.png", "columns": SLASH_SHEET_COLUMNS, "rows": SLASH_SHEET_ROWS,
        "burstCount": 14, "lifetimeMin": 0.3, "lifetimeMax": 0.4, "spawnRadius": 6,
        "velocityMin": Vec2.new(-140, -140), "velocityMax": Vec2.new(140, 140), "startScale": 0.5, "endScale": 0.08,
        "startColor": Vec4.new(255, 150, 60, 255), "endColor": Vec4.new(255, 150, 60, 0), "additive": false
    },
    "crushShock": {
        "texturePath": "Sprites/sword_slash.png", "columns": SLASH_SHEET_COLUMNS, "rows": SLASH_SHEET_ROWS,
        "burstCount": 20, "lifetimeMin": 0.35, "lifetimeMax": 0.45, "spawnRadius": 8,
        "velocityMin": Vec2.new(-180, -180), "velocityMax": Vec2.new(180, 180), "startScale": 0.65, "endScale": 0.1,
        "startColor": Vec4.new(255, 70, 60, 255), "endColor": Vec4.new(255, 70, 60, 0), "additive": true
    },
    "onslaughtBlast": {
        "texturePath": "Sprites/sword_slash.png", "columns": SLASH_SHEET_COLUMNS, "rows": SLASH_SHEET_ROWS,
        "burstCount": 28, "lifetimeMin": 0.4, "lifetimeMax": 0.6, "spawnRadius": 10,
        "velocityMin": Vec2.new(-220, -220), "velocityMax": Vec2.new(220, 220), "startScale": 0.8, "endScale": 0.15,
        "startColor": Vec4.new(200, 40, 180, 255), "endColor": Vec4.new(200, 40, 180, 0), "additive": true
    },
    "boltSpark": {
        "texturePath": "Sprites/magic.png", "columns": 1, "rows": 1,
        "burstCount": 10, "lifetimeMin": 0.18, "lifetimeMax": 0.28, "spawnRadius": 4,
        "velocityMin": Vec2.new(-100, -160), "velocityMax": Vec2.new(100, -40), "startScale": 0.3, "endScale": 0.05,
        "startColor": Vec4.new(80, 220, 255, 255), "endColor": Vec4.new(80, 220, 255, 0), "additive": true
    },
    "slowMist": {
        "texturePath": "Sprites/magic.png", "columns": 1, "rows": 1,
        "burstCount": 8, "lifetimeMin": 0.5, "lifetimeMax": 0.7, "spawnRadius": 10,
        "velocityMin": Vec2.new(-30, -30), "velocityMax": Vec2.new(30, 30), "startScale": 0.5, "endScale": 0.2,
        "startColor": Vec4.new(150, 110, 200, 220), "endColor": Vec4.new(150, 110, 200, 0), "additive": true
    },
    // Bright burst where a parry meets the incoming blow (Combat/BattleStage.playImpact).
    "parrySpark": {
        "texturePath": "Sprites/sword_slash.png", "columns": SLASH_SHEET_COLUMNS, "rows": SLASH_SHEET_ROWS,
        "burstCount": 16, "lifetimeMin": 0.2, "lifetimeMax": 0.35, "spawnRadius": 5,
        "velocityMin": Vec2.new(-200, -200), "velocityMax": Vec2.new(200, 200), "startScale": 0.4, "endScale": 0.05,
        "startColor": Vec4.new(255, 255, 255, 255), "endColor": Vec4.new(120, 200, 255, 0), "additive": true
    },
    // Soft green motes drifting up off whoever just mended.
    "healGlow": {
        "texturePath": "Sprites/magic.png", "columns": 1, "rows": 1,
        "burstCount": 12, "lifetimeMin": 0.6, "lifetimeMax": 0.9, "spawnRadius": 12,
        "velocityMin": Vec2.new(-25, -90), "velocityMax": Vec2.new(25, -40), "startScale": 0.45, "endScale": 0.1,
        "startColor": Vec4.new(120, 255, 140, 255), "endColor": Vec4.new(120, 255, 140, 0), "additive": true
    }
}

class MoveParticles {
    static trigger(emitter, id) {
        var p = PARTICLES[id]
        if (p == null) {
            // Unrecognised / null id (non-offensive moves never call this - see
            // States/Combat/CombatResolver.wren) - no effect.
            return
        }
        MoveParticles.configure_(emitter, p["texturePath"], p["columns"], p["rows"], p["burstCount"],
                                 p["lifetimeMin"], p["lifetimeMax"], p["spawnRadius"], p["velocityMin"],
                                 p["velocityMax"], p["startScale"], p["endScale"], p["startColor"],
                                 p["endColor"], p["additive"])
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
