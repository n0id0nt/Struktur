// Combat/BattleStage.wren
// The *physical* side of a fight - everything you can see in the world while CombatState runs the
// turn logic on top. When the player is dragged into combat we don't fight in place (a corridor
// gives no room for a 1-vs-3 line); instead the camera cuts to a separate arena and a Battler
// (Combat/Battler.wren) is staged per combatant there.
//
// The arena is a fixed patch of world well clear of every level (DEFAULT_ARENA below), with a plain
// backdrop drawn behind the battlers so it reads as its own space. If the ldtk world contains an
// entity with identifier "BattleAnchor" its position is used instead - drop one into an authored
// arena room to fight there and frame it however that room is built.
import "gameObject" for GameObject
import "gameObjectComponents" for WorldTransform, Sprite, Camera, RenderLayer
import "math" for Vec2, Vec3
import "resourceManager" for Texture
import "renderer" for FlipBit
import "Colors" for DARKGRAY
import "Combat/Battler" for Battler
import "Combat/BattlePlayer" for BattlePlayer

// Where the fight happens when there's no authored BattleAnchor - far above the level grid (levels
// occupy roughly x 0..2432, y 0..1600), so the battle camera never sees overworld geometry.
var DEFAULT_ARENA = Vec3.new(1200, -1500, 0)

// Formation, as world-pixel offsets from the arena centre. +y is down. Player holds stage left
// facing right; opponents fan out stage right facing left. Sized to sit inside the battle camera's
// view (~256x144 world px at CAMERA_ZOOM 5 on a 1280x720 game).
var PLAYER_STATION = Vec2.new(-108, 18)
var ENEMY_ORIGIN   = Vec2.new(92, 0)
var ENEMY_STEP     = Vec2.new(9, 58)     // each further opponent is down-and-slightly-right
var CAMERA_FOCUS   = Vec2.new(-6, 4)
var CAMERA_ZOOM    = 5
var CAMERA_DROP    = 90                   // camera eases down this far onto the formation during the intro
var SLIDE_DISTANCE = 110                  // how far off its mark each battler starts the slide-in
var BACKDROP_SCALE = Vec3.new(44, 26, 1)  // background.png is 16x16 -> ~704x416 world px

class BattleStage {
    // anchorEntity: an authored "BattleAnchor" marker, or null to use DEFAULT_ARENA.
    // playerEntity: the real overworld player (hidden for the fight).
    // worldParent: entity to parent every spawned battle entity under (the world root).
    // playerCombatant: Player.combatant (persistent). enemyDefs / enemyCombatants: parallel lists,
    // one BattleCritter + its Combatant per opponent.
    construct new(anchorEntity, playerEntity, worldParent, playerCombatant, enemyDefs, enemyCombatants) {
        _anchor = anchorEntity == null ? DEFAULT_ARENA : WorldTransform.getPosition(anchorEntity)
        _playerEntity = playerEntity
        _n = enemyDefs.count
        _focus = worldOf_(CAMERA_FOCUS)

        // Hide the overworld player. Its Camera drops out of the active set with it (the camera
        // system excludes Inactive entities), leaving _camEntity below as the only camera - so we
        // don't need the camera-priority control script doesn't have yet.
        GameObject.setInactive(_playerEntity)

        // Backdrop - a plain dark rect behind the battlers so the camera isn't staring into the void.
        _backdrop = GameObject.create("BattleBackdrop", worldParent)
        var bg = Texture.load("Tiles/Background/background.png")
        Sprite.create(_backdrop, bg, DARKGRAY, Vec2.new(8, 8), 1, 1, FlipBit.NONE, 0, RenderLayer.BACKGROUND_FAR, 0)
        bg.unload()
        WorldTransform.setPosition(_backdrop, _focus)
        WorldTransform.setScale(_backdrop, BACKDROP_SCALE)

        // Battlers: player stage left, opponents stacked stage right centred on the anchor line.
        _player = Battler.new(BattlePlayer, playerCombatant, worldParent)
        _playerHome = worldOf_(PLAYER_STATION)
        _player.face(1)

        _enemies = []
        _enemyHomes = []
        var yShift = (_n - 1) * ENEMY_STEP.y / 2
        for (i in 0..._n) {
            var b = Battler.new(enemyDefs[i], enemyCombatants[i], worldParent)
            b.face(-1)
            _enemies.add(b)
            _enemyHomes.add(worldOf_(Vec2.new(ENEMY_ORIGIN.x + i * ENEMY_STEP.x,
                                              ENEMY_ORIGIN.y + i * ENEMY_STEP.y - yShift)))
        }

        // Battle camera - its own entity. Starts a little high and eases down onto the formation.
        _camStart = Vec3.new(_focus.x, _focus.y - CAMERA_DROP, 0)
        _camEntity = GameObject.create("BattleCamera", worldParent)
        WorldTransform.setPosition(_camEntity, _camStart)
        _camera = Camera.create(_camEntity)
        _camera.zoom = CAMERA_ZOOM

        intro(0)   // drop everything onto its slide-in start mark
    }

    // t: 0..1 progress through the entrance. Eases the camera down onto the formation and slides
    // every battler in from its wing. Safe to call every frame while the field is frozen - it only
    // writes transforms and forces the camera (which ignores timescale).
    intro(t) {
        var e = smooth_(t)

        WorldTransform.setPosition(_camEntity, lerp3_(_camStart, _focus, e))
        _camera.forcePosition = true

        _player.slide(offBy_(_playerHome, -SLIDE_DISTANCE), _playerHome, e)
        for (i in 0..._n) {
            _enemies[i].slide(offBy_(_enemyHomes[i], SLIDE_DISTANCE), _enemyHomes[i], e)
        }
    }

    settle() { intro(1) }

    // The acting battler swings, the struck one recoils; call rest() once the log clears.
    strike(actorCombatant, targetCombatant) {
        var a = battlerFor_(actorCombatant)
        if (a != null) {
            a.play("attack")
        }
        var d = battlerFor_(targetCombatant)
        if (d != null) {
            d.play("hurt")
        }
    }

    rest() {
        _player.play("idle")
        for (b in _enemies) {
            b.play("idle")
        }
    }

    // Fold the arena away: destroy every battler + the backdrop + the camera, bring the real player
    // back. The overworld critter entities were never moved (CombatState.win() destroys the beaten ones).
    teardown() {
        _player.teardown()
        for (b in _enemies) {
            b.teardown()
        }
        if (GameObject.isValid(_backdrop)) {
            GameObject.destroy(_backdrop)
        }
        if (GameObject.isValid(_camEntity)) {
            GameObject.destroy(_camEntity)
        }
        GameObject.setActive(_playerEntity)
    }

    // --- helpers ------------------------------------------------------------

    battlerFor_(combatant) {
        if (combatant == null) {
            return null
        }
        if (_player.combatant == combatant) {
            return _player
        }
        for (b in _enemies) {
            if (b.combatant == combatant) {
                return b
            }
        }
        return null
    }

    worldOf_(offset) { Vec3.new(_anchor.x + offset.x, _anchor.y + offset.y, 0) }

    offBy_(pos, dx) { Vec3.new(pos.x + dx, pos.y, 0) }

    lerp3_(a, b, t) { Vec3.new(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, 0) }

    smooth_(t) {
        var x = t < 0 ? 0 : (t > 1 ? 1 : t)
        return x * x * (3 - 2 * x)
    }
}
