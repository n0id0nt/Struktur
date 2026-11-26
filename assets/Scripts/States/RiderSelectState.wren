// States/RiderSelectState.wren
// "Who's Riding?" - after device pairing, before the main menu. Lists the rider profiles (Rider/RiderProfiles.wren);
// choosing one makes it the current rider and goes on to the main menu, Edit / New Rider open States/RiderEditState,
// and Back (UICancel) returns to device pairing. UI/Rider/RiderSelectUI.wren is the view.
//
// params: {"focusId": id} focuses that rider (the editor passes back the one it saved); otherwise the last ridden.
// Clicks are queued and handled in update() - they arrive inside the UI input pass, and every action here leaves
// the state (see UI/Theme/AlertPopup for why that isn't done from inside a click).
import "input" for Input
import "States/BaseState" for BaseState
import "UI/Rider/RiderSelectUI" for RiderSelectUI
import "UI/Theme/UIKit" for UIKit
import "Rider/RiderProfiles" for RiderProfiles

// Where choosing a rider leads, and where Back returns to.
var NEXT_STATE = "MainMenu"
var PREVIOUS_STATE = "DevicePairing"

class RiderSelectState is BaseState {
    construct new() {
        super()
        name = "RiderSelectState"
        _stateManager = null
        _view = null
        _pending = []
    }

    enter(stateManager, params) {
        super.enter(stateManager, params)
        _stateManager = stateManager
        _pending = []
        var focusId = params != null && params.containsKey("focusId") ? params["focusId"] : RiderProfiles.selectedId

        UIKit.open()
        _view = RiderSelectUI.new(RiderProfiles.profiles, focusId, RiderProfiles.isFull)
        _view.onChoose = Fn.new { |id| _pending.add(["choose", id]) }
        _view.onEdit = Fn.new { |id| _pending.add(["edit", id]) }
        _view.onNew = Fn.new { _pending.add(["new", null]) }
    }

    update(stateManager) {
        if (_view == null) return
        if (!_pending.isEmpty) {
            var action = _pending[0]
            _pending = []
            perform_(action[0], action[1])
            return
        }
        if (Input.isInputJustReleased("UICancel")) {
            _stateManager.changeState(PREVIOUS_STATE)
        }
    }

    exit() {
        super.exit()
        if (_view != null) {
            _view.teardown()
            _view = null
        }
        _pending = []
        UIKit.close()
    }

    perform_(kind, id) {
        if (kind == "choose") {
            RiderProfiles.select(id)
            RiderProfiles.save()
            _stateManager.changeState(NEXT_STATE)
        } else if (kind == "edit") {
            _stateManager.changeState("RiderEdit", {"id": id})
        } else if (kind == "new") {
            _stateManager.changeState("RiderEdit", {})
        }
    }
}
