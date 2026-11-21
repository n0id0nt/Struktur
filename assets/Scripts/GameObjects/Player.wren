import "gameObject" for GameObject
import "gameObjectComponents" for WorldTransform, Script, Camera, PhysicsBody, SpriteAnimation
import "math" for Vec2, Vec3, Vec4, Math
import "physics" for BodyDefinition, PhysicsCircleShape, BodyType, CollisionLayers, PhysicsQuery
import "Util/CharacterCollider" for CharacterCollider
import "input" for Input
import "app" for Time
import "random" for Random

import "reflect" for Reflect

import "World/Rooms" for Rooms
import "Party/Party" for Party
import "Combat/Config/BattlePlayer" for BattlePlayer
import "Sprites/ExplorerSprite" for ExplorerSprite
import "Sprites/SpriteSheet" for SpriteSheet
import "Util/Shadow" for Shadow
import "debug" for Debug

var INTERACTABLE_DISTANCE = 32.0
// Interactables that ask for it (requiresFacing() - NPCs, signs) only count when they're within this many degrees
// of the way the player is facing.
var INTERACT_FACING_DEGREES = 90
// An interactable only counts when nothing solid stands between it and the player (clearLineTo_). Things built into the
// level (a mural, a statue) sit inside solid tiles, so a hit inside the interactable's own footprint doesn't count - its
// size comes from interactSize() ([width, height] px) if the script has one, else DEFAULT_INTERACT_SIZE. The footprint
// is grown by FOOTPRINT_SLACK px to absorb hits landing exactly on its edge.
var DEFAULT_INTERACT_SIZE = [16, 16]
var FOOTPRINT_SLACK = 2.0
// Left standing still this long (seconds of game time, no movement input), the player rests: sits or lies, picked at
// random (REST_CLIPS). Either way they go down with the lying-down animation (DOWN_CLIP - a sit then settles into its
// sitting idle) and come back up with the getting-up animation when moved again.
var DOWN_CLIP = "LyingDown"
var IDLE_REST_SECONDS = 12
var REST_CLIPS = ["SittingIdle", "LyingDown"]
// The party rests with the player (GameObjects/PartyMember.wren restSoon): each follower this many seconds after
// them, its own random delay in the range, so they don't all drop at once.
var PARTY_REST_DELAY_MIN = 0.2
var PARTY_REST_DELAY_MAX = 0.5
var RNG = Random.new()

class Player {
    construct new(entity, args) {
        _entity = entity
        _name = args["Name"]
        _timeAccumulator = 0
        _initialized = false
        _facing = Vec2.new(0, 1)
        _walkSpeed = 1.5
        _runSpeed = 3
        _running = false
        _fixedUpdateCount = 0
        _combatant = null
        _interacting = false
        _scripted = false   // moved by a dialogue scene (World/StageDirector.wren) - see setScripted
        _stillSince = null  // Time.scaledTime the player last stood still from - see rest_
        _resting = null     // the rest clip playing ("SittingIdle" / "LyingDown"), or null
        _upAt = null        // Time.scaledTime a getting-up animation ends (getUp), or null
        _settleAt = null    // Time.scaledTime the lying-down animation ends and a sit settles into its idle, or null

        Debug.info("Player constructed for: %(_name)")
    }

    name { _name }

    // Movement speeds in m/s: walking is the default, holding the "Run" input runs - except in walk-only rooms
    // (LDtk level field "Runable" off - World/Rooms.wren), where Run does nothing.
    #!export
    walkSpeed { _walkSpeed }
    walkSpeed=(value) { _walkSpeed = value }

    #!export
    runSpeed { _runSpeed }
    runSpeed=(value) { _runSpeed = value }

    // True during an interact / dialogue sequence (States/InteractState.wren): the player ignores every input and
    // just stands idle, while the world - NPCs, critters, animations - carries on around them.
    interacting { _interacting }

    // Handed to a dialogue scene (World/StageDirector.wren "moveTo"): while scripted, the player's own movement and
    // the interacting mode's idle pose stand aside so the scene can move and animate them.
    setScripted(value) {
        _scripted = value
    }

    // The way the player faces (+y down the screen) - a dialogue scene can turn them ("face" command).
    facing { _facing }
    facing=(value) { _facing = value }

    // Plays the getting-up animation, facing down - after waking from a fall (States/GameWorldState.wren respawn_), or
    // rising from lying down. The player can't move until it has finished.
    getUp() {
        if (_resting != null) Player.standParty_()
        _resting = null
        _settleAt = null
        _stillSince = null
        _facing = Vec2.new(0, 1)
        PhysicsBody.setLinearVelocity(_entity, Vec2.new())
        var animation = getPlayerAnimation("GettingUp")
        var clip = SpriteSheet.clip(_entity, animation)
        if (clip == null) {
            _upAt = null
            return
        }
        SpriteAnimation.setCurrentAnimation(_entity, animation)
        _upAt = Time.scaledTime + clip[2]
    }

    setInteracting(value) {
        _interacting = value
        if (value) {
            _running = false
            playerForceStop()
        }
    }

