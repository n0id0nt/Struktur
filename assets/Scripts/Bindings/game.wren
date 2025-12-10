// AUTO-GENERATED FILE - DO NOT EDIT
// Generated from C++ bindings
// Module: game

class Math {
    // Positive infinity
    foreign static infinity
    // Negative infinity
    foreign static negInfinity
    // Not a Number
    foreign static nan
    // Pi constant
    foreign static pi
    // Euler's number
    foreign static e
    // Maximum of two numbers
    foreign static max(arg0,arg1)
    // Minimum of two numbers
    foreign static min(arg0,arg1)
    // Clamp value between min and max
    foreign static clamp(arg0,arg1,arg2)
    // Absolute value
    foreign static abs(arg0)
    // Square root
    foreign static sqrt(arg0)
    // Power
    foreign static pow(arg0,arg1)
    // Linear interpolation
    foreign static lerp(arg0,arg1,arg2)
    // Sign of number (-1, 0, 1)
    foreign static sign(arg0)
    // Sine
    foreign static sin(arg0)
    // Cosine
    foreign static cos(arg0)
    // Tangent
    foreign static tan(arg0)
    // Arc sine
    foreign static asin(arg0)
    // Arc cosine
    foreign static acos(arg0)
    // Arc tangent
    foreign static atan(arg0)
    // Arc tangent of y/x
    foreign static atan2(arg0,arg1)
    // Floor
    foreign static floor(arg0)
    // Ceiling
    foreign static ceil(arg0)
    // Round to nearest integer
    foreign static round(arg0)
    // Check if infinite
    foreign static isInfinite(arg0)
    // Check if NaN
    foreign static isNaN(arg0)
    // Check if finite
    foreign static isFinite(arg0)
    // Convert degrees to radians
    foreign static radians(arg0)
    // Convert radians to degrees
    foreign static degrees(arg0)
}

// 2D vector class wrapping glm::vec2
foreign class Vec2 {
    // Create zero vector
    foreign construct new()
    // Create vector with x, y components
    foreign construct new(x, y)
    // Get X component
    foreign x
    // Get Y component
    foreign y
    // Set X component
    foreign x=(arg0)
    // Set Y component
    foreign y=(arg0)
    // Convert to string
    foreign toString
    // Add two vectors
    foreign +(arg0)
    // Subtract vectors
    foreign -(arg0)
    // Multiply by scalar
    foreign *(arg0)
    // Get vector length
    foreign length
    // Get normalized vector
    foreign normalize()
    // Distance between two vectors
    foreign static distance(arg0,arg1)
    // Squared distance
    foreign static distanceSquared(arg0,arg1)
    // Linear interpolation
    foreign static lerp(arg0,arg1,arg2)
    // Dot product (static)
    foreign static dot(arg0,arg1)
    // Reflect vector
    foreign static reflect(arg0,arg1)
    // Component-wise minimum
    foreign static min(arg0,arg1)
    // Component-wise maximum
    foreign static max(arg0,arg1)
    // Clamp vector
    foreign static clamp(arg0,arg1,arg2)
    // Zero vector (0, 0)
    foreign static zero()
    // One vector (1, 1)
    foreign static one()
    // Right vector (1, 0)
    foreign static right()
    // Up vector (0, 1)
    foreign static up()
    // Left vector (-1, 0)
    foreign static left()
    // Down vector (0, -1)
    foreign static down()
}

// 3D vector class wrapping glm::vec3
foreign class Vec3 {
    // Create zero vector
    foreign construct new()
    // Create vector with x, y, z components
    foreign construct new(x, y, z)
    // Get X component
    foreign x
    // Get Y component
    foreign y
    // Get Z component
    foreign z
    // Set X component
    foreign x=(arg0)
    // Set Y component
    foreign y=(arg0)
    // Set Z component
    foreign z=(arg0)
    // Convert to string
    foreign toString
    // Add two vectors
    foreign +(arg0)
    // Subtract vectors
    foreign -(arg0)
    // Multiply by scalar
    foreign *(arg0)
    // Get vector length
    foreign length
    // Get normalized vector
    foreign normalize()
    // Distance between two vectors
    foreign static distance(arg0,arg1)
    // Squared distance
    foreign static distanceSquared(arg0,arg1)
    // Linear interpolation
    foreign static lerp(arg0,arg1,arg2)
    // Dot product (static)
    foreign static dot(arg0,arg1)
    // Cross product (static)
    foreign static cross(arg0,arg1)
    // Reflect vector
    foreign static reflect(arg0,arg1)
    // Refract vector
    foreign static refract(arg0,arg1,arg2)
    // Component-wise minimum
    foreign static min(arg0,arg1)
    // Component-wise maximum
    foreign static max(arg0,arg1)
    // Clamp vector
    foreign static clamp(arg0,arg1,arg2)
    // Zero vector (0, 0, 0)
    foreign static zero()
    // One vector (1, 1, 1)
    foreign static one()
    // Right vector (1, 0, 0)
    foreign static right()
    // Up vector (0, 1, 0)
    foreign static up()
    // Forward vector (0, 0, -1)
    foreign static forward()
    // Left vector (-1, 0, 0)
    foreign static left()
    // Down vector (0, -1, 0)
    foreign static down()
    // Back vector (0, 0, 1)
    foreign static back()
}

