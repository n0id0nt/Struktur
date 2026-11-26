// UI/Ride/RideHudUI.wren
// The ride screen (Free Ride and Workout share it), built from the UI kit (UI/Theme/UIKit.wren): translucent "hud"
// frames over the 3D world, the kit's pixel font, kit buttons. Which panels exist, where they sit and how big their
// text is comes from a layout in assets/Settings/RideLayout.json (UI/Ride/RideLayout.wren) - everything inside a
// panel is laid out from the panel's size and size class, so a new layout is a data edit:
//
//   tiles    stat tiles in a row or column (power, cadence, heart rate, speed, grade, target, ...)
//   workout  workout name, step, countdown, step target, next step, profile - or Free Ride time / lap / distance
//   erg      ERG badge, resistance -/+ (trim or grade), Free Ride target or workout skip / +30 s
//   graph    power and heart rate history; status: sensors + End ride; hints: keys; cue / banner: shown when needed
//
// Pure view: refresh(state) shows a Ride.state() snapshot; clicks go back through the callbacks the state sets.
// Each panel is its own UI batch, so a value changing in one panel doesn't rebuild the others.
import "app" for Application
import "math" for Vec2, Vec4
import "ui" for UIManager, UIPanel, UIGraph, NavigationDirection
import "localization" for Localization
import "ride" for Ride
import "UI/Theme/UIKit" for UIKit
import "UI/Ride/RideTheme" for RideTheme, TextField
import "UI/Ride/RideFormat" for RideFormat
import "UI/Ride/RideLayout" for RideLayout

var ROOT_Z = 1
var PANEL_Z = 2
var TEXT_Z = 6
var OVERLAY_Z = 3
var HUD_TINT = Vec4.new(255, 255, 255, 220)
var CUE_TINT = Vec4.new(150, 210, 255, 235)

// Size classes: [caption, value, hero (power), mid (secondary values), button height, padding]. Text sizes are whole
// multiples of the kit font's 9 px grid, so they draw crisp.
var SIZES = {
    "L": [18, 45, 72, 27, 34, 12],
    "M": [18, 36, 54, 27, 30, 10],
    "S": [9, 27, 36, 18, 26, 8]
}

// A kit button for the HUD: orange "button" art, lighter while focused; "active" (ERG ON) keeps the lit look, and an
// "off" toggle shows the grey disabled art.
class HudButton {
    construct new(parent, font, labelSize, text, absPos, size, z) {
        _active = null
        _focused = false
        _panel = UIKit.frame(parent, "button", absPos, Vec2.new(0, 0), size, Vec2.new(0, 0), z)
        _panel.setFocusable(true)
        _label = TextField.new(_panel, font, labelSize, Vec2.new(0, 0), Vec2.new(0.5, 0.5), Vec2.new(0.5, 0.5), UIKit.text, z + 1)
        _label.text = text
        _panel.setOnFocus { |sender|
            _focused = true
            applyLook_()
        }
        _panel.setOnLoseFocus { |sender|
            _focused = false
            applyLook_()
        }
        // The HUD reads its own keys: arrows must never walk focus around the screen.
        for (direction in [NavigationDirection.UP, NavigationDirection.DOWN, NavigationDirection.LEFT, NavigationDirection.RIGHT]) {
            _panel.setNavigationNeighbor(direction, _panel)
        }
    }

    panel { _panel }
    text=(value) { _label.text = value }
    setOnClick(fn) { _panel.setOnClick(fn) }
    visible=(value) { _panel.setVisible(value) }

    // null = a plain button; true / false = a toggle shown lit / grey.
    active=(value) {
        if (_active == value) return
        _active = value
        applyLook_()
    }

    applyLook_() {
        if (_active == false) {
            UIKit.applyStyle(_panel, "buttonDisabled")
            _label.color = UIKit.textMuted
        } else if (_focused || _active == true) {
            UIKit.applyStyle(_panel, "buttonHover")
            _label.color = UIKit.textDark
        } else {
            UIKit.applyStyle(_panel, "button")
            _label.color = UIKit.text
        }
    }
}

