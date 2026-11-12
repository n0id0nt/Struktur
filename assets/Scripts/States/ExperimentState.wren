// states/ExperimentState.wren
// Main gameplay state - loads level, creates entities, manages gameplay sub-states. This is the
// live gameplay path (Main.wren starts this, not GameWorldState). Owns the world/level streaming,
// critter spawning, combat-trigger detection, the nested sub-StateManager (PlayState/
// InventoryState/InteractState/GameOverState/CombatState), and the UI/Gameplay Fight!/Interact prompts.
import "gameObject" for GameObject
import "gameObjectComponents" for WorldTransform, World, Script, ParticleEmitter
import "math" for Vec2, Vec3, Vec4
import "resourceManager" for Texture, Music
import "input" for Input
import "flags" for FlagManager
import "app" for Time, Application

import "States/BaseState" for BaseState
import "States/StateManager" for StateManager
import "States/PlayState" for PlayState
import "States/InventoryState" for InventoryState
import "States/InteractState" for InteractState
import "States/GameOverState" for GameOverState
import "States/CombatState" for CombatState
import "States/ImpactState" for ImpactState
import "UI/Gameplay/FightPromptUI" for FightPromptUI
import "UI/Gameplay/InteractPromptUI" for InteractPromptUI

import "GameObjects/Critter" for Critter
import "Party/Party" for Party
import "Party/DuelRequest" for DuelRequest
import "Combat/CombatOutcome" for CombatOutcome
import "Camera/RoomCamera" for RoomCamera
import "World/Rooms" for Rooms
import "World/Signals" for Signals
import "World/PressurePlates" for PressurePlates
import "World/ImpactEvents" for ImpactEvents
import "World/Warps" for Warps
import "debug" for Debug

// Placeholder art for the rest spot (a heart - it heals the party) until there is campfire art.
var REST_SPOT_SPRITE = "Items/Potion/Heart.png"

// Debug input (F6 / controller Select, see InputConfig.json) that opens the battle test sandbox
// (States/BattleTestState.wren) from the overworld.
var BATTLE_TEST_INPUT = "DebugBattleTest"

// Where the player starts - and where they wake up after the party is wiped (respawn_).
var SPAWN_POSITION = Vec3.new(208.0, 208.0, 0.0)
// Gap between followers lined up behind the player on a respawn.
var RESPAWN_FOLLOWER_SPACING = 24.0
// How far from the DoorWorp you arrive next to (towards the room's centre), so you don't land back on it.
var WARP_ARRIVAL_OFFSET = 24.0
// Frames to wait for the target room's DoorWorps to start (their scripts run a frame after the room loads) before
// giving up on finding the one that leads back and arriving at the room's centre.
var WARP_MAX_WAIT_FRAMES = 10
// Seconds after arriving during which stepping on a DoorWorp does nothing - so being nudged out of a wall onto the
// doorway you came through (or a follower bumping you) doesn't bounce you straight back.
var WARP_COOLDOWN = 0.75

var WORLD_FILE_PATH = "Levels/SevenGods.ldtk"
// Authored arena level (see Combat/BattleStage.wren) - every fight loads this level and stages the
// battle there instead of fighting in place. World.getLevelIndex returns null if it's ever renamed
// or removed, and CombatState falls back to the BattleAnchor/DEFAULT_ARENA behaviour in that case.
var BATTLE_ARENA_LEVEL_NAME = "BattleStage"
// Combat proximity ranges, in metres (converted to world pixels at use via Application.pixelsPerMeter,
// default 64). An aggressive critter (Chinlin) within FORCE range forces combat outright rather than
// just offering it via the interact prompt - meant to feel like being caught, not spotted. When a
// fight starts, every other critter within GROUP range OF THE PLAYER is dragged in too, nearest
// first, up to MAX_COMBAT_GROUP total (the closest 3 when more than that are in range).
var FORCE_COMBAT_METERS = 1.0
var GROUP_COMBAT_METERS = 3.0
var MAX_COMBAT_GROUP = 3
// Int flag (Dialogue/OverworldDialogue.json) tracking the villager's fetch quest: 0 = not asked, 1 = looking for
// the Lost Locket, 2 = done (the villager has joined the party).
var VILLAGER_QUEST_FLAG = "villager_quest_state"

