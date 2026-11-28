// States/WorkoutBrowserState.wren
// Pick a workout to ride (UI/Ride/WorkoutBrowserUI.wren is the view). Lists the built-in workouts and the player's own
// (<save folder>/Workouts), plus any .zwo/.mrc dropped on the window or given on the command line. Every file is
// loaded through the hardened C++ loader; a broken file shows why instead of riding.
//
// params: {"preselect": path} adds that file (e.g. from the command line) and focuses it.
import "input" for Input
import "localization" for Localization
import "ride" for Ride
import "States/BaseState" for BaseState
import "UI/Ride/WorkoutBrowserUI" for WorkoutBrowserUI
import "Rider/RiderProfiles" for RiderProfiles

class WorkoutBrowserState is BaseState {
    construct new() {
        super()
        name = "WorkoutBrowserState"
        _stateManager = null
        _view = null
        _pending = []
        _extra = []  // dropped / command-line files, kept while the game runs
    }

    enter(stateManager, params) {
        super.enter(stateManager, params)
        _stateManager = stateManager
        _pending = []
        var rider = RiderProfiles.current
        _ftp = rider == null ? 200 : rider["ftpWatts"]
        _view = WorkoutBrowserUI.new(Ride.userWorkoutsFolder())
        _view.onSelect = Fn.new { |index| _pending.add(["select", index]) }
        _view.onRide = Fn.new { _pending.add(["ride", null]) }
        _view.onBack = Fn.new { _pending.add(["back", null]) }
        var focus = 0
        if (params != null && params["preselect"] is String) {
            addFile_(params["preselect"])
        }
        rebuild_(focus)
    }

    update(stateManager) {
        if (_view == null) return
        var actions = _pending
        _pending = []
        for (action in actions) {
            perform_(action[0], action[1])
            if (_view == null) return
        }
        if (Input.isInputJustReleased("UICancel")) _stateManager.changeState("MainMenu")
    }

    onEvent(type, data) {
        if (_view == null || type != "fileDropped" || data == null) return
        addFile_(data["path"])
        rebuild_(0)
    }

    exit() {
        super.exit()
        if (_view != null) {
            _view.teardown()
            _view = null
        }
        _pending = []
    }

    perform_(kind, argument) {
        if (kind == "select") {
            _selected = argument
            var listing = _listings[argument]
            _view.showPreview(Ride.loadWorkout(listing["id"], _ftp), listing, _ftp)
        } else if (kind == "ride") {
            if (_selected == null || _selected >= _listings.count) return
            var listing = _listings[_selected]
            if (listing["error"] != "") return
            _stateManager.changeState("Ride", {"mode": "workout", "workoutId": listing["id"]})
        } else if (kind == "back") {
            _stateManager.changeState("MainMenu")
        }
    }

    // Dropped / command-line files go to the top of the list (once each).
    addFile_(path) {
        if (!(path is String)) return
        for (entry in _extra) {
            if (entry["id"] == path) return
        }
        var preview = Ride.loadWorkout(path, _ftp)
        var entry = {}
        entry["id"] = path
        entry["source"] = "file"
        entry["name"] = preview["ok"] ? preview["name"] : path.replace("\\", "/").split("/")[-1]
        entry["durationSeconds"] = preview["ok"] ? preview["durationSeconds"] : 0
        entry["error"] = preview["ok"] ? "" : preview["error"]
        _extra.insert(0, entry)
    }

    rebuild_(focusIndex) {
        _listings = []
        for (entry in _extra) _listings.add(entry)
        for (entry in Ride.workouts()) _listings.add(entry)
        _selected = null
        _view.setListings(_listings, focusIndex)
    }
}
