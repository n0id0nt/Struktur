// UI/Ride/WorkoutBrowserUI.wren
// The workout library (States/WorkoutBrowserState.wren owns it): a scrolling list of built-in and user workouts on the
// left, a preview of the focused one on the right (name, length, author, profile, description, or why it won't
// load), and Ride / Back. Pure view - the state supplies listings and previews and handles the callbacks.
import "app" for Application
import "math" for Vec2, Vec4
import "ui" for UIManager, UIPanel, UILabel, UIScroll, UIGraph, NavigationDirection, TextAlignment, TextWrapping
import "localization" for Localization
import "ride" for Ride
import "UI/Ride/RideTheme" for RideTheme, RideFonts, TextField, RideButton, RideListRow
import "UI/Ride/RideFormat" for RideFormat

var ROOT_Z = 1
var PANEL_Z = 2
var TEXT_Z = 6
var LIST_POSITION = Vec2.new(40, 140)
var LIST_SIZE = Vec2.new(520, 520)
var ROW_HEIGHT = 46
var ROW_GAP = 6
var PREVIEW_POSITION = Vec2.new(580, 140)
var PREVIEW_SIZE = Vec2.new(660, 520)

class WorkoutBrowserUI {
    construct new(userFolder) {
        _fonts = RideFonts.new()
        _rows = []

        _root = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(Application.gameWidth, Application.gameHeight), Vec2.new(0, 0))
        _root.setBackgroundColor(Vec4.new(20, 24, 30, 255))
        _root.setBorderColor(RideTheme.clear)
        _root.setZIndex(ROOT_Z)
        UIManager.addUIElement(_root)

        var title = TextField.new(_root, _fonts.bold(40), 40, Vec2.new(40, 22), Vec2.new(0, 0), Vec2.new(0, 0), RideTheme.text, TEXT_Z)
        title.text = Localization.get("ride.workouts_title")
        var info = TextField.new(_root, _fonts.regular(16), 16, Vec2.new(40, 80), Vec2.new(0, 0), Vec2.new(0, 0), RideTheme.muted, TEXT_Z)
        info.text = Localization.get("ride.workouts_drop_hint")
        var folder = TextField.new(_root, _fonts.regular(16), 16, Vec2.new(40, 102), Vec2.new(0, 0), Vec2.new(0, 0), RideTheme.accent, TEXT_Z)
        folder.text = userFolder

        var listPanel = RideTheme.panel(_root, LIST_POSITION, Vec2.new(0, 0), Vec2.new(0, 0), LIST_SIZE, PANEL_Z)
        _scroll = UIScroll.new(Vec2.new(10, 10), Vec2.new(0, 0), Vec2.new(-20, -20), Vec2.new(1, 1))
        _scroll.setVerticalScrollEnabled(true)
        _scroll.setZIndex(PANEL_Z + 1)
        listPanel.addChild(_scroll)

        var preview = RideTheme.panel(_root, PREVIEW_POSITION, Vec2.new(0, 0), Vec2.new(0, 0), PREVIEW_SIZE, PANEL_Z)
        preview.setBatchRoot(true)
        _name = TextField.new(preview, _fonts.bold(30), 30, Vec2.new(20, 16), Vec2.new(0, 0), Vec2.new(0, 0), RideTheme.text, TEXT_Z)
        _meta = TextField.new(preview, _fonts.regular(17), 17, Vec2.new(20, 58), Vec2.new(0, 0), Vec2.new(0, 0), RideTheme.muted, TEXT_Z)
        var well = UIPanel.new(Vec2.new(20, 90), Vec2.new(0, 0), Vec2.new(PREVIEW_SIZE.x - 40, 180), Vec2.new(0, 0))
        well.setBackgroundColor(Vec4.new(0, 0, 0, 90))
        well.setBorderColor(RideTheme.clear)
        well.setZIndex(PANEL_Z + 1)
        preview.addChild(well)
        _profile = UIGraph.new(Vec2.new(6, 6), Vec2.new(0, 0), Vec2.new(PREVIEW_SIZE.x - 52, 168), Vec2.new(0, 0))
        _profile.setZIndex(PANEL_Z + 2)
        well.addChild(_profile)
        _description = UILabel.new(Vec2.new(20, 288), Vec2.new(0, 0), "", 18)
        _description.setFont(_fonts.regular(18))
        _description.setTextColor(RideTheme.text)
        _description.setAlignment(TextAlignment.LEFT)
        _description.setWordWrap(TextWrapping.WORD_WRAP)
        _description.setSize(Vec2.new(PREVIEW_SIZE.x - 40, 150), Vec2.new(0, 0))
        _description.setZIndex(TEXT_Z)
        preview.addChild(_description)