class ExperimentState is BaseState {
    // Where a run starts the player: the editor level viewer's "Play from here" spot if it was picked in this world
    // (Application.debugSpawn), otherwise SPAWN_POSITION.
    static startPosition {
        var debugSpawn = Application.debugSpawn
        if (debugSpawn != null && debugSpawn[0] == WORLD_FILE_PATH) {
            return Vec3.new(debugSpawn[1], debugSpawn[2], 0.0)
        }
        return SPAWN_POSITION
    }

    construct new() {
        super()
        name = "ExperimentState"

        _view = null
        _talkView = null   // "Interact" prompt for NPCs / signposts (_view is the "Fight!" prompt for critters)
        _worldEntity = null //TODO Create an entity constant for invalid entity or null entity
        _particleEntity = null
        _playerEntity = null
        // The level the player was last found in, per updateLevelStreaming_() - starts null so the
        // very first streaming pass always runs (see the == check there).
        _currentLevelIndex = null
        _roomCamera = null   // keeps the player's camera inside _currentLevelIndex (Camera/RoomCamera.wren)
        _stateManager = StateManager.new()

        _stateManager.insertState("PlayState", PlayState)
        _stateManager.insertState("InventoryState", InventoryState)
        _stateManager.insertState("InteractState", InteractState)
        _stateManager.insertState("GameOverState", GameOverState)
        _stateManager.insertState("CombatState", CombatState)
        _stateManager.insertState("ImpactState", ImpactState)
        _gameMusic = null
        // Grace window after a fight ends (see update()) - long enough to run clear of an aggressive
        // critter before its proximity check can re-trigger combat. Covers every outcome: after a
        // loss or a flee the enemy is still alive and still next to you, and without this you'd be
        // stuck re-fighting it in place.
        _combatCooldownEndTime = 0
        _warp = null   // a warp under way - see startWarp_
        _warpCooldownEndTime = 0
    }

    enter(stateManager, params) {
        super.enter(stateManager, params)

        Debug.info("Loading game world...")

        _gameMusic = Music.load("Audio/Musics/33 - Calm Village.ogg")
        if (_gameMusic) {
            _gameMusic.setLooping(true)
            _gameMusic.play()
        }

        var worldEntity = World.createWorldEntity(WORLD_FILE_PATH)
        _worldEntity = worldEntity
        Rooms.world = worldEntity

        var playerEntity = GameObject.create("Player", worldEntity)
        Script.createArg(playerEntity, "Player", {"Name": "Player"})
        WorldTransform.setPosition(playerEntity, ExperimentState.startPosition)
        _playerEntity = playerEntity
        // The player leads the party - NPCs who join follow the member ahead of them (see Party/Party.wren).
        Party.registerPlayer(playerEntity, "Player")
        _roomCamera = RoomCamera.new(playerEntity)

        // Streams in the level the player spawned in (plus its neighbours) instead of loading every
        // level in the world up front - see updateLevelStreaming_().
        updateLevelStreaming_()

        // "Fight" prompt for non-aggressive critters - same UILabel-above-target pattern
        // UI/Gameplay/InteractPromptUI.wren (GameWorldState's "Interact" prompt) uses.
        _view = FightPromptUI.new("Fight!")
        _talkView = InteractPromptUI.new("Interact")

        spawnParticleDemo(worldEntity)

        // Two villagers and two signposts ~100px from where the player spawns (placeholder sprites; their
        // dialogue is Dialogue/OverworldDialogue.json). Walk up to one and press Interact - see
        // checkCombatTriggers().
        // The villager's fetch quest (Lost Locket) ends with them joining the party - if that's already
        // happened (saved flag) they're in the party (Party.load restored it; the join is a no-op safety net),
        // not standing here. The Duelist only stands here until you beat them and they join.
        if (FlagManager.getIntFlag(VILLAGER_QUEST_FLAG) == 2) {
            Party.join("Villager")
        }
        if (!Party.contains("Villager")) {
            spawnNpc("Villager", Vec3.new(108.0, 208.0, 0.0), worldEntity)
        }
        if (!Party.contains("Duelist")) {
            spawnNpc("Duelist", Vec3.new(308.0, 108.0, 0.0), worldEntity)
        }
        spawnNpc("Traveler", Vec3.new(308.0, 208.0, 0.0), worldEntity)
        spawnSign("Welcome Sign", Vec3.new(208.0, 108.0, 0.0), worldEntity)
        spawnSign("Warning Sign", Vec3.new(208.0, 308.0, 0.0), worldEntity)
        // A rest spot: interacting runs its dialogue's "restParty" command - the party is healed and anyone downed
        // gets back up (Party.restAll).
        spawnSign("Rest Spot", Vec3.new(120.0, 108.0, 0.0), worldEntity, REST_SPOT_SPRITE)

        // Doors, pressure plates and items are placed in the level (LDtk) - World::Level spawns them with their scripts.
    }

