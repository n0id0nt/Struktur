// UI/Shared/ButtonUI.wren
// A focusable panel + centered label, with the focus/unfocus highlight (LIGHTGRAY <-> WHITE) wired
// in internally. Replaces what used to be three independent hand-rolled copies of the same widget -
// States/CombatState.wren's move-menu buttons, States/MainMenuState.wren's nav buttons,
// States/SettingsState.wren's rows - each reimplementing panel+label+focus-swap separately.
// Pure view: callers only ever call setOnClick(fn)/setText(text) - the highlight swap never needs
// wiring by the caller.
import "ui" for UIPanel, UILabel, TextAlignment
import "math" for Vec2
import "Colors" for WHITE, BLACK, LIGHTGRAY

class ButtonUI {
    // absPos/relPos/size/relSize: same 4-arg shape as UIPanel's own constructor - relSize lets a row
    // stretch with its parent (Settings' rows: relSize.x=1 so the row tracks the scroll list's
    // width) rather than always being a fixed pixel size. labelSize defaults to 19.0 (Combat's
    // move-menu size); pass it explicitly for a larger label (e.g. Main Menu's 26.0).
    construct new(parent, text, absPos, relPos, size, font) {
        init_(parent, text, absPos, relPos, size, Vec2.new(0, 0), font, 19.0)
    }

    construct new(parent, text, absPos, relPos, size, font, labelSize) {
        init_(parent, text, absPos, relPos, size, Vec2.new(0, 0), font, labelSize)
    }

    construct new(parent, text, absPos, relPos, size, relSize, font, labelSize) {
        init_(parent, text, absPos, relPos, size, relSize, font, labelSize)
    }

    init_(parent, text, absPos, relPos, size, relSize, font, labelSize) {
        _panel = UIPanel.new(absPos, relPos, size, relSize)
        _panel.setBackgroundColor(LIGHTGRAY)
        _panel.setBorderColor(WHITE)
        _panel.setBorderWidth(2)
        _panel.setFocusable(true)
        parent.addChild(_panel)

        _label = UILabel.new(Vec2.new(0, 0), Vec2.new(0.5, 0.5), text, labelSize)
        _label.setFont(font)
        _label.setTextColor(BLACK)
        _label.setAlignment(TextAlignment.CENTER)
        _label.setBoundingBoxToText()
        _label.setAnchorPoint(Vec2.new(0.5, 0.5))
        _label.setZIndex(10)
        _panel.addChild(_label)

        _panel.setOnFocus { |s| _panel.setBackgroundColor(WHITE) }
        _panel.setOnLoseFocus { |s| _panel.setBackgroundColor(LIGHTGRAY) }
    }

    // The raw panel - for callers that need to reposition/reparent/read focus state directly
    // (UIManager.setFocus(button.panel), etc.).
    panel { _panel }

    setText(text) {
        _label.setText(text)
        _label.setBoundingBoxToText()
    }

    setOnClick(fn) { _panel.setOnClick(fn) }
}
