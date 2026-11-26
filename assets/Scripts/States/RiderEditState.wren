// States/RiderEditState.wren
// Create or edit a rider profile: name, units (metric / imperial), weight, height, FTP and threshold heart rate. Opened from
// States/RiderSelectState (params {"id": id} edits that rider, {} creates one) and returns there, focusing the
// rider it saved. UI/Rider/RiderEditUI.wren is the view.
//
//   name     Accept opens the name keyboard. A physical keyboard types straight in (Input text entry - while it runs,
//            keyboard keys stop driving UI actions; Enter = done, Esc = cancel, Backspace = delete), and a pad uses
//            the on-screen keys (B = cancel). New riders start with the keyboard open.
//   units    Left/Right or Accept switches metric / imperial. Values are stored in kg / cm either way.
//   weight   Left/Right steps 0.5 kg or 1 lb; hold to repeat.
//   height   Left/Right steps 1 cm or 1 inch; hold to repeat.
//   FTP      Left/Right steps 5 W; hold to repeat.
//   max / threshold heart rate   Left/Right steps 1 bpm; hold to repeat.
//
// Clicks are queued and handled in update() - they arrive inside the UI input pass, and several actions remove UI
// or leave the state.
import "app" for Time
import "input" for Input
import "localization" for Localization
import "States/BaseState" for BaseState
import "UI/Rider/RiderEditUI" for RiderEditUI
import "UI/Theme/UIKit" for UIKit
import "UI/Theme/AlertPopup" for AlertPopup
import "Rider/RiderProfiles" for RiderProfiles, Units

var RETURN_STATE = "RiderSelect"
var REPEAT_DELAY_SECONDS = 0.4
var REPEAT_INTERVAL_SECONDS = 0.07
var DIRECTION_THRESHOLD = 0.5
// Text entry stays on until these keys have been up for this many frames, so the Enter/Esc that closed the keyboard
// isn't seen by the UI as a UIAccept/UICancel release the moment keyboard actions come back.
var RELEASE_KEYS = ["enter", "kpenter", "escape"]
var RELEASE_FRAMES = 2

class RiderEditState is BaseState {
    construct new() {
        super()
        name = "RiderEditState"
        _stateManager = null
        _view = null
        _popup = null
        _pending = []
    }

    enter(stateManager, params) {
        super.enter(stateManager, params)
        _stateManager = stateManager
        _pending = []
        _popup = null
        _heldDirection = 0
        _nextRepeatSeconds = 0
        _shifted = true
        _nameBeforeKeyboard = ""
        _releaseFramesLeft = -1

        var existing = params != null && params.containsKey("id") ? RiderProfiles.find(params["id"]) : null
        _isNew = existing == null
        _profile = existing == null ? RiderProfiles.newProfile() : copy_(existing)

        UIKit.open()
        _view = RiderEditUI.new(_isNew, !_isNew)
        _view.onFieldChosen = Fn.new { |field| _pending.add(["field", field]) }
        _view.onButton = Fn.new { |action| _pending.add(["button", action]) }
        _view.onKey = Fn.new { |keyName| _pending.add(["key", keyName]) }
        refreshFields_()
        if (_isNew) openKeyboard_()
    }

    update(stateManager) {
        if (_view == null) return
        finishTextInputWhenReleased_()

        if (_popup != null) {
            if (!_popup.update()) _popup = null
        }
        var actions = _pending
        _pending = []
        for (action in actions) {
            perform_(action[0], action[1])
            if (_view == null) return   // left the state
        }
        if (_popup != null) return

        if (_view.isKeyboardOpen) {
            updateKeyboard_()
            return
        }
        if (Input.isInputJustReleased("UICancel")) {
            _stateManager.changeState(RETURN_STATE, {"focusId": _profile["id"]})
            return
        }
        updateStepper_()
    }

    exit() {
        super.exit()
        if (Input.isTextInputActive) Input.stopTextInput()
        if (_popup != null) {
            _popup.close()
            _popup = null
        }
        if (_view != null) {
            _view.teardown()
            _view = null
        }
        _pending = []
        UIKit.close()
    }