    // name is both the NPC's id (GameObjects/NPC.wren's sprite table, the dialogue entry point and speaker
    // name) and the entity identifier.
    spawnNpc(npcName, position, worldEntity) {
        var entity = GameObject.create(npcName, worldEntity)
        Script.createArg(entity, "NPC", {"Name": npcName})
        WorldTransform.setPosition(entity, position)
    }

    // Keeps the world in step with Party.members: every NPC in the party gets a follower entity
    // (GameObjects/PartyMember.wren), spawned where they joined (or just behind whoever they trail) - so
    // recruiting someone is just Party.join(), wherever it happens (dialogue, a won duel, a loaded save).
    // Cheap enough to run every frame.
    syncPartyFollowers() {
        for (member in Party.members) {
            if (member.isPlayer) {
                continue
            }
            if (member.entity != null && GameObject.isValid(member.entity)) {
                continue
            }
            spawnFollower_(member)
        }
    }

    spawnFollower_(member) {
        var position = member.spawnPosition
        if (position == null) {
            var ahead = Party.ahead(member)
            var aheadPosition = ahead == null ? null : Party.positionOf(ahead)
            if (aheadPosition == null) {
                aheadPosition = WorldTransform.getPosition(_playerEntity)
            }
            position = Vec3.new(aheadPosition.x - 24.0, aheadPosition.y, 0.0)
        }
        var entity = GameObject.create("PartyMember", _worldEntity)
        Script.createArg(entity, "PartyMember", {"Name": member.name})
        WorldTransform.setPosition(entity, position)
        member.entity = entity
        member.spawnPosition = null
        member.trail.clear()
    }

    // A signpost (GameObjects/Sign.wren) - name is its dialogue entry point key.
    spawnSign(signName, position, worldEntity) { spawnSign(signName, position, worldEntity, null) }

    // spritePath: an image to show instead of the sign's default (null keeps it).
    spawnSign(signName, position, worldEntity, spritePath) {
        var entity = GameObject.create(signName, worldEntity)
        var args = {"Name": signName}
        if (spritePath != null) {
            args["Sprite"] = spritePath
        }
        Script.createArg(entity, "Sign", args)
        WorldTransform.setPosition(entity, position)
    }

