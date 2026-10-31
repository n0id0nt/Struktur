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
import "UI/Gameplay/FightPromptUI" for FightPromptUI
import "UI/Gameplay/InteractPromptUI" for InteractPromptUI

import "GameObjects/Critter" for Critter
import "Party" for Party
import "Party/DuelRequest" for DuelRequest
import "Combat/CombatOutcome" for CombatOutcome

// Placeholder art for the rest spot (Tiles/Items.png frame 2 - the bread loaf) until there is campfire art.
var REST_SPOT_SPRITE_INDEX = 2

// Debug input (F6 / controller Select, see InputConfig.json) that opens the battle test sandbox
// (States/BattleTestState.wren) from the overworld.
var BATTLE_TEST_INPUT = "DebugBattleTest"

// Where the player starts - and where they wake up after the party is wiped (respawn_).
var SPAWN_POSITION = Vec3.new(208.0, 208.0, 0.0)
// Gap between followers lined up behind the player on a respawn.
var RESPAWN_FOLLOWER_SPACING = 24.0

var WORLD_FILE_PATH = "Levels/SevenGods.ldtk"
// Authored arena level (see Combat/BattleStage.wren) - every fight loads this level and stages the
// battle there instead of fighting in place. World.getLevelIndex returns null if it's ever renamed
// or removed, and CombatState falls back to the BattleAnchor/DEFAULT_ARENA behaviour in that case.
var BATTLE_ARENA_LEVEL_NAME = "Battle_Arena"
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
        _stateManager = StateManager.new()

        _stateManager.insertState("PlayState", PlayState)
        _stateManager.insertState("InventoryState", InventoryState)
        _stateManager.insertState("InteractState", InteractState)
        _stateManager.insertState("GameOverState", GameOverState)
        _stateManager.insertState("CombatState", CombatState)
        _gameMusic = null
        // Grace window after a fight ends (see update()) - long enough to run clear of an aggressive
        // critter before its proximity check can re-trigger combat. Covers every outcome: after a
        // loss or a flee the enemy is still alive and still next to you, and without this you'd be
        // stuck re-fighting it in place.
        _combatCooldownEndTime = 0
    }

    enter(stateManager, params) {
        super.enter(stateManager, params)

        System.print("Loading game world...")

        _gameMusic = Music.load("Sounds/gameMusic.wav")
        if (_gameMusic) {
            _gameMusic.setLooping(true)
            _gameMusic.play()
        }

        var worldEntity = World.createWorldEntity(WORLD_FILE_PATH)
        _worldEntity = worldEntity

        var playerEntity = GameObject.create("Player", worldEntity)
        Script.createArg(playerEntity, "Player", {"Name": "Player"})
        WorldTransform.setPosition(playerEntity, SPAWN_POSITION)
        _playerEntity = playerEntity
        // The player leads the party - NPCs who join follow the member ahead of them (see Party.wren).
        Party.registerPlayer(playerEntity, "Player")

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
        spawnSign("Rest Spot", Vec3.new(120.0, 108.0, 0.0), worldEntity, REST_SPOT_SPRITE_INDEX)

        // Pickups: walk up and press Interact to add them to the inventory (the Inventory key opens it).
        // The Lost Locket is the quest item; an item already picked up (saved flag) disables itself.
        spawnItem("Lost Locket", Vec3.new(300.0, 300.0, 0.0), worldEntity)
        spawnItem("Lucky Coin", Vec3.new(120.0, 300.0, 0.0), worldEntity)
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

    // A pickup (GameObjects/Item.wren): every item entity shares the "Item" identifier - that is what the
    // dialogue system's pickupEntity command searches by - and itemName picks its sprite and dialogue.
    spawnItem(itemName, position, worldEntity) {
        var entity = GameObject.create("Item", worldEntity)
        Script.createArg(entity, "Item", {"Name": itemName, "Returnable": false})
        WorldTransform.setPosition(entity, position)
    }

    // Same idea for a signpost (GameObjects/Sign.wren) - name is its dialogue entry point key.
    spawnSign(signName, position, worldEntity) { spawnSign(signName, position, worldEntity, null) }

    // spriteIndex: a Tiles/Items.png frame to use instead of the sign's default (null keeps it).
    spawnSign(signName, position, worldEntity, spriteIndex) {
        var entity = GameObject.create(signName, worldEntity)
        var args = {"Name": signName}
        if (spriteIndex != null) {
            args["SpriteIndex"] = spriteIndex
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

        // Sprites/magic.png is a single 60x60 frame - leave columns/rows at their 1x1 default so each
        // particle draws the whole texture. The emitter keeps its own reference to the texture, so the
        // script's handle can be released immediately afterwards (same pattern as Sprite.create).
        var texture = Texture.load("Sprites/magic.png")
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
        updateLevelStreaming_()

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
    // levelIndexAtPosition_ checks world bounds straight from the parsed LDtk metadata rather than
    // needing every level loaded to know where they are.
    updateLevelStreaming_() {
        var levelIndex = levelIndexAtPosition_(WorldTransform.getPosition(_playerEntity))
        if (levelIndex == null || levelIndex == _currentLevelIndex) {
            return
        }
        _currentLevelIndex = levelIndex

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

    // The index of the level whose world-space bounds contain position, or null if none match.
    // Checked directly against World.getLevelBounds (the parsed LDtk metadata), so it works even
    // before that level has ever been loaded.
    levelIndexAtPosition_(position) {
        for (i in 0...World.getLevelsCount(_worldEntity)) {
            var bounds = World.getLevelBounds(_worldEntity, i)
            if (position.x >= bounds[0] && position.x <= bounds[0] + bounds[2] &&
                position.y >= bounds[1] && position.y <= bounds[1] + bounds[3]) {
                return i
            }
        }
        return null
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

            for (critter in Critter.all) {
                if (!critter.aggressive) {
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
        System.print("[ExperimentState] combat ended: %(outcome)")
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
    // enemies stay gone (Defeated.wren); survivors keep their wounds until their level unloads, and streaming
    // swaps levels as normal on the next update. The usual post-fight cooldown applies.
    respawn_() {
        Party.restAll()
        WorldTransform.setPosition(_playerEntity, SPAWN_POSITION)
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
                    Vec3.new(SPAWN_POSITION.x - RESPAWN_FOLLOWER_SPACING * behind, SPAWN_POSITION.y, 0.0))
            }
        }
        _combatCooldownEndTime = Time.scaledTime + 2.0
        System.print("[ExperimentState] respawned at the start point")
    }

    // duel: true for a challenge fight (startDuel) - carried through to the fight's CombatOutcome.
    beginCombat_(opponents, playerEntity, duel) {
        // Nobody left standing (everyone downed, nobody rested): no fight - and a cooldown, so an aggressive
        // critter doesn't retry every frame.
        if (!Party.anyoneStanding) {
            System.print("[ExperimentState] nobody in the party can fight - not starting combat")
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

    // The critter you engaged, plus every other critter within GROUP_COMBAT_METERS of the player -
    // nearest first, capped at MAX_COMBAT_GROUP (so when more than that are in range you fight the
    // closest ones). primaryEntity is always in, even if it's somehow outside the group range.
    combatGroup(primaryEntity, playerEntity) {
        var group = [primaryEntity]
        var playerPosition = WorldTransform.getPosition(playerEntity)
        if (playerPosition == null) {
            return group
        }
        var groupRange = GROUP_COMBAT_METERS * Application.pixelsPerMeter

        // Gather candidates as [distance, entity], then sort by distance so the cap keeps the closest.
        var candidates = []
        for (critter in Critter.all) {
            if (critter.entity == primaryEntity) {
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

    render() {
        // Delegate to gameplay sub-state
        if (_stateManager.currentState) {
            _stateManager.render()
        }
    }

    exit() {
        super.exit()

        System.print("Unloading game world...")

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

        System.print("Game world unloaded")
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
