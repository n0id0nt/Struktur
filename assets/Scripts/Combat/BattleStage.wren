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
//
// Animation is data driven (Combat/Config/ActionAnimations.wren) and played by each Battler; this
// class is the thin layer the fight's phases talk to: it finds the right battler for a Combatant and
// forwards beginCharge / drive / react / playImpact / recover to it, and owns the formation, the camera
// and the arena entrance.
import "gameObject" for GameObject
import "gameObjectComponents" for WorldTransform, Camera, World
import "math" for Math, Vec2, Vec3
import "Combat/Battler" for Battler

// Where the fight happens when there's no Battle_Arena or authored BattleAnchor - far above the
// level grid (levels occupy roughly x 0..2432, y 0..1600), so the battle camera never sees
// overworld geometry.
var DEFAULT_ARENA = Vec3.new(1200, -1500, 0)

// Formation, as world-pixel offsets from the arena centre. +y is down. Player holds stage left
// facing right; opponents fan out stage right facing left. Sized to sit inside the battle camera's
// view (~320x180 world px at CAMERA_ZOOM 4 on a 1280x720 game) with the battle sheets' fighters at about
// 64px tall (twice the overworld size - 3dModelsToSprites job.battle.json height_px). Positions are feet.
var PLAYER_STATION = Vec2.new(-118, 34)
var PARTY_STEP     = Vec2.new(-12, 50)   // each further party member is down-and-slightly-left
var ENEMY_ORIGIN   = Vec2.new(104, 20)
var ENEMY_STEP     = Vec2.new(12, 50)    // each further opponent is down-and-slightly-right
var CAMERA_FOCUS   = Vec2.new(10, 4)     // keeps the zoom-4 view inside the 336px-wide Battle_Arena backdrop
var CAMERA_ZOOM    = 4
var CAMERA_DROP    = 90                   // camera eases down this far onto the formation during the intro
var SLIDE_DISTANCE = 110                  // how far off its mark each battler starts the slide-in