    // Demonstrates Component::ParticleEmitter: a looping "magic fountain" that sprays glowing motes
    // upward (screen -y) and lets a constant downward acceleration arc them back down. The emitter
    // simulates at this entity's world position, so it just needs a Transform (added by
    // GameObject.create) - no Sprite required.
    spawnParticleDemo(worldEntity) {
        _particleEntity = GameObject.create("ParticleDemo", worldEntity)
        WorldTransform.setPosition(_particleEntity, Vec3.new(600.0, 220.0, 0.0))

        // Actors/magic.png is a single 60x60 frame - leave columns/rows at their 1x1 default so each
        // particle draws the whole texture. The emitter keeps its own reference to the texture, so the
        // script's handle can be released immediately afterwards (same pattern as Sprite.create).
        var texture = Texture.load("Actors/magic.png")
        var emitter = ParticleEmitter.create(_particleEntity, texture)
        texture.unload()

        emitter.looping      = true
        emitter.emissionRate = 45.0                       // motes per second
        emitter.maxParticles = 250                        // pool capacity
        emitter.spawnRadius  = 8.0                        // spawn within a small disc, not a single point

        emitter.lifetimeMin  = 0.7
        emitter.lifetimeMax  = 1.4

        emitter.velocityMin  = Vec2.new(-70.0, -230.0)    // up-and-out (screen space: -y is up)
        emitter.velocityMax  = Vec2.new(70.0, -120.0)
        emitter.acceleration = Vec2.new(0.0, 260.0)       // gravity pulls them back down into an arc

        emitter.startColor   = Vec4.new(120.0, 180.0, 255.0, 255.0)  // bright blue
        emitter.endColor     = Vec4.new(255.0, 120.0, 220.0, 0.0)    // fade out to pink
        emitter.startScale   = 0.6
        emitter.endScale     = 0.05                       // shrink to nothing over the lifetime

        emitter.rotationSpeedMin = -6.0                   // radians/sec - lazy tumble
        emitter.rotationSpeedMax = 6.0

        emitter.additive     = true                       // glow - additive blend, no depth sort needed

        // One-shot puff on spawn, on top of the continuous stream, so the fountain "pops" into life.
        emitter.burstCount   = 40
    }

    // Mirrors update(stateManager)'s own substate delegation below - GameWorldState is the one real
    // nested-subStateManager case today, so this doubles as the proof that fixed-update delegation works through
    // a substate, not just at the leaf level.
    fixedUpdate(stateManager) {
        if (_stateManager.currentState) {
            _stateManager.fixedUpdate()
        } else {
            // Out in the world: record where every party member walks this step, so followers (whose own
            // fixedUpdate runs right after this) can retrace their leader's route.
            Party.tick()
        }
    }

    update(stateManager) {
        // Before the substate check, so a room slide finishes even if a dialogue or menu opens partway through.
        _roomCamera.update()

        // if substate return out here
        if (_stateManager.currentState) {
            var wasCombat = _stateManager.currentState.name == "CombatState"
            var wasInteract = _stateManager.currentState.name == "InteractState"
            _stateManager.update()
            if (wasCombat && !_stateManager.currentState) {
                _combatCooldownEndTime = Time.scaledTime + 2.0
                onCombatEnded_(CombatOutcome.take())
            }
            if (wasInteract && !_stateManager.currentState) {
                // A dialogue ended - if it was a challenge ("startDuel" command), the fight starts now.
                var challenger = DuelRequest.take()
                if (challenger != null) {
                    startDuel(challenger)
                }
            }
            return
        }
        if (_gameMusic && !_gameMusic.isPlaying()) {
            _gameMusic.play()
        }

        syncPartyFollowers()

        // A DoorWorp the player stepped on (GameObjects/DoorWorp.wren) - load its room, then move the party there.
        var warp = Warps.take()
        if (warp != null && _warp == null && Time.scaledTime >= _warpCooldownEndTime) {
            startWarp_(warp[0], warp[1], warp[2])
        }
        if (_warp != null) {
            updateWarp_()
        }

        updateLevelStreaming_()

        // Something in the world wants its moment (a door opening - see World/ImpactEvents.wren): play it now, while
        // nothing else (dialogue, a fight, a menu) is running.
        var impact = ImpactEvents.take()
        if (impact != null) {
            playImpact_(impact)
            return
        }

        if (Input.isInputJustReleased("Inventory")) {
            openInventory()
            return
        }

        // Debug: F6 / Select opens the battle test sandbox (States/BattleTestState.wren); its Back button returns here.
        if (Input.isInputJustPressed(BATTLE_TEST_INPUT)) {
            stateManager.changeState("BattleTest")
            return
        }

        if (Time.scaledTime >= _combatCooldownEndTime) {
            checkCombatTriggers()
        }
    }

