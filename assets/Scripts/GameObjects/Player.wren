import "game" for Transform, Vec2, Vec3, Vec4, PhysicsBody, SpriteAnimation, Math, GameObject, Sprite, Script, SpriteAnimationDefinition, ResourceManager, Camera, BodyDefinition, PhysicsCircleShape

var INTERACTABLE_DISTANCE = 64.0
var WHITE = Vec4.new(255, 255, 255, 255)

class Player {
    construct new(entity, name) {
        _entity = entity
        _name = name
        _timeAccumulator = 0
        _initialized = false
        _facing = Vec2.new()
        _speed = Vec2.new()
        
        System.print("Player constructed for: %(_name)")
    }
    
    // Called after C++ has created base components
    // Script configures/initializes component values
    create(entity) {
        var texture = ResourceManager.getTextureResource("assets/Tiles/PlayerSpriteSheet.png")
        Sprite.create(entity, texture, WHITE, Vec2.new(48, 64), 6, 3, false, 0, 3)
        var camera = Camera.create(entity)
        camera.zoom = 1.5
        camera.forcePosition = true
        camera.damping = Vec2.new(4, 4)
        var bodyDef = BodyDefinition.createDynamicBody()
        var playerShape = PhysicsCircleShape.new(0.25)
        var physicsBody = PhysicsBody.create(entity, bodyDef, playerShape)
        physicsBody.fixedRotation = true
        physicsBody.syncFromPhysics = true
        physicsBody.syncToPhysics = true

        var spriteAnimation = SpriteAnimation.create(entity)

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
    }
    
    update(dt) {

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
        if (_facing.y > 0.01) {
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

    playerControl(dirX, dirY) {
        var dir = Vec2.new(dirX, dirY)
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

    canInteract() {
        var closestDistance = Math.infinity
        var playerWorldPosition = Transform.getPosition(_entity)
        var closestEntity = null

        Transform.setTransform()
        GameObject.forEachWithComponent(["WorldTransform", "Script"]) { |entity|
            if (!Script.call(entity, "isInteractable")) {
                return
            }
            var interactableWorldPosition = Transform.getPosition(entity)
            var distance = Vec3.distance(interactableWorldPosition, playerWorldPosition)
            if (distance < closestDistance) {
                closestDistance = distance
                closestEntity = entity
            }
        }
        if (closestDistance < INTERACTABLE_DISTANCE) {
            return closestEntity
        }
        return null
    }
}
