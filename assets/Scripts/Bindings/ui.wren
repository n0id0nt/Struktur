// AUTO-GENERATED FILE - DO NOT EDIT
// Generated from C++ bindings
// Module: ui

// Enum to set the navigation direction for UI Elements
class NavigationDirection {
    static UP { 0 }
    static DOWN { 1 }
    static LEFT { 2 }
    static RIGHT { 3 }
}

// Enum to set the text alignment for UI Labels
class TextAlignment {
    static LEFT { 0 }
    static CENTER { 1 }
    static RIGHT { 2 }
    static JUSTIFY { 3 }
}

// Enum to set the text wrapping for UI Labels
class TextWrapping {
    static NONE { 0 }
    static WORD_WRAP { 1 }
    static CHARACTER_WRAP { 2 }
}

class UIManager {
    // Add a UI Element to the UI system.
    foreign static addUIElement(arg0)
    // Remove and clean up an element in the UI system.
    foreign static removeUIElement(arg0)
    // Will set the focus of the current element.
    foreign static setFocus(arg0)
    // Will set the focus of the current element.
    foreign static clearFocusElements()
}

// UI Element component
foreign class UIElement {
    // Sets UI Element to be visible
    foreign isVisible=(arg0)
    // Sets UI Element to be visible
    foreign setVisible(arg0)
    // Gets UI Element to be visible
    foreign isVisible
    // Sets UI Element to be enabled
    foreign IsEnabled=(arg0)
    // Gets UI Element to be enabled
    foreign IsEnabled
    // Sets UI Element to be focusable
    foreign IsFocusable=(arg0)
    // Sets UI Element to be focusable
    foreign setFocusable(arg0)
    // Gets UI Element to be focusable
    foreign IsFocusable
    // Sets the UI Elements position
    foreign setPosition(arg0,arg1)
    // Gets the UI Elements position
    foreign getPosition()
    // Sets the UI Elements anchor point
    foreign setAnchorPoint(arg0)
    // Sets the UI Elements anchor point
    foreign setSize(arg0,arg1)
    // Gets the UI Elements size
    foreign getSize()
    // Gets the UI Elements bounds
    foreign getBounds()
    // Sets the UI Elements tab index
    foreign setTabIndex(arg0)
    // Gets the UI Elements tab index
    foreign getTabIndex()
    // adds a UI Element as a navigation neighbor
    foreign setNavigationNeighbor(arg0,arg1)
    // Gets a UI Element navigation neighbor from a direction
    foreign getNavigationNeighbor(arg0)
    // Sets the UI Elements background color
    foreign setBackgroundColor(arg0)
    // Sets the UI Elements border color
    foreign setBorderColor(arg0)
    // Sets the UI Elements border width
    foreign setBorderWidth(arg0)
    // Gets the UI Elements parent
    foreign getParent()
    // Sets the UI Elements children
    foreign getChildren()
    // Sets the UI Elements Z index
    foreign setZIndex(arg0)
    // Sets the UI Elements Z index
    foreign getZIndex()
    // Adds a UI Element to elements children
    foreign addChild(arg0)
    // Removes a UI Element from the children
    foreign removeChild(arg0)
    // Sets the UI Elements on click callback
    foreign setOnClick(arg0)
    // Sets the UI Elements on focus callback
    foreign setOnFocus(arg0)
    // Sets the UI Elements on lose focus callback
    foreign setOnLoseFocus(arg0)
    // Sets the UI Elements on hover callback
    foreign setOnHover(arg0)
    // Sets the UI Elements on key pressed callback
    foreign setOnKeyPressed(arg0)
    // Sets the UI Elements on activate callback
    foreign setOnActivate(arg0)
}

