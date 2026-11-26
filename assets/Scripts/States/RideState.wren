// States/RideState.wren
// A ride - Free Ride or Workout (UI/Ride/RideHudUI.wren is the view, the ride itself runs in C++ behind the "ride"
// module). params: {"mode": "freeRide"} or {"mode": "workout", "workoutId": id}.
//
// Driven by events: rideTick (~4 Hz) refreshes the HUD, rideCue shows a workout text cue, rideWorkoutFinished and
// rideSaveFailed show a cue. Keys (raw keys, so they never clash with menu navigation):
//
//   E               ERG on / off                      + / -  (also Up / Down, pad D-pad)  resistance
//   P               pause / resume (pad Start)        M      trim in watts / percent
//   N               skip step (workout, pad X)        ]      extend step by 30 s (workout)
//   L               new lap (Free Ride, pad X)        T      type the Free Ride target (ERG on)
//   PgUp / PgDn     Free Ride target +/- 5 W (pad bumpers)
//   Esc             end the ride (asks first; pad B / Back)
//
// Clicks are queued and handled in update() - they arrive inside the UI input pass.
import "app" for Time
import "input" for Input
import "localization" for Localization
import "ride" for Ride
import "States/BaseState" for BaseState
import "UI/Ride/RideHudUI" for RideHudUI
import "UI/Ride/RideLayout" for RideLayout
import "UI/Ride/RideFormat" for RideFormat
import "UI/Theme/AlertPopup" for AlertPopup
import "Rider/RiderProfiles" for RiderProfiles
import "debug" for Debug

var EXIT_STATE = "MainMenu"
var EXTEND_SECONDS = 30
var MAX_TARGET_DIGITS = 4
var RELEASE_KEYS = ["enter", "kpenter", "escape"]
var RELEASE_FRAMES = 2
var DIRECTION_THRESHOLD = 0.5

class RideState is BaseState {
    construct new() {
        super()
        name = "RideState"
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
        _ending = false
        _leave = false
        _entryDigits = ""
        _releaseFramesLeft = -1
        _lastVertical = 0

        var rider = RiderProfiles.rideInfo
        if (rider == null) {
            Debug.warning("No rider selected - back to rider select")
            _stateManager.changeState("RiderSelect")
            return
        }
        var profile = RiderProfiles.current
        _imperial = profile["units"] == "imperial"
        _mode = params != null && params["mode"] == "workout" ? "workout" : "freeRide"

        var started = true
        var error = null
        if (_mode == "workout") {
            var result = Ride.startWorkout(params["workoutId"], rider["name"], rider["ftpWatts"], rider["maxHeartRateBpm"],
                rider["thresholdHeartRateBpm"], rider["weightKg"])
            started = result["ok"]
            error = result["error"]
        } else {
            Ride.startFreeRide(rider["name"], rider["ftpWatts"], rider["maxHeartRateBpm"], rider["thresholdHeartRateBpm"],
                rider["weightKg"])
        }

        buildView_()

        if (!started) {
            showMessage_(Localization.get("ride.start_failed"), error, true)
            return
        }
        _view.refresh(Ride.state(), Time.unscaledTime)
    }

    // The HUD in the saved layout (Ride Settings / H). Rebuilding is cheap, so switching layouts just rebuilds.
    buildView_() {
        var settings = Ride.settings()
        _view = RideHudUI.new(_mode, _imperial, Ride.isDemo(), settings["graphWindowSeconds"], settings["hudLayout"])
        _view.onErg = Fn.new { _pending.add(["erg", 0]) }
        _view.onResistance = Fn.new { |direction| _pending.add(["resistance", direction]) }
        _view.onTrimMode = Fn.new { _pending.add(["trimMode", 0]) }
        _view.onTargetNudge = Fn.new { |direction| _pending.add(["nudge", direction]) }
        _view.onTargetEdit = Fn.new { _pending.add(["editTarget", 0]) }
        _view.onSkip = Fn.new { _pending.add(["skip", 0]) }
        _view.onExtend = Fn.new { _pending.add(["extend", 0]) }
        _view.onEnd = Fn.new { _pending.add(["end", 0]) }
    }

