// AUTO-GENERATED FILE - DO NOT EDIT
// Generated from C++ bindings
// Module: game

class Entity {
    // Create a new entity with the given name and parent. Returns entity ID.
    foreign static create(arg0,arg1)
    // Destroy an entity and all its children.
    foreign static destroy(arg0)
    // Check if an entity ID is valid.
    foreign static isValid(arg0)
}

class Transform {
    // Get the world position of an entity. Returns [x, y, z] or null if no transform.
    foreign static getPosition(arg0)
    // Set the world position of an entity.
    foreign static setPosition(arg0,arg1,arg2,arg3)
    // Set the local position of an entity.
    foreign static setLocal(arg0,arg1,arg2,arg3)
    // Get the local position of an entity. Returns [x, y, z] or null if no transform.
    foreign static getLocal(arg0)
}

