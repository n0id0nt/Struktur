// GameObjects/Chicken.wren
// A wandering chicken, built from Critter + the shared state mixins in GameObjects/States/. Two
// states: wander <-> idle. See GameObjects/Critter.wren for the shared FSM/animation plumbing this
// class relies on.
//
// Sprite sheet: Sprites/chicken.png - 112x96, 7x6 grid, 16x16 cells, pivot (8,8).
//   frames  anim
//   0-2     wander_front   3-5   wander_back    6-8   wander_right
//   9-11    run_front      12-14 run_back       15-17 run_right
//   18-24   peck_front     25-31 peck_back      32-38 peck_right
// There is no "left" strip - left-facing reuses the right strip mirrored (see Critter.playAnimation).

import "gameObjectComponents" for Sprite, PhysicsBody, SpriteAnimation, RenderLayer
import "renderer" for FlipBit
import "math" for Vec2
import "resourceManager" for Texture
import "animation" for SpriteAnimationDefinition
import "physics" for BodyDefinition, PhysicsCircleShape, BodyType, CollisionLayers
import "Colors" for WHITE
import "GameObjects/Critter" for Critter
import "GameObjects/States/IdleMixin" for IdleMixin
import "GameObjects/States/WanderMixin" for WanderMixin
import "GameObjects/Steering/Separation" for Separation

class Chicken is Critter {
    construct new(entity, args) {
        super(entity, args)
    }

    // Called by C++ once the base components exist - configure them here.
    start() {
        var texture = Texture.load("Sprites/chicken.png")
        Sprite.create(entity, texture, WHITE, Vec2.new(8, 8), 7, 6, FlipBit.NONE, 0, RenderLayer.ENTITIES, 0)
        texture.unload()

        var body = PhysicsBody.create(entity, BodyDefinition.new(BodyType.DYNAMIC_BODY),
                                      PhysicsCircleShape.new(0.12))
        body.fixedRotation = true
        // Collide with the level (Wall bit), pass through the player and other critters (see Main.start()).
        body.setCollisionFilter(CollisionLayers.getLayer("Actor"), CollisionLayers.getLayer("Wall"))

        var anim = SpriteAnimation.create(entity)
        // endFrame is exclusive - SpriteAnimationDefinition.new(start, end, seconds, loop).
        anim.addAnimation("WanderFront", SpriteAnimationDefinition.new(0, 3, 0.5, true))
        anim.addAnimation("WanderBack", SpriteAnimationDefinition.new(3, 6, 0.5, true))
        anim.addAnimation("WanderSide", SpriteAnimationDefinition.new(6, 9, 0.5, true))
        anim.addAnimation("RunFront", SpriteAnimationDefinition.new(9, 12, 0.35, true))
        anim.addAnimation("RunBack", SpriteAnimationDefinition.new(12, 15, 0.35, true))
        anim.addAnimation("RunSide", SpriteAnimationDefinition.new(15, 18, 0.35, true))
        anim.addAnimation("PeckFront", SpriteAnimationDefinition.new(18, 25, 1.0, true))
        anim.addAnimation("PeckBack", SpriteAnimationDefinition.new(25, 32, 1.0, true))
        anim.addAnimation("PeckSide", SpriteAnimationDefinition.new(32, 39, 1.0, true))

        addState("wander", WanderMixin.new(0.8, "Wander", 1.0, 5.0, "idle"))
        addState("idle", IdleMixin.new("Peck", 1.0, 3.0, "wander"))
        // Boid-style separation so chickens visually avoid overlapping - see Separation's own
        // comment for why this is a steering nudge rather than turning physical collision back on.
        addSteering(Separation.new(20, 1.2))
        changeState("wander")
    }

    fixedUpdate() {
        tickStates()
    }

    // Non-aggressive: chickens never chase, so they just join every other interactable (Item, NPC,
    // Door) in Player.getInteractEntity()'s closest-target scan - the player has to walk up and
    // press Interact to start a fight (see ExperimentState.checkCombatTriggers()).
    isInteractable() { true }
}