class BattleStage {
    // anchorPosition: a world-space Vec3 - the centre of the authored battle level, an authored
    // "BattleAnchor" marker's position, or null to use DEFAULT_ARENA. CombatState resolves this
    // (rather than handing over an entity) so it can centre a battle level's bounds instead of
    // anchoring on its top-left corner, which is where World/LDtk places a level's WorldTransform.
    // playerEntity: the real overworld player (hidden for the fight) - null when there is none (the battle test).
    // worldParent: entity to parent every spawned battle entity under (the world root).
    // partyCombatants / partyDefs: parallel lists for the human-controlled side, leader first - each member's
    // persistent Combatant and its look (Combat/Config/BattlePlayer for a player, a BattleCritter for an NPC).
    // enemyDefs / enemyCombatants: parallel lists, one BattleCritter + its Combatant per opponent.
    // battleLevelEntity: the level entity CombatState loaded for this fight, or null when fighting
    // in place - kept so teardown() knows to deactivate it again.
    construct new(anchorPosition, playerEntity, worldParent, partyCombatants, partyDefs, enemyDefs, enemyCombatants,
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
        if (_playerEntity != null) {
            GameObject.setInactive(_playerEntity)
        }

        // Battle_Arena is standing in for the backdrop now, so every other loaded level gets
        // deactivated for the duration - nothing else should be visible or simulating behind it.
        // Skipped entirely in the fallback (no battle level resolved) case, since then we're
        // fighting in place on top of the overworld geometry that's still needed.
        if (_battleLevelEntity != null) {
            setOtherLevelsActive_(false)
        }

        // Battlers: the party stage left (leader first, fanning down and slightly left like the opponents fan
        // down and slightly right), opponents stacked stage right centred on the anchor line.
        _party = []
        _partyHomes = []
        var partyShift = (partyCombatants.count - 1) * PARTY_STEP.y / 2
        for (i in 0...partyCombatants.count) {
            var member = Battler.new(partyDefs[i], partyCombatants[i], worldParent)
            member.awaySign = -1
            member.face(1)
            var memberHome = worldOf_(Vec2.new(PLAYER_STATION.x + i * PARTY_STEP.x,
                                               PLAYER_STATION.y + i * PARTY_STEP.y - partyShift))
            member.home = memberHome
            _party.add(member)
            _partyHomes.add(memberHome)
        }

        _enemies = []
        _enemyHomes = []
        var yShift = (_n - 1) * ENEMY_STEP.y / 2
        for (i in 0..._n) {
            var b = Battler.new(enemyDefs[i], enemyCombatants[i], worldParent)
            b.awaySign = 1
            b.face(-1)
            var home = worldOf_(Vec2.new(ENEMY_ORIGIN.x + i * ENEMY_STEP.x,
                                         ENEMY_ORIGIN.y + i * ENEMY_STEP.y - yShift))
            b.home = home
            _enemies.add(b)
            _enemyHomes.add(home)
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
    // every battler in from its wing. Safe to call every frame - it only writes transforms and forces
    // the camera.
    intro(t) {
        var e = Math.smoothstep(t)

        WorldTransform.setPosition(_camEntity, Vec3.lerp(_camStart, _focus, e))
        _camera.forcePosition = true

        for (i in 0..._party.count) {
            _party[i].slide(offBy_(_partyHomes[i], -SLIDE_DISTANCE), _partyHomes[i], e)
        }
        for (i in 0..._n) {
            _enemies[i].slide(offBy_(_enemyHomes[i], SLIDE_DISTANCE), _enemyHomes[i], e)
        }
    }

    // Everyone runs in place while the entrance slides them onto their marks.
    playEntrance() {
        for (b in _party) {
            b.playEntrance()
        }
        for (b in _enemies) {
            b.playEntrance()
        }
    }

    // The entrance is over: everyone is on their mark and stops running (CombatSession's per-frame
    // drive takes over from here - committed combatants pick their move's clip back up).
    settle() {
        intro(1)
        for (b in _party) {
            b.playIdle()
        }
        for (b in _enemies) {
            b.playIdle()
        }
    }

    // --- animation: forwarded to the combatant's Battler -------------------------------

    // A move was committed: the combatant starts playing its clip (cost = effective time units).
    beginCharge(combatant, move, cost) {
        var b = battlerFor_(combatant)
        if (b != null) {
            b.beginCharge(move, cost)
        }
    }

    // The move is gone (resolved or cancelled): back to idle.
    endCharge(combatant) {
        var b = battlerFor_(combatant)
        if (b != null) {
            b.endCharge()
        }
    }

    // Every frame, for every combatant: sync its clip + lunge to the Timeline fraction of its move.
    // targetCombatant: who the move is aimed at (null = nobody).
    drive(combatant, fraction, targetCombatant) {
        var b = battlerFor_(combatant)
        if (b == null) {
            return
        }
        var t = battlerFor_(targetCombatant)
        b.update(fraction, t == null ? null : t.home)
    }

    // Plays one of ActionAnimations.reaction(id) on the combatant ("hurt" "parry" "fumble" "defeat" "flee").
    react(combatant, id) {
        var b = battlerFor_(combatant)
        if (b != null) {
            b.react(id)
        }
    }

    // The moment a move lands (called right after its damage/heal is applied): the target's hurt / defeat reaction
    // and the move's impact particles - or, when the target's parry deflected some of it (parry: a Combat/Arbiter.wren
    // ParryResult, null with no parry), sparks on them, still a hurt if any damage got through, and a fumble on a
    // stalled attacker (it stumbles back from its lunge to its mark) - or the heal glow for a Mend. A staggered
    // target needs nothing extra: its clip is anchored to the pushed-back charge and rewinds with it. evaded: the
    // attack missed outright (Combat/Arbiter.evade) - the target just hops aside.
    playImpact(actorCombatant, targetCombatant, move, parry, evaded) {
        var a = battlerFor_(actorCombatant)
        var d = battlerFor_(targetCombatant)
        if (move.offensive && d != null && evaded) {
            d.react("evade")
        } else if (move.offensive && d != null) {
            if (parry != null && parry.deflected) {
                d.playImpact("parrySpark")
            } else {
                d.playImpact(move.particleId)
            }
            if (parry == null || parry.damageThrough > 0) {
                d.react(targetCombatant.alive ? "hurt" : "defeat")
            }
            if (parry != null && parry.stallsAttacker && a != null) {
                a.react("fumble")
            }
        } else if (move.healAmount > 0 && a != null) {
            a.playImpact("healGlow")
        }
    }

    // After a move lands the attacker eases back to its mark: needsRecover says whether it has anywhere
    // to go, beginRecover notes where it is, recover(t) (t: 0..1) moves it.
    needsRecover(combatant) {
        var b = battlerFor_(combatant)
        return b != null && b.needsRecover
    }

    beginRecover(combatant) {
        var b = battlerFor_(combatant)
        if (b != null) {
            b.beginRecover()
        }
    }

    recover(combatant, t) {
        var b = battlerFor_(combatant)
        if (b != null) {
            b.recover(t)
        }
    }

    // t: 0..1 progress of the combatant running off-stage (the flee animation).
    runOff(combatant, t) {
        var b = battlerFor_(combatant)
        if (b != null) {
            b.runOff(t)
        }
    }

    // Fold the arena away: destroy every battler + the camera, bring the real player and every
    // other level back. The overworld critter entities were never moved (CombatResolver.win() destroys
    // the beaten ones).
    teardown() {
        for (b in _party) {
            b.teardown()
        }
        for (b in _enemies) {
            b.teardown()
        }
        if (GameObject.isValid(_camEntity)) {
            GameObject.destroy(_camEntity)
        }
        if (_playerEntity == null) {
            if (_battleLevelEntity != null) {
                setOtherLevelsActive_(true)
                if (GameObject.isValid(_battleLevelEntity)) {
                    GameObject.setInactive(_battleLevelEntity)
                }
            }
            return
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
        for (b in _party) {
            if (b.combatant == combatant) {
                return b
            }
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
}