    // The speed the player is moving at right now - party followers (GameObjects/PartyMember.wren) match it,
    // so they walk when the player walks and run when the player runs.
    speed { _running ? _runSpeed : _walkSpeed }

    // The player's persistent battle Combatant. Its numbers, move kit and arena look are defined in
    // Combat/BattlePlayer.wren (the same shape as one Combat/BattleCritter.wren entry); it's minted
    // once and kept here, so damage and fatigue from one fight carry into the next. A full reset
    // only happens on death/respawn, when GameWorldState rebuilds the player entity and this script
    // with it. entity is null: a defeated player goes to GameOverState, the field entity is never
    // destroyed out from under the fight the way a beaten critter's is.
    combatant {
        if (_combatant == null) {
            _combatant = BattlePlayer.makeCombatant(null)
        }
        return _combatant
    }

    // Called after C++ has created base components
    // Script configures/initializes component values
    start() {
        // Sprite + every <direction><Animation>Animation clip, generated from the 3D model by
        // 3dModelsToSprites/build_sprites.bat explorer (see Sprites/ExplorerSprite.wren).
        ExplorerSprite.create(_entity)
        Shadow.attach(_entity)
        var camera = Camera.create(_entity)
        camera.zoom = 5
        camera.forcePosition = true
        camera.damping = Vec2.new(4, 4)
        var bodyDef = BodyDefinition.new(BodyType.DYNAMIC_BODY)
        var playerShape = CharacterCollider.shape()
        var physicsBody = PhysicsBody.create(_entity, bodyDef, playerShape)
        physicsBody.fixedRotation = true
        // Collide with the level (Wall bit), pass through other actors (see Main.start()).
        physicsBody.setCollisionFilter(CollisionLayers.getLayer("Actor"),
                                       CollisionLayers.getLayer("Wall") | CollisionLayers.getLayer("Trigger"))
    }
    
    update() {

    }

    fixedUpdate() {
        if (_scripted) {
            return
        }
        if (_interacting) {
            playerForceStop()
            return
        }
        if (_upAt != null) {
            if (Time.scaledTime < _upAt) {
                PhysicsBody.setLinearVelocity(_entity, Vec2.new())
                return
            }
            _upAt = null
            _stillSince = null
        }
        var inputDir = Input.getInputAxis2("Move")
        inputDir.y = inputDir.y * -1

        playerControl(inputDir)
    }

    onDestroy() {
        Debug.info("NPCBehavior.OnDestroy() called for: %(_name)")
    }
    
    onEvent(type, data) {        
        if (type == "CollisionBegin") {
            var other = event["otherEntity"]
            Debug.info("NPC %(_name) collided with entity: %(other)")
        } else if (type == "MessageReceived") {
            Debug.info("NPC %(_name) received message")
        }
    }

    // Snaps _facing to the nearest of the 8 sheet directions (+y is down the screen).
    getPlayerAnimation(animationType) { SpriteSheet.clipName(_facing, animationType) }

    playerForceStop() {
        if (_resting != null) Player.standParty_()
        _resting = null
        _settleAt = null
        _stillSince = null
        _upAt = null
        var velocity = Vec2.new()
        PhysicsBody.setLinearVelocity(_entity, velocity)
        var animation = getPlayerAnimation("Idle")
        SpriteAnimation.setCurrentAnimation(_entity, animation)
    }

    playerControl(dir) {
        var moving = dir.length() > 0.001
        // Resting, the first push gets them up before they can walk.
        if (moving && _resting != null) {
            getUp()
            return
        }
        if (moving) {
            if (_resting != null) Player.standParty_()
            _resting = null
            _stillSince = null
        } else if (rest_()) {
            return
        }

        // set player facing
        if (dir.length() > 0.001) {
            dir = dir.normalize()
            _facing = dir
        }

        _running = Input.isInputDown("Run") && Rooms.runnableAt(WorldTransform.getPosition(_entity))
        var velocity = dir * speed
        PhysicsBody.setLinearVelocity(_entity, velocity)

        if (dir.length() > 0.001) {
            var animation = getPlayerAnimation(_running ? "Run" : "Walk")
            SpriteAnimation.setCurrentAnimation(_entity, animation)
        } else {
            var animation = getPlayerAnimation("Idle")
            SpriteAnimation.setCurrentAnimation(_entity, animation)
        }
    }