// 4D vector class wrapping glm::vec4
foreign class Vec4 {
    // Create zero vector
    foreign construct new()
    // Create vector with x, y, z, w components
    foreign construct new(x, y, z, w)
    // Get X component
    foreign x
    // Get Y component
    foreign y
    // Get Z component
    foreign z
    // Get W component
    foreign w
    // Set X component
    foreign x=(arg0)
    // Set Y component
    foreign y=(arg0)
    // Set Z component
    foreign z=(arg0)
    // Set W component
    foreign w=(arg0)
    // Convert to string
    foreign toString
    // Add two vectors
    foreign +(arg0)
    // Subtract vectors
    foreign -(arg0)
    // Multiply by scalar
    foreign *(arg0)
    // Get vector length
    foreign length
    // Get normalized vector
    foreign normalize()
    // Distance between two vectors
    foreign static distance(arg0,arg1)
    // Squared distance
    foreign static distanceSquared(arg0,arg1)
    // Linear interpolation
    foreign static lerp(arg0,arg1,arg2)
    // Dot product (static)
    foreign static dot(arg0,arg1)
    // Component-wise minimum
    foreign static min(arg0,arg1)
    // Component-wise maximum
    foreign static max(arg0,arg1)
    // Clamp vector
    foreign static clamp(arg0,arg1,arg2)
    // Zero vector (0, 0, 0, 0)
    foreign static zero()
    // One vector (1, 1, 1, 1)
    foreign static one()
}

// Quaternion class for 3D rotations wrapping glm::quat
foreign class Quat {
    // Create identity quaternion
    foreign construct new()
    // Create quaternion with w, x, y, z components
    foreign construct new(w, x, y, z)
    // Get W component
    foreign w
    // Get X component
    foreign x
    // Get Y component
    foreign y
    // Get Z component
    foreign z
    // Set W component
    foreign w=(arg0)
    // Set X component
    foreign x=(arg0)
    // Set Y component
    foreign y=(arg0)
    // Set Z component
    foreign z=(arg0)
    // Convert to string
    foreign toString
    // Get normalized quaternion
    foreign normalize()
    // Get inverse quaternion
    foreign inverse()
    // Get conjugate quaternion
    foreign conjugate()
    // Get quaternion length
    foreign length
    // Convert to Euler angles (radians)
    foreign toEuler()
    // Convert to Euler angles (degrees)
    foreign toEulerDegrees()
    // Convert to axis-angle representation
    foreign toAxisAngle()
    // Multiply quaternions
    foreign *(arg0)
    // Rotate vector by quaternion
    foreign rotate(arg0)
    // Dot product
    foreign dot(arg0)
    // Get forward direction vector
    foreign forward()
    // Get up direction vector
    foreign up()
    // Get right direction vector
    foreign right()
    // Create identity quaternion
    foreign static identity()
    // Create quaternion from axis and angle
    foreign static fromAxisAngle(arg0,arg1)
    // Create quaternion from Euler angles (radians)
    foreign static fromEuler(arg0,arg1,arg2)
    // Create quaternion from Euler angles (degrees)
    foreign static fromEulerDegrees(arg0,arg1,arg2)
    // Create quaternion from forward and up vectors
    foreign static lookAt(arg0,arg1)
    // Spherical linear interpolation
    foreign static slerp(arg0,arg1,arg2)
    // Linear interpolation
    foreign static lerp(arg0,arg1,arg2)
}

// Texture resource handle
foreign class Texture {
    // Load texture from path
    foreign static load(arg0)
    // Check if texture is valid
    foreign isValid
    // Get texture path
    foreign path
    // Get texture width
    foreign width
    // Get texture height
    foreign height
    // Convert to string
    foreign toString
}

// Font resource handle
foreign class Music {
    // Load music from path
    foreign static load(arg0)
    // Check if music is valid
    foreign isValid
    // Get music path
    foreign path
    // Convert to string
    foreign toString
}

// Font resource handle
foreign class Font {
    // Load font from path
    foreign static load(arg0)
    // Check if font is valid
    foreign isValid
    // Get font path
    foreign path
    // Get font size
    foreign size
    // Convert to string
    foreign toString
}