    // The Inventory key (States/InventoryState.wren shows what the player has picked up and closes itself
    // on the same key, handing control back here). The player is stopped and the game music paused for
    // the duration, like for dialogue.
    openInventory() {
        _view.hide()
        _talkView.hide()
        for (entity in GameObject.getAllWithIdentifier("Player")) {
            var script = Script.getInstance(entity)
            if (script) {
                script.playerForceStop()
            }
        }
        if (_gameMusic) {
            _gameMusic.stop()
        }
        _stateManager.changeState("InventoryState")
    }

    // Keeps only the level the player is currently standing in, plus its immediate LDtk neighbours,
    // loaded - everything else gets unloaded to free its tiles/collision/entities. Cheap to call every
    // frame: the streaming work only runs when the player's current level actually changes, and
    // Rooms.indexAt checks world bounds straight from the parsed LDtk metadata rather than
    // needing every level loaded to know where they are.
    updateLevelStreaming_() {
        var levelIndex = Rooms.indexAt(WorldTransform.getPosition(_playerEntity))
        if (levelIndex == null || levelIndex == _currentLevelIndex) {
            return
        }
        _currentLevelIndex = levelIndex
        // The first room (spawn) is snapped to; every room after that is slid to.
        _roomCamera.moveTo(levelIndex)

        var wanted = [levelIndex]
        for (neighbour in World.getLevelNeighbours(_worldEntity, levelIndex)) {
            wanted.add(neighbour[0])
        }

        // The battle arena isn't part of the walkable world and never shows up in `wanted`, but
        // CombatState deliberately leaves it loaded-but-inactive between fights to skip the reload -
        // don't undo that here.
        var battleArenaIndex = World.getLevelIndex(_worldEntity, BATTLE_ARENA_LEVEL_NAME)

        for (i in 0...World.getLevelsCount(_worldEntity)) {
            if (wanted.contains(i) || i == battleArenaIndex) {
                continue
            }
            if (World.getLoadedLevelEntity(_worldEntity, i) != null) {
                World.unloadLevelEntities(_worldEntity, i)
            }
        }

        for (i in wanted) {
            var levelEntity = World.getLoadedLevelEntity(_worldEntity, i)
            if (levelEntity == null) {
                levelEntity = World.loadLevelEntities(_worldEntity, i)
                GameObject.setParent(levelEntity, _worldEntity)
            }
            GameObject.setActive(levelEntity)
        }
    }

    // Aggressive critters (Chinlin) force the player straight into CombatState once they close the
    // distance - no prompt, no choice. Non-aggressive critters (Chicken) just become another
    // interactable, reusing the exact scan/prompt every Item/NPC/Door already uses
    // (Player.getInteractEntity(), see GameWorldState for the original of this pattern) - the player
    // has to press Interact to start that fight.
    // NPCs and signposts get the "Interact" prompt instead and open dialogue (startDialogue).
    checkCombatTriggers() {
        var playerEntities = GameObject.getAllWithIdentifier("Player")
        for (playerEntity in playerEntities) {
            var playerScript = Script.getInstance(playerEntity)
            if (!playerScript) {
                continue
            }
            var playerPosition = WorldTransform.getPosition(playerEntity)
            var forceRange = FORCE_COMBAT_METERS * Application.pixelsPerMeter
            var playerRoom = Rooms.indexAt(playerPosition)

            for (critter in Critter.all) {
                // Only critters of the room the player is in - not one on the far side of a wall in the next room.
                if (!critter.aggressive || !sharesRoom_(critter, playerRoom)) {
                    continue
                }
                var critterPosition = critter.position
                if (critterPosition && Vec3.distance(playerPosition, critterPosition) <= forceRange) {
                    _view.hide()
                    enterCombat(critter.entity, playerEntity)
                    return
                }
            }

            var interactEntity = playerScript.getInteractEntity()
            var interactScript = interactEntity ? Script.getInstance(interactEntity) : null
            if (interactScript is Critter) {
                _talkView.hide()
                _view.showAbove(WorldTransform.getPosition(interactEntity))
                if (Input.isInputJustReleased("Interact")) {
                    _view.hide()
                    enterCombat(interactEntity, playerEntity)
                    return
                }
            } else if (interactScript != null) {
                // Any other interactable (NPC, signpost, item): talk / read / pick up. Not an `is NPC` check -
                // registered scripts live in their own module instance, so `is` only works for shared base
                // classes like Critter. Player.getInteractEntity() already only returns real interactables.
                _view.hide()
                _talkView.showAbove(WorldTransform.getPosition(interactEntity))
                if (Input.isInputJustReleased("Interact")) {
                    _talkView.hide()
                    startDialogue(interactEntity, playerScript)
                    return
                }
            } else {
                _view.hide()
                _talkView.hide()
            }
        }
    }

