import "gameObject" for GameObject
import "gameObjectComponents" for WorldTransform, Script, Camera, PhysicsBody, SpriteAnimation
import "math" for Vec2, Vec3, Vec4, Math
import "physics" for BodyDefinition, PhysicsCircleShape, BodyType, CollisionLayers
import "input" for Input

import "reflect" for Reflect

import "Combat/Config/BattlePlayer" for BattlePlayer
import "Sprites/ExplorerSprite" for ExplorerSprite
import "Sprites/SpriteSheet" for SpriteSheet
import "debug" for Debug

var INTERACTABLE_DISTANCE = 64.0
// Interactables that ask for it (requiresFacing() - NPCs, signs) only count when they're within this many degrees
// of the way the player is facing.
var INTERACT_FACING_DEGREES = 90

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

        Debug.info("Player constructed for: %(_name)")
    }

    name { _name }

    // Movement speeds in m/s: walking is the default, holding the "Run" input runs.
    #!export
    walkSpeed { _walkSpeed }
    walkSpeed=(value) { _walkSpeed = value }

    #!export
    runSpeed { _runSpeed }
    runSpeed=(value) { _runSpeed = value }

    // True during an interact / dialogue sequence (States/InteractState.wren): the player ignores every input and
    // just stands idle, while the world - NPCs, critters, animations - carries on around them.
    interacting { _interacting }

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
    // only happens on death/respawn, when ExperimentState rebuilds the player entity and this script
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
        var camera = Camera.create(_entity)
        camera.zoom = 5
        camera.forcePosition = true
        camera.damping = Vec2.new(4, 4)
        var bodyDef = BodyDefinition.new(BodyType.DYNAMIC_BODY)
        var playerShape = PhysicsCircleShape.new(0.25)
        var physicsBody = PhysicsBody.create(_entity, bodyDef, playerShape)
        physicsBody.fixedRotation = true
        // Collide with the level (Wall bit), pass through other actors (see Main.start()).
        physicsBody.setCollisionFilter(CollisionLayers.getLayer("Actor"),
                                       CollisionLayers.getLayer("Wall") | CollisionLayers.getLayer("Trigger"))
    }
    
    update() {

    }

    fixedUpdate() {
        if (_interacting) {
            playerForceStop()
            return
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
        var velocity = Vec2.new()
        PhysicsBody.setLinearVelocity(_entity, velocity)
        var animation = getPlayerAnimation("Idle")
        SpriteAnimation.setCurrentAnimation(_entity, animation)
    }

    playerControl(dir) {
        // set player facing
        if (dir.length() > 0.001) {
            dir = dir.normalize()
            _facing = dir
        }

        _running = Input.isInputDown("Run")
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

    // True when `target` lies within INTERACT_FACING_DEGREES of the way the player faces (or right on top of them).
    isFacing_(from, target) {
        var toTarget = Vec2.new(target.x - from.x, target.y - from.y)
        if (toTarget.length() < 0.001) {
            return true
        }
        var cosLimit = (INTERACT_FACING_DEGREES * Num.pi / 180).cos
        return Vec2.dot(toTarget.normalize(), _facing.normalize()) >= cosLimit - 0.0001
    }
}
