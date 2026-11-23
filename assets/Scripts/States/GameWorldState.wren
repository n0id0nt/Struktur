// states/GameWorldState.wren
// Main gameplay state - the state the game boots into (Main.wren). Loads the world (Levels/SevenGods.ldtk), creates
// the player, streams levels, detects combat / interact triggers and owns the nested sub-StateManager (PlayState/
// InventoryState/InteractState/GameOverState/CombatState/ImpactState) plus the UI/Gameplay Fight!/Interact prompts.
// Everything else in the world - NPCs, signs, doors, items, critters, warps - is placed in the LDtk level and spawned
// by World::Level with its script; nothing but the player and party followers is created from here.
// Story progress (Dialogue/Arc1/'s "arc1_stage" flag) shows and hides level-placed entities through
// World/StoryGates.wren, refreshed every update.
import "gameObject" for GameObject
import "gameObjectComponents" for WorldTransform, World, Script
import "math" for Vec3
import "resourceManager" for Music
import "World/AreaMusic" for AreaMusic
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
import "World/MapLog" for MapLog
import "Party/DuelRequest" for DuelRequest
import "Combat/CombatOutcome" for CombatOutcome
import "Camera/RoomCamera" for RoomCamera
import "World/Rooms" for Rooms
import "World/Signals" for Signals
import "World/PressurePlates" for PressurePlates
import "World/ImpactEvents" for ImpactEvents
import "World/Warps" for Warps
import "World/StoryGates" for StoryGates
import "World/StoryTriggers" for StoryTriggers
import "World/SoulSight" for SoulSight
import "World/SoulVeil" for SoulVeil
import "UI/Gameplay/SoulSightOverlay" for SoulSightOverlay
import "UI/Gameplay/ScreenFade" for ScreenFade
import "Util/SoulFx" for SoulFx
import "Events/GameEvents" for GameEvents
import "World/Npcs" for Npcs
import "World/SpawnPoint" for SpawnPoint
import "World/StageDirector" for StageDirector
import "World/PathWalkers" for PathWalkers
import "Story/QuestLog" for QuestLog
import "reflect" for Reflect
import "UI/Notifications/Notifications" for Notifications
import "Story/ArcEnd" for ArcEnd
import "localization" for Localization
import "debug" for Debug

// Debug input (F6 / controller Select, see InputConfig.json) that opens the battle test sandbox
// (States/BattleTestState.wren) from the overworld.
var BATTLE_TEST_INPUT = "DebugBattleTest"

// Where the player starts - and where they wake up after the party is wiped (respawn_): the current spawn point
// (World/SpawnPoint.wren - an LDtk "Spawn" entity, by iid, saved in flags.sav; the last RestSpot rested at, else the
// one in Finn's workshop). Its level is streamed in, the party is put at the level's centre, and moved onto the Spawn
// once its script has started (updateSpawnArrival_). FALLBACK_SPAWN_POSITION is only used if the level is missing.
var FALLBACK_SPAWN_POSITION = Vec3.new(208.0, 208.0, 0.0)
// Frames to wait for the spawn level's Spawn entities to start before giving up and staying at the level's centre.
var SPAWN_MAX_WAIT_FRAMES = 30
// A fresh save opens straight into this conversation (Dialogue/Arc1/) once the player has arrived at the
// spawn point; OPENING_PLAYED_FLAG (saved) makes sure it only ever plays once.
var OPENING_NODE = "opening"
var OPENING_PLAYED_FLAG = "opening_played"
// Soul Sight (World/SoulSight.wren): its screen tint's z index (under every other piece of UI), and the sparks the
// player gives off while it's on - px above their feet, and sparks per second.
var SOUL_OVERLAY_Z = -20
var SOUL_LIGHT_HEIGHT = 14
var SOUL_LIGHT_RATE = 14