    // Talks to an NPC / reads a signpost / picks up an item: InteractState (shared with GameWorldState) runs the dialogue for
    // the interactable's name (Dialogue/DialogueEntryPoints.wren) and clears itself when it ends, handing
    // control back here. The game music stops for the duration, as it does for a fight.
    startDialogue(interactEntity, playerScript) {
        playerScript.playerForceStop()
        if (_gameMusic) {
            _gameMusic.stop()
        }
        _stateManager.changeState("InteractState", {"interactingEntity": interactEntity})
    }

    // Pans the camera to the event, plays it with a shake and pans back, with the players held still
    // (States/ImpactState.wren).
    playImpact_(event) {
        _view.hide()
        _talkView.hide()
        _stateManager.changeState("ImpactState", {
            "event": event,
            "roomCamera": _roomCamera,
            "cameraEntity": _playerEntity
        })
    }

    enterCombat(primaryEntity, playerEntity) {
        beginCombat_(combatGroup(primaryEntity, playerEntity), playerEntity, false)
    }

    // A duel: just the challenger (an NPC - see Dialogue/OverworldDialogue.json's "startDuel"), no critters
    // dragged in. Beating a recruitable NPC adds them to the party (States/Combat/CombatResolver.win).
    startDuel(npcName) {
        var challengers = GameObject.getAllWithIdentifier(npcName)
        if (challengers.count > 0) {
            beginCombat_([challengers[0]], _playerEntity, true)
        }
    }

    // A fight has closed. outcome: the Combat/CombatOutcome it left (null if it was aborted before starting).
    // The fallen are already gone from the world (CombatResolver.removeFallen); what a loss means - game over,
    // or nothing for a duel - hooks in here.
    onCombatEnded_(outcome) {
        if (outcome == null) {
            return
        }
        Debug.info("[ExperimentState] combat ended: %(outcome)")
        if (outcome.duel) {
            // Duels are non-lethal: anyone knocked out (you or an NPC) is back up on 1 HP, win or lose, and a lost
            // duel just ends - the challenger is still standing there to be challenged again.
            Party.reviveAllFallen()
        } else if (!outcome.lost) {
            Party.reviveLeaders()
        }
        // Wounds (and anyone downed) carry across a restart.
        Party.save("party.sav")
        if (outcome.lost && !outcome.duel) {
            gameOver_()
        }
    }

    // The whole party fell in a real fight: the "You fell" screen (States/GameOverState.wren, game time paused);
    // its Continue button respawns.
    gameOver_() {
        _view.hide()
        _talkView.hide()
        if (_gameMusic) {
            _gameMusic.stop()
        }
        _stateManager.changeState("GameOverState", {
            "title": "You fell",
            "continueText": "Continue",
            "onContinue": Fn.new { respawn_() }
        })
    }

    // Back on your feet at the start point: the whole party fully healed (downed members too - Party.restAll),
    // followers lined up behind you, their trails wiped so nobody walks back to where you fell. Defeated
    // enemies stay gone (Combat/Defeated.wren); survivors keep their wounds until their level unloads, and streaming
    // swaps levels as normal on the next update. The usual post-fight cooldown applies.
    respawn_() {
        Party.restAll()
        teleportParty_(SPAWN_POSITION)
        _combatCooldownEndTime = Time.scaledTime + 2.0
        Debug.info("[ExperimentState] respawned at the start point")
    }

