// AUTO-GENERATED FILE - DO NOT EDIT
// Generated from C++ bindings
// Module: game

class UIManager {
    // Creates the UI component for UILabel.
    foreign static createUILabel(arg0,arg1,arg2,arg3)
    // Creates the UI component for UILabel.
    foreign static removeUILabel(arg0)
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