    perform_(kind, argument) {
        if (kind == "field") {
            if (argument == "name") {
                openKeyboard_()
            } else if (argument == "units") {
                toggleUnits_()
            }
        } else if (kind == "button") {
            if (argument == "save") {
                save_()
            } else if (argument == "cancel") {
                _stateManager.changeState(RETURN_STATE, {"focusId": _profile["id"]})
            } else if (argument == "delete") {
                confirmDelete_()
            }
        } else if (kind == "key") {
            pressKey_(argument)
        } else if (kind == "deleteConfirmed") {
            RiderProfiles.remove(_profile["id"])
            RiderProfiles.save()
            _stateManager.changeState(RETURN_STATE)
        }
    }

    // --- fields ------------------------------------------------------------------------------------------------

    refreshFields_() {
        var units = _profile["units"]
        var name = _profile["name"]
        _view.setField("name", Localization.get("menu.rider.name") +
            (name == "" ? Localization.get("menu.rider.name_empty") : name))
        _view.setField("units", "<  " + Localization.get("menu.rider.units") +
            Localization.get("menu.rider.units_" + units) + "  >")
        _view.setField("weight", "<  " + Localization.get("menu.rider.weight") +
            Units.formatWeight(_profile["weightKg"], units) + "  >")
        _view.setField("height", "<  " + Localization.get("menu.rider.height") +
            Units.formatHeight(_profile["heightCm"], units) + "  >")
        _view.setField("ftp", "<  " + Localization.get("menu.rider.ftp") + _profile["ftpWatts"].toString + " W  >")
        _view.setField("maxHeartRate", "<  " + Localization.get("menu.rider.max_hr") +
            _profile["maxHeartRateBpm"].toString + " bpm  >")
        _view.setField("thresholdHeartRate", "<  " + Localization.get("menu.rider.threshold_hr") +
            _profile["thresholdHeartRateBpm"].toString + " bpm  >")
        _view.setCanSave(name.trim() != "")
    }

    toggleUnits_() {
        _profile["units"] = _profile["units"] == "metric" ? "imperial" : "metric"
        refreshFields_()
    }

    // Left/Right on any row but the name, with hold-to-repeat for the numbers.
    updateStepper_() {
        var field = _view.focusedField
        var stepsValue = field != null && field != "name"
        var horizontal = Input.getInputAxis2("UIDir").x
        var direction = horizontal > DIRECTION_THRESHOLD ? 1 : (horizontal < -DIRECTION_THRESHOLD ? -1 : 0)
        if (!stepsValue || direction == 0) {
            _heldDirection = 0
            return
        }
        var now = Time.unscaledTime
        if (direction != _heldDirection) {
            _heldDirection = direction
            _nextRepeatSeconds = now + REPEAT_DELAY_SECONDS
            step_(field, direction)
        } else if (field != "units" && now >= _nextRepeatSeconds) {
            _nextRepeatSeconds = now + REPEAT_INTERVAL_SECONDS
            step_(field, direction)
        }
    }

    step_(field, direction) {
        var units = _profile["units"]
        if (field == "units") {
            toggleUnits_()
            return
        }
        if (field == "weight") {
            _profile["weightKg"] = Units.stepWeightKg(_profile["weightKg"], units, direction)
        } else if (field == "height") {
            _profile["heightCm"] = Units.stepHeightCm(_profile["heightCm"], units, direction)
        } else if (field == "ftp") {
            _profile["ftpWatts"] = RiderProfiles.stepFtp(_profile["ftpWatts"], direction)
        } else if (field == "maxHeartRate") {
            _profile["maxHeartRateBpm"] = RiderProfiles.stepMaxHeartRate(_profile["maxHeartRateBpm"], direction)
        } else if (field == "thresholdHeartRate") {
            _profile["thresholdHeartRateBpm"] = RiderProfiles.stepThresholdHeartRate(_profile["thresholdHeartRateBpm"],
                direction)
        }
        refreshFields_()
    }

    // --- name keyboard -----------------------------------------------------------------------------------------