    // H: the next HUD layout in RideLayout.json.
    cycleLayout_() {
        var names = RideLayout.names
        var index = names.indexOf(Ride.settings()["hudLayout"])
        var next = names[(index + 1) % names.count]
        Ride.setSetting("hudLayout", next)
        Ride.saveSettings()
        _view.teardown()
        buildView_()
        _view.refresh(Ride.state(), Time.unscaledTime)
    }

    update(stateManager) {
        if (_view == null) return
        finishTextInputWhenReleased_()
        if (_popup != null) {
            if (!_popup.update()) _popup = null
        }
        if (_leave) {
            _stateManager.changeState(EXIT_STATE)
            return
        }
        var actions = _pending
        _pending = []
        for (action in actions) perform_(action[0], action[1])
        if (_popup != null || _view == null) return

        if (_view.isTargetEntryOpen) {
            updateTargetEntry_()
            return
        }
        readKeys_()
    }

    onEvent(type, data) {
        if (_view == null) return
        if (type == "rideTick") {
            _view.refresh(Ride.state(), Time.unscaledTime)
        } else if (type == "rideCue") {
            _view.showCue(data["message"], data["duration"], Time.unscaledTime)
        } else if (type == "rideWorkoutFinished") {
            _view.showCue(Localization.get("ride.workout_complete_cue"), 10, Time.unscaledTime)
        } else if (type == "rideSaveFailed") {
            _view.showCue(Localization.get("ride.save_failed") + " " + data["message"], 10, Time.unscaledTime)
        }
    }

    exit() {
        super.exit()
        if (Input.isTextInputActive) Input.stopTextInput()
        if (_popup != null) {
            _popup.close()
            _popup = null
        }
        // Leaving any other way still saves the ride (the autosave already has most of it).
        if (Ride.isActive()) Ride.finish()
        Ride.discard()
        if (_view != null) {
            _view.teardown()
            _view = null
        }
        _pending = []
    }

    perform_(kind, argument) {
        if (kind == "erg") {
            Ride.toggleErg()
        } else if (kind == "resistance") {
            Ride.adjustResistance(argument)
        } else if (kind == "trimMode") {
            var state = Ride.state()
            Ride.setTrimMode(state["trimMode"] == "percent" ? "watts" : "percent")
        } else if (kind == "nudge") {
            Ride.nudgeTarget(argument)
        } else if (kind == "editTarget") {
            openTargetEntry_()
        } else if (kind == "skip") {
            Ride.skipStep()
        } else if (kind == "extend") {
            Ride.extendStep(EXTEND_SECONDS)
        } else if (kind == "end") {
            confirmEnd_()
        } else if (kind == "finish") {
            finishRide_()
        }
        // Show the change at once rather than on the next tick.
        if (_view != null) _view.refresh(Ride.state(), Time.unscaledTime)
    }

    readKeys_() {
        if (Input.isKeyJustPressed("e") || Input.isControllerButtonJustPressed("y")) perform_("erg", 0)
        if (Input.isKeyJustPressed("equal") || Input.isKeyJustPressed("kpadd")) perform_("resistance", 1)
        if (Input.isKeyJustPressed("minus") || Input.isKeyJustPressed("kpsubtract")) perform_("resistance", -1)
        // Up / Down (arrows, D-pad, stick) also change resistance - edge-triggered.
        var vertical = Input.getInputAxis2("UIDir").y
        var direction = vertical > DIRECTION_THRESHOLD ? 1 : (vertical < -DIRECTION_THRESHOLD ? -1 : 0)
        if (direction != 0 && direction != _lastVertical) perform_("resistance", direction)
        _lastVertical = direction
        if (Input.isKeyJustPressed("p") || Input.isControllerButtonJustPressed("start")) Ride.togglePause()
        if (Input.isKeyJustPressed("m")) perform_("trimMode", 0)
        // F3: world debug overlay (frame time, draw calls, chunk build times). Not saved.
        if (Input.isKeyJustPressed("f3")) Ride.setSetting("worldDebugOverlay", !Ride.settings()["worldDebugOverlay"])
        if (Input.isKeyJustPressed("h")) {
            cycleLayout_()
            return
        }
        if (_mode == "workout") {
            if (Input.isKeyJustPressed("n") || Input.isControllerButtonJustPressed("x")) perform_("skip", 0)
            if (Input.isKeyJustPressed("rightbracket")) perform_("extend", 0)
        } else {
            if (Input.isKeyJustPressed("l") || Input.isControllerButtonJustPressed("x")) Ride.lap()
            if (Input.isKeyJustPressed("pageup") || Input.isControllerButtonJustPressed("rbumper")) perform_("nudge", 1)
            if (Input.isKeyJustPressed("pagedown") || Input.isControllerButtonJustPressed("lbumper")) perform_("nudge", -1)
            if (Input.isKeyJustPressed("t")) perform_("editTarget", 0)
        }
        // On release, so the same key press can't also reach the popup that opens.
        if (Input.isInputJustReleased("UICancel") || Input.isControllerButtonJustPressed("select")) confirmEnd_()
    }

