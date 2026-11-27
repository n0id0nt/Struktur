// AUTO-GENERATED FILE - DO NOT EDIT
// Generated from C++ bindings
// Module: gameObjectComponents

import "serialisation" for MapUtil

// PhysicsBody class wraps PhysicsBody component
foreign class PhysicsBody {
    // Sets the physics body fixed rotation
    foreign fixedRotation=(arg0)
    // Sets the linear velocity of a physics body.
    foreign linearVelocity
    // Sets which collision layer this body belongs to (categoryBits) and which layers it collides with (maskBits) - see physics.CollisionLayers for allocating layer bits by name.
    foreign setCollisionFilter(arg0,arg1)
    // Gets whether this body is a sensor (reports collisionEnter/Exit but pushes nothing)
    foreign isSensor
    // Sets whether this body is a sensor (reports collisionEnter/Exit but pushes nothing)
    foreign isSensor=(arg0)
    // Create a physics body
    foreign static create(arg0,arg1,arg2)
    // Gets a physics body
    foreign static get(arg0)
    // Sets the linear velocity of a physics body.
    foreign static setLinearVelocity(arg0,arg1)
}

// Sprite animation class wraps SpriteAnimation component
foreign class LocalTransform {
    // Sets the sprites texture
    foreign position
    // Sets the sprites texture
    foreign position=(arg0)
    // Sets the sprites texture
    foreign rotation
    // Sets the sprites texture
    foreign rotation=(arg0)
    // Sets the sprites texture
    foreign scale
    // Sets the sprites texture
    foreign scale=(arg0)
    // Sets the sprites texture
    foreign matrix
    // Sets the sprites texture
    foreign matrix=(arg0)
    // Get the position of an entity. Returns vec3 or null if no transform.
    foreign static get(arg0)
    // Get the position of an entity. Returns vec3 or null if no transform.
    foreign static getPosition(arg0)
    // Set the position of an entity.
    foreign static setPosition(arg0,arg1)
    // Get rotation of entity as Quat.
    foreign static getRotation(arg0)
    // Set rotation of entity from Quat.
    foreign static setRotation(arg0,arg1)
    // Get the position of an entity. Returns vec3 or null if no transform.
    foreign static getScale(arg0)
    // Set the position of an entity.
    foreign static setScale(arg0,arg1)
    // Get the position of an entity. Returns vec3 or null if no transform.
    foreign static getMatrix(arg0)
    // Set the position of an entity.
    foreign static setMatrix(arg0,arg1)
}

// Sprite animation class wraps SpriteAnimation component
foreign class WorldTransform {
    // Sets the sprites texture
    foreign position
    // Sets the sprites texture
    foreign position=(arg0)
    // Sets the sprites texture
    foreign rotation
    // Sets the sprites texture
    foreign rotation=(arg0)
    // Sets the sprites texture
    foreign scale
    // Sets the sprites texture
    foreign scale=(arg0)
    // Sets the sprites texture
    foreign matrix
    // Sets the sprites texture
    foreign matrix=(arg0)
    // Get the position of an entity. Returns vec3 or null if no transform.
    foreign static get(arg0)
    // Get the position of an entity. Returns vec3 or null if no transform.
    foreign static getPosition(arg0)
    // Set the position of an entity.
    foreign static setPosition(arg0,arg1)
    // Get rotation of entity as Quat.
    foreign static getRotation(arg0)
    // Set rotation of entity from Quat.
    foreign static setRotation(arg0,arg1)
    // Get the position of an entity. Returns vec3 or null if no transform.
    foreign static getScale(arg0)
    // Set the position of an entity.
    foreign static setScale(arg0,arg1)
    // Get the position of an entity. Returns vec3 or null if no transform.
    foreign static getMatrix(arg0)
    // Set the position of an entity.
    foreign static setMatrix(arg0,arg1)
}

// Script class wraps SpriteAnimation component
foreign class Script {
    // Gets a script instance
    foreign getInstance()
    // Checks if script is initialised
    foreign isInitialised
    // Checks if script has error
    foreign hasError
    // Gets Scripts error message
    foreign errorMessage
    // Creates the script Component.
    foreign static create(arg0,arg1)
    // Internal: creates the script Component from a MapUtil-encoded arg map. Prefer createArg(_,_,_).
    foreign static createArgPairs(arg0,arg1,arg2)
    // Gets the script component
    foreign static get(arg0)
    // Gets a script instance
    foreign static getInstance(arg0)
    static createArg(entity, className, args) {
        return createArgPairs(entity, className, MapUtil.mapToPairs(args))
    }
}

