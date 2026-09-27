// UI/Settings/SettingsUI.wren
// The settings screen's view: a scrollable list (UIScroll/UIClip) with a synced scrollbar thumb,
// header/info labels, and focusable rows (fullscreen toggle, volume stepper, repeated Back) built
// via UI/Shared/ButtonUI. Pure construction - row click actions are wired by
// States/SettingsState.wren, which also owns fullscreenText()/volumeText() string composition
// and passes the initial strings in here.
import "app" for Application
import "math" for Vec2
import "resourceManager" for Font
import "ui" for UIManager, UILabel, UIPanel, UIScroll, TextAlignment
import "localization" for Localization
import "Colors" for WHITE, BLACK, BLANK, DARKGRAY, LIGHTGRAY
import "UI/Shared/ButtonUI" for ButtonUI

var ROW_HEIGHT = 44
var ROW_SPACING = 8
var HEADER_HEIGHT = 34
var INFO_HEIGHT = 28
var BACK_ROW_COUNT = 7   // repeated deliberately, so the list overflows its viewport and needs scrolling

class SettingsUI {
    // fullscreenText/volumeText: initial row labels - the Controller has already computed these
    // from Application.isFullScreen/Audio.masterVolume before building the view.
    construct new(fullscreenText, volumeText) {
        var titleFont = Font.load("Fonts/medieval_sharp/MedievalSharp-Bold.ttf", 56)
        var rowFont = Font.load("Fonts/medieval_sharp/MedievalSharp-Book.ttf", 20)
        var headerFont = Font.load("Fonts/medieval_sharp/MedievalSharp-Bold.ttf", 20)

        _screenPanel = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0),
            Vec2.new(Application.gameWidth, Application.gameHeight), Vec2.new(0, 0))
        _screenPanel.setBackgroundColor(DARKGRAY)
        _screenPanel.setBorderColor(BLANK)
        UIManager.addUIElement(_screenPanel)

        var titleLabel = UILabel.new(Vec2.new(0, 90), Vec2.new(0.5, 0), Localization.get("menu.settings.title"), 56.0)
        titleLabel.setFont(titleFont)
        titleLabel.setTextColor(WHITE)
        titleLabel.setAlignment(TextAlignment.CENTER)
        titleLabel.setBoundingBoxToText()
        titleLabel.setAnchorPoint(Vec2.new(0.5, 0))
        titleLabel.setZIndex(10)
        _screenPanel.addChild(titleLabel)

        // Deliberately shorter than its content (see the row layout below) so reaching "Back" genuinely requires
        // scrolling - either via the synced thumb or by navigating focus down past the deadzone band.
        var listPanel = UIPanel.new(Vec2.new(0, 200), Vec2.new(0.5, 0), Vec2.new(480, 260), Vec2.new(0, 0))
        listPanel.setAnchorPoint(Vec2.new(0.5, 0))
        listPanel.setBackgroundColor(LIGHTGRAY)
        listPanel.setBorderColor(WHITE)
        listPanel.setBorderWidth(2)
        listPanel.setZIndex(5)
        _screenPanel.addChild(listPanel)

        _scroll = UIScroll.new(Vec2.new(8, 8), Vec2.new(0, 0), Vec2.new(-40, -16), Vec2.new(1, 1))
        _scroll.setHorizontalScrollEnabled(false)
        _scroll.setZIndex(6)
        listPanel.addChild(_scroll)

        var y = 0
        y = addHeaderLabel_(Localization.get("menu.settings.controls_header"), headerFont, y)
        y = addInfoLabel_(Localization.get("menu.settings.move_info"), rowFont, y)
        y = addInfoLabel_(Localization.get("menu.settings.interact_info"), rowFont, y)
        y = addInfoLabel_(Localization.get("menu.settings.inventory_info"), rowFont, y)
        y = addHeaderLabel_(Localization.get("menu.settings.audio_display_header"), headerFont, y)

        _fullscreenRow = row_(fullscreenText, rowFont, y)
        y = y + ROW_HEIGHT + ROW_SPACING
        _volumeRow = row_(volumeText, rowFont, y)
        y = y + ROW_HEIGHT + ROW_SPACING

        _backRows = []
        for (i in 0...BACK_ROW_COUNT) {
            _backRows.add(row_(Localization.get("menu.settings.back"), rowFont, y))
            y = y + ROW_HEIGHT + ROW_SPACING
        }

        var track = UIPanel.new(Vec2.new(-24, 8), Vec2.new(1, 0), Vec2.new(16, -16), Vec2.new(0, 1))
        track.setBackgroundColor(DARKGRAY)
        track.setBorderColor(BLANK)
        track.setZIndex(6)
        listPanel.addChild(track)

        var thumb = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(16, 40), Vec2.new(0, 0))
        thumb.setBackgroundColor(WHITE)
        thumb.setBorderColor(BLANK)
        thumb.setZIndex(7)
        track.addChild(thumb)

        _scroll.setScrollIndicator(thumb)

        // Mouse click handling is dead in this engine (see UIManager::HandleInput) - OnClick only ever fires via
        // UIAccept on the currently focused element, so without an initial focus this whole screen would be
        // unreachable by keyboard/gamepad, matching Main Menu's own initial-focus convention.
        UIManager.setFocus(_fullscreenRow.panel)

        titleFont.unload()
        rowFont.unload()
        headerFont.unload()
    }

    // Non-focusable section heading, left-aligned within the row.
    addHeaderLabel_(text, font, y) {
        var label = UILabel.new(Vec2.new(4, y), Vec2.new(0, 0), text, 20.0)
        label.setFont(font)
        label.setTextColor(BLACK)
        label.setAlignment(TextAlignment.LEFT)
        label.setSize(Vec2.new(-8, HEADER_HEIGHT - 6), Vec2.new(1, 0))
        label.setZIndex(1)
        _scroll.addChild(label)
        return y + HEADER_HEIGHT
    }

    // Non-focusable informational text row (reflects the real bindings in InputConfig.json, not placeholder text).
    addInfoLabel_(text, font, y) {
        var label = UILabel.new(Vec2.new(12, y), Vec2.new(0, 0), text, 16.0)
        label.setFont(font)
        label.setTextColor(DARKGRAY)
        label.setAlignment(TextAlignment.LEFT)
        label.setSize(Vec2.new(-20, INFO_HEIGHT), Vec2.new(1, 0))
        label.setZIndex(1)
        _scroll.addChild(label)
        return y + INFO_HEIGHT
    }

    // Focusable full-row-width row, stretching with the scroll list's width (relSize.x=1).
    row_(text, font, y) {
        return ButtonUI.new(_scroll, text, Vec2.new(0, y), Vec2.new(0, 0), Vec2.new(-8, ROW_HEIGHT),
                            Vec2.new(1, 0), font, 18.0)
    }

    fullscreenRow { _fullscreenRow }
    volumeRow { _volumeRow }
    backRows { _backRows }

    teardown() {
        if (_screenPanel != null) {
            UIManager.removeUIElement(_screenPanel)
            _screenPanel = null
        }
        _scroll = null
        _fullscreenRow = null
        _volumeRow = null
        _backRows = []
    }
}
