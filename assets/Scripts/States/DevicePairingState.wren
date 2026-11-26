// States/DevicePairingState.wren
// The first screen: pair a smart trainer / power meter / heart rate strap before the main menu (the pairing screen
// cycling apps open with). Remembered devices reconnect on their own and a scan runs while the screen is up; the
// player picks a tile, chooses a device from what the scan found, and "Let's Ride" unlocks once a power source -
// a trainer or a power meter - is connected. Heart rate is optional.
//
// Driven by the fitness* engine events (ECS/System/FitnessSystem.h), never by polling: connection changes refresh
// a tile, fitnessSample updates its live value, fitnessDeviceFound refreshes an open picker. UI/Pairing/
// DevicePairingUI.wren is the view.
//
// Clicks are queued and handled in update(): a click arrives from inside the UI manager's input pass, and the
// actions here remove UI (the picker) or leave the state, which isn't safe from there (see UI/Theme/AlertPopup).
import "input" for Input
import "localization" for Localization
import "fitness" for Fitness
import "States/BaseState" for BaseState
import "UI/Pairing/DevicePairingUI" for DevicePairingUI
import "UI/Theme/UIKit" for UIKit
import "debug" for Debug

var ROLES = ["trainer", "powerMeter", "heartRate"]
// Where "Let's Ride" leads.
var NEXT_STATE = "MainMenu"

class DevicePairingState is BaseState {
    construct new() {
        super()
        name = "DevicePairingState"
        _stateManager = null
        _view = null
        _pending = []
    }

    enter(stateManager, params) {
        super.enter(stateManager, params)
        _stateManager = stateManager
        _pending = []

        UIKit.open()
        _view = DevicePairingUI.new()
        _view.onTileChosen = Fn.new { |role| _pending.add(["openPicker", role]) }
        _view.onDeviceChosen = Fn.new { |device| _pending.add(["pick", device]) }
        _view.onUnpair = Fn.new { |role| _pending.add(["unpair", role]) }
        _view.onPickerBack = Fn.new { _pending.add(["closePicker", null]) }
        _view.onContinue = Fn.new { _pending.add(["continue", null]) }

        if (Fitness.isAvailable()) {
            Fitness.connectRemembered()
            Fitness.startScan()
        }
        refreshStatusLine_()
        for (role in ROLES) refreshTile_(role)
        refreshContinue_()
    }

    update(stateManager) {
        if (_view == null) return
        var actions = _pending
        _pending = []
        for (action in actions) {
            perform_(action[0], action[1])
            // Continuing leaves the state - nothing after it may touch the (now torn down) view.
            if (_view == null) return
        }
        if (_view.isPickerOpen && Input.isInputJustReleased("UICancel")) {
            _view.closePicker()
        }
    }

    onEvent(type, data) {
        if (_view == null) return
        if (type == "fitnessDeviceFound") {
            if (_view.isPickerOpen && data["roles"].contains(_view.pickerRole)) {
                _view.setPickerDevices(devicesFor_(_view.pickerRole))
            }
            return
        }
        if (!type.startsWith("fitness") || data == null || !data.containsKey("role")) return
        var role = data["role"]
        if (type == "fitnessSample") {
            _view.setTileValue(role, valueText_(role, data))
            return
        }
        // Lifecycle and control events: the device's state line, and whether a power source is now connected.
        refreshTile_(role)
        refreshContinue_()
    }

    exit() {
        super.exit()
        Fitness.stopScan()
        if (_view != null) {
            _view.teardown()
            _view = null
        }
        _pending = []
        UIKit.close()
    }

    perform_(kind, argument) {
        if (kind == "openPicker") {
            _view.openPicker(argument, devicesFor_(argument), Fitness.status(argument) != null)
        } else if (kind == "pick") {
            var role = _view.pickerRole
            if (role != null) {
                var deviceName = argument["name"] == "" ? argument["address"] : argument["name"]
                if (!Fitness.assign(role, argument["address"], deviceName)) {
                    Debug.warning("Could not pair %(deviceName) as %(role)")
                }
                _view.closePicker()
                refreshTile_(role)
                refreshContinue_()
            }
        } else if (kind == "unpair") {
            Fitness.unassign(argument)
            _view.closePicker()
            refreshTile_(argument)
            refreshContinue_()
        } else if (kind == "closePicker") {
            _view.closePicker()
        } else if (kind == "continue") {
            if (Fitness.hasPowerSource()) {
                _stateManager.changeState(NEXT_STATE)
            }
        }
    }

    // Scan results that can serve `role` (a trainer advertising Cycling Power also shows under Power Meter).
    devicesFor_(role) {
        return Fitness.devices().where { |device| device["roles"].contains(role) }.toList
    }

    refreshStatusLine_() {
        if (Fitness.isSimulated()) {
            _view.setStatus(Localization.get("menu.pairing.simulated"), false)
        } else if (!Fitness.isAvailable()) {
            _view.setStatus(Localization.get("menu.pairing.no_bluetooth"), true)
        } else {
            _view.setStatus(Localization.get("menu.pairing.searching"), false)
        }
    }

    refreshTile_(role) {
        var status = Fitness.status(role)
        if (status == null) {
            _view.setTile(role, Localization.get("menu.pairing.state.unpaired"), false, "")
            return
        }
        var state = status["state"]
        var connected = state == "connected"
        var key = "menu.pairing.state.unpaired"
        if (connected) {
            key = "menu.pairing.state.connected"
        } else if (state == "connecting") {
            key = "menu.pairing.state.connecting"
        } else if (state == "reconnecting") {
            key = "menu.pairing.state.reconnecting"
        } else if (state == "failed") {
            key = "menu.pairing.state.failed"
        }
        // A trainer can be connected yet held by another app (FTMS allows one controller).
        if (connected && status["control"] == "denied") {
            key = "menu.pairing.state.control_denied"
        }
        _view.setTile(role, Localization.get(key), connected, status["name"])
        if (connected) _view.setTileValue(role, valueText_(role, Fitness.sample(role)))
    }

    refreshContinue_() {
        _view.setCanContinue(Fitness.hasPowerSource())
    }

    // The tile's big number: watts for trainer / power meter, bpm for heart rate, "--" until the device reports.
    valueText_(role, values) {
        if (role == "heartRate") {
            return values.containsKey("bpm") ? values["bpm"].round.toString + " bpm" : "--"
        }
        return values.containsKey("power") ? values["power"].round.toString + " W" : "--"
    }
}
