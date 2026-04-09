// AUTO-GENERATED FILE - DO NOT EDIT
// Generated from C++ bindings
// Module: gameObjectComponents

// PhysicsBody class wraps PhysicsBody component
foreign class PhysicsBody {
    // Sets the physics body fixed rotation
    foreign fixedRotation=(arg0)
    // Get if physics bodys to transform sync with the physics position
    foreign syncFromPhysics
    // Set if physics bodys to transform sync with the physics position
    foreign syncFromPhysics=(arg0)
    // Get if physics bodys to transform sync with the physics position
    foreign syncToPhysics
    // Set if physics bodys to transform sync with the physics position
    foreign syncToPhysics=(arg0)
    // Sets the linear velocity of a physics body.
    foreign linearVelocity
    // Create a physics body
    foreign static create(arg0,arg1,arg2)
    // Gets a physics body
    foreign static get(arg0)
    // Sets the linear velocity of a physics body.
    foreign static setLinearVelocity(arg0,arg1)
}

// Camera component class
foreign class Camera {
    // Get the zoom
    foreign zoom
    // Set the zoom
    foreign zoom=(arg0)
    // Get the forcePosition, will directly set the position of the next frame
    foreign forcePosition
    // Set the forcePosition, will directly set the position of the next frame
    foreign forcePosition=(arg0)
    // Get the damping
    foreign damping
    // Set the damping
    foreign damping=(arg0)
    // Add Trauma to camera for screen shake
    foreign addCameraTrauma(arg0)
    // Creates a camera component.
    foreign static create(arg0)
    // Gets a camera component.
    foreign static get(arg0)
    // Converts a world position to the screen position from the currently active camera.
    foreign static worldPosToScreenPos(arg0)
    // Converts a screen position to the world position from the currently active camera.
    foreign static screenPosToWorldPos(arg0)
    // Add Trauma to camera for screen shake active camera.
    foreign static addCameraTrauma(arg0,arg1)
}

// Level component class
foreign class Level {
    // Get the level index
    foreign index
    // Get the level width
    foreign width
    // Get the level height
    foreign height
    // Gets a level component.
    foreign static get(arg0)
}

// World component class
foreign class World {
    // Creates a level in the game and all its corresponding objects and entities.
    foreign loadLevelEntities(arg0)
    // Get the index of an Level in the world.
    foreign static getLevelIndex(arg0)
    // Gets a world component.
    foreign static get(arg0)
    // Creates a level in the game and all its corresponding objects and entities.
    foreign static loadLevelEntities(arg0,arg1)
    // Loads in a LDTK world file and creates the world game object and corresponding components.
    foreign static createWorldEntity(arg0)
    // Get the index of an Level in the world.
    foreign static getLevelIndex(arg0,arg1)
}

// Shader class wraps Shader component
foreign class Shader {
    // Sets the physics body fixed rotation
    foreign setFloatUniform(arg0,arg1)
    // Sets the physics body fixed rotation
    foreign setIntUniform(arg0,arg1)
    // Sets the physics body fixed rotation
    foreign setVec2Uniform(arg0,arg1)
    // Sets the physics body fixed rotation
    foreign setVec3Uniform(arg0,arg1)
    // Sets the physics body fixed rotation
    foreign setVec4Uniform(arg0,arg1)
    // Sets the physics body fixed rotation
    foreign setMat4Uniform(arg0,arg1)
    // Create a Shader body
    foreign static create(arg0,arg1)
    // Gets a Shader body
    foreign static get(arg0)
}

// Sprite animation class wraps SpriteAnimation component
foreign class SpriteAnimation {
    // Adds an animation to the sprite animation component
    foreign addAnimation(arg0,arg1)
    // Create a Sprite animation
    foreign static create(arg0)
    // Gets a Sprite animation
    foreign static get(arg0)
    // Will set and play a current sprite animation, is already playing the animation continue it.
    foreign static setCurrentAnimation(arg0,arg1)
    // Will play a sprite animation, and if playering animation will forcibly restart it.
    foreign static forcePlayAnimation(arg0,arg1)
    // Checks if a cirtain animation is playing.
    foreign static isAnimationPlaying(arg0,arg1)
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
foreign class Sprite {
    // Sets the sprites texture
    foreign texture
    // Sets the sprites texture
    foreign texture=(arg0)
    // Sets the sprites color
    foreign color
    // Sets the sprites color
    foreign color=(arg0)
    // Sets the sprites offset
    foreign offset
    // Sets the sprites offset
    foreign offset=(arg0)
    // Sets the sprites columns
    foreign columns
    // Sets the sprites columns
    foreign columns=(arg0)
    // Sets the sprites rows
    foreign rows
    // Sets the sprites rows
    foreign rows=(arg0)
    // Sets the sprites flipped
    foreign flipped
    // Sets the sprites flipped
    foreign flipped=(arg0)
    // Sets the sprites index
    foreign index
    // Sets the sprites index
    foreign index=(arg0)
    // Sets the sprites render
    foreign renderPriority
    // Sets the sprites render
    foreign renderPriority=(arg0)
    // Creates the sprite Component.
    foreign static create(arg0,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8)
    // Gets a Sprite animation
    foreign static get(arg0)
    // Sets the render priority of a sprite component
    foreign static setRenderPriority(arg0,arg1)
    // Flips a sprite in a horizontal direction
    foreign static setFlipped(arg0,arg1)
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
    // Creates the script Component with an arg.
    foreign static createArg(arg0,arg1,arg2,arg3)
    // Gets the script component
    foreign static get(arg0)
    // Gets a script instance
    foreign static getInstance(arg0)
}