    // Moves the player to position and lines the followers up behind them, their trails wiped so nobody walks back
    // to where they were. A teleport, not a walk - the camera lands in the new room's view rather than sliding there.
    teleportParty_(position) {
        WorldTransform.setPosition(_playerEntity, position)
        _roomCamera.snapTo(Rooms.indexAt(position))
        var player = Script.getInstance(_playerEntity)
        if (player != null) {
            player.playerForceStop()
        }
        var behind = 0
        for (member in Party.members) {
            member.trail.clear()
            if (!member.isPlayer && member.entity != null && GameObject.isValid(member.entity)) {
                behind = behind + 1
                WorldTransform.setPosition(member.entity,
                    Vec3.new(position.x - RESPAWN_FOLLOWER_SPACING * behind, position.y, 0.0))
            }
        }
    }

    // --- warps (GameObjects/DoorWorp.wren) -----------------------------------
    // The target room is loaded first so the DoorWorp leading back can be found; the party moves once it has
    // started (updateWarp_), a frame or two later.
    startWarp_(targetIndex, fromIndex, destination) {
        Debug.info("[ExperimentState] warping from room %(fromIndex) to room %(targetIndex)")
        World.loadLevelEntities(_worldEntity, targetIndex)
        _warp = {"target": targetIndex, "from": fromIndex, "destination": destination, "frames": 0}
        var player = Script.getInstance(_playerEntity)
        if (player != null) {
            player.playerForceStop()
        }
    }

    updateWarp_() {
        _warp["frames"] = _warp["frames"] + 1
        var arrival = warpArrival_(_warp["target"], _warp["from"], _warp["destination"],
                                   _warp["frames"] >= WARP_MAX_WAIT_FRAMES)
        if (arrival == null) {
            return
        }
        _warp = null
        teleportParty_(arrival)
        _warpCooldownEndTime = Time.scaledTime + WARP_COOLDOWN
        // Stepping out of a doorway shouldn't drop you straight into a fight.
        _combatCooldownEndTime = Time.scaledTime + 1.0
    }

    // Where a warp into room target arrives: next to its destination DoorWorp (the one with that iid - or, with no
    // destination, the one there that leads back to room from): one tile out in that door's direction, or one step
    // towards the room's centre if it has no direction - else the room's centre. Null while that room's DoorWorps
    // haven't started yet (unless giveUp).
    warpArrival_(target, from, destination, giveUp) {
        var bounds = Rooms.bounds(target)
        var centre = Vec3.new(bounds[0] + bounds[2] / 2, bounds[1] + bounds[3] / 2, 0.0)
        var waiting = false
        for (entity in GameObject.getAllWithIdentifier("DoorWorp")) {
            var position = WorldTransform.getPosition(entity)
            if (Rooms.indexAt(position) != target) {
                continue
            }
            var doorWorp = Script.getInstance(entity)
            if (doorWorp == null) {
                waiting = true
                continue
            }
            var isDestination = destination != null ? doorWorp.iid == destination : doorWorp.targetIndex == from
            if (isDestination) {
                var offset = doorWorp.arrivalOffset
                if (offset != null) {
                    return Vec3.new(position.x + offset.x, position.y + offset.y, 0.0)
                }
                // No direction set: step one tile off it, along whichever axis points more towards the room's centre.
                var dx = centre.x - position.x
                var dy = centre.y - position.y
                if (dx.abs > dy.abs) {
                    return Vec3.new(position.x + WARP_ARRIVAL_OFFSET * dx.sign, position.y, 0.0)
                }
                return Vec3.new(position.x, position.y + WARP_ARRIVAL_OFFSET * dy.sign, 0.0)
            }
        }
        if (waiting && !giveUp) {
            return null
        }
        return centre
    }

