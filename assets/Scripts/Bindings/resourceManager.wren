// AUTO-GENERATED FILE - DO NOT EDIT
// Generated from C++ bindings
// Module: resourceManager

// Font resource handle
foreign class Sound {
    // Load sound from path
    foreign static load(arg0)
    // Unload sound from path. Manually unload reference because even though wren's garbage collector will do it, it will happen at an unknown time.
    foreign unload()
    // Check if sound is valid
    foreign isValid()
    // Get sound path
    foreign path
    // Convert to string
    foreign toString()
}

// Font resource handle
foreign class Music {
    // Load music from path
    foreign static load(arg0)
    // Unload music from path. Manually unload reference because even though wren's garbage collector will do it, it will happen at an unknown time.
    foreign unload()
    // Check if music is valid
    foreign isValid()
    // Get music path
    foreign path
    // Convert to string
    foreign toString()
}

// Texture resource handle
foreign class Texture {
    // Load texture from path
    foreign static load(arg0)
    // Unload texture from path. Manually unload reference because even though wren's garbage collector will do it, it will happen at an unknown time.
    foreign unload()
    // Check if texture is valid
    foreign isValid()
    // Get texture path
    foreign path
    // Get texture width
    foreign width
    // Get texture height
    foreign height
    // Convert to string
    foreign toString()
}

// Font resource handle
foreign class Font {
    // Load font from path
    foreign static load(arg0)
    // Unload font from path. Manually unload reference because even though wren's garbage collector will do it, it will happen at an unknown time.
    foreign unload()
    // Check if font is valid
    foreign isValid()
    // Get font path
    foreign path
    // Get font size
    foreign size
    // Convert to string
    foreign toString()
}