// UI Panel component
foreign class UIPanel {
    // Create UI panel with absolutePosition, relativePosition, absoluteSize, relativeSize components
    foreign construct new(absolutePosition, relativePosition, absoluteSize, relativeSize)
    // Sets the UI Panels background texture
    foreign setBackgroundTexture(arg0)
    // Clears the UI Panels background texture
    foreign ClearBackgroundTexture()
    // Sets UI Element to be visible
    foreign isVisible=(arg0)
    // Sets UI Element to be visible
    foreign setVisible(arg0)
    // Gets UI Element to be visible
    foreign isVisible
    // Sets UI Element to be enabled
    foreign IsEnabled=(arg0)
    // Gets UI Element to be enabled
    foreign IsEnabled
    // Sets UI Element to be focusable
    foreign IsFocusable=(arg0)
    // Sets UI Element to be focusable
    foreign setFocusable(arg0)
    // Gets UI Element to be focusable
    foreign IsFocusable
    // Sets the UI Elements position
    foreign setPosition(arg0,arg1)
    // Gets the UI Elements position
    foreign getPosition()
    // Sets the UI Elements anchor point
    foreign setAnchorPoint(arg0)
    // Sets the UI Elements anchor point
    foreign setSize(arg0,arg1)
    // Gets the UI Elements size
    foreign getSize()
    // Gets the UI Elements bounds
    foreign getBounds()
    // Sets the UI Elements tab index
    foreign setTabIndex(arg0)
    // Gets the UI Elements tab index
    foreign getTabIndex()
    // adds a UI Element as a navigation neighbor
    foreign setNavigationNeighbor(arg0,arg1)
    // Gets a UI Element navigation neighbor from a direction
    foreign getNavigationNeighbor(arg0)
    // Sets the UI Elements background color
    foreign setBackgroundColor(arg0)
    // Sets the UI Elements border color
    foreign setBorderColor(arg0)
    // Sets the UI Elements border width
    foreign setBorderWidth(arg0)
    // Gets the UI Elements parent
    foreign getParent()
    // Sets the UI Elements children
    foreign getChildren()
    // Sets the UI Elements Z index
    foreign setZIndex(arg0)
    // Sets the UI Elements Z index
    foreign getZIndex()
    // Adds a UI Element to elements children
    foreign addChild(arg0)
    // Removes a UI Element from the children
    foreign removeChild(arg0)
    // Sets the UI Elements on click callback
    foreign setOnClick(arg0)
    // Sets the UI Elements on focus callback
    foreign setOnFocus(arg0)
    // Sets the UI Elements on lose focus callback
    foreign setOnLoseFocus(arg0)
    // Sets the UI Elements on hover callback
    foreign setOnHover(arg0)
    // Sets the UI Elements on key pressed callback
    foreign setOnKeyPressed(arg0)
    // Sets the UI Elements on activate callback
    foreign setOnActivate(arg0)
}

// UI Label component
foreign class UILabel {
    // Create UI Label with absolutePosition, relativePosition, labelText components
    foreign construct new(absolutePosition, relativePosition, labelText)
    // Create UI Label with absolutePosition, relativePosition, labelText, fontSize components
    foreign construct new(absolutePosition, relativePosition, labelText, fontSize)
    // Sets the UI Labels Font
    foreign setFont(arg0)
    // Sets the UI Labels text color
    foreign setTextColor(arg0)
    // Sets the UI Labels alignment
    foreign setAlignment(arg0)
    // Sets the UI Labels font size
    foreign setFontSize(arg0)
    // Sets the UI Labels word wrap
    foreign setWordWrap(arg0)
    // set the UI Labels text
    foreign setText(arg0)
    // get the UI Labels text
    foreign getText()
    // Sets UI Element to be visible
    foreign isVisible=(arg0)
    // Sets UI Element to be visible
    foreign setVisible(arg0)
    // Gets UI Element to be visible
    foreign isVisible
    // Sets UI Element to be enabled
    foreign IsEnabled=(arg0)
    // Gets UI Element to be enabled
    foreign IsEnabled
    // Sets UI Element to be focusable
    foreign IsFocusable=(arg0)
    // Sets UI Element to be focusable
    foreign setFocusable(arg0)
    // Gets UI Element to be focusable
    foreign IsFocusable
    // Sets the UI Elements position
    foreign setPosition(arg0,arg1)
    // Gets the UI Elements position
    foreign getPosition()
    // Sets the UI Elements anchor point
    foreign setAnchorPoint(arg0)
    // Sets the UI Elements anchor point
    foreign setSize(arg0,arg1)
    // Gets the UI Elements size
    foreign getSize()
    // Gets the UI Elements bounds
    foreign getBounds()
    // Sets the UI Elements tab index
    foreign setTabIndex(arg0)
    // Gets the UI Elements tab index
    foreign getTabIndex()
    // adds a UI Element as a navigation neighbor
    foreign setNavigationNeighbor(arg0,arg1)
    // Gets a UI Element navigation neighbor from a direction
    foreign getNavigationNeighbor(arg0)
    // Sets the UI Elements background color
    foreign setBackgroundColor(arg0)
    // Sets the UI Elements border color
    foreign setBorderColor(arg0)
    // Sets the UI Elements border width
    foreign setBorderWidth(arg0)
    // Gets the UI Elements parent
    foreign getParent()
    // Sets the UI Elements children
    foreign getChildren()
    // Sets the UI Elements Z index
    foreign setZIndex(arg0)
    // Sets the UI Elements Z index
    foreign getZIndex()
    // Adds a UI Element to elements children
    foreign addChild(arg0)
    // Removes a UI Element from the children
    foreign removeChild(arg0)
    // Sets the UI Elements on click callback
    foreign setOnClick(arg0)
    // Sets the UI Elements on focus callback
    foreign setOnFocus(arg0)
    // Sets the UI Elements on lose focus callback
    foreign setOnLoseFocus(arg0)
    // Sets the UI Elements on hover callback
    foreign setOnHover(arg0)
    // Sets the UI Elements on key pressed callback
    foreign setOnKeyPressed(arg0)
    // Sets the UI Elements on activate callback
    foreign setOnActivate(arg0)
}

