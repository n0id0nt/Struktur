// AUTO-GENERATED FILE - DO NOT EDIT
// Generated from C++ bindings
// Module: physics

// BodyDefinition class wraps b2BodyDef
foreign class BodyDefinition {
    // Get the BodyDefinition as a dynamic Body
    foreign static createDynamicBody()
    // Get the BodyDefinition as a static Body
    foreign static createStaticBody()
    // Get the BodyDefinition as a kinematic Body
    foreign static createKinematicBody()
}

// BodyDefinition class wraps b2BodyDef
foreign class PhysicsCircleShape {
    // Create physics circle shape
    foreign construct new()
    // Create physics circle shape with a radius components
    foreign construct new(radius)
    // Get physics circle shape's radius
    foreign radius
    // Set physics circle shape's radius
    foreign radius=(arg0)
}