    // --- Free Ride target entry ----------------------------------------------------------------------------------

    openTargetEntry_() {
        var state = Ride.state()
        if (_mode != "freeRide" || state == null || !state["erg"]) return
        _entryDigits = ""
        _view.openTargetEntry()
        _view.setTargetEntryText(_entryDigits)
        _releaseFramesLeft = -1
        if (!Input.isTextInputActive) Input.startTextInput()
    }

    updateTargetEntry_() {
        for (codePoint in Input.takeTypedText().codePoints) {
            if (codePoint >= 48 && codePoint <= 57 && _entryDigits.count < MAX_TARGET_DIGITS) {
                _entryDigits = _entryDigits + String.fromCodePoint(codePoint)
            }
        }
        if (Input.isKeyJustPressed("backspace") && _entryDigits.count > 0) {
            _entryDigits = _entryDigits[0...(_entryDigits.count - 1)]
        }
        _view.setTargetEntryText(_entryDigits)
        var confirm = Input.isKeyJustPressed("enter") || Input.isKeyJustPressed("kpenter")
        var cancel = Input.isKeyJustPressed("escape") || Input.isInputJustReleased("UICancel")
        if (confirm || cancel) {
            if (confirm && _entryDigits.count > 0) {
                // Validated and clamped to the trainer's range by the ride itself.
                Ride.setFreeRideTarget(Num.fromString(_entryDigits))
            }
            _view.closeTargetEntry()
            _releaseFramesLeft = RELEASE_FRAMES
            _view.refresh(Ride.state(), Time.unscaledTime)
        }
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

    // --- ending ------------------------------------------------------------------------------------------------

    confirmEnd_() {
        if (_popup != null || _ending) return
        var config = {}
        config["title"] = Localization.get("ride.end_title")
        config["body"] = Localization.get("ride.end_body")
        config["buttons"] = [[Localization.get("ride.end_save"), Fn.new { _pending.add(["finish", 0]) }],
            [Localization.get("ride.end_keep_riding"), null]]
        config["cancel"] = 1
        _popup = AlertPopup.new(config)
    }

    finishRide_() {
        _ending = true
        var result = Ride.finish()
        if (result["ok"]) {
            var lines = [
                Localization.get("ride.summary_time") + " " + RideFormat.duration(result["timer"]),
                Localization.get("ride.summary_distance") + " " + RideFormat.distance(result["distance"], _imperial) + " " + RideFormat.distanceUnit(_imperial),
                Localization.get("ride.summary_power") + " " + RideFormat.whole(result["averagePower"]) + " W"
            ]
            showMessage_(Localization.get("ride.saved_title"), lines.join("\n") + "\n" + Localization.get("ride.saved_to") + " " + fileName_(result["fitPath"]), true)
        } else {
            showMessage_(Localization.get("ride.save_failed"), result["error"], true)
        }
    }

    // "C:/.../Rides/2026-11-26_18-00-00_Sweet_Spot.fit" -> "2026-11-26_18-00-00_Sweet_Spot.fit"
    fileName_(path) {
        var parts = path.replace("\\", "/").split("/")
        return parts[-1]
    }

    // A one-button popup; leaveAfter returns to the menu once it's dismissed.
    showMessage_(title, body, leaveAfter) {
        var config = {}
        config["title"] = title
        config["body"] = body == null ? "" : body
        if (leaveAfter) config["onClose"] = Fn.new { |index| _leave = true }
        _popup = AlertPopup.new(config)
    }
}