class RideHudUI {
    // mode: "freeRide" | "workout"; imperial: units for speed/distance/elevation; layoutName: a RideLayout.json layout.
    construct new(mode, imperial, isDemo, graphWindowSeconds, layoutName) {
        _mode = mode
        _imperial = imperial
        _isDemo = isDemo
        _graphWindow = graphWindowSeconds
        _cueUntil = 0
        _targetEntry = null
        _tiles = []
        _buttons = []
        _graphs = []
        _profiles = []
        _dots = {}
        _cuePanels = []
        _bannerPanels = []
        _workoutFields = {}
        _ergFields = {}
        UIKit.open()
        _font = UIKit.font()

        _root = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(Application.gameWidth, Application.gameHeight), Vec2.new(0, 0))
        // Transparent over the procedural world (drawn behind the UI); a solid backdrop when the world is off.
        _root.setBackgroundColor(Ride.worldActive() ? RideTheme.clear : Vec4.new(20, 24, 30, 255))
        _root.setBorderColor(RideTheme.clear)
        _root.setZIndex(ROOT_Z)
        UIManager.addUIElement(_root)

        for (spec in RideLayout.load(layoutName)) build_(spec)
    }

    // --- callbacks the state sets ------------------------------------------------------------------------------
    onErg=(fn) { _onErg = fn }                  // fn.call()
    onResistance=(fn) { _onResistance = fn }    // fn.call(direction)
    onTrimMode=(fn) { _onTrimMode = fn }        // fn.call()
    onTargetNudge=(fn) { _onTargetNudge = fn }  // fn.call(direction)
    onTargetEdit=(fn) { _onTargetEdit = fn }    // fn.call()
    onSkip=(fn) { _onSkip = fn }                // fn.call()
    onExtend=(fn) { _onExtend = fn }            // fn.call()
    onEnd=(fn) { _onEnd = fn }                  // fn.call()

    // --- building ----------------------------------------------------------------------------------------------

    tr_(key) { Localization.get(key) }

    build_(spec) {
        var sizes = SIZES[spec.sizeClass]
        var panel = UIKit.frame(_root, "hud", spec.position, spec.anchor, spec.size, Vec2.new(0, 0), PANEL_Z)
        panel.setAnchorPoint(spec.anchor)
        panel.setTint(HUD_TINT)
        panel.setBatchRoot(true)
        var widget = spec.widget
        if (widget == "tiles") {
            buildTiles_(panel, spec.tiles, spec.flow, sizes, 0, 0, spec.size.x, spec.size.y)
        } else if (widget == "workout") {
            buildWorkout_(panel, spec.size, sizes)
        } else if (widget == "erg") {
            buildErg_(panel, spec.size, sizes)
        } else if (widget == "graph") {
            buildGraph_(panel, spec.size, sizes)
        } else if (widget == "status") {
            buildStatus_(panel, spec.size, sizes)
        } else if (widget == "hints") {
            buildHints_(panel, spec.size, sizes)
        } else if (widget == "cue") {
            panel.setTint(CUE_TINT)
            var text = text_(panel, sizes[3], spec.size.x / 2, (spec.size.y - sizes[3]) / 2, Vec2.new(0.5, 0), UIKit.text)
            panel.setVisible(false)
            _cuePanels.add([panel, text])
        } else if (widget == "banner") {
            var text = text_(panel, sizes[3], spec.size.x / 2, (spec.size.y - sizes[3]) / 2, Vec2.new(0.5, 0), UIKit.accent)
            panel.setVisible(false)
            _bannerPanels.add([panel, text])
        }
    }

    text_(parent, size, x, y, anchor, color) {
        return TextField.new(parent, _font, size, Vec2.new(x, y), Vec2.new(0, 0), anchor, color, TEXT_Z)
    }

    button_(parent, sizes, text, x, y, width, onClick) {
        var height = sizes[4]
        var button = HudButton.new(parent, _font, height > 28 ? 18 : 9, text, Vec2.new(x, y), Vec2.new(width, height), TEXT_Z)
        button.setOnClick(onClick)
        _buttons.add(button)
        return button
    }

    tileCaption_(id) {
        var caption = tr_("ride.tile_" + id)
        if (id == "speed" || id == "averageSpeed") return caption + " " + RideFormat.speedUnit(_imperial)
        if (id == "distance") return caption + " " + RideFormat.distanceUnit(_imperial)
        if (id == "climbed") return caption + " " + RideFormat.elevationUnit(_imperial)
        return caption
    }

    // Stat tiles in the rectangle (x, y, width, height) of `panel`. Row: equal columns, caption over value, centred.
    // Column: equal rows, caption left and value right on one line.
    buildTiles_(panel, ids, flow, sizes, x, y, width, height) {
        var caption = sizes[0]
        var pad = sizes[5]
        var count = ids.count
        for (i in 0...count) {
            var id = ids[i]
            var valueSize = id == "power" ? sizes[2] : sizes[1]
            var captionField = null
            var valueField = null
            if (flow == "row") {
                var cellWidth = (width - 2 * pad) / count
                var centre = x + pad + cellWidth * i + cellWidth / 2
                // The hero size only where the panel is tall enough for it.
                if (pad * 2 + caption + 3 + valueSize > height) valueSize = sizes[1]
                captionField = text_(panel, caption, centre, y + pad, Vec2.new(0.5, 0), UIKit.textMuted)
                valueField = text_(panel, valueSize, centre, y + pad + caption + 3, Vec2.new(0.5, 0), UIKit.text)
            } else {
                var cellHeight = (height - 2 * pad) / count
                if (valueSize > cellHeight) valueSize = sizes[1]
                var top = y + pad + cellHeight * i
                captionField = text_(panel, caption, x + pad, top + (cellHeight - caption) / 2, Vec2.new(0, 0), UIKit.textMuted)
                valueField = text_(panel, valueSize, x + width - pad, top + (cellHeight - valueSize) / 2, Vec2.new(1, 0), UIKit.text)
            }
            captionField.text = tileCaption_(id)
            valueField.text = "--"
            _tiles.add([id, valueField, captionField])
        }
    }

    buildWorkout_(panel, size, sizes) {
        var caption = sizes[0]
        var value = sizes[1]
        var mid = sizes[3]
        var pad = sizes[5]
        var width = size.x
        var height = size.y
        var fields = _workoutFields
        if (_mode == "workout") {
            var y = pad
            fields["name"] = text_(panel, caption, pad, y, Vec2.new(0, 0), UIKit.accent)
            fields["time"] = text_(panel, caption, width - pad, y, Vec2.new(1, 0), UIKit.textMuted)
            y = y + caption + 3
            fields["step"] = text_(panel, caption, pad, y, Vec2.new(0, 0), UIKit.textMuted)
            y = y + caption + 3
            fields["countdown"] = text_(panel, value, pad, y, Vec2.new(0, 0), UIKit.text)
            fields["stepTarget"] = text_(panel, mid, width - pad, y + value - mid, Vec2.new(1, 0), UIKit.text)
            y = y + value + 3
            fields["next"] = text_(panel, caption, pad, y, Vec2.new(0, 0), UIKit.textMuted)
            fields["cadence"] = text_(panel, caption, width - pad, y, Vec2.new(1, 0), UIKit.textMuted)
            y = y + caption + 4
            var graphHeight = height - pad - y
            if (graphHeight >= 16) {
                var profile = UIGraph.new(Vec2.new(pad, y), Vec2.new(0, 0), Vec2.new(width - 2 * pad, graphHeight), Vec2.new(0, 0))
                profile.setZIndex(PANEL_Z + 1)
                panel.addChild(profile)
                _profiles.add(profile)
            }
        } else {
            var title = text_(panel, caption, pad, pad, Vec2.new(0, 0), UIKit.accent)
            title.text = tr_("ride.free_ride")
            var hint = text_(panel, caption, width - pad, pad, Vec2.new(1, 0), UIKit.textMuted)
            hint.text = tr_("ride.lap_hint")
            var top = pad + caption + 8
            buildTiles_(panel, ["elapsed", "lapTime", "distance"], "row", sizes, 0, top - pad, width, height - top + pad)
        }
    }

    buildErg_(panel, size, sizes) {
        var mid = 18  // beside three buttons: the 2x font fits at every size class
        var caption = sizes[0]
        var buttonHeight = sizes[4]
        var pad = sizes[5]
        var width = size.x
        var fields = _ergFields
        var gap = 4
        var small = (buttonHeight * 1.3).round
        var y = pad
        fields["erg"] = button_(panel, sizes, "ERG", pad, y, (buttonHeight * 3.4).round, Fn.new { |sender, mousePos|
            if (_onErg != null) _onErg.call()
        })
        fields["detail"] = text_(panel, caption, width - pad, y + (buttonHeight - caption) / 2, Vec2.new(1, 0), UIKit.textMuted)
        y = y + buttonHeight + gap
        fields["resistance"] = text_(panel, mid, pad, y + (buttonHeight - mid) / 2, Vec2.new(0, 0), UIKit.text)
        fields["trimMode"] = button_(panel, sizes, "W", width - pad - small * 3 - gap * 2, y, small, Fn.new { |sender, mousePos|
            if (_onTrimMode != null) _onTrimMode.call()
        })
        button_(panel, sizes, "-", width - pad - small * 2 - gap, y, small, Fn.new { |sender, mousePos|
            if (_onResistance != null) _onResistance.call(-1)
        })
        button_(panel, sizes, "+", width - pad - small, y, small, Fn.new { |sender, mousePos|
            if (_onResistance != null) _onResistance.call(1)
        })
        y = y + buttonHeight + gap
        if (y + buttonHeight > size.y - pad + 2) return
        if (_mode == "freeRide") {
            fields["targetCaption"] = text_(panel, mid, pad, y + (buttonHeight - mid) / 2, Vec2.new(0, 0), UIKit.text)
            fields["targetEdit"] = button_(panel, sizes, "--", width - pad - small * 2 - gap * 2 - small * 2, y, small * 2, Fn.new { |sender, mousePos|
                if (_onTargetEdit != null) _onTargetEdit.call()
            })
            fields["nudgeDown"] = button_(panel, sizes, "-", width - pad - small * 2 - gap, y, small, Fn.new { |sender, mousePos|
                if (_onTargetNudge != null) _onTargetNudge.call(-1)
            })
            fields["nudgeUp"] = button_(panel, sizes, "+", width - pad - small, y, small, Fn.new { |sender, mousePos|
                if (_onTargetNudge != null) _onTargetNudge.call(1)
            })
        } else {
            var half = (width - 2 * pad - gap) / 2
            button_(panel, sizes, tr_("ride.skip_step"), pad, y, half, Fn.new { |sender, mousePos|
                if (_onSkip != null) _onSkip.call()
            })
            button_(panel, sizes, tr_("ride.extend_step"), pad + half + gap, y, half, Fn.new { |sender, mousePos|
                if (_onExtend != null) _onExtend.call()
            })
        }
    }

    buildGraph_(panel, size, sizes) {
        var caption = sizes[0]
        var pad = sizes[5]
        var powerKey = text_(panel, caption, pad, pad, Vec2.new(0, 0), UIKit.text)
        powerKey.text = tr_("ride.graph_power")
        var heartKey = text_(panel, caption, pad + caption * 7, pad, Vec2.new(0, 0), UIKit.textMuted)
        heartKey.text = tr_("ride.graph_heart_rate")
        var window = text_(panel, caption, size.x - pad, pad, Vec2.new(1, 0), UIKit.textMuted)
        window.text = tr_("ride.graph_last") + " " + (_graphWindow / 60).round.toString + " min"
        var top = pad + caption + 3
        var graph = UIGraph.new(Vec2.new(pad, top), Vec2.new(0, 0), Vec2.new(size.x - 2 * pad, size.y - top - pad), Vec2.new(0, 0))
        graph.setZIndex(PANEL_Z + 1)
        panel.addChild(graph)
        _graphs.add(graph)
    }

    buildStatus_(panel, size, sizes) {
        var caption = sizes[0]
        var dotSize = (caption * 0.8).round
        var x = sizes[5]
        var middle = size.y / 2
        for (entry in [["trainer", "ride.sensor_trainer"], ["heartRate", "ride.sensor_heart_rate"], ["powerMeter", "ride.sensor_power_meter"]]) {
            var dot = UIPanel.new(Vec2.new(x, middle - dotSize / 2), Vec2.new(0, 0), Vec2.new(dotSize, dotSize), Vec2.new(0, 0))
            dot.setBackgroundColor(UIKit.textMuted)
            dot.setBorderColor(RideTheme.clear)
            dot.setZIndex(TEXT_Z)
            panel.addChild(dot)
            var label = text_(panel, caption, x + dotSize + 5, middle - caption / 2, Vec2.new(0, 0), UIKit.textMuted)
            label.text = tr_(entry[1])
            _dots[entry[0]] = [dot, label, null]
            x = x + dotSize + 5 + caption * 8
        }
        if (_isDemo) {
            var demo = text_(panel, caption, x, middle - caption / 2, Vec2.new(0, 0), UIKit.accent)
            demo.text = "DEMO"
        }
        var endSizes = [sizes[0], sizes[1], sizes[2], sizes[3], size.y - 4, sizes[5]]
        button_(panel, endSizes, tr_("ride.end_ride"), size.x - 2 - caption * 9, 2, caption * 9, Fn.new { |sender, mousePos|
            if (_onEnd != null) _onEnd.call()
        })
    }

    buildHints_(panel, size, sizes) {
        var caption = sizes[0]
        var hints = text_(panel, caption, size.x - sizes[5], (size.y - caption) / 2, Vec2.new(1, 0), UIKit.textMuted)
        hints.text = (_mode == "workout" ? tr_("ride.keys_workout") : tr_("ride.keys_free_ride")) + "   " + tr_("ride.keys_layout")
    }

    // --- refresh -----------------------------------------------------------------------------------------------

    refresh(state, now) {
        if (_root == null || state == null) return
        for (tile in _tiles) refreshTile_(tile, state)
        refreshErg_(state)
        refreshStatus_(state)
        refreshBanner_(state)
        refreshWorkout_(state)
        if (_cueShown && now >= _cueUntil) {
            _cueShown = false
            for (entry in _cuePanels) entry[0].setVisible(false)
        }
        for (graph in _graphs) Ride.updateGraph(graph, _graphWindow)
    }

    refreshTile_(tile, state) {
        var id = tile[0]
        var field = tile[1]
        var imperial = _imperial
        if (id == "power") {
            field.text = RideFormat.whole(state["power"])
            field.color = state["power"] == null ? UIKit.textMuted : RideTheme.color(state["powerColor"])
        } else if (id == "averagePower") {
            field.text = RideFormat.whole(state["averagePower"])
        } else if (id == "cadence") {
            field.text = RideFormat.whole(state["cadence"])
        } else if (id == "heartRate") {
            field.text = RideFormat.whole(state["heartRate"])
            field.color = state["heartRate"] == null ? UIKit.textMuted : RideTheme.color(state["heartRateColor"])
        } else if (id == "speed") {
            field.text = RideFormat.speed(state["speed"], imperial)
        } else if (id == "averageSpeed") {
            field.text = RideFormat.speed(state["averageSpeed"], imperial)
        } else if (id == "grade") {
            field.text = RideFormat.grade(state["grade"])
        } else if (id == "climbed") {
            field.text = RideFormat.elevation(state["elevationGain"], imperial)
        } else if (id == "target") {
            var target = state["target"]
            field.text = target == null ? "--" : target.round.toString + " W"
        } else if (id == "inTarget") {
            var seconds = state["targetSeconds"]
            field.text = seconds > 0 ? (state["inTargetSeconds"] * 100 / seconds).round.toString + "\%" : "--"
        } else if (id == "distance") {
            field.text = RideFormat.distance(state["distance"], imperial)
        } else if (id == "elapsed") {
            field.text = RideFormat.duration(state["timer"])
        } else if (id == "lapTime") {
            field.text = RideFormat.duration(state["lapSeconds"])
            tile[2].text = tr_("ride.tile_lapTime") + " " + state["lap"].toString
        }
    }

    refreshErg_(state) {
        var fields = _ergFields
        if (!fields.containsKey("erg")) return
        var erg = state["erg"]
        fields["erg"].text = erg ? "ERG ON" : "ERG OFF"
        fields["erg"].active = erg
        var detail = fields["detail"]
        if (state["cadenceProtection"]) {
            detail.text = tr_("ride.cadence_protection")
            detail.color = UIKit.accent
        } else if (state["controlMode"] == "erg") {
            detail.text = tr_("ride.holding") + " " + RideFormat.whole(state["target"]) + " W"
            detail.color = UIKit.textMuted
        } else {
            detail.text = tr_("ride.simulation")
            detail.color = UIKit.textMuted
        }
        if (erg) {
            var percent = state["trimMode"] == "percent"
            fields["resistance"].text = tr_("ride.trim") + " " + (percent ? RideFormat.signedWhole(state["trimPercent"]) + "\%" : RideFormat.signedWhole(state["trimWatts"]) + " W")
            fields["trimMode"].text = percent ? "\%" : "W"
        } else {
            fields["resistance"].text = tr_("ride.grade") + " " + RideFormat.grade(state["userGrade"])
        }
        fields["trimMode"].visible = erg
        if (fields.containsKey("targetEdit")) {
            fields["targetCaption"].text = tr_("ride.target")
            fields["targetEdit"].text = state["freeRideTarget"].round.toString + " W"
            fields["targetCaption"].visible = erg
            fields["targetEdit"].visible = erg
            fields["nudgeDown"].visible = erg
            fields["nudgeUp"].visible = erg
        }
    }

    refreshStatus_(state) {
        var sensors = state["sensors"]
        for (key in _dots.keys) {
            var entry = _dots[key]
            var status = sensors[key]
            if (status != entry[2]) {
                entry[2] = status
                var color = status == "connected" ? RideTheme.good : (status == "lost" ? RideTheme.bad : Vec4.new(70, 78, 90, 255))
                entry[0].setBackgroundColor(color)
                entry[1].color = status == "none" ? UIKit.textMuted : UIKit.text
            }
        }
    }

    refreshBanner_(state) {
        var text = null
        if (state["waiting"]) {
            text = tr_("ride.banner_start")
        } else if (state["autoPaused"]) {
            text = tr_("ride.banner_auto_paused")
        } else if (state["paused"]) {
            text = tr_("ride.banner_paused")
        } else if (_workoutComplete) {
            text = tr_("ride.banner_workout_complete")
        }
        var show = text != null
        for (entry in _bannerPanels) {
            if (show) entry[1].text = text
            if (show != _bannerShown) entry[0].setVisible(show)
        }
        _bannerShown = show
    }

    refreshWorkout_(state) {
        var fields = _workoutFields
        if (!fields.containsKey("name")) return
        var workout = state["workout"]
        if (workout == null) return
        _workoutComplete = workout["finished"]
        fields["name"].text = workout["name"]
        fields["step"].text = tr_("ride.step") + " " + (workout["stepIndex"] + 1).toString + "/" + workout["stepCount"].toString + "  " + workout["stepLabel"]
        fields["countdown"].text = workout["finished"] ? tr_("ride.done") : RideFormat.duration(workout["stepRemaining"])
        if (workout["stepHasTarget"] && !workout["finished"]) {
            var stepTarget = state["stepTarget"]
            var ftp = state["ftpWatts"]
            fields["stepTarget"].text = RideFormat.whole(stepTarget) + " W " + (stepTarget == null ? "" : (stepTarget * 100 / ftp).round.toString + "\%")
        } else {
            fields["stepTarget"].text = workout["finished"] ? "" : tr_("ride.free")
        }
        var cadence = workout["stepCadence"]
        fields["cadence"].text = cadence == null ? "" : cadence.round.toString + " rpm"
        if (workout.containsKey("nextLabel")) {
            var nextTarget = workout["nextHasTarget"] ? " @ " + workout["nextLow"].round.toString + " W" : ""
            fields["next"].text = tr_("ride.next") + " " + RideFormat.duration(workout["nextDuration"]) + nextTarget
        } else {
            fields["next"].text = tr_("ride.next") + " " + tr_("ride.finish")
        }
        fields["time"].text = RideFormat.duration(workout["planPosition"]) + " / " + RideFormat.duration(workout["totalDuration"])
        for (profile in _profiles) Ride.updateProfile(profile)
    }

    showCue(message, durationSeconds, now) {
        if (_root == null) return
        _cueUntil = now + (durationSeconds > 0 ? durationSeconds : 8)
        for (entry in _cuePanels) {
            entry[1].text = message
            entry[0].setVisible(true)
        }
        _cueShown = true
    }

    // --- Free Ride target entry ---------------------------------------------------------------------------------

    isTargetEntryOpen { _targetEntry != null }

    openTargetEntry() {
        if (_targetEntry != null) return
        _targetEntry = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(Application.gameWidth, Application.gameHeight), Vec2.new(0, 0))
        _targetEntry.setBackgroundColor(Vec4.new(0, 0, 0, 150))
        _targetEntry.setBorderColor(RideTheme.clear)
        _targetEntry.setZIndex(OVERLAY_Z)
        UIManager.addUIElement(_targetEntry)
        var box = UIKit.frame(_targetEntry, "window", Vec2.new(0, 0), Vec2.new(0.5, 0.5), Vec2.new(420, 200), Vec2.new(0, 0), OVERLAY_Z + 1)
        box.setAnchorPoint(Vec2.new(0.5, 0.5))
        var caption = TextField.new(box, _font, 18, Vec2.new(0, 28), Vec2.new(0.5, 0), Vec2.new(0.5, 0), UIKit.text, OVERLAY_Z + 4)
        caption.text = tr_("ride.set_target_title")
        _entryText = TextField.new(box, _font, 54, Vec2.new(0, 64), Vec2.new(0.5, 0), Vec2.new(0.5, 0), UIKit.accent, OVERLAY_Z + 4)
        var hint = TextField.new(box, _font, 9, Vec2.new(0, 150), Vec2.new(0.5, 0), Vec2.new(0.5, 0), UIKit.text, OVERLAY_Z + 4)
        hint.text = tr_("ride.set_target_hint")
    }

    setTargetEntryText(digits) {
        if (_targetEntry != null) _entryText.text = digits + "_ W"
    }

    closeTargetEntry() {
        if (_targetEntry == null) return
        UIManager.removeUIElement(_targetEntry)
        _targetEntry = null
        _entryText = null
    }

    teardown() {
        closeTargetEntry()
        if (_root != null) {
            UIManager.removeUIElement(_root)
            _root = null
        }
        // Every field below pointed into the removed tree (see the reused-state rule).
        _tiles = []
        _buttons = []
        _graphs = []
        _profiles = []
        _dots = {}
        _cuePanels = []
        _bannerPanels = []
        _workoutFields = {}
        _ergFields = {}
        if (_font != null) {
            _font.unload()
            _font = null
        }
        UIKit.close()
    }
}
