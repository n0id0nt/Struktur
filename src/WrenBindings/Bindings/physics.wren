// AUTO-GENERATED FILE - DO NOT EDIT
// Generated from C++ bindings
// Module: physics

// Enum of the different phyics body types
class BodyType {
    static STATIC_BODY { 0 }
    static KINEMATIC_BODY { 1 }
    static DYNAMIC_BODY { 2 }
}

// PhysicsChainShape class wraps b2ChainShape
foreign class PhysicsChainShape {
    // Create physics chain shape
    foreign construct new()
    // Create closed loop from list of Vec2
    foreign createLoop(arg0)
    // Create open chain from list of Vec2
    foreign createChain(arg0)
}

// PhysicsBoxShape class wraps b2PolygonShape as box
foreign class PhysicsBoxShape {
    // Create physics box shape
    foreign construct new()
    // Create physics box shape with half-width and half-height
    foreign construct new(arg0,arg1)
    // Create physics box shape with half-width, half-height, center, and angle
    foreign construct new(arg0,arg1,arg2,arg3,arg4)
    // Set as box with half-width and half-height
    foreign setAsBox(arg0,arg1)
    // Set as box with half-width, half-height, center, and angle
    foreign setAsBox(arg0,arg1,arg2,arg3,arg4)
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

// PhysicsCircleShape class wraps b2CircleShape
foreign class PhysicsCircleShape {
    // Create physics circle shape
    foreign construct new()
    // Create physics circle shape with radius
    foreign construct new(arg0)
    // Get physics circle shape's radius
    foreign radius
    // Set physics circle shape's radius
    foreign radius=(arg0)
}

// PhysicsPolygonShape class wraps b2PolygonShape
foreign class PhysicsPolygonShape {
    // Create physics polygon shape
    foreign construct new()
    // Set polygon vertices from list of Vec2
    foreign setVertices(arg0)
}

// PhysicsEdgeShape class wraps b2EdgeShape
foreign class PhysicsEdgeShape {
    // Create physics edge shape
    foreign construct new()
    // Create physics edge shape with two points
    foreign construct new(arg0,arg1,arg2,arg3)
    // Set two-sided edge with two points
    foreign setTwoSided(arg0,arg1,arg2,arg3)
    // Set one-sided edge with ghost vertices
    foreign setOneSided(arg0,arg1,arg2,arg3,arg4,arg5,arg6,arg7)
}

