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
    // Get the games delta time.
    foreign static deltaTime
    // Get the games time.
    foreign static gameTime
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

class Inventory {
    // Check if the inventory contains an item.
    foreign static contains(arg0)
    // Get a list of all items in inventory.
    foreign static getItems()
    // add an item to the inventory.
    foreign static addItem(arg0)
    // remove an item to the inventory.
    foreign static removeItem(arg0)
}

