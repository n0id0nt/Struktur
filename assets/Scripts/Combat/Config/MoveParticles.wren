// Combat/Config/MoveParticles.wren
// Per-move impact-effect presets for the persistent ParticleEmitter every Combat/Battler.wren
// carries (Battler.playImpact). trigger(emitter, id) looks the id up in PARTICLES, reconfigures that
// ALREADY-EXISTING emitter with it - swaps its texture/color/velocity/etc. - and fires a fresh burst via
// emitter.emit(), rather than creating and destroying a one-shot emitter/entity per hit.
//
// Melee moves use Actors/sword_slash.png, Control moves Actors/magic.png (the same sprite
// the old overworld particle demo used) - both are known-good textures.
//
// A preset can animate its particles: "animation" is [startFrame, endFrame (exclusive), seconds, loop] - a
// SpriteAnimationDefinition - and "animationMode" a ParticleAnimationMode (NONE / ONCE / LOOP / LIFETIME; left out,
// it's LOOP or ONCE from the definition's loop flag). A preset without one draws cell 0, unanimated.
//
// hitCloud(emitter, element, strength) is the puff of dust every hit throws up (Combat/Battler.playHitCloud - its
// own emitter, so it never restyles sparks still in flight): HIT_CLOUD's six-cell smoke sheet (a neutral copy of
// FX/Smoke/Smoke) animated over each particle's life (LIFETIME - it swells, then breaks up, however long that particle
// lives), tinted by the blow's element (ELEMENT_TINTS - plain dust for none) and scaled by how hard the hit was.
import "math" for Vec2, Vec4
import "resourceManager" for Texture
import "animation" for SpriteAnimationDefinition
import "gameObjectComponents" for ParticleAnimationMode

// Actors/sword_slash.png is a 6x6 sheet of individual tick/spark cells, not one single image -
// every sword_slash-backed preset must pass these so the emitter renders one cell (frame 0)
// instead of the whole sheet squashed into a single sprite.
var SLASH_SHEET_COLUMNS = 6
var SLASH_SHEET_ROWS = 6

var SMOKE_SHEET = "FX/Smoke/Smoke/SpriteSheetWhite.png"   // 6 cells of 32x32, one row - a neutral grey copy of
                                                           // SpriteSheet.png (that one is green-grey, which turned warm tints olive)
var SMOKE_COLUMNS = 6

// element -> the cloud's colour (null = no element: pale dust). Alpha is the puff's opacity at birth.
var ELEMENT_TINTS = {
    "Flame": Vec4.new(255, 75, 35, 240),     // hot red - stands out from the orange arena dirt
    "Frost": Vec4.new(150, 225, 255, 235),
    "Storm": Vec4.new(150, 105, 255, 240),   // electric violet, so it never reads as Gold
    "Iron": Vec4.new(160, 172, 190, 240),    // steel grey-blue rather than earth brown
    "Wild": Vec4.new(130, 225, 110, 235),
    "Light": Vec4.new(255, 248, 175, 240),   // warm cream, brighter than plain dust
    "Gold": Vec4.new(255, 205, 50, 240)
}
// The hit cloud's animation: all six smoke cells, tied to each particle's life (the 1 s is unused in LIFETIME).
var HIT_CLOUD_ANIMATION = [0, SMOKE_COLUMNS, 1.0, false]

var DUST_TINT = Vec4.new(205, 198, 188, 225)   // cool grey-beige, so it never reads as Light

