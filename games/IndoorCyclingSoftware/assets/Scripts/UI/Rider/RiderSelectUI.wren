// UI/Rider/RiderSelectUI.wren
// "Who's Riding?" - the rider list (States/RiderSelectState.wren owns it). One row per profile showing its name,
// weight and height in that rider's units, FTP and threshold heart rate, an Edit button beside each row, and New Rider underneath. Pure view:
// clicks go back through the callbacks the state sets (onChoose, onEdit, onNew). Built from the UI kit, so the
// owner brackets it with UIKit.open()/close().
import "app" for Application
import "math" for Vec2
import "ui" for UIManager, UIPanel, NavigationDirection
import "localization" for Localization
import "UI/Theme/UIKit" for UIKit
import "UI/Theme/KitButton" for KitButton
import "UI/Rider/RiderTheme" for RiderTheme
import "Rider/RiderProfiles" for Units

var WINDOW_SIZE = Vec2.new(960, 640)
var LIST_TOP = 132
var ROW_SIZE = Vec2.new(660, 52)
var EDIT_SIZE = Vec2.new(140, 52)
var ROW_GAP = 10
var COLUMN_GAP = 16
var NEW_SIZE = Vec2.new(300, 52)

class RiderSelectUI {
    // profiles: RiderProfiles.profiles; focusId: the rider to focus first (or null); isFull disables New Rider.
    construct new(profiles, focusId, isFull) {
        _font = UIKit.font()
        _rows = []

        _root = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(Application.gameWidth, Application.gameHeight),
            Vec2.new(0, 0))
        _root.setBackgroundColor(RiderTheme.background)
        _root.setBorderColor(RiderTheme.clear)
        _root.setZIndex(RiderTheme.screenZ)
        UIManager.addUIElement(_root)

        var window = UIKit.frame(_root, "window", Vec2.new(0, 0), Vec2.new(0.5, 0.5), WINDOW_SIZE, Vec2.new(0, 0),
            RiderTheme.frameZ)
        window.setAnchorPoint(Vec2.new(0.5, 0.5))

        RiderTheme.label(window, _font, Localization.get("menu.rider.select_title"), 26, 36.0, UIKit.accent)
        var subtitleKey = profiles.isEmpty ? "menu.rider.select_empty" : "menu.rider.select_subtitle"
        RiderTheme.label(window, _font, Localization.get(subtitleKey), 78, 20.0, UIKit.textMuted)

        var rowLeft = -(ROW_SIZE.x + COLUMN_GAP + EDIT_SIZE.x) / 2
        for (i in 0...profiles.count) {
            var profile = profiles[i]
            var top = LIST_TOP + i * (ROW_SIZE.y + ROW_GAP)
            var row = KitButton.new(window, rowText_(profile), Vec2.new(rowLeft, top), Vec2.new(0.5, 0), ROW_SIZE,
                _font)
            var edit = KitButton.new(window, Localization.get("menu.rider.edit"),
                Vec2.new(rowLeft + ROW_SIZE.x + COLUMN_GAP, top), Vec2.new(0.5, 0), EDIT_SIZE, _font)
            var id = profile["id"]
            row.setOnClick { |sender, mousePos|
                if (_onChoose != null) _onChoose.call(id)
            }
            edit.setOnClick { |sender, mousePos|
                if (_onEdit != null) _onEdit.call(id)
            }
            _rows.add({"id": id, "row": row, "edit": edit})
        }

        var newTop = LIST_TOP + profiles.count * (ROW_SIZE.y + ROW_GAP) + 14
        _newButton = KitButton.new(window, Localization.get("menu.rider.new"), Vec2.new(-NEW_SIZE.x / 2, newTop),
            Vec2.new(0.5, 0), NEW_SIZE, _font)
        _newButton.setEnabled(!isFull)
        _newButton.setOnClick { |sender, mousePos|
            if (_newButton.isEnabled && _onNew != null) _onNew.call()
        }
        if (isFull) {
            RiderTheme.label(window, _font, Localization.get("menu.rider.full"), newTop + NEW_SIZE.y + 12, 18.0,
                UIKit.textMuted)
        }

        UIKit.richText(window, Localization.get("menu.rider.select_hints"), Vec2.new(0, -24), Vec2.new(0.5, 1),
            Vec2.new(0.5, 1), 18.0, UIKit.text, RiderTheme.textZ)

        linkNavigation_()
        var focus = _rows.isEmpty ? _newButton : _rows[0]["row"]
        for (entry in _rows) {
            if (entry["id"] == focusId) focus = entry["row"]
        }
        UIManager.setFocus(focus.panel)
    }

    onChoose=(fn) { _onChoose = fn }   // fn.call(id)
    onEdit=(fn) { _onEdit = fn }       // fn.call(id)
    onNew=(fn) { _onNew = fn }         // fn.call()

    teardown() {
        if (_root != null) {
            UIManager.removeUIElement(_root)
            _root = null
        }
        _rows = []
        _newButton = null
        if (_font != null) {
            _font.unload()
            _font = null
        }
    }

    rowText_(profile) {
        var units = profile["units"]
        return profile["name"] + "   " + Units.formatWeight(profile["weightKg"], units) + "   " +
            Units.formatHeight(profile["heightCm"], units) + "   " + Localization.get("menu.rider.list_ftp") +
            profile["ftpWatts"].toString + " W   " + Localization.get("menu.rider.list_threshold_hr") +
            profile["thresholdHeartRateBpm"].toString
    }

    // Rows go up/down, Right steps onto the row's Edit button; everything explicit (see ui-focus-navigation notes).
    linkNavigation_() {
        var newPanel = _newButton.panel
        for (i in 0..._rows.count) {
            var row = _rows[i]["row"].panel
            var edit = _rows[i]["edit"].panel
            var rowAbove = i > 0 ? _rows[i - 1]["row"].panel : row
            var editAbove = i > 0 ? _rows[i - 1]["edit"].panel : edit
            var rowBelow = i < _rows.count - 1 ? _rows[i + 1]["row"].panel : newPanel
            var editBelow = i < _rows.count - 1 ? _rows[i + 1]["edit"].panel : newPanel
            row.setNavigationNeighbor(NavigationDirection.UP, rowAbove)
            row.setNavigationNeighbor(NavigationDirection.DOWN, rowBelow)
            row.setNavigationNeighbor(NavigationDirection.LEFT, row)
            row.setNavigationNeighbor(NavigationDirection.RIGHT, edit)
            edit.setNavigationNeighbor(NavigationDirection.UP, editAbove)
            edit.setNavigationNeighbor(NavigationDirection.DOWN, editBelow)
            edit.setNavigationNeighbor(NavigationDirection.LEFT, row)
            edit.setNavigationNeighbor(NavigationDirection.RIGHT, edit)
        }
        var lastRow = _rows.isEmpty ? newPanel : _rows[-1]["row"].panel
        newPanel.setNavigationNeighbor(NavigationDirection.UP, lastRow)
        newPanel.setNavigationNeighbor(NavigationDirection.DOWN, newPanel)
        newPanel.setNavigationNeighbor(NavigationDirection.LEFT, newPanel)
        newPanel.setNavigationNeighbor(NavigationDirection.RIGHT, newPanel)
    }
}
