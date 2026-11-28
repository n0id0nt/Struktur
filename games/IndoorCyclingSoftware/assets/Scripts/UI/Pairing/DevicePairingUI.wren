// UI/Pairing/DevicePairingUI.wren
// The device pairing screen's view (States/DevicePairingState.wren owns it): one tile per device role - smart
// trainer, power meter, heart rate - showing its connection state, device name and live value, a "Let's Ride"
// button that stays disabled until a power source is connected, and a device picker that opens over the screen
// when a tile is chosen. Pure view: it shows what the state tells it and hands clicks back through the callbacks
// the state sets (onTileChosen, onDeviceChosen, onUnpair, onPickerBack, onContinue). Built from the UI kit, so the
// owner brackets it with UIKit.open()/close().
import "app" for Application
import "math" for Vec2, Vec4
import "ui" for UIManager, UIPanel, UILabel, TextAlignment, NavigationDirection
import "localization" for Localization
import "UI/Theme/UIKit" for UIKit
import "UI/Theme/KitButton" for KitButton

var ROLES = ["trainer", "powerMeter", "heartRate"]

var BACKGROUND = Vec4.new(24, 28, 34, 255)
var OVERLAY = Vec4.new(0, 0, 0, 170)
var CLEAR = Vec4.new(0, 0, 0, 0)
var CONNECTED_COLOR = Vec4.new(150, 220, 120, 255)
var WARNING_COLOR = Vec4.new(240, 110, 95, 255)

var WINDOW_SIZE = Vec2.new(1100, 620)
var TILE_SIZE = Vec2.new(320, 290)
var TILE_GAP = 30
var TILE_TOP = 140
var CONTINUE_SIZE = Vec2.new(300, 56)
var CONTINUE_TOP = 500

var PICKER_SIZE = Vec2.new(660, 520)
var PICKER_ROW_SIZE = Vec2.new(580, 44)
var PICKER_ROW_GAP = 8
var PICKER_LIST_TOP = 92
var PICKER_MAX_ROWS = 6
var PICKER_BUTTON_SIZE = Vec2.new(180, 48)

// Z order. Each top-level element (the screen, the picker) is its own UI batch, drawn in the order of its root's z -
// but inside a batch z is absolute, so every child sits above its root and both roots stay below KitButton's fixed
// 5 (pill) / 6 (text). The picker root is 2 so its batch draws over the screen's.
var SCREEN_Z = 1
var PICKER_Z = 2
var FRAME_Z = 3
var TILE_Z = 4
var LIST_Z = 4
var TEXT_Z = 7
var RING_Z = 8

class DevicePairingUI {
    construct new() {
        _font = UIKit.font()
        _tiles = {}
        _picker = null
        _pickerRole = null
        _pickerRows = []

        _root = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(Application.gameWidth, Application.gameHeight),
            Vec2.new(0, 0))
        _root.setBackgroundColor(BACKGROUND)
        _root.setBorderColor(CLEAR)
        _root.setZIndex(SCREEN_Z)
        UIManager.addUIElement(_root)

        _window = UIKit.frame(_root, "window", Vec2.new(0, 0), Vec2.new(0.5, 0.5), WINDOW_SIZE, Vec2.new(0, 0), FRAME_Z)
        _window.setAnchorPoint(Vec2.new(0.5, 0.5))

        label_(_window, Localization.get("menu.pairing.title"), Vec2.new(0, 26), 36.0, UIKit.accent)
        _statusLabel = label_(_window, "", Vec2.new(0, 74), 20.0, UIKit.textMuted)
        label_(_window, Localization.get("menu.pairing.subtitle"), Vec2.new(0, 102), 18.0, UIKit.textMuted)

        var tilesWidth = TILE_SIZE.x * ROLES.count + TILE_GAP * (ROLES.count - 1)
        var left = (WINDOW_SIZE.x - tilesWidth) / 2
        for (i in 0...ROLES.count) {
            buildTile_(ROLES[i], Vec2.new(left + i * (TILE_SIZE.x + TILE_GAP), TILE_TOP))
        }

        _requirementLabel = label_(_window, "", Vec2.new(0, 460), 20.0, UIKit.textMuted)
        _continueButton = KitButton.new(_window, Localization.get("menu.pairing.continue"),
            Vec2.new(-CONTINUE_SIZE.x / 2, CONTINUE_TOP), Vec2.new(0.5, 0), CONTINUE_SIZE, _font, 24.0)
        _continueButton.setOnClick { |sender, mousePos|
            if (_onContinue != null) _onContinue.call()
        }