// Font resource handle
foreign class Sound {
    // Load sound from path
    foreign static load(arg0)
    // Check if sound is valid
    foreign isValid
    // Get sound path
    foreign path
    // Convert to string
    foreign toString
}

// UI Label component
foreign class UILabel {
    // Sets Label to be visible
    foreign setVisible(arg0)
    // Sets the labels font
    foreign setFont(arg0)
    // Sets the labels text color
    foreign setTextColor(arg0)
    // Sets the labels position
    foreign setPosition(arg0)
    // Sets the labels anchor point
    foreign setAnchorPoint(arg0)
}

class GameObject {
    // Create a new Game Object with the given name and parent. Returns entity ID.
    foreign static create(arg0,arg1)
    // Destroy an Game Object and all its children.
    foreign static destroy(arg0)
    // Check if an entity ID is valid.
    foreign static isValid(arg0)
    // Get all entities with a specific component. Returns list of entity IDs.
    foreign static getAllWithComponent(arg0)
    // Get all entities with ALL specified components. Pass list of component names.
    foreign static getAllWithComponents(arg0)
    // Get entities with ANY of the specified components.
    foreign static getAllWithAnyComponents(arg0)
    // Get all entities in the registry. Returns list of entity IDs.
    foreign static getAll()
    // Get all entities with a specific identifier. Returns list of entity IDs.
    foreign static getAllWithIdentifier(arg0)
    // Iterate all entities, calling callback for each.
    foreign static forEach(arg0)
    // Iterate entities with component, calling callback for each.
    foreign static forEachWithComponent(arg0,arg1)
    // Iterate entities with component, calling callback for each.
    foreign static forEachWithComponents(arg0,arg1)
    // Iterate entities with component, calling callback for each.
    foreign static forEachWithAnyComponents(arg0,arg1)
}

class Application {
    // Change the size of the game window.
    foreign static setWindowSize(arg0,arg1)
    // Changes the name of the game window.
    foreign static setApplicationName(arg0)
}

class Input {
    // Gets input dir of a key code.
    foreign static getInputAxis2(arg0)
    // Gets input was just released.
    foreign static isInputJustReleased(arg0)
}

class Transform {
    // Get the world position of an entity. Returns vec3 or null if no transform.
    foreign static getPosition(arg0)
    // Set the world position of an entity.
    foreign static setPosition(arg0,arg1)
    // Set the local position of an entity.
    foreign static setLocalPosition(arg0,arg1)
    // Get the local position of an entity. Returns [x, y, z] or null if no transform.
    foreign static getLocalPosition(arg0)
    // Get rotation of entity as Quat.
    foreign static getRotation(arg0)
    // Set local rotation of entity from Quat.
    foreign static setLocalRotation(arg0,arg1)
    // Set rotation of entity from Quat.
    foreign static setRotation(arg0,arg1)
    // Get local rotation of entity as Quat.
    foreign static getLocalRotation(arg0)
}

class Inventory {
    // Check if an item is contained in the inventorty.
    foreign static contains(arg0)
}

class World {
    // Get the index of an Level in the world.
    foreign static getLevelIndex(arg0,arg1)
}

class Level {
    // Loads in a LDTK world file and creates the world game object and corresponding components.
    foreign static createWorldEntity(arg0)
    // Creates a level in the game and all its corresponding objects and entities.
    foreign static loadLevelEntities(arg0,arg1)
}

class ResourceManager {
    // Creates a pointer to the font resource in the resource pool.
    foreign static getFontResource(arg0)
    // Creates a pointer to the texture resource in the resource pool.
    foreign static getTextureResource(arg0)
}

class SpriteComponent {
    // Creates the sprite Component.
    foreign static create(arg0,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8)
}

class ScriptComponent {
    // Sets the render priority of a sprite component
    foreign static setRenderPriority(arg0,arg1)
    // Creates the script Component.
    foreign static create(arg0,arg1,arg2)
    // Creates the script Component with an arg.
    foreign static createArg(arg0,arg1,arg2,arg3)
    // Check if entity's script has a method
    foreign static hasMethod(arg0,arg1)
    // Call a method on another entity's script with arguments
    foreign static callArg(arg0,arg1,arg2)
    // Call a method on another entity's script
    foreign static call(arg0,arg1)
}

class UIManager {
    // Creates the UI component for UILabel.
    foreign static createUILabel(arg0,arg1,arg2,arg3)
    // Creates the UI component for UILabel.
    foreign static removeUILabel(arg0)
}

class Camera {
    // Converts a world position to the screen position.
    foreign static worldPosToScreenPos(arg0)
    // Converts a screen position to the world position.
    foreign static screenPosToWorldPos(arg0)
}

