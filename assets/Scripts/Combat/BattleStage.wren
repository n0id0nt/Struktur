// Combat/BattleStage.wren
// The *physical* side of a fight - everything you can see in the world while CombatState runs the
// turn logic on top. When the player is dragged into combat we don't fight in place (a corridor
// gives no room for a 1-vs-3 line); instead we cut to a dedicated arena the designer authored in
// the ldtk world and stage a Battler (Combat/Battler.wren) per combatant there.
//
// ---------------------------------------------------------------------------------------------
// LEVEL CONTRACT - author this in assets/Levels/SevenGods.ldtk:
//   * one extra level, placed anywhere in the world grid (ideally not touching Level_0..3 so the
//     intro camera sweep doesn't clip through them). Give it ground / collision tiles as you like.
//   * one entity in it with identifier  BattleAnchor  near the middle. The whole formation is
//     built as fixed offsets from this point and the battle camera frames it - move the anchor (or
//     redraw the room around it) to reframe, no code change.
// With no BattleAnchor entity present, CombatState falls back to the old fight-in-place dim
// overlay, so this is safe to ship before the level exists.
// ---------------------------------------------------------------------------------------------
import "gameObject" for GameObject
import "gameObjectComponents" for WorldTransform, Camera
import "math" for Vec2, Vec3
import "Combat/Battler" for Battler
import "Combat/BattlePlayer" for BattlePlayer

// Formation, as world-pixel offsets from the BattleAnchor. +y is down. Player holds stage left
// facing right; opponents fan out stage right facing left. Camera frames a point between them.
var PLAYER_STATION = Vec2.new(-150, 24)
var ENEMY_ORIGIN   = Vec2.new(120, 0)
var ENEMY_STEP     = Vec2.new(12, 74)   // each further opponent is down-and-slightly-right
var CAMERA_FOCUS   = Vec2.new(-12, 0)
var CAMERA_ZOOM    = 4.5
var SLIDE_DISTANCE = 90                  // how far off its mark each battler starts the slide-in

class BattleStage {
    // anchorEntity: the BattleAnchor marker in the arena level. cameraStartPos: Vec3, where the
    // camera begins the sweep (the player's overworld position). playerEntity: the real overworld
    // player (hidden for the fight). playerCombatant: Player.combatant (persistent). enemyDefs /
    // enemyCombatants: parallel lists, one BattleCritter + its Combatant per opponent.
    construct new(anchorEntity, cameraStartPos, playerEntity, playerCombatant, enemyDefs, enemyCombatants) {
        _anchor = WorldTransform.getPosition(anchorEntity)
        _playerEntity = playerEntity

        // Hide the overworld player. Its Camera drops out of the active set with it (the camera
        // system excludes Inactive entities), leaving _camEntity below as the only camera - so we
        // don't need the camera-priority control script doesn't have yet.
        GameObject.setInactive(_playerEntity)

        _n = enemyDefs.count

        // Battlers: player stage left, opponents stacked stage right centred on the anchor line.
        _player = Battler.new(BattlePlayer, playerCombatant, anchorEntity)
        _playerHome = worldOf_(PLAYER_STATION)
        _player.face(1)

        _enemies = []
        _enemyHomes = []
        var yShift = (_n - 1) * ENEMY_STEP.y / 2
        for (i in 0..._n) {
            var b = Battler.new(enemyDefs[i], enemyCombatants[i], anchorEntity)
            b.face(-1)
            _enemies.add(b)
            _enemyHomes.add(worldOf_(Vec2.new(ENEMY_ORIGIN.x + i * ENEMY_STEP.x,
                                              ENEMY_ORIGIN.y + i * ENEMY_STEP.y - yShift)))
        }

        // Battle camera - its own entity, framed on CAMERA_FOCUS, starts where the player stood.
        _focus = worldOf_(CAMERA_FOCUS)
        _camStart = cameraStartPos
        _camEntity = GameObject.create("BattleCamera", anchorEntity)
        WorldTransform.setPosition(_camEntity, cameraStartPos)
        _camera = Camera.create(_camEntity)
        _camera.zoom = CAMERA_ZOOM

        intro(0)   // drop everything onto its slide-in start mark
    }

    // t: 0..1 progress through the entrance. Eases the camera from where the player stood to the
    // formation and slides every battler in from its wing. Safe to call every frame while the field
    // is frozen - it only writes transforms and forces the camera (which ignores timescale).
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

    // Fold the arena away: destroy every battler + the camera, bring the real player back. The
    // overworld critter entities were never moved (CombatState.win() destroys the defeated ones).
    teardown() {
        _player.teardown()
        for (b in _enemies) {
            b.teardown()
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
