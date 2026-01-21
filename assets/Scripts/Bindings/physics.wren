// AUTO-GENERATED FILE - DO NOT EDIT
// Generated from C++ bindings
// Module: physics

// Enum of the different phyics body types
class BodyType {
    static STATIC_BODY { 0 }
    static KINEMATIC_BODY { 1 }
    static DYNAMIC_BODY { 2 }
}

// BodyDefinition class wraps b2BodyDef
foreign class BodyDefinition {
    // Create the physics body definition, takes the body type as the argement
    foreign construct new(arg0)
    // Gets the Body Definition Type
    foreign type
    // Sets the Body Definition Type
    foreign type=(arg0)
}

// BodyDefinition class wraps b2BodyDef
foreign class PhysicsCircleShape {
    // Create physics circle shape
    foreign construct new()
    // Create physics circle shape with a radius components
    foreign construct new(arg0)
    // Get physics circle shape's radius
    foreign radius
    // Set physics circle shape's radius
    foreign radius=(arg0)
}