    // duel: true for a challenge fight (startDuel) - carried through to the fight's CombatOutcome.
    beginCombat_(opponents, playerEntity, duel) {
        // Nobody left standing (everyone downed, nobody rested): no fight - and a cooldown, so an aggressive
        // critter doesn't retry every frame.
        if (!Party.anyoneStanding) {
            Debug.info("[ExperimentState] nobody in the party can fight - not starting combat")
            _combatCooldownEndTime = Time.scaledTime + 2.0
            return
        }
        _talkView.hide()
        for (entity in GameObject.getAllWithIdentifier("Player")) {
            var script = Script.getInstance(entity)
            if (script) {
                script.playerForceStop()
            }
        }
        if (_gameMusic) {
            _gameMusic.stop()
        }
        _stateManager.changeState("CombatState", {
            "opponents": opponents,
            "player": playerEntity,
            "duel": duel,
            "world": _worldEntity,
            "battleLevelIndex": World.getLevelIndex(_worldEntity, BATTLE_ARENA_LEVEL_NAME)
        })
    }

    // The critter you engaged, plus every other critter of the player's room within GROUP_COMBAT_METERS
    // of the player - nearest first, capped at MAX_COMBAT_GROUP (so when more than that are in range you
    // fight the closest ones). primaryEntity is always in, even if it's somehow outside the group range.
    combatGroup(primaryEntity, playerEntity) {
        var group = [primaryEntity]
        var playerPosition = WorldTransform.getPosition(playerEntity)
        if (playerPosition == null) {
            return group
        }
        var groupRange = GROUP_COMBAT_METERS * Application.pixelsPerMeter
        var playerRoom = Rooms.indexAt(playerPosition)

        // Gather candidates as [distance, entity], then sort by distance so the cap keeps the closest.
        var candidates = []
        for (critter in Critter.all) {
            if (critter.entity == primaryEntity || !sharesRoom_(critter, playerRoom)) {
                continue
            }
            var p = critter.position
            if (p == null) {
                continue
            }
            var d = Vec3.distance(playerPosition, p)
            if (d <= groupRange) {
                candidates.add([d, critter.entity])
            }
        }
        candidates.sort { |a, b| a[0] < b[0] }

        for (candidate in candidates) {
            if (group.count >= MAX_COMBAT_GROUP) {
                break
            }
            group.add(candidate[1])
        }
        return group
    }

    // True if the critter belongs to (Critter.homeRoom) the room the player is in. With no room known for either,
    // anything counts, as before rooms mattered.
    sharesRoom_(critter, playerRoom) {
        return playerRoom == null || critter.homeRoom == null || critter.homeRoom == playerRoom
    }

    render() {
        // Delegate to gameplay sub-state
        if (_stateManager.currentState) {
            _stateManager.render()
        }
    }

    exit() {
        super.exit()

        Debug.info("Unloading game world...")

        // Exit sub-state manager
        if (_stateManager && _stateManager.currentState) {
            _stateManager.currentState.exit()
        }

        // The player slot goes with the world; the NPCs stay in the party (their follower entities are
        // destroyed with the world, so they respawn next time - see syncPartyFollowers).
        Party.unregisterPlayers()

        // Child of _worldEntity, so destroying the world tears the emitter down with it.
        _particleEntity = null
        GameObject.destroy(_worldEntity)
        // So the next enter() streams its spawn room in and snaps the new camera to it.
        _currentLevelIndex = null
        _roomCamera = null
        Rooms.world = null
        Signals.clear()
        PressurePlates.clear()
        ImpactEvents.clear()
        Warps.clear()
        _warp = null

        if (_view != null) {
            _view.teardown()
            _view = null
        }
        if (_talkView != null) {
            _talkView.teardown()
            _talkView = null
        }
        _gameMusic.stop()
        _gameMusic.unload()
        _gameMusic = null

        Debug.info("Game world unloaded")
    }

    onEvent(type, data) {
        super.onEvent(type, data)
        if (_stateManager && _stateManager.currentState) {
            _stateManager.currentState.onEvent(type, data)
        }
    }

    // Getters
    worldEntity { _worldEntity }
    subStateManager { _stateManager }
}