        UIKit.hint(_window, "UIAccept", Localization.get("menu.pairing.select"), Vec2.new(0, -26),
            Vec2.new(0.5, 1), Vec2.new(0.5, 1), TEXT_Z)

        linkMainNavigation_()
        UIManager.setFocus(_tiles["trainer"]["panel"])
    }

    // --- callbacks the state sets ------------------------------------------------------------------------------
    onTileChosen=(fn) { _onTileChosen = fn }       // fn.call(role)
    onDeviceChosen=(fn) { _onDeviceChosen = fn }   // fn.call(device map from Fitness.devices())
    onUnpair=(fn) { _onUnpair = fn }               // fn.call(role)
    onPickerBack=(fn) { _onPickerBack = fn }       // fn.call()
    onContinue=(fn) { _onContinue = fn }           // fn.call()

    isPickerOpen { _picker != null }
    pickerRole { _pickerRole }

    // --- main screen -------------------------------------------------------------------------------------------

    setStatus(text, isWarning) {
        _statusLabel.setText(text)
        _statusLabel.setBoundingBoxToText()
        _statusLabel.setTextColor(isWarning ? WARNING_COLOR : UIKit.textMuted)
    }

    // stateText: the localized state line; isConnected tints it; deviceName may be "".
    setTile(role, stateText, isConnected, deviceName) {
        var tile = _tiles[role]
        setLabel_(tile["state"], stateText)
        tile["state"].setTextColor(isConnected ? CONNECTED_COLOR : UIKit.textMuted)
        setLabel_(tile["name"], deviceName)
        if (!isConnected) setTileValue(role, "--")
    }

    setTileValue(role, valueText) {
        setLabel_(_tiles[role]["value"], valueText)
    }

    setCanContinue(canContinue) {
        _continueButton.setEnabled(canContinue)
        setLabel_(_requirementLabel,
            Localization.get(canContinue ? "menu.pairing.ready" : "menu.pairing.need_power"))
        _requirementLabel.setTextColor(canContinue ? CONNECTED_COLOR : UIKit.textMuted)
    }

    canContinue { _continueButton.isEnabled }

    // --- picker ------------------------------------------------------------------------------------------------

    // devices: Fitness.devices() entries that can serve `role`; hasPairedDevice adds the Unpair button.
    openPicker(role, devices, hasPairedDevice) {
        closePicker()
        _pickerRole = role
        _picker = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(Application.gameWidth, Application.gameHeight),
            Vec2.new(0, 0))
        _picker.setBackgroundColor(OVERLAY)
        _picker.setBorderColor(CLEAR)
        _picker.setZIndex(PICKER_Z)
        UIManager.addUIElement(_picker)

        _pickerBox = UIKit.frame(_picker, "window", Vec2.new(0, 0), Vec2.new(0.5, 0.5), PICKER_SIZE, Vec2.new(0, 0),
            FRAME_Z)
        _pickerBox.setAnchorPoint(Vec2.new(0.5, 0.5))
        label_(_pickerBox, Localization.get("menu.pairing.picker_title_prefix") + roleName_(role), Vec2.new(0, 26),
            30.0, UIKit.accent)
        _pickerEmptyLabel = label_(_pickerBox, Localization.get("menu.pairing.picker_empty"),
            Vec2.new(0, PICKER_LIST_TOP + 20), 20.0, UIKit.textMuted)

        var buttonDefs = []
        if (hasPairedDevice) {
            buttonDefs.add([Localization.get("menu.pairing.unpair"), Fn.new {
                if (_onUnpair != null) _onUnpair.call(_pickerRole)
            }])
        }
        buttonDefs.add([Localization.get("menu.pairing.back"), Fn.new {
            if (_onPickerBack != null) _onPickerBack.call()
        }])
        var buttonGap = 20
        var buttonsWidth = PICKER_BUTTON_SIZE.x * buttonDefs.count + buttonGap * (buttonDefs.count - 1)
        _pickerButtons = []
        for (i in 0...buttonDefs.count) {
            var position = Vec2.new(-buttonsWidth / 2 + i * (PICKER_BUTTON_SIZE.x + buttonGap), -PICKER_BUTTON_SIZE.y - 70)
            _pickerButtons.add(pickerButton_(buttonDefs[i][0], buttonDefs[i][1], position))
        }
        UIKit.hint(_pickerBox, "UICancel", Localization.get("menu.pairing.back"), Vec2.new(0, -24),
            Vec2.new(0.5, 1), Vec2.new(0.5, 1), TEXT_Z)

        _pickerList = null
        setPickerDevices(devices)
    }

    // Rebuilds the device rows (a fitnessDeviceFound event, or the picker opening). Keeps focus on the same device
    // when it is still listed.
    setPickerDevices(devices) {
        if (_picker == null) return
        var focusedAddress = _focusedAddress
        if (_pickerList != null) {
            _pickerBox.removeChild(_pickerList)
        }
        _pickerRows = []
        _focusedAddress = null
        _pickerList = UIPanel.new(Vec2.new(0, PICKER_LIST_TOP), Vec2.new(0, 0), Vec2.new(PICKER_SIZE.x, 0),
            Vec2.new(0, 0))
        _pickerList.setBackgroundColor(CLEAR)
        _pickerList.setBorderColor(CLEAR)
        _pickerList.setZIndex(LIST_Z)
        _pickerBox.addChild(_pickerList)

        var shown = devices.count < PICKER_MAX_ROWS ? devices.count : PICKER_MAX_ROWS
        for (i in 0...shown) {
            _pickerRows.add(deviceRow_(devices[i], i))
        }
        _pickerEmptyLabel.setVisible(_pickerRows.isEmpty)
        linkPickerNavigation_()

        var focus = _pickerRows.isEmpty ? _pickerButtons[0] : _pickerRows[0]["button"]
        for (row in _pickerRows) {
            if (row["address"] == focusedAddress) focus = row["button"]
        }
        UIManager.setFocus(focus.panel)
    }

    closePicker() {
        if (_picker == null) return
        UIManager.removeUIElement(_picker)
        _picker = null
        _pickerBox = null
        _pickerList = null
        _pickerEmptyLabel = null
        _pickerButtons = []
        _pickerRows = []
        _focusedAddress = null
        var role = _pickerRole
        _pickerRole = null
        if (_tiles.containsKey(role)) UIManager.setFocus(_tiles[role]["panel"])
    }

    teardown() {
        closePicker()
        if (_root != null) {
            UIManager.removeUIElement(_root)
            _root = null
        }
        // Everything below pointed into the removed tree - see wren-reused-state rule in the state's exit().
        _window = null
        _statusLabel = null
        _requirementLabel = null
        _continueButton = null
        _tiles = {}
        if (_font != null) {
            _font.unload()
            _font = null
        }
    }

    // --- building ----------------------------------------------------------------------------------------------

    buildTile_(role, position) {
        var tile = UIKit.frame(_window, "tile", position, Vec2.new(0, 0), TILE_SIZE, Vec2.new(0, 0), TILE_Z)
        tile.setFocusable(true)
        var ring = UIKit.focusRing(tile, RING_Z)
        tile.setOnFocus { |sender| ring.setVisible(true) }
        tile.setOnLoseFocus { |sender| ring.setVisible(false) }
        tile.setOnClick { |sender, mousePos|
            if (_onTileChosen != null) _onTileChosen.call(role)
        }

        label_(tile, roleName_(role), Vec2.new(0, 22), 27.0, UIKit.text)
        var tagKey = role == "heartRate" ? "menu.pairing.tag.optional" : "menu.pairing.tag.power_source"
        label_(tile, Localization.get(tagKey), Vec2.new(0, 62), 18.0, UIKit.accent)
        var value = label_(tile, "--", Vec2.new(0, 104), 54.0, UIKit.text)
        var state = label_(tile, Localization.get("menu.pairing.state.unpaired"), Vec2.new(0, 196), 20.0,
            UIKit.textMuted)
        var name = label_(tile, "", Vec2.new(0, 232), 18.0, UIKit.textMuted)
        _tiles[role] = {"panel": tile, "value": value, "state": state, "name": name}
    }

    pickerButton_(text, action, position) {
        var button = KitButton.new(_pickerBox, text, position, Vec2.new(0.5, 1), PICKER_BUTTON_SIZE, _font)
        button.setOnClick { |sender, mousePos| action.call() }
        return button
    }

    deviceRow_(device, index) {
        var name = device["name"] == "" ? device["address"] : device["name"]
        var inUseAs = device["inUseAs"]
        var text = name
        if (inUseAs == _pickerRole) {
            text = name + "  (" + Localization.get("menu.pairing.current") + ")"
        } else if (inUseAs != null) {
            text = name + "  (" + Localization.get("menu.pairing.in_use_prefix") + roleName_(inUseAs) + ")"
        }
        var top = index * (PICKER_ROW_SIZE.y + PICKER_ROW_GAP)
        var button = KitButton.new(_pickerList, text, Vec2.new(-PICKER_ROW_SIZE.x / 2, top), Vec2.new(0.5, 0),
            PICKER_ROW_SIZE, _font)
        // A device can serve one role at a time - one Bluetooth link per device.
        button.setEnabled(inUseAs == null || inUseAs == _pickerRole)
        var address = device["address"]
        button.setOnFocused(Fn.new { _focusedAddress = address })
        button.setOnClick { |sender, mousePos|
            if (button.isEnabled && _onDeviceChosen != null) _onDeviceChosen.call(device)
        }
        return {"button": button, "address": address}
    }

    // Every direction set explicitly (see ui-focus-navigation notes): spatial search would otherwise reach
    // across the screen.
    linkMainNavigation_() {
        for (i in 0...ROLES.count) {
            var panel = _tiles[ROLES[i]]["panel"]
            var leftPanel = i > 0 ? _tiles[ROLES[i - 1]]["panel"] : panel
            var rightPanel = i < ROLES.count - 1 ? _tiles[ROLES[i + 1]]["panel"] : panel
            panel.setNavigationNeighbor(NavigationDirection.LEFT, leftPanel)
            panel.setNavigationNeighbor(NavigationDirection.RIGHT, rightPanel)
            panel.setNavigationNeighbor(NavigationDirection.UP, panel)
            panel.setNavigationNeighbor(NavigationDirection.DOWN, _continueButton.panel)
        }
        var continuePanel = _continueButton.panel
        continuePanel.setNavigationNeighbor(NavigationDirection.UP, _tiles[ROLES[0]]["panel"])
        continuePanel.setNavigationNeighbor(NavigationDirection.DOWN, continuePanel)
        continuePanel.setNavigationNeighbor(NavigationDirection.LEFT, continuePanel)
        continuePanel.setNavigationNeighbor(NavigationDirection.RIGHT, continuePanel)
    }

    linkPickerNavigation_() {
        var firstButton = _pickerButtons[0].panel
        for (i in 0..._pickerRows.count) {
            var panel = _pickerRows[i]["button"].panel
            var upPanel = i > 0 ? _pickerRows[i - 1]["button"].panel : panel
            var downPanel = i < _pickerRows.count - 1 ? _pickerRows[i + 1]["button"].panel : firstButton
            panel.setNavigationNeighbor(NavigationDirection.UP, upPanel)
            panel.setNavigationNeighbor(NavigationDirection.DOWN, downPanel)
            panel.setNavigationNeighbor(NavigationDirection.LEFT, panel)
            panel.setNavigationNeighbor(NavigationDirection.RIGHT, panel)
        }
        var lastRow = _pickerRows.isEmpty ? null : _pickerRows[-1]["button"].panel
        for (i in 0..._pickerButtons.count) {
            var panel = _pickerButtons[i].panel
            var leftPanel = i > 0 ? _pickerButtons[i - 1].panel : panel
            var rightPanel = i < _pickerButtons.count - 1 ? _pickerButtons[i + 1].panel : panel
            panel.setNavigationNeighbor(NavigationDirection.LEFT, leftPanel)
            panel.setNavigationNeighbor(NavigationDirection.RIGHT, rightPanel)
            panel.setNavigationNeighbor(NavigationDirection.UP, lastRow != null ? lastRow : panel)
            panel.setNavigationNeighbor(NavigationDirection.DOWN, panel)
        }
    }

    roleName_(role) { Localization.get("menu.pairing.role." + role) }

    // A label centred horizontally on its parent, `position.y` from the top.
    label_(parent, text, position, size, color) {
        var label = UILabel.new(position, Vec2.new(0.5, 0), text, size)
        label.setFont(_font)
        label.setTextColor(color)
        label.setAlignment(TextAlignment.CENTER)
        label.setBoundingBoxToText()
        label.setAnchorPoint(Vec2.new(0.5, 0))
        label.setZIndex(TEXT_Z)
        parent.addChild(label)
        return label
    }

    setLabel_(label, text) {
        label.setText(text)
        label.setBoundingBoxToText()
    }
}
