// AUTO-GENERATED FILE - DO NOT EDIT
// Generated from C++ bindings
// Module: gameObject

class GameObject {
    // Destroy an Game Object and all its children.
    foreign static destroy(arg0)
    // Create a new Game Object with the given name and parent. Returns entity ID.
    foreign static create(arg0,arg1)
    // Create a new Game Object with the given name and parent. Returns entity ID.
    foreign static setParent(arg0,arg1)
    // Check if an entity ID is valid.
    foreign static isValid(arg0)
    // Checks if entity has a specific component.
    foreign static hasComponent(arg0)
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
    // Sets an entity to acitve.
    foreign static setActive(arg0)
    // Sets an entity to inacitve.
    foreign static setInactive(arg0)
}

