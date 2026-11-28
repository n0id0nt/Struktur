// states/SettingsState.wren
// Settings menu - demonstrates UIScroll/UIClip via UI/Settings/SettingsUI.wren (a scrollable
// settings list with a synced scrollbar thumb, real focusable rows, and a couple of informational
// rows so the list overflows its viewport and genuinely needs scrolling). Reads/writes the real
// engine settings directly (Application.isFullScreen, Audio.masterVolume - these are already the
// model, no wrapper needed) and composes the row display strings.
import "app" for Application
import "audio" for Audio
import "localization" for Localization
import "States/BaseState" for BaseState
import "UI/Settings/SettingsUI" for SettingsUI

// Cycled through by the master volume row - a stepper rather than a drag slider, matching UIScroll's own v1
// decision to not require mouse-drag input.
var VOLUME_LEVELS = [0.0, 0.25, 0.5, 0.75, 1.0]

class SettingsState is BaseState {
    construct new() {
        super()
        name = "SettingsState"
        _stateManager = null
        _view = null
        _volumeIndex = VOLUME_LEVELS.count - 1
    }

    enter(stateManager, params) {
        super.enter(stateManager, params)
        _stateManager = stateManager
        _volumeIndex = closestVolumeIndex(Audio.masterVolume)

        _view = SettingsUI.new(fullscreenText(), volumeText())
        _view.fullscreenRow.setOnClick { |sender, mousePos| toggleFullscreen() }
        _view.volumeRow.setOnClick { |sender, mousePos| cycleVolume() }
        for (row in _view.backRows) {
            row.setOnClick { |sender, mousePos| goBack() }
        }
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
        _view.fullscreenRow.setText(fullscreenText())
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
        _view.volumeRow.setText(volumeText())
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
        if (_view != null) {
            _view.teardown()
            _view = null
        }
    }
}