// Gap between followers lined up behind the player on a respawn.
var RESPAWN_FOLLOWER_SPACING = 24.0
// How far from the DoorWorp you arrive next to (towards the room's centre), so you don't land back on it.
var WARP_ARRIVAL_OFFSET = 24.0
// Frames to wait for the target room's DoorWorps to start (their scripts run a frame after the room loads) before
// giving up on finding the one that leads back and arriving at the room's centre.
var WARP_MAX_WAIT_FRAMES = 10
// Seconds after arriving during which stepping on a DoorWorp does nothing. None: arrivals stand clear of the door's
// trigger (GameObjects/DoorWorp.wren), so stepping straight back onto it should warp straight back.
var WARP_COOLDOWN = 0
// A warp fades to black (seconds, real time), moves the party while the screen is black, then fades back in.
var WARP_FADE_OUT = 0.25
var WARP_FADE_IN = 0.3
// Waking after the party fell: the fade in from black once you're back at the spawn point, while you get up.
var WAKE_FADE_IN = 1.0

var WORLD_FILE_PATH = "Levels/SevenGods.ldtk"
// Authored arena levels (see Combat/BattleStage.wren) - a fight loads the arena matching the level it starts in and
// stages the battle there instead of fighting in place: the level's "Terrain" field + ARENA_LEVEL_SUFFIX (Grass ->
// "GrassBattleStage", Woods, Shrine, IndoorBrick...). A level with no Terrain, or one with no arena of its own, uses
// DEFAULT_ARENA_LEVEL_NAME. With no arena at all, CombatState falls back to the BattleAnchor/DEFAULT_ARENA behaviour.
var ARENA_LEVEL_SUFFIX = "BattleStage"
var DEFAULT_ARENA_LEVEL_NAME = "GrassBattleStage"
// Combat proximity ranges, in metres (converted to world pixels at use via Application.pixelsPerMeter,
// default 64). An aggressive critter (Chinlin) within FORCE range forces combat outright rather than
// just offering it via the interact prompt - meant to feel like being caught, not spotted. When a
// fight starts, every other critter within GROUP range OF THE PLAYER and standing in the player's room is dragged in
// too, nearest first, up to MAX_COMBAT_GROUP total (the closest 3 when more than that are in range).
var FORCE_COMBAT_METERS = 1.0
var GROUP_COMBAT_METERS = 6.0
var MAX_COMBAT_GROUP = 3
// Int flag a won duel or story battle leaves for the challenger's dialogue to branch on: "duel_won_<Name>" = 1.
var DUEL_WON_FLAG_PREFIX = "duel_won_"
// Int flag counting the kills of each opponent (name without spaces) in real fights: "defeated_StarvingThornwolf".
var DEFEATED_FLAG_PREFIX = "defeated_"

class GameWorldState is BaseState {
    // Where a run starts the player: the editor level viewer's "Play from here" spot if it was picked in this world
    // (Application.debugSpawn), otherwise the current spawn point (beginSpawnArrival_).
    startPosition_() {
        var debugSpawn = Application.debugSpawn
        if (debugSpawn != null && debugSpawn[0] == WORLD_FILE_PATH) {
            return Vec3.new(debugSpawn[1], debugSpawn[2], 0.0)
        }
        return beginSpawnArrival_()
    }

    // Starts heading for the current spawn point (World/SpawnPoint.wren): returns the centre of its level - where the
    // party goes first, so streaming loads that level - and remembers the Spawn's iid for updateSpawnArrival_.
    beginSpawnArrival_() {
        var spawn = SpawnPoint.current
        var levelIndex = levelIndexOf_(spawn[1])
        if (levelIndex == null) {
            var missing = spawn[1]
            Debug.warning("[GameWorldState] spawn level %(missing) not found - using the default spawn point")
            spawn = [SpawnPoint.defaultIid, SpawnPoint.defaultLevel]
            levelIndex = levelIndexOf_(spawn[1])
        }
        if (levelIndex == null) {
            Debug.warning("[GameWorldState] default spawn level not found either - spawning at the fallback position")
            _spawnArrival = null
            return FALLBACK_SPAWN_POSITION
        }
        _spawnArrival = {"iid": spawn[0], "frames": 0}
        var bounds = World.getLevelBounds(_worldEntity, levelIndex)
        return Vec3.new(bounds[0] + bounds[2] / 2, bounds[1] + bounds[3] / 2, 0.0)
    }