// particle id -> preset. Every key is a configure_ argument of the same name.
var PARTICLES = {
    "slashSpark": {
        "texturePath": "Actors/sword_slash.png", "columns": SLASH_SHEET_COLUMNS, "rows": SLASH_SHEET_ROWS,
        "burstCount": 10, "lifetimeMin": 0.2, "lifetimeMax": 0.3, "spawnRadius": 4,
        "velocityMin": Vec2.new(-90, -90), "velocityMax": Vec2.new(90, 90), "startScale": 0.35, "endScale": 0.05,
        "startColor": Vec4.new(255, 255, 200, 255), "endColor": Vec4.new(255, 255, 200, 0), "additive": false
    },
    "flurrySparks": {
        "texturePath": "Actors/sword_slash.png", "columns": SLASH_SHEET_COLUMNS, "rows": SLASH_SHEET_ROWS,
        "burstCount": 18, "lifetimeMin": 0.15, "lifetimeMax": 0.25, "spawnRadius": 3,
        "velocityMin": Vec2.new(-120, -120), "velocityMax": Vec2.new(120, 120), "startScale": 0.25, "endScale": 0.03,
        "startColor": Vec4.new(200, 220, 255, 255), "endColor": Vec4.new(200, 220, 255, 0), "additive": false
    },
    "cleaveBurst": {
        "texturePath": "Actors/sword_slash.png", "columns": SLASH_SHEET_COLUMNS, "rows": SLASH_SHEET_ROWS,
        "burstCount": 14, "lifetimeMin": 0.3, "lifetimeMax": 0.4, "spawnRadius": 6,
        "velocityMin": Vec2.new(-140, -140), "velocityMax": Vec2.new(140, 140), "startScale": 0.5, "endScale": 0.08,
        "startColor": Vec4.new(255, 150, 60, 255), "endColor": Vec4.new(255, 150, 60, 0), "additive": false
    },
    "crushShock": {
        "texturePath": "Actors/sword_slash.png", "columns": SLASH_SHEET_COLUMNS, "rows": SLASH_SHEET_ROWS,
        "burstCount": 20, "lifetimeMin": 0.35, "lifetimeMax": 0.45, "spawnRadius": 8,
        "velocityMin": Vec2.new(-180, -180), "velocityMax": Vec2.new(180, 180), "startScale": 0.65, "endScale": 0.1,
        "startColor": Vec4.new(255, 70, 60, 255), "endColor": Vec4.new(255, 70, 60, 0), "additive": true
    },
    "onslaughtBlast": {
        "texturePath": "Actors/sword_slash.png", "columns": SLASH_SHEET_COLUMNS, "rows": SLASH_SHEET_ROWS,
        "burstCount": 28, "lifetimeMin": 0.4, "lifetimeMax": 0.6, "spawnRadius": 10,
        "velocityMin": Vec2.new(-220, -220), "velocityMax": Vec2.new(220, 220), "startScale": 0.8, "endScale": 0.15,
        "startColor": Vec4.new(200, 40, 180, 255), "endColor": Vec4.new(200, 40, 180, 0), "additive": true
    },
    "boltSpark": {
        "texturePath": "Actors/magic.png", "columns": 1, "rows": 1,
        "burstCount": 10, "lifetimeMin": 0.18, "lifetimeMax": 0.28, "spawnRadius": 4,
        "velocityMin": Vec2.new(-100, -160), "velocityMax": Vec2.new(100, -40), "startScale": 0.3, "endScale": 0.05,
        "startColor": Vec4.new(80, 220, 255, 255), "endColor": Vec4.new(80, 220, 255, 0), "additive": true
    },
    "slowMist": {
        "texturePath": "Actors/magic.png", "columns": 1, "rows": 1,
        "burstCount": 8, "lifetimeMin": 0.5, "lifetimeMax": 0.7, "spawnRadius": 10,
        "velocityMin": Vec2.new(-30, -30), "velocityMax": Vec2.new(30, 30), "startScale": 0.5, "endScale": 0.2,
        "startColor": Vec4.new(150, 110, 200, 220), "endColor": Vec4.new(150, 110, 200, 0), "additive": true
    },
    // Bright burst where a parry meets the incoming blow (Combat/BattleStage.playImpact).
    "parrySpark": {
        "texturePath": "Actors/sword_slash.png", "columns": SLASH_SHEET_COLUMNS, "rows": SLASH_SHEET_ROWS,
        "burstCount": 16, "lifetimeMin": 0.2, "lifetimeMax": 0.35, "spawnRadius": 5,
        "velocityMin": Vec2.new(-200, -200), "velocityMax": Vec2.new(200, 200), "startScale": 0.4, "endScale": 0.05,
        "startColor": Vec4.new(255, 255, 255, 255), "endColor": Vec4.new(120, 200, 255, 0), "additive": true
    },
    // Soft green motes drifting up off whoever just mended.
    "healGlow": {
        "texturePath": "Actors/magic.png", "columns": 1, "rows": 1,
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
        MoveParticles.animate_(emitter, p["animation"], p["animationMode"])
        emitter.emit()
    }

    // Applies a preset's animation ([start, end, seconds, loop] and an optional ParticleAnimationMode), or clears it.
    static animate_(emitter, animation, mode) {
        if (animation == null) {
            emitter.clearAnimation()
            return
        }
        var definition = SpriteAnimationDefinition.new(animation[0], animation[1], animation[2], animation[3])
        if (mode == null) {
            emitter.setAnimation(definition)
        } else {
            emitter.setAnimation(definition, mode)
        }
    }

    // The colour a hit cloud takes for `element` (one of Combat/Gear.wren's ELEMENTS, or null for plain dust).
    static tint(element) {
        var tint = element == null ? null : ELEMENT_TINTS[element]
        return tint == null ? DUST_TINT : tint
    }

    // A cloud of puffs where a hit lands. strength 0..1: how hard it was (a blocked or glancing hit is a small cloud,
    // a heavy blow a big one).
    static hitCloud(emitter, element, strength) {
        var s = strength < 0 ? 0 : (strength > 1 ? 1 : strength)
        var tint = MoveParticles.tint(element)
        var count = 8 + (s * 6).round
        var speed = 20 + s * 26
        MoveParticles.configure_(emitter, SMOKE_SHEET, SMOKE_COLUMNS, 1, count, 0.55, 0.85, 6 + s * 4,
            Vec2.new(-speed, -speed * 0.7), Vec2.new(speed, speed * 0.5), 0.65 + s * 0.25, 1.15 + s * 0.4,
            tint, Vec4.new(tint.x, tint.y, tint.z, 0), false)
        // A slight drift upward as it clears, and only a gentle spin - smoke, not sparks.
        emitter.acceleration = Vec2.new(0, -10)
        emitter.rotationSpeedMin = -1.5
        emitter.rotationSpeedMax = 1.5
        MoveParticles.animate_(emitter, HIT_CLOUD_ANIMATION, ParticleAnimationMode.LIFETIME)
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