        _rideButton = RideButton.new(preview, _fonts.bold(20), Localization.get("ride.ride_button"), Vec2.new(PREVIEW_SIZE.x - 20, PREVIEW_SIZE.y - 20), Vec2.new(0, 0), Vec2.new(1, 1), Vec2.new(180, 48), TEXT_Z)
        _rideButton.active = true
        _rideButton.activeColor = RideTheme.good
        _rideButton.setOnClick { |sender, mousePos|
            if (_onRide != null) _onRide.call()
        }
        _backButton = RideButton.new(preview, _fonts.bold(20), Localization.get("ride.back"), Vec2.new(PREVIEW_SIZE.x - 216, PREVIEW_SIZE.y - 20), Vec2.new(0, 0), Vec2.new(1, 1), Vec2.new(140, 48), TEXT_Z)
        _backButton.setOnClick { |sender, mousePos|
            if (_onBack != null) _onBack.call()
        }
    }

    onSelect=(fn) { _onSelect = fn }  // fn.call(index) - a row gained focus
    onRide=(fn) { _onRide = fn }      // fn.call()
    onBack=(fn) { _onBack = fn }      // fn.call()

    // listings: [{"name", "durationSeconds", "source", "error"}]; focusIndex: the row to focus.
    setListings(listings, focusIndex) {
        for (row in _rows) _scroll.removeChild(row.panel)
        _rows = []
        var width = LIST_SIZE.x - 20
        for (i in 0...listings.count) {
            var listing = listings[i]
            var value = listing["error"] != "" ? Localization.get("ride.workout_invalid") : RideFormat.duration(listing["durationSeconds"])
            var label = listing["source"] == "file" ? listing["name"] + "  (" + Localization.get("ride.dropped") + ")" : listing["name"]
            var row = RideListRow.new(_scroll, _fonts, label, value, Vec2.new(0, i * (ROW_HEIGHT + ROW_GAP)), Vec2.new(width, ROW_HEIGHT), TEXT_Z)
            if (listing["error"] != "") row.labelColor = RideTheme.muted
            var index = i
            row.setOnFocused(Fn.new {
                if (_onSelect != null) _onSelect.call(index)
            })
            row.setOnClick { |sender, mousePos|
                if (_onRide != null) _onRide.call()
            }
            _rows.add(row)
        }
        linkNavigation_()
        if (_rows.isEmpty) {
            UIManager.setFocus(_backButton.panel)
        } else {
            UIManager.setFocus(_rows[focusIndex.clamp(0, _rows.count - 1)].panel)
        }
    }

    // preview: Ride.loadWorkout(...) result; workoutId + ftp to draw the profile.
    showPreview(preview, listing, ftpWatts) {
        if (preview["ok"]) {
            _name.text = preview["name"]
            var source = listing["source"] == "builtin" ? Localization.get("ride.source_builtin") : (listing["source"] == "user" ? Localization.get("ride.source_user") : Localization.get("ride.dropped"))
            var author = preview["author"] == "" ? "" : "  -  " + preview["author"]
            _meta.text = RideFormat.duration(preview["durationSeconds"]) + "  -  " + preview["steps"].count.toString + " " + Localization.get("ride.steps") + author + "  -  " + source
            _meta.color = RideTheme.muted
            _description.setText(preview["description"])
            Ride.previewProfile(_profile, listing["id"], ftpWatts)
        } else {
            _name.text = listing["name"]
            _meta.text = Localization.get("ride.workout_error") + " " + preview["error"]
            _meta.color = RideTheme.bad
            _description.setText("")
            _profile.clear()
        }
        _rideButton.active = preview["ok"]
    }

    linkNavigation_() {
        for (i in 0..._rows.count) {
            var panel = _rows[i].panel
            panel.setNavigationNeighbor(NavigationDirection.UP, i > 0 ? _rows[i - 1].panel : panel)
            panel.setNavigationNeighbor(NavigationDirection.DOWN, i < _rows.count - 1 ? _rows[i + 1].panel : panel)
            panel.setNavigationNeighbor(NavigationDirection.LEFT, panel)
            panel.setNavigationNeighbor(NavigationDirection.RIGHT, _rideButton.panel)
        }
        var firstRow = _rows.isEmpty ? _backButton.panel : _rows[0].panel
        _rideButton.panel.setNavigationNeighbor(NavigationDirection.LEFT, _backButton.panel)
        _rideButton.panel.setNavigationNeighbor(NavigationDirection.RIGHT, _rideButton.panel)
        _rideButton.panel.setNavigationNeighbor(NavigationDirection.UP, firstRow)
        _rideButton.panel.setNavigationNeighbor(NavigationDirection.DOWN, _rideButton.panel)
        _backButton.panel.setNavigationNeighbor(NavigationDirection.LEFT, firstRow)
        _backButton.panel.setNavigationNeighbor(NavigationDirection.RIGHT, _rideButton.panel)
        _backButton.panel.setNavigationNeighbor(NavigationDirection.UP, firstRow)
        _backButton.panel.setNavigationNeighbor(NavigationDirection.DOWN, _backButton.panel)
    }

    teardown() {
        if (_root != null) {
            UIManager.removeUIElement(_root)
            _root = null
        }
        _rows = []
        _scroll = null
        _profile = null
        _description = null
        _rideButton = null
        _backButton = null
        _fonts.unloadAll()
    }
}
