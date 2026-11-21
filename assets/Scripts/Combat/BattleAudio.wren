// Combat/BattleAudio.wren
// A fight's music and sound effects: the battle song (a heavier one for a boss), a sound for what happens in the
// battle (States/CombatState.show_ hands it each engine event - hits, blocks, dodges, heals, knock-outs...), the
// command panel's menu sounds, and a jingle for how the fight ended. One per fight: made when the fight starts,
// torn down when it closes.
import "resourceManager" for Music, Sound

var SOUND_DIR = "Audio/Sounds/"
var BATTLE_SONG = "Audio/Musics/17 - Fight.ogg"
var BOSS_SONG = "Audio/Musics/34  - Fight.ogg"
var WIN_JINGLE = "Audio/Jingles/Success2.wav"
var LOSE_JINGLE = "Audio/Jingles/GameOver2.wav"

// Sound name -> file.
var SOUNDS = {
    "hit": SOUND_DIR + "Hit & Impact/Hit2.wav",
    "hitHeavy": SOUND_DIR + "Hit & Impact/Impact3.wav",
    "hitMagic": SOUND_DIR + "Magic & Skill/Magic2.wav",
    "blocked": SOUND_DIR + "Hit & Impact/Impact.wav",
    "parry": SOUND_DIR + "Whoosh & Slash/Sword2.wav",
    "miss": SOUND_DIR + "Whoosh & Slash/Whoosh.wav",
    "dodge": SOUND_DIR + "Whoosh & Slash/Whoosh2.wav",
    "soulStep": SOUND_DIR + "Magic & Skill/Spirit.wav",
    "heal": SOUND_DIR + "Magic & Skill/Heal.wav",
    "defeat": SOUND_DIR + "Hit & Impact/Impact5.wav",
    "spawn": SOUND_DIR + "Magic & Skill/Strange.wav",
    "falter": SOUND_DIR + "Menu/Cancel2.wav",
    "item": SOUND_DIR + "Bonus/PowerUp1.wav",
    "caught": SOUND_DIR + "Hit & Impact/Hit7.wav",
    "focus": SOUND_DIR + "Menu/Move1.wav",
    "accept": SOUND_DIR + "Menu/Accept2.wav",
    "cancel": SOUND_DIR + "Menu/Cancel.wav",
    "start": SOUND_DIR + "Menu/Accept6.wav"
}
var HEAVY_HIT = 30            // damage from which a hit sounds heavy
var SOUL_STEP = "Soul Step"

class BattleAudio {
    // boss: a boss fight gets the heavier song.
    construct new(boss) {
        _music = Music.load(boss ? BOSS_SONG : BATTLE_SONG)
        if (_music != null) {
            _music.setLooping(true)
            _music.play()
        }
        _sounds = {}
        for (name in SOUNDS.keys) {
            _sounds[name] = Sound.load(SOUNDS[name])
        }
        _jingle = null
    }

    // One of SOUNDS by name.
    play(name) {
        var sound = _sounds[name]
        if (sound != null) sound.play()
    }

    // A sound for one engine event (Combat/BattleEngine.takeEvents), if it has one.
    onEvent(event) {
        var type = event["type"]
        if (type == "hit") {
            var move = event["move"]
            if (event["blocked"]) {
                play("blocked")
            } else if (move != null && move.scaling == "mana") {
                play("hitMagic")
            } else {
                play(event["amount"] >= HEAVY_HIT ? "hitHeavy" : "hit")
            }
        } else if (type == "block") {
            play("parry")
        } else if (type == "miss") {
            play("miss")
        } else if (type == "dodge") {
            var who = event["who"]
            play(who != null && who.move != null && who.move.name == SOUL_STEP ? "soulStep" : "dodge")
        } else if (type == "heal") {
            play("heal")
        } else if (type == "defeat") {
            play("defeat")
        } else if (type == "spawn") {
            play("spawn")
        } else if (type == "falter") {
            play("falter")
        } else if (type == "item") {
            play("item")
        } else if (type == "caught") {
            play("caught")
        }
    }

    // The fight is over: the song stops for a jingle - result "won", "lost" or "fled" (no jingle).
    finish(result) {
        if (_music != null) _music.stop()
        var path = result == "won" ? WIN_JINGLE : (result == "lost" ? LOSE_JINGLE : null)
        if (path == null) return
        _jingle = Sound.load(path)
        if (_jingle != null) _jingle.play()
    }

    teardown() {
        if (_music != null) {
            _music.stop()
            _music.unload()
            _music = null
        }
        for (sound in _sounds.values) {
            if (sound != null) sound.unload()
        }
        _sounds = {}
        if (_jingle != null) {
            _jingle.unload()
            _jingle = null
        }
    }
}
