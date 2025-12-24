import "gameObject" for GameObject
import "gameObjectComponents" for WorldTransform, Script, Sprite, Camera, PhysicsBody, SpriteAnimation
import "math" for Vec2, Vec3, Vec4, Math
import "resourceManager" for Texture
import "animation" for SpriteAnimationDefinition
import "physics" for BodyDefinition, PhysicsCircleShape

import "reflect" for Reflect

var INTERACTABLE_DISTANCE = 64.0
var WHITE = Vec4.new(255, 255, 255, 255)

class Player {
    construct new(entity, args) {
        _entity = entity
        _name = args["1"]
        _timeAccumulator = 0
        _initialized = false
        _facing = Vec2.new(0, 1)
        _speed = 5
        
        System.print("Player constructed for: %(_name)")
    }
    
    // Called after C++ has created base components
    // Script configures/initializes component values
    start() {
        var texture = Texture.load("assets/Tiles/PlayerSpriteSheet.png")
        Sprite.create(_entity, texture, WHITE, Vec2.new(48, 64), 6, 3, false, 0, 3)
        texture.unload()
        var camera = Camera.create(_entity)
        camera.zoom = 1.5
        camera.forcePosition = true
        camera.damping = Vec2.new(4, 4)
        var bodyDef = BodyDefinition.createDynamicBody()
        var playerShape = PhysicsCircleShape.new(0.25)
        var physicsBody = PhysicsBody.create(_entity, bodyDef, playerShape)
        physicsBody.fixedRotation = true
        physicsBody.syncFromPhysics = true
        physicsBody.syncToPhysics = true

        var spriteAnimation = SpriteAnimation.create(_entity)

        var downIdleAnimation = SpriteAnimationDefinition.new(0, 2, 1, true)
        var upIdleAnimation = SpriteAnimationDefinition.new(6, 8, 1, true)
        var sideIdleAnimation = SpriteAnimationDefinition.new(12, 14, 1, true)
        var downRunAnimation = SpriteAnimationDefinition.new(2, 6, 0.7, true)
        var upRunAnimation = SpriteAnimationDefinition.new(8, 12, 0.7, true)
        var sideRunAnimation = SpriteAnimationDefinition.new(14, 18, 0.7, true)

        spriteAnimation.addAnimation("upIdleAnimation", upIdleAnimation)
        spriteAnimation.addAnimation("downIdleAnimation", downIdleAnimation)
        spriteAnimation.addAnimation("sideIdleAnimation", sideIdleAnimation)
        spriteAnimation.addAnimation("upRunAnimation", upRunAnimation)
        spriteAnimation.addAnimation("downRunAnimation", downRunAnimation)
        spriteAnimation.addAnimation("sideRunAnimation", sideRunAnimation)

        SpriteAnimation.setCurrentAnimation(_entity, "downIdleAnimation")
    }
    
    update() {

    }
    
    onDestroy() {
        System.print("NPCBehavior.OnDestroy() called for: %(_name)")
    }
    
    onEvent(event) {
        var eventType = event["type"]
        
        if (eventType == "CollisionBegin") {
            var other = event["otherEntity"]
            System.print("NPC %(_name) collided with entity: %(other)")
        } else if (eventType == "MessageReceived") {
            System.print("NPC %(_name) received message")
        }
    }

    getPlayerAnimation(animationType) {
        if (_facing.x > 0.01 || _facing.x < -0.01) {
            return "side%(animationType)Animation"
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

            if (dir.x > 0) {
                Sprite.setFlipped(_entity, false)
            } else if (dir.x < 0) {
                Sprite.setFlipped(_entity, true)
            }
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
