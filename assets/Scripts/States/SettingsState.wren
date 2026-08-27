// states/SettingsState.wren
// Settings menu - demonstrates UIScroll/UIClip: a scrollable settings list (taller than its viewport) with a
// synced scrollbar thumb, real focusable rows (fullscreen toggle, master volume stepper, back), and a couple of
// informational rows so the list overflows its viewport and genuinely needs scrolling to reach every row.

import "app" for Application
import "audio" for Audio
import "math" for Vec2
import "resourceManager" for Font
import "ui" for UIManager, UILabel, UIPanel, UIScroll, TextAlignment
import "localization" for Localization

import "States/BaseState" for BaseState
import "Colors" for WHITE, BLACK, BLANK, DARKGRAY, LIGHTGRAY

var ROW_HEIGHT = 44
var ROW_SPACING = 8
var HEADER_HEIGHT = 34
var INFO_HEIGHT = 28

// Cycled through by the master volume row - a stepper rather than a drag slider, matching UIScroll's own v1
// decision to not require mouse-drag input.
var VOLUME_LEVELS = [0.0, 0.25, 0.5, 0.75, 1.0]

class SettingsState is BaseState {
    construct new() {
        super()
        name = "SettingsState"
        _stateManager = null
        _screenPanel = null
        _scroll = null
        _fullscreenRow = null
        _fullscreenLabel = null
        _volumeLabel = null
        _volumeIndex = VOLUME_LEVELS.count - 1
    }

    enter(stateManager, params) {
        super.enter(stateManager, params)
        _stateManager = stateManager

        var titleFont = Font.load("Fonts/medieval_sharp/MedievalSharp-Bold.ttf", 56)
        var rowFont = Font.load("Fonts/medieval_sharp/MedievalSharp-Book.ttf", 20)
        var headerFont = Font.load("Fonts/medieval_sharp/MedievalSharp-Bold.ttf", 20)

        _volumeIndex = closestVolumeIndex(Audio.masterVolume)

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
        y = addHeaderLabel(Localization.get("menu.settings.controls_header"), headerFont, y)
        y = addInfoLabel(Localization.get("menu.settings.move_info"), rowFont, y)
        y = addInfoLabel(Localization.get("menu.settings.interact_info"), rowFont, y)
        y = addInfoLabel(Localization.get("menu.settings.inventory_info"), rowFont, y)
        y = addHeaderLabel(Localization.get("menu.settings.audio_display_header"), headerFont, y)
        y = addFullscreenRow(rowFont, y)
        y = addVolumeRow(rowFont, y)
        y = addBackRow(rowFont, y)
        y = addBackRow(rowFont, y)
        y = addBackRow(rowFont, y)
        y = addBackRow(rowFont, y)
        y = addBackRow(rowFont, y)
        y = addBackRow(rowFont, y)
        y = addBackRow(rowFont, y)

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
        // unreachable by keyboard/gamepad, matching MainMenuState's own UIManager.setFocus(startButton) pattern.
        UIManager.setFocus(_fullscreenRow)

        titleFont.unload()
        rowFont.unload()
        headerFont.unload()
    }