    // Standing still: once it has been IDLE_REST_SECONDS, goes down with the lying-down animation and rests (a random
    // REST_CLIPS clip the sheet has) - a sit settles into its sitting idle once the lying-down animation ends. True
    // while resting, so the caller leaves the animation alone.
    rest_() {
        if (_resting != null) {
            PhysicsBody.setLinearVelocity(_entity, Vec2.new())
            if (_settleAt != null && Time.scaledTime >= _settleAt) {
                _settleAt = null
                SpriteAnimation.setCurrentAnimation(_entity, getPlayerAnimation(_resting))
            }
            return true
        }
        if (_stillSince == null) {
            _stillSince = Time.scaledTime
            return false
        }
        if (Time.scaledTime - _stillSince < IDLE_REST_SECONDS) return false
        var choice = REST_CLIPS[RNG.int(REST_CLIPS.count)]
        var animation = getPlayerAnimation(choice)
        if (SpriteSheet.clip(_entity, animation) == null) {
            _stillSince = Time.scaledTime
            return false
        }
        _resting = choice
        PhysicsBody.setLinearVelocity(_entity, Vec2.new())
        var down = getPlayerAnimation(DOWN_CLIP)
        var downClip = SpriteSheet.clip(_entity, down)
        if (downClip != null && choice != DOWN_CLIP) {
            SpriteAnimation.setCurrentAnimation(_entity, down)
            _settleAt = Time.scaledTime + downClip[2]
        } else {
            SpriteAnimation.setCurrentAnimation(_entity, animation)
        }
        Player.restParty_()
        return true
    }

    // The followers' scripts (GameObjects/PartyMember.wren) - the ones that can rest.
    static followers_ {
        var scripts = []
        for (member in Party.members) {
            if (member.isPlayer || member.entity == null || !GameObject.isValid(member.entity)) continue
            var script = Script.getInstance(member.entity)
            if (script != null && Reflect.hasMethod(script, "restSoon(_)")) scripts.add(script)
        }
        return scripts
    }

    // The player sat or lay down: every follower follows suit a moment later, each at its own time.
    static restParty_() {
        for (follower in Player.followers_) {
            follower.restSoon(RNG.float(PARTY_REST_DELAY_MIN, PARTY_REST_DELAY_MAX))
        }
    }

    // The player is getting up: so is everyone resting with them.
    static standParty_() {
        for (follower in Player.followers_) follower.standUp()
    }

    getInteractEntity() {
        var closestDistance = Math.infinity
        var playerWorldPosition = WorldTransform.getPosition(_entity)
        var closestEntity = null

        var interactEntities = GameObject.getAllWithComponents(["WorldTransform", "Script"])
        for (entity in interactEntities) {
            var script = Script.getInstance(entity)
            if (!script) {
                continue
            }
            if (!Reflect.hasMethod(script, "isInteractable()")) {
                continue
            }
            if (!script.isInteractable()) {
                continue
            }
            var interactableWorldPosition = WorldTransform.getPosition(entity)
            if (Reflect.hasMethod(script, "requiresFacing()") && script.requiresFacing() &&
                !isFacing_(playerWorldPosition, interactableWorldPosition)) {
                continue
            }
            var distance = Vec3.distance(interactableWorldPosition, playerWorldPosition)
            // Only the near ones are ray-cast (the costly check), and only if they'd beat the closest so far.
            if (distance >= INTERACTABLE_DISTANCE || distance >= closestDistance) {
                continue
            }
            if (!clearLineTo_(playerWorldPosition, interactableWorldPosition, entity, script)) {
                continue
            }
            if (distance < closestDistance) {
                //Debug.info(("Test the distance %(distance) for entity %(entity)"))
                closestDistance = distance
                closestEntity = entity
            }
        }
        //Debug.info(("Test for %(closestEntity) with %(closestDistance) < %(INTERACTABLE_DISTANCE)"))
        if (closestDistance < INTERACTABLE_DISTANCE) {
            return closestEntity
        }
        return null
    }

    // True when nothing solid is in the way from `from` to `to` (the position of the interactable `target`, whose
    // script is targetScript): a wall, a level prop, another NPC or door. Only static bodies block - the party following
    // you and roaming critters don't - and neither do the player or the target's own body. A hit inside the target's
    // footprint (see DEFAULT_INTERACT_SIZE) doesn't block either.
    clearLineTo_(from, to, target, targetScript) {
        var size = Reflect.hasMethod(targetScript, "interactSize()") ? targetScript.interactSize() : DEFAULT_INTERACT_SIZE
        var halfWidth = size[0] / 2 + FOOTPRINT_SLACK
        var halfHeight = size[1] / 2 + FOOTPRINT_SLACK
        for (hit in PhysicsQuery.rayCast(from.x, from.y, to.x, to.y, CollisionLayers.ALL)) {
            var hitEntity = hit[0]
            if (!hit[2] || hitEntity == _entity || hitEntity == target) {
                continue
            }
            var hitX = from.x + (to.x - from.x) * hit[1]
            var hitY = from.y + (to.y - from.y) * hit[1]
            if ((hitX - to.x).abs <= halfWidth && (hitY - to.y).abs <= halfHeight) {
                continue
            }
            return false
        }
        return true
    }

    // True when `target` lies within INTERACT_FACING_DEGREES of the way the player faces (or right on top of them).
    isFacing_(from, target) {
        var toTarget = Vec2.new(target.x - from.x, target.y - from.y)
        if (toTarget.length() < 0.001) {
            return true
        }
        var cosLimit = Math.cos(Math.radians(INTERACT_FACING_DEGREES))
        return Vec2.dot(toTarget.normalize(), _facing.normalize()) >= cosLimit - 0.0001
    }
}
