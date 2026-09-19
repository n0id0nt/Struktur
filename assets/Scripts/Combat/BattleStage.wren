// Combat/BattleStage.wren
// The *physical* side of a fight - everything you can see in the world while CombatState runs the
// turn logic on top. When the player is dragged into combat we don't fight in place (a corridor
// gives no room for a 1-vs-3 line); instead the camera cuts to a separate arena and a Battler
// (Combat/Battler.wren) is staged per combatant there.
//
// The arena is the authored "Battle_Arena" level (ExperimentState.BATTLE_ARENA_LEVEL_NAME), loaded
// and activated by CombatState with every other loaded level deactivated for the duration - see
// setOtherLevelsActive_ below - so its own tiles read as the battle's backdrop with nothing
// bleeding through from the overworld. Falls back to a fixed patch of world (DEFAULT_ARENA) or an
// authored "BattleAnchor" marker entity if Battle_Arena can't be found.
import "gameObject" for GameObject
import "gameObjectComponents" for WorldTransform, Camera, World
import "math" for Vec2, Vec3
import "Combat/Battler" for Battler
import "Combat/BattlePlayer" for BattlePlayer

// Where the fight happens when there's no Battle_Arena or authored BattleAnchor - far above the
// level grid (levels occupy roughly x 0..2432, y 0..1600), so the battle camera never sees
// overworld geometry.
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

class BattleStage {
    // anchorPosition: a world-space Vec3 - the centre of the authored battle level, an authored
    // "BattleAnchor" marker's position, or null to use DEFAULT_ARENA. CombatState resolves this
    // (rather than handing over an entity) so it can centre a battle level's bounds instead of
    // anchoring on its top-left corner, which is where World/LDtk places a level's WorldTransform.
    // playerEntity: the real overworld player (hidden for the fight).
    // worldParent: entity to parent every spawned battle entity under (the world root).
    // playerCombatant: Player.combatant (persistent). enemyDefs / enemyCombatants: parallel lists,
    // one BattleCritter + its Combatant per opponent.
    // battleLevelEntity: the level entity CombatState loaded for this fight, or null when fighting
    // in place - kept so teardown() knows to deactivate it again.
    construct new(anchorPosition, playerEntity, worldParent, playerCombatant, enemyDefs, enemyCombatants,
                 battleLevelEntity) {
        _anchor = anchorPosition == null ? DEFAULT_ARENA : anchorPosition
        _playerEntity = playerEntity
        _worldParent = worldParent
        _battleLevelEntity = battleLevelEntity
        _n = enemyDefs.count
        _focus = worldOf_(CAMERA_FOCUS)

        // Hide the overworld player. Its Camera drops out of the active set with it (the camera
        // system excludes Inactive entities), leaving _camEntity below as the only camera - so we
        // don't need the camera-priority control script doesn't have yet.
        GameObject.setInactive(_playerEntity)

        // Battle_Arena is standing in for the backdrop now, so every other loaded level gets
        // deactivated for the duration - nothing else should be visible or simulating behind it.
        // Skipped entirely in the fallback (no battle level resolved) case, since then we're
        // fighting in place on top of the overworld geometry that's still needed.
        if (_battleLevelEntity != null) {
            setOtherLevelsActive_(false)
        }

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

    // Fold the arena away: destroy every battler + the camera, bring the real player and every
    // other level back. The overworld critter entities were never moved (CombatState.win() destroys
    // the beaten ones).
    teardown() {
        _player.teardown()
        for (b in _enemies) {
            b.teardown()
        }
        if (GameObject.isValid(_camEntity)) {
            GameObject.destroy(_camEntity)
        }
        GameObject.setActive(_playerEntity)

        // The player's own camera hasn't moved (the player is frozen for the whole fight), but the
        // shared camera reference is left wherever the battle camera last drew - reactivating it
        // straight into CameraSystem's damped follow (Player.wren's camera.damping) would visibly
        // lerp it back in from the arena over several frames. forcePosition is a one-shot flag -
        // CameraSystem clears it again after this one frame - so this just snaps the return, it
        // doesn't disable normal following afterwards.
        var playerCamera = Camera.get(_playerEntity)
        if (playerCamera != null) {
            playerCamera.forcePosition = true
        }

        if (_battleLevelEntity != null) {
            setOtherLevelsActive_(true)

            // Deactivate (not unload) the battle level itself so the next fight in the same arena
            // doesn't pay a reload cost - swap to World.unloadLevelEntities(...) instead if memory
            // turns out to matter.
            if (GameObject.isValid(_battleLevelEntity)) {
                GameObject.setInactive(_battleLevelEntity)
            }
        }
    }

    // --- helpers ------------------------------------------------------------

    // Sets every *other* currently-loaded level (i.e. not _battleLevelEntity) active/inactive -
    // used to hide the overworld behind the battle arena and bring it back afterwards.
    setOtherLevelsActive_(active) {
        for (i in 0...World.getLevelsCount(_worldParent)) {
            var levelEntity = World.getLoadedLevelEntity(_worldParent, i)
            if (levelEntity == null || levelEntity == _battleLevelEntity) {
                continue
            }
            if (active) {
                GameObject.setActive(levelEntity)
            } else {
                GameObject.setInactive(levelEntity)
            }
        }
    }

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
