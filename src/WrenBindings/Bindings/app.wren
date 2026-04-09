// AUTO-GENERATED FILE - DO NOT EDIT
// Generated from C++ bindings
// Module: app

class Application {
    // Change the size of the game window.
    foreign static setWindowSize(arg0,arg1)
    // Changes the name of the game window.
    foreign static setApplicationName(arg0)
    // Changes the name of the game window.
    foreign static registerComponentScript(arg0,arg1)
    // Get the games width.
    foreign static gameWidth
    // Get the games height.
    foreign static gameHeight
    // Get the pixels per meter for the physics system.
    foreign static pixelsPerMeter
    // Set the pixels per meter for the physics system.
    foreign static setPixelsPerMeter(arg0)
    // Get the velocity iterations for the physics system.
    foreign static velocityIterations
    // Set the velocity iterations for the physics system.
    foreign static setVelocityIterations(arg0)
    // Get the position iterations for the physics system.
    foreign static positionIterations
    // Set the position iterations for the physics system.
    foreign static setPositionIterations(arg0)
}

class Time {
    // Gets the unsclaled delta time.
    foreign static unscaledDelta
    // Gets the sclaled delta time.
    foreign static scaledDelta
    // Get the unsclaled time.
    foreign static unscaledTime
    // Get the sclaled time.
    foreign static scaledTime
    // Get the sclaled time.
    foreign static timeScale
    // Set the time scale.
    foreign static setTimeScale(arg0)
}