    openKeyboard_() {
        _nameBeforeKeyboard = _profile["name"]
        _shifted = _profile["name"] == "" || _profile["name"].endsWith(" ")
        _view.openKeyboard()
        _view.setKeyboardText(_profile["name"], _shifted)
        _releaseFramesLeft = -1
        if (!Input.isTextInputActive) Input.startTextInput()
    }

    // keep: false restores the name from before the keyboard opened.
    closeKeyboard_(keep) {
        if (!keep) _profile["name"] = _nameBeforeKeyboard
        _profile["name"] = _profile["name"].trim()
        _view.closeKeyboard()
        _releaseFramesLeft = RELEASE_FRAMES
        refreshFields_()
    }

    finishTextInputWhenReleased_() {
        if (_releaseFramesLeft < 0) return
        for (key in RELEASE_KEYS) {
            if (Input.isKeyDown(key)) {
                _releaseFramesLeft = RELEASE_FRAMES
                return
            }
        }
        _releaseFramesLeft = _releaseFramesLeft - 1
        if (_releaseFramesLeft <= 0) {
            _releaseFramesLeft = -1
            if (Input.isTextInputActive) Input.stopTextInput()
        }
    }

    updateKeyboard_() {
        // Physical typing: printable ASCII only (the pixel font has no other glyphs).
        for (codePoint in Input.takeTypedText().codePoints) {
            if (codePoint >= 32 && codePoint <= 126) append_(String.fromCodePoint(codePoint))
        }
        if (Input.isKeyJustPressed("backspace")) deleteLast_()
        if (Input.isKeyJustPressed("enter") || Input.isKeyJustPressed("kpenter")) {
            closeKeyboard_(true)
        } else if (Input.isKeyJustPressed("escape") || Input.isInputJustReleased("UICancel")) {
            closeKeyboard_(false)
        }
    }

    pressKey_(keyName) {
        if (!_view.isKeyboardOpen) return
        if (keyName.startsWith("char:")) {
            var character = keyName[5..-1]
            var code = character.bytes[0]
            var isLetter = code >= 65 && code <= 90
            append_(isLetter && !_shifted ? String.fromByte(code + 32) : character)
            if (isLetter) _shifted = false
        } else if (keyName == "shift") {
            _shifted = !_shifted
        } else if (keyName == "space") {
            append_(" ")
            _shifted = true
        } else if (keyName == "delete") {
            deleteLast_()
        } else if (keyName == "done") {
            closeKeyboard_(true)
            return
        }
        _view.setKeyboardText(_profile["name"], _shifted)
    }

    append_(text) {
        var name = _profile["name"]
        if (name.count + text.count > RiderProfiles.maxNameLength) return
        if (text == " " && (name == "" || name.endsWith(" "))) return
        _profile["name"] = name + text
        _view.setKeyboardText(_profile["name"], _shifted)
    }

    deleteLast_() {
        var name = _profile["name"]
        if (name == "") return
        _profile["name"] = name[0...(name.count - 1)]
        _view.setKeyboardText(_profile["name"], _shifted)
    }

    // --- save / delete -----------------------------------------------------------------------------------------

    save_() {
        if (_profile["name"].trim() == "") return
        var id = _profile["id"]
        if (_isNew) {
            id = RiderProfiles.create(_profile)
        } else {
            RiderProfiles.update(_profile)
        }
        RiderProfiles.select(id)
        RiderProfiles.save()
        _stateManager.changeState(RETURN_STATE, {"focusId": id})
    }

    confirmDelete_() {
        var config = {}
        config["title"] = Localization.get("menu.rider.delete_title")
        config["body"] = _profile["name"] + "\n" + Localization.get("menu.rider.delete_body")
        config["buttons"] = [[Localization.get("menu.rider.delete"), Fn.new { _pending.add(["deleteConfirmed", null]) }],
            [Localization.get("menu.rider.cancel"), null]]
        config["cancel"] = 1
        config["returnFocus"] = _view.deleteButtonPanel
        _popup = AlertPopup.new(config)
    }

    copy_(profile) {
        var copy = {}
        for (key in profile.keys) copy[key] = profile[key]
        return copy
    }
}
