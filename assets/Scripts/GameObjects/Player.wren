import "gameObject" for GameObject
import "gameObjectComponents" for WorldTransform, Script, Camera, PhysicsBody, SpriteAnimation
import "math" for Vec2, Vec3, Vec4, Math
import "physics" for BodyDefinition, PhysicsCircleShape, BodyType, CollisionLayers
import "input" for Input

import "reflect" for Reflect

import "Combat/Config/BattlePlayer" for BattlePlayer
import "Sprites/ExplorerSprite" for ExplorerSprite

var INTERACTABLE_DISTANCE = 64.0
// Animation direction prefixes by octant of atan2(y, x) - +x is right, +y is down the screen.
var FACING_NAMES = ["right", "downRight", "down", "downLeft", "left", "upLeft", "up", "upRight"]

class Player {
    construct new(entity, args) {
        _entity = entity
        _name = args["Name"]
        _timeAccumulator = 0
        _initialized = false
        _facing = Vec2.new(0, 1)
        _speed = 3
        _fixedUpdateCount = 0
        _combatant = null

        System.print("Player constructed for: %(_name)")
    }

    name { _name }

    #!export
    speed { _speed }
    speed=(value) { _speed = value }

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
        physicsBody.setCollisionFilter(CollisionLayers.getLayer("Actor"), CollisionLayers.getLayer("Wall"))
    }
    
    update() {

    }

    fixedUpdate() {
        var inputDir = Input.getInputAxis2("Move")
        inputDir.y = inputDir.y * -1

        playerControl(inputDir)
    }

    onDestroy() {
        System.print("NPCBehavior.OnDestroy() called for: %(_name)")
    }
    
    onEvent(type, data) {        
        if (type == "CollisionBegin") {
            var other = event["otherEntity"]
            System.print("NPC %(_name) collided with entity: %(other)")
        } else if (type == "MessageReceived") {
            System.print("NPC %(_name) received message")
        }
    }

    // Snaps _facing to the nearest of the 8 sheet directions (+y is down the screen).
    getPlayerAnimation(animationType) {
        var octant = (_facing.y.atan(_facing.x) / (Num.pi / 4)).round % 8
        if (octant < 0) octant = octant + 8
        return "%(FACING_NAMES[octant])%(animationType)Animation"
    }

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

        var velocity = dir * _speed
        PhysicsBody.setLinearVelocity(_entity, velocity)

        if (dir.length() > 0.001) {
            var animation = getPlayerAnimation("Run")
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
            var distance = Vec3.distance(interactableWorldPosition, playerWorldPosition)
            if (distance < closestDistance) {
                //System.print(("Test the distance %(distance) for entity %(entity)"))
                closestDistance = distance
                closestEntity = entity
            }
        }
        //System.print(("Test for %(closestEntity) with %(closestDistance) < %(INTERACTABLE_DISTANCE)"))
        if (closestDistance < INTERACTABLE_DISTANCE) {
            return closestEntity
        }
        return null
    }
}