    // Once the spawn level's Spawn entities have started: the party moves onto the one being headed for.
    updateSpawnArrival_() {
        _spawnArrival["frames"] = _spawnArrival["frames"] + 1
        var wantedIid = _spawnArrival["iid"]
        for (entity in GameObject.getAllWithIdentifier("Spawn")) {
            var spawn = Script.getInstance(entity)
            if (spawn != null && spawn.iid == wantedIid) {
                _spawnArrival = null
                teleportParty_(WorldTransform.getPosition(entity))
                return
            }
        }
        if (_spawnArrival["frames"] >= SPAWN_MAX_WAIT_FRAMES) {
            Debug.warning("[GameWorldState] no Spawn with iid %(wantedIid) in its level - staying at the level's centre")
            _spawnArrival = null
        }
    }

    // The new-game opening (OPENING_NODE): a conversation with nobody to talk to, started straight from its node.
    startOpening_() {
        _openingPending = false
        FlagManager.setFlag(OPENING_PLAYED_FLAG, true)
        FlagManager.save("flags.sav")
        var player = Script.getInstance(_playerEntity)
        if (player != null) {
            player.playerForceStop()
        }
        if (_gameMusic) {
            _gameMusic.stop()
        }
        _stateManager.changeState("InteractState", {"entryNode": OPENING_NODE})
    }

    // A Trigger's moment: a conversation started straight from its node, the player stopped where they stand.
    startTriggered_(node) {
        var player = Script.getInstance(_playerEntity)
        if (player != null) {
            player.playerForceStop()
        }
        if (_gameMusic) {
            _gameMusic.stop()
        }
        _stateManager.changeState("InteractState", {"entryNode": node})
    }

    // A level's index by its identifier, or null.
    levelIndexOf_(identifier) {
        var index = World.getLevelIndex(_worldEntity, identifier)
        return (index == null || index < 0) ? null : index
    }

    // The arena for a fight starting in level levelIndex (see ARENA_LEVEL_SUFFIX), or null if there is none.
    arenaIndexFor_(levelIndex) {
        var index = null
        if (levelIndex != null) {
            var terrain = World.getLevelField(_worldEntity, levelIndex, "Terrain")
            if (terrain is String && terrain != "") {
                index = levelIndexOf_(terrain + ARENA_LEVEL_SUFFIX)
            }
        }
        if (index == null) {
            index = levelIndexOf_(DEFAULT_ARENA_LEVEL_NAME)
        }
        if (index != null) {
            _arenaIndices[index] = true
        }
        return index
    }

    construct new() {
        super()
        name = "GameWorldState"

        _view = null
        _talkView = null   // "Interact" prompt for NPCs / signposts (_view is the "Fight!" prompt for critters)
        _worldEntity = null //TODO Create an entity constant for invalid entity or null entity
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
        _duelChallenger = null   // name of the NPC in the duel under way (startDuel), for onCombatEnded_
        _returnToMenu = false    // set by the end-of-arc card's button (showArcEnd_)
        _spawnArrival = null     // {"iid", "frames"} while heading for a spawn point - see beginSpawnArrival_
        _openingPending = false  // a fresh save's opening conversation is waiting for the spawn arrival - see enter
        _arenaIndices = {}       // arena levels used so far (level index -> true) - kept loaded, see updateLevelStreaming_
        _soulOverlay = null      // Soul Sight's screen tint (UI/Gameplay/SoulSightOverlay.wren)
        _soulLight = null        // the sparks the player gives off while Soul Sight is on (Util/SoulFx.wren), a child entity
        _fade = null             // the black screen fade (UI/Gameplay/ScreenFade.wren) - warps and waking after a fall
        _waking = false          // respawned: fade in and get up once the party is at the spawn point - see respawn_
    }