    // Non-focusable section heading, left-aligned within the row.
    addHeaderLabel(text, font, y) {
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
    addInfoLabel(text, font, y) {
        var label = UILabel.new(Vec2.new(12, y), Vec2.new(0, 0), text, 16.0)
        label.setFont(font)
        label.setTextColor(DARKGRAY)
        label.setAlignment(TextAlignment.LEFT)
        label.setSize(Vec2.new(-20, INFO_HEIGHT), Vec2.new(1, 0))
        label.setZIndex(1)
        _scroll.addChild(label)
        return y + INFO_HEIGHT
    }

    // Focusable panel+label row, full row width, matching MainMenuState.makeButton's visual style.
    makeRow(text, font, y) {
        var row = UIPanel.new(Vec2.new(0, y), Vec2.new(0, 0), Vec2.new(-8, ROW_HEIGHT), Vec2.new(1, 0))
        row.setBackgroundColor(LIGHTGRAY)
        row.setBorderColor(WHITE)
        row.setBorderWidth(1)
        row.setFocusable(true)
        row.setZIndex(2)
        _scroll.addChild(row)

        var label = UILabel.new(Vec2.new(0, 0), Vec2.new(0.5, 0.5), text, 18.0)
        label.setFont(font)
        label.setTextColor(BLACK)
        label.setAlignment(TextAlignment.CENTER)
        label.setBoundingBoxToText()
        label.setAnchorPoint(Vec2.new(0.5, 0.5))
        label.setZIndex(3)
        row.addChild(label)

        row.setOnFocus { |sender| row.setBackgroundColor(WHITE) }
        row.setOnLoseFocus { |sender| row.setBackgroundColor(LIGHTGRAY) }

        return [row, label]
    }

    addFullscreenRow(font, y) {
        var pair = makeRow(fullscreenText(), font, y)
        _fullscreenRow = pair[0]
        _fullscreenLabel = pair[1]
        _fullscreenRow.setOnClick { |sender, mousePos| toggleFullscreen() }
        return y + ROW_HEIGHT + ROW_SPACING
    }

    addVolumeRow(font, y) {
        var pair = makeRow(volumeText(), font, y)
        var row = pair[0]
        _volumeLabel = pair[1]
        row.setOnClick { |sender, mousePos| cycleVolume() }
        return y + ROW_HEIGHT + ROW_SPACING
    }

    addBackRow(font, y) {
        var pair = makeRow(Localization.get("menu.settings.back"), font, y)
        var row = pair[0]
        row.setOnClick { |sender, mousePos| goBack() }
        return y + ROW_HEIGHT + ROW_SPACING
    }

    // Composed strings - separate keys for the prefix/value/suffix pieces, concatenated here, rather than one
    // templated key, so each piece can be reused/reordered independently per language.
    fullscreenText() {
        return Localization.get("menu.settings.fullscreen_prefix") +
            (Application.isFullScreen ? Localization.get("menu.settings.on") : Localization.get("menu.settings.off"))
    }
    volumeText() {
        return Localization.get("menu.settings.volume_prefix") + (VOLUME_LEVELS[_volumeIndex] * 100).round.toString +
            Localization.get("menu.settings.volume_suffix")
    }

    toggleFullscreen() {
        Application.setIsFullScreen(!Application.isFullScreen)
        _fullscreenLabel.setText(fullscreenText())
        _fullscreenLabel.setBoundingBoxToText()
    }

    cycleVolume() {
        applyVolumeIndex((_volumeIndex + 1) % VOLUME_LEVELS.count)
    }

    // Applies a new stepper index the same way cycleVolume() always has (master volume + on-screen label) -
    // shared with the #!export volumeIndex setter below so editing it from the state-debug window has the
    // exact same live effect as clicking the row in-game, not just a silent field write.
    applyVolumeIndex(index) {
        _volumeIndex = index
        Audio.setMasterVolume(VOLUME_LEVELS[_volumeIndex])
        _volumeLabel.setText(volumeText())
        _volumeLabel.setBoundingBoxToText()
    }

    // Demonstrates the exported-field convention (see BaseState/the state-debug window) on a real, observable
    // field - editing this from the editor immediately changes the master volume and the on-screen label.
    #!export
    volumeIndex { _volumeIndex }
    volumeIndex=(value) { applyVolumeIndex(value) }

    // Snaps the mixer's current volume (which may not land exactly on a stepper value) to the closest preset,
    // so the row's displayed value/cycle order stays sensible regardless of how the volume was last set.
    closestVolumeIndex(current) {
        var bestIndex = 0
        var bestDistance = (VOLUME_LEVELS[0] - current).abs
        for (i in 1...VOLUME_LEVELS.count) {
            var distance = (VOLUME_LEVELS[i] - current).abs
            if (distance < bestDistance) {
                bestDistance = distance
                bestIndex = i
            }
        }
        return bestIndex
    }

    goBack() {
        _stateManager.changeState("MainMenu")
    }

    update(stateManager) {
        // Mouse clicks and keyboard/gamepad focus navigation (including UIScroll's own focus-follow) are both
        // handled by UIManager itself - nothing needed here.
    }

    exit() {
        super.exit()

        if (_screenPanel) {
            UIManager.removeUIElement(_screenPanel)
            _screenPanel = null
        }
        _scroll = null
        _fullscreenRow = null
        _fullscreenLabel = null
        _volumeLabel = null
    }
}
