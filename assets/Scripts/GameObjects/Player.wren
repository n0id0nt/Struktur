import "gameObject" for GameObject
import "gameObjectComponents" for WorldTransform, Script, Sprite, Camera, PhysicsBody, SpriteAnimation, RenderLayer
import "math" for Vec2, Vec3, Vec4, Math
import "resourceManager" for Texture
import "animation" for SpriteAnimationDefinition
import "physics" for BodyDefinition, PhysicsCircleShape, BodyType
import "input" for Input

import "reflect" for Reflect
import "Colors" for WHITE

var INTERACTABLE_DISTANCE = 64.0

class Player {
    construct new(entity, args) {
        _entity = entity
        _name = args["Name"]
        _timeAccumulator = 0
        _initialized = false
        _facing = Vec2.new(0, 1)
        _speed = 2
        _fixedUpdateCount = 0

        System.print("Player constructed for: %(_name)")
    }
    
    name { _name }

    #!export
    speed { _speed }
    speed=(value) { _speed = value }

    // Called after C++ has created base components
    // Script configures/initializes component values
    start() {
        var texture = Texture.load("Sprites/player.png")
        Sprite.create(_entity, texture, WHITE, Vec2.new(19, 17), 10, 10, false, 0, RenderLayer.ENTITIES, 0)
        texture.unload()
        var camera = Camera.create(_entity)
        camera.zoom = 5
        camera.forcePosition = true
        camera.damping = Vec2.new(4, 4)
        var bodyDef = BodyDefinition.new(BodyType.DYNAMIC_BODY)
        var playerShape = PhysicsCircleShape.new(0.25)
        var physicsBody = PhysicsBody.create(_entity, bodyDef, playerShape)
        physicsBody.fixedRotation = true

        var spriteAnimation = SpriteAnimation.create(_entity)

        // Define Idle Animations
        var downIdleAnimation = SpriteAnimationDefinition.new(0, 1, 1, true)
        var upIdleAnimation = SpriteAnimationDefinition.new(1, 2, 1, true)
        var leftIdleAnimation = SpriteAnimationDefinition.new(2, 3, 1, true)
        var rightIdleAnimation = SpriteAnimationDefinition.new(3, 4, 1, true)

        // Define Run Animations
        var downRunAnimation = SpriteAnimationDefinition.new(4, 12, 0.7, true)
        var upRunAnimation = SpriteAnimationDefinition.new(12, 17, 0.7, true)
        var leftRunAnimation = SpriteAnimationDefinition.new(18, 26, 0.7, true)
        var rightRunAnimation = SpriteAnimationDefinition.new(26, 34, 0.7, true)

        // Define Attack Animations
        var downAttackAnimation = SpriteAnimationDefinition.new(34, 42, 0.7, true)
        var upAttackAnimation = SpriteAnimationDefinition.new(42, 50, 0.7, true)
        var leftAttackAnimation = SpriteAnimationDefinition.new(50, 58, 0.7, true)
        var rightAttackAnimation = SpriteAnimationDefinition.new(58, 65, 0.7, true)

        spriteAnimation.addAnimation("downIdleAnimation", downIdleAnimation)
        spriteAnimation.addAnimation("upIdleAnimation", upIdleAnimation)
        spriteAnimation.addAnimation("leftIdleAnimation", leftIdleAnimation)
        spriteAnimation.addAnimation("rightIdleAnimation", rightIdleAnimation)
        spriteAnimation.addAnimation("downRunAnimation", downRunAnimation)
        spriteAnimation.addAnimation("upRunAnimation", upRunAnimation)
        spriteAnimation.addAnimation("leftRunAnimation", leftRunAnimation)
        spriteAnimation.addAnimation("rightRunAnimation", rightRunAnimation)
        spriteAnimation.addAnimation("downAttackAnimation", downAttackAnimation)
        spriteAnimation.addAnimation("upAttackAnimation", upAttackAnimation)
        spriteAnimation.addAnimation("leftAttackAnimation", leftAttackAnimation)
        spriteAnimation.addAnimation("rightAttackAnimation", rightAttackAnimation)

        SpriteAnimation.setCurrentAnimation(_entity, "downIdleAnimation")
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

    getPlayerAnimation(animationType) {
        if (_facing.x > 0.01) {
            return "right%(animationType)Animation"
        }
        if (_facing.x < -0.01) {
            return "left%(animationType)Animation"
        }
        if (_facing.y < 0.01) {
            return "up%(animationType)Animation"
        }
        return "down%(animationType)Animation"
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