    enter(stateManager, params) {
        super.enter(stateManager, params)

        Debug.info("Loading game world...")

        // The song follows the level's terrain (World/AreaMusic.wren) - picked once the first level is known.
        _gameMusic = null
        _gameSong = null

        var worldEntity = World.createWorldEntity(WORLD_FILE_PATH)
        _worldEntity = worldEntity
        Rooms.world = worldEntity

        var playerEntity = GameObject.create("Player", worldEntity)
        Script.createArg(playerEntity, "Player", {"Name": "Player"})
        WorldTransform.setPosition(playerEntity, startPosition_())
        _playerEntity = playerEntity
        // The player leads the party - NPCs who join follow the member ahead of them (see Party/Party.wren).
        Party.registerPlayer(playerEntity, "Player")
        _roomCamera = RoomCamera.new(playerEntity)

        // Streams in the level the player spawned in (plus its neighbours) instead of loading every
        // level in the world up front - see updateLevelStreaming_().
        updateLevelStreaming_()

        // A fresh save: the opening conversation plays as soon as the player is standing at the spawn point.
        _openingPending = !FlagManager.getFlag(OPENING_PLAYED_FLAG)

        // "Fight" prompt for non-aggressive critters, "Interact" prompt for everything else you can talk to / use.
        _view = FightPromptUI.new("Fight!")
        _talkView = InteractPromptUI.new("Interact")
        // Soul Sight starts each session off (World/SoulSight.wren); its tint sits under every other piece of UI.
        SoulSight.reset()
        _soulOverlay = SoulSightOverlay.new(false, SOUL_OVERLAY_Z)
        _fade = ScreenFade.new()
        _waking = false

        // NPCs, signs, doors, pressure plates, items and critters are placed in the level (LDtk) - World::Level spawns
        // them with their scripts.
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

    // Substates get the fixed step while one is open; otherwise the party trail is recorded.
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
        // Dialogue staging (moveTo / wait... - World/StageDirector.wren) plays out whether or not a dialogue is open.
        StageDirector.update()
        // NPCs wandering their Paths - they stop for conversations and scenes on their own.
        PathWalkers.update()
        // Banners (game events become them - onEvent) show over the world and dialogue, not over the inventory or a fight.
        var substate = _stateManager.currentState
        Notifications.update(substate == null || substate.name == "InteractState")
        // Soul Sight's tint shows over the world, a conversation and an impact moment - a fight draws its own.
        _soulOverlay.setShowing(substate == null || substate.name == "InteractState" || substate.name == "ImpactState")
        _soulOverlay.update()
        _fade.update()

        // Someone who joins mid-conversation ("joinParty") appears straight away, so the scene can walk them over.
        if (substate != null && substate.name == "InteractState") syncPartyFollowers()

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
                // A dialogue ended - if it was the last line of the arc ("endArc" command), the end card shows now;
                // if it was a challenge ("startDuel" / "startBattle" command), the fight starts now.
                if (ArcEnd.take()) {
                    showArcEnd_()
                    return
                }
                var pending = DuelRequest.take()
                if (pending != null) {
                    startDuel(pending[0], pending[1])
                }
            }
            return
        }
        // The end-of-arc card's button was pressed (and the card has closed): back to the main menu.
        if (_returnToMenu) {
            _returnToMenu = false
            stateManager.changeState("MainMenu")
            return
        }
        if (_gameMusic && !_gameMusic.isPlaying()) {
            _gameMusic.play()
        }

        syncPartyFollowers()
        // Story-gated NPCs / signs appear and disappear as the arc moves on (World/StoryGates.wren).
        StoryGates.refresh()

        // A DoorWorp the player stepped on (GameObjects/DoorWorp.wren) - load its room, then move the party there.
        var warp = Warps.take()
        if (warp != null && _warp == null && Time.scaledTime >= _warpCooldownEndTime) {
            startWarp_(warp[0], warp[1], warp[2])
        }
        if (_warp != null) {
            updateWarp_()
        }

        updateLevelStreaming_()
        if (_spawnArrival != null) {
            updateSpawnArrival_()
        }
        if (_waking && _spawnArrival == null) {
            wake_()
        }
        if (_openingPending && _spawnArrival == null) {
            startOpening_()
            return
        }

        // Something in the world wants its moment (a door opening - see World/ImpactEvents.wren): play it now, while
        // nothing else (dialogue, a fight, a menu) is running.
        var impact = ImpactEvents.take()
        if (impact != null) {
            playImpact_(impact)
            return
        }

        // A walk-in story moment (GameObjects/Trigger.wren) - also only while nothing else is running.
        var triggered = StoryTriggers.take()
        if (triggered != null) {
            startTriggered_(triggered)
            return
        }

        if (Input.isInputJustReleased("Inventory")) {
            openInventory()
            return
        }

        // Soul Sight on / off (once the Heart has given it) - everything that shows it reacts to the soulSight event.
        if (Input.isInputJustPressed(SoulSight.input)) SoulSight.toggle()

        // Debug: F6 / Select opens the battle test sandbox (States/BattleTestState.wren); its Back button returns here.
        if (Input.isInputJustPressed(BATTLE_TEST_INPUT)) {
            stateManager.changeState("BattleTest")
            return
        }

        // Neither runs for a moment after a fight (_combatCooldownEndTime), so the player isn't dragged straight back in.
        if (Time.scaledTime >= _combatCooldownEndTime) {
            if (startForcedCombat_()) return
            updateInteractPrompt_()
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

    // Switches to the song for level levelIndex's terrain (World/AreaMusic.wren) if it isn't already the one loaded.
    // It plays straight away when nothing else is on (a conversation, a fight...); otherwise update starts it once
    // they're over, as it does after every interruption.
    playAreaSong_(levelIndex) {
        var song = AreaMusic.songFor(World.getLevelField(_worldEntity, levelIndex, "Terrain"))
        if (song == _gameSong) return
        _gameSong = song
        if (_gameMusic) {
            _gameMusic.stop()
            _gameMusic.unload()
        }
        _gameMusic = Music.load(song)
        if (_gameMusic) {
            _gameMusic.setLooping(true)
            if (_stateManager.currentState == null) _gameMusic.play()
        }
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
        playAreaSong_(levelIndex)
        MapLog.visit(_worldEntity, levelIndex)   // the inventory's Map tab (World/MapLog.wren)
        // The first room (spawn) is snapped to; every room after that is slid to.
        _roomCamera.moveTo(levelIndex)

        var wanted = [levelIndex]
        for (neighbour in World.getLevelNeighbours(_worldEntity, levelIndex)) {
            wanted.add(neighbour[0])
        }

        // The battle arenas aren't part of the walkable world and never show up in `wanted`, but
        // CombatState deliberately leaves an arena loaded-but-inactive between fights to skip the reload -
        // don't undo that here.
        for (i in 0...World.getLevelsCount(_worldEntity)) {
            if (wanted.contains(i) || _arenaIndices.containsKey(i)) {
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
                // A newly loaded SoulVeil tile layer comes in drawn - hide it if Soul Sight is on (World/SoulVeil.wren).
                SoulVeil.refresh()
            }
            GameObject.setActive(levelEntity)
            // Only the player's room runs: the neighbours are drawn (and keep their walls), but everything in them -
            // critters, wandering NPCs - holds still until the player walks in (System::Frozen in the engine).
            GameObject.setFrozen(levelEntity, i != levelIndex)
        }
    }

    // Aggressive critters (Chinlin) force the player straight into CombatState once they close the distance - no
    // prompt, no choice. Only critters in the player's own room count, not one on the far side of a wall in the next.
    // Returns true if a fight started.
    startForcedCombat_() {
        for (playerEntity in GameObject.getAllWithIdentifier("Player")) {
            if (!Script.getInstance(playerEntity)) {
                continue
            }
            var playerPosition = WorldTransform.getPosition(playerEntity)
            var forceRange = FORCE_COMBAT_METERS * Application.pixelsPerMeter
            var playerRoom = Rooms.indexAt(playerPosition)

            for (critter in Critter.all) {
                if (!critter.aggressive || !sharesRoom_(critter, playerRoom)) {
                    continue
                }
                var critterPosition = critter.position
                if (critterPosition && Vec3.distance(playerPosition, critterPosition) <= forceRange) {
                    _view.hide()
                    enterCombat(critter.entity, playerEntity, true)
                    return true
                }
            }
        }
        return false
    }

    // The prompt over whatever the player can reach (Player.getInteractEntity()), and what Interact does to it:
    //   - a non-aggressive critter (Chicken) shows "Fight!" - Interact starts that fight (enterCombat);
    //   - any other interactable (NPC, signpost, item) shows "Interact" - Interact opens its dialogue (startDialogue);
    //   - nothing in reach hides both prompts.
    updateInteractPrompt_() {
        for (playerEntity in GameObject.getAllWithIdentifier("Player")) {
            var playerScript = Script.getInstance(playerEntity)
            if (!playerScript) {
                continue
            }
            var interactEntity = playerScript.getInteractEntity()
            var interactScript = interactEntity ? Script.getInstance(interactEntity) : null
            if (interactScript is Critter) {
                _talkView.hide()
                _view.showAbove(WorldTransform.getPosition(interactEntity))
                if (Input.isInputJustReleased("Interact")) {
                    _view.hide()
                    enterCombat(interactEntity, playerEntity, false)
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

    // Talks to an NPC / reads a signpost / picks up an item: InteractState runs the dialogue for
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

    // forced: an aggressive critter caught the player - the only kind of fight that can be fled. One the player started
    // (Interact on a critter's Fight! prompt) can't be.
    enterCombat(primaryEntity, playerEntity, forced) {
        // A creature someone is watching you fight (the perimeter wolf - Thomas) names them, and they speak once it's won.
        var primary = Script.getInstance(primaryEntity)
        _talkAfterFight = (primary != null && Reflect.hasMethod(primary, "talkAfterFight()")) ? primary.talkAfterFight() : null
        beginCombat_(combatGroup(primaryEntity, playerEntity), playerEntity, false, forced)
    }

    // A fight with a named challenger (the "startDuel" / "startBattle" dialogue commands), no critters dragged in.
    // Everyone standing here with that name fights together - the three Wardens (LDtk Interactables sharing a
    // MessageKey) are one fight. lethal false: a duel - nobody dies (onCombatEnded_ revives everyone). lethal true: a
    // story battle - losing is a game over, like any real fight. Either way the challengers stay standing in the world
    // (States/Combat/CombatResolver.removeFallen); beating a recruitable NPC adds them to the party, beating anyone else
    // sets DUEL_WON_FLAG_PREFIX + name and opens their dialogue again (onCombatEnded_).
    startDuel(npcName, lethal) {
        var challengers = Npcs.find(npcName)
        if (challengers.count == 0) {
            Debug.warning("[GameWorldState] duel requested with %(npcName), but they're not here")
            return
        }
        if (challengers.count > MAX_COMBAT_GROUP) {
            challengers = challengers.take(MAX_COMBAT_GROUP).toList
        }
        _duelChallenger = npcName
        beginCombat_(challengers, _playerEntity, !lethal, false)   // started from dialogue - no running away
    }

    // A fight has closed. outcome: the Combat/CombatOutcome it left (null if it was aborted before starting).
    // The fallen are already gone from the world (CombatResolver.removeFallen); what a loss means - game over,
    // or nothing for a duel - hooks in here.
    onCombatEnded_(outcome) {
        if (outcome == null) {
            return
        }
        Debug.info("[GameWorldState] combat ended: %(outcome)")
        var challenger = _duelChallenger
        _duelChallenger = null
        var watcher = _talkAfterFight
        _talkAfterFight = null
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
            return
        }
        // Kills count towards quests ("defeated_StarvingThornwolf" - Thomas's wolf hunt).
        if (!outcome.duel) {
            for (name in outcome.fallen) {
                var flag = DEFEATED_FLAG_PREFIX + name.replace(" ", "")
                QuestLog.setFlag(flag, FlagManager.getIntFlag(flag) + 1)
            }
        }
        // A won duel or story battle: remember it and let the challenger have their say straight away (their dialogue
        // branches on the flag - Thomas hands over gear, the Wardens start to remember...).
        // A won fight someone watched (enterCombat): they have their say straight away.
        if (outcome.won && challenger == null && watcher != null) {
            var watching = Npcs.first(watcher)
            if (watching != null) {
                if (_gameMusic) {
                    _gameMusic.stop()
                }
                _stateManager.changeState("InteractState", {"interactingEntity": watching})
                return
            }
        }
        if (outcome.won && challenger != null) {
            QuestLog.setFlag(DUEL_WON_FLAG_PREFIX + challenger, 1)   // may move a quest on - announced as an event
            var entity = Npcs.first(challenger)
            if (entity != null) {
                if (_gameMusic) {
                    _gameMusic.stop()
                }
                _stateManager.changeState("InteractState", {"interactingEntity": entity})
            }
        }
    }

    // The arc's last line has been read: the end card (States/GameOverState.wren with a title and button, game time
    // paused). Its button returns to the main menu - via _returnToMenu, once the card has closed itself.
    showArcEnd_() {
        _view.hide()
        _talkView.hide()
        if (_gameMusic) {
            _gameMusic.stop()
        }
        _stateManager.changeState("GameOverState", {
            "title": Localization.get("arc.end.title"),
            "continueText": Localization.get("arc.end.continue"),
            "onContinue": Fn.new { _returnToMenu = true }
        })
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
        // Black under the "You fell" screen as it closes; wake_ fades in once the party is at the spawn point.
        _fade.setBlack()
        _waking = true
        Party.restAll()
        teleportParty_(beginSpawnArrival_())
        _combatCooldownEndTime = Time.scaledTime + 2.0
        Debug.info("[GameWorldState] respawned at the start point")
    }

    // Back at the spawn point after a fall: the screen fades in from black while the player gets up off the ground.
    wake_() {
        _waking = false
        _fade.fadeIn(WAKE_FADE_IN)
        var player = Script.getInstance(_playerEntity)
        if (player != null) player.getUp()
    }

    // Moves the player to position and lines the followers up behind them, their trails wiped so nobody walks back
    // to where they were. A teleport, not a walk - the camera lands in the new room's view rather than sliding there.
    teleportParty_(position) { teleportParty_(position, null) }

    // The same, with every follower put on followerPosition instead (a warp door's centre) - null lines them up.
    teleportParty_(position, followerPosition) {
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
                var spot = followerPosition
                if (spot == null) spot = Vec3.new(position.x - RESPAWN_FOLLOWER_SPACING * behind, position.y, 0.0)
                WorldTransform.setPosition(member.entity, spot)
            }
        }
    }

    // Soul Sight went on or off: the screen tint, the sparks off the player and the SoulVeil tile layers follow (the
    // player entity is rebuilt on a respawn, so its spark emitter is made again when it has gone).
    showSoulSight_(active) {
        if (_soulOverlay != null) _soulOverlay.setActive(active)
        SoulVeil.show(!active)
        if (_playerEntity == null || !GameObject.isValid(_playerEntity)) return
        if (_soulLight == null || !GameObject.isValid(_soulLight)) _soulLight = SoulFx.lightEmitter(_playerEntity, SOUL_LIGHT_HEIGHT)
        SoulFx.setLit(_soulLight, active ? SOUL_LIGHT_RATE : 0)
    }

    // --- warps (GameObjects/DoorWorp.wren) -----------------------------------
    // The screen fades to black, holding the player still; then the target room is loaded so the DoorWorp leading
    // back can be found, and the party moves once it has started (updateWarp_), a frame or two later - and the
    // screen fades back in.
    startWarp_(targetIndex, fromIndex, destination) {
        Debug.info("[GameWorldState] warping from room %(fromIndex) to room %(targetIndex)")
        // origin: where the player stepped on the warp door - the Map tab's doorway marker (World/MapLog.warped).
        _warp = {"target": targetIndex, "from": fromIndex, "destination": destination, "frames": 0,
                 "origin": WorldTransform.getPosition(_playerEntity), "loaded": false}
        _fade.fadeOut(WARP_FADE_OUT)
        var player = Script.getInstance(_playerEntity)
        if (player != null) {
            player.setInteracting(true)
        }
    }

    updateWarp_() {
        if (!_warp["loaded"]) {
            if (!_fade.black) return
            World.loadLevelEntities(_worldEntity, _warp["target"])
            SoulVeil.refresh()
            _warp["loaded"] = true
            return
        }
        _warp["frames"] = _warp["frames"] + 1
        var arrival = warpArrival_(_warp["target"], _warp["from"], _warp["destination"],
                                   _warp["frames"] >= WARP_MAX_WAIT_FRAMES)
        if (arrival == null) {
            return
        }
        MapLog.warped(_worldEntity, _warp["from"], _warp["target"], _warp["origin"], arrival[0])
        _warp = null
        // The player just outside the door, the followers on the door itself (always inside the room - they don't
        // trigger it), so they walk out after the player.
        teleportParty_(arrival[0], arrival[1])
        var player = Script.getInstance(_playerEntity)
        if (player != null) {
            player.setInteracting(false)
        }
        _fade.fadeIn(WARP_FADE_IN)
        _warpCooldownEndTime = Time.scaledTime + WARP_COOLDOWN
        // Stepping out of a doorway shouldn't drop you straight into a fight.
        _combatCooldownEndTime = Time.scaledTime + 1.0
    }

    // Where a warp into room target arrives: next to its destination DoorWorp (the one with that iid - or, with no
    // destination, the one there that leads back to room from): just clear of its trigger in that door's direction
    // (GameObjects/DoorWorp.wren arrivalOffset), or one step towards the room's centre if it has no direction - else
    // the room's centre. Returns [player position, followers' position - the door's centre, or null to line them up].
    // Null while that room's DoorWorps haven't started yet (unless giveUp).
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
                var doorCentre = Vec3.new(position.x, position.y, 0.0)
                var offset = doorWorp.arrivalOffset
                if (offset != null) {
                    return [Vec3.new(position.x + offset.x, position.y + offset.y, 0.0), doorCentre]
                }
                // No direction set: step one tile off it, along whichever axis points more towards the room's centre.
                var dx = centre.x - position.x
                var dy = centre.y - position.y
                if (dx.abs > dy.abs) {
                    return [Vec3.new(position.x + WARP_ARRIVAL_OFFSET * dx.sign, position.y, 0.0), doorCentre]
                }
                return [Vec3.new(position.x, position.y + WARP_ARRIVAL_OFFSET * dy.sign, 0.0), doorCentre]
            }
        }
        if (waiting && !giveUp) {
            return null
        }
        return [centre, null]
    }

    // duel: true for a challenge fight (startDuel) - carried through to the fight's CombatOutcome. canFlee: whether the
    // fight offers Flee - only when a critter forced it on the player (enterCombat); never for one started by an
    // interaction (a critter's Fight! prompt, a duel, a story battle).
    beginCombat_(opponents, playerEntity, duel, canFlee) {
        // Nobody left standing (everyone downed, nobody rested): no fight - and a cooldown, so an aggressive
        // critter doesn't retry every frame.
        if (!Party.anyoneStanding) {
            Debug.info("[GameWorldState] nobody in the party can fight - not starting combat")
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
            "canFlee": canFlee,
            // A fight started from dialogue may have a script (Combat/Encounter.wren - Thomas's lessons, the Warden).
            "challenger": _duelChallenger,
            "world": _worldEntity,
            "battleLevelIndex": arenaIndexFor_(_currentLevelIndex)
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
            var critterPosition = critter.position
            // Standing in the player's room, not just belonging to it - never one through a wall in the next room.
            if (critterPosition == null || (playerRoom != null && Rooms.indexAt(critterPosition) != playerRoom)) {
                continue
            }
            var distance = Vec3.distance(playerPosition, critterPosition)
            if (distance <= groupRange) {
                candidates.add([distance, critter.entity])
            }
        }
        candidates.sort { |lhs, rhs| lhs[0] < rhs[0] }

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

        GameObject.destroy(_worldEntity)
        // So the next enter() streams its spawn room in and snaps the new camera to it.
        _currentLevelIndex = null
        _roomCamera = null
        Rooms.world = null
        Signals.clear()
        PressurePlates.clear()
        ImpactEvents.clear()
        Warps.clear()
        StoryGates.clear()
        _warp = null
        _duelChallenger = null
        _spawnArrival = null
        _arenaIndices = {}
        _openingPending = false
        StageDirector.clear()
        PathWalkers.clear()
        Notifications.clear()

        if (_view != null) {
            _view.teardown()
            _view = null
        }
        if (_talkView != null) {
            _talkView.teardown()
            _talkView = null
        }
        if (_soulOverlay != null) {
            _soulOverlay.teardown()
            _soulOverlay = null
        }
        if (_fade != null) {
            _fade.teardown()
            _fade = null
        }
        _waking = false
        _soulLight = null   // destroyed with the player
        SoulSight.reset()
        if (_gameMusic) {
            _gameMusic.stop()
            _gameMusic.unload()
        }
        _gameMusic = null
        _gameSong = null

        Debug.info("Game world unloaded")
    }

    onEvent(type, data) {
        super.onEvent(type, data)
        // Game events (Events/GameEvents.wren) that deserve a banner - a quest moving on, an item received...
        Notifications.onEvent(type, data)
        if (type == GameEvents.soulSightType) showSoulSight_(SoulSight.fromEvent(data))
        if (_stateManager && _stateManager.currentState) {
            _stateManager.currentState.onEvent(type, data)
        }
    }

    // Getters
    worldEntity { _worldEntity }
    subStateManager { _stateManager }
}
