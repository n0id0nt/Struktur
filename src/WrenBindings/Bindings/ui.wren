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

// UI minimal textured rect component
foreign class UITexture {
    // Create UITexture with absolutePosition, relativePosition, absoluteSize, relativeSize components
    foreign construct new(arg0,arg1,arg2,arg3)
    // Sets the UITexture's texture
    foreign setTexture(arg0)
    // Sets the UITexture's tint color
    foreign setTint(arg0)
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

// UI minimal solid-color rect component
foreign class UIColor {
    // Create UIColor with absolutePosition, relativePosition, absoluteSize, relativeSize components
    foreign construct new(arg0,arg1,arg2,arg3)
    // Sets the UIColor's color
    foreign setColor(arg0)
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

// UI element that masks its children to its own bounds
foreign class UIClip {
    // Create UIClip with absolutePosition, relativePosition, absoluteSize, relativeSize components
    foreign construct new(arg0,arg1,arg2,arg3)
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

// UI minimal border outline component
foreign class UIBorder {
    // Create UIBorder with absolutePosition, relativePosition, absoluteSize, relativeSize components
    foreign construct new(arg0,arg1,arg2,arg3)
    // Sets the UIBorder's color
    foreign setColor(arg0)
    // Sets the UIBorder's width
    foreign setWidth(arg0)
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

// Markup-driven UI Label component supporting inline bold/italic/color/icon formatting
foreign class UIRichLabel {
    // Create UIRichLabel with absolutePosition, relativePosition, markupText components
    foreign construct new(arg0,arg1,arg2)
    // Create UIRichLabel with absolutePosition, relativePosition, markupText, fontSize components
    foreign construct new(arg0,arg1,arg2,arg3)
    // Sets the UIRichLabel's regular-style font
    foreign setFont(arg0)
    // Sets the font used for [b] runs
    foreign setBoldFont(arg0)
    // Sets the font used for [i] runs
    foreign setItalicFont(arg0)
    // Sets the font used for [b][i] runs
    foreign setBoldItalicFont(arg0)
    // Sets the UIRichLabel's base text color (the color runs without an explicit [color=] use)
    foreign setTextColor(arg0)
    // Sets the UIRichLabel's font size
    foreign setFontSize(arg0)
    // Sets the UIRichLabel's word wrap
    foreign setWordWrap(arg0)
    // Sets the UIRichLabel's markup text
    foreign setMarkupText(arg0)
    // Gets the UIRichLabel's markup text
    foreign getMarkupText()
    // Sets the IconAtlas [icon=name] tags resolve against
    foreign setIconAtlas(arg0)
    // Caps rendering to the first N drawable glyphs, for a typewriter/reveal effect - negative means show everything
    foreign setVisibleGlyphCount(arg0)
    // Gets the total drawable glyph count, for a reveal loop to compare its progress against
    foreign getGlyphCount()
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
    foreign construct new(arg0,arg1,arg2,arg3)
    // Sets the UI Panels background texture
    foreign setBackgroundTexture(arg0)
    // Clears the UI Panels background texture
    foreign ClearBackgroundTexture()
    // Sets the UI Panels background color
    foreign setBackgroundColor(arg0)
    // Sets the UI Panels border color
    foreign setBorderColor(arg0)
    // Sets the UI Panels border width
    foreign setBorderWidth(arg0)
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
    foreign construct new(arg0,arg1,arg2)
    // Create UI Label with absolutePosition, relativePosition, labelText, fontSize components
    foreign construct new(arg0,arg1,arg2,arg3)
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
    // Will change the width and height to match the text
    foreign setBoundingBoxToText()
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

// Named-icon lookup table over a texture, for UIRichLabel's [icon=name] tags
foreign class IconAtlas {
    // Create an empty IconAtlas
    foreign construct new()
    // Sets the texture this atlas's icons are sub-rects of
    foreign setTexture(arg0)
    // Registers name -> a pixel-space (x, y, width, height) sub-rect of the atlas texture
    foreign addIcon(arg0,arg1,arg2,arg3,arg4)
}

// UI element that adds scrolling on top of UIClip's masking
foreign class UIScroll {
    // Create UIScroll with absolutePosition, relativePosition, absoluteSize, relativeSize components
    foreign construct new(arg0,arg1,arg2,arg3)
    // Toggles vertical scrolling (default true)
    foreign setVerticalScrollEnabled(arg0)
    // Toggles horizontal scrolling (default true)
    foreign setHorizontalScrollEnabled(arg0)
    // Sets the scroll offset directly (clamped to content bounds)
    foreign setScrollOffset(arg0)
    // Gets the current scroll offset
    foreign getScrollOffset()
    // Gets the combined extent of this element's direct children
    foreign getContentSize()
    // Sets the camera-style focus-follow deadzone band
    foreign setFocusDeadzone(arg0)
    // Sets the camera-style focus-follow lerp damping rate
    foreign setFocusDamping(arg0)
    // Sets a UIElement (typically a UIPanel) to two-way sync as this scroll's thumb/indicator
    foreign setScrollIndicator(arg0)
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

