// UI/Ride/RideHudUI.wren
// The ride screen (Free Ride and Workout share it). Every panel is placed from UI/Ride/RideLayout.wren
// (assets/Settings/RideLayout.json), so the layout can be rearranged without code changes:
//
//   workout  top left   - Workout: name, step + countdown, step target, next step, profile with position marker.
//                         Free Ride: "Free Ride", elapsed time, lap and lap time, distance.
//   stats    top centre - power (zone colour), cadence, heart rate (zone colour), speed, average speed, average power
//   grade    top right  - grade and virtual elevation climbed
//   target   top right  - the target being held (ERG) or the workout step's target (ERG off), time in target
//   erg      top right  - ERG badge (click/E), resistance with -/+ (ERG on: trim; off: grade), Free Ride target entry,
//                         workout skip / extend
//   cue      under the stats - workout text cues
//   banner   centre     - start / paused / auto-paused / workout complete
//   graph    bottom     - power and heart rate over the last N minutes, zone-coloured, target step line behind
//   status   bottom left - trainer / HR / power meter connection, End ride
//   hints    bottom right - keyboard keys
//
// Pure view: refresh(state) shows a Ride.state() snapshot; clicks go back through the callbacks the state sets.
// Each panel is its own UI batch, so a value changing in one panel doesn't rebuild the others.
import "app" for Application
import "math" for Vec2, Vec4
import "ui" for UIManager, UIPanel, UIGraph
import "localization" for Localization
import "ride" for Ride
import "UI/Ride/RideTheme" for RideTheme, RideFonts, TextField, RideButton
import "UI/Ride/RideFormat" for RideFormat
import "UI/Ride/RideLayout" for RideLayout

var ROOT_Z = 1
var PANEL_Z = 2
var TEXT_Z = 6
var OVERLAY_Z = 3
var STATUS_DOT_SIZE = 10

class RideHudUI {
    // mode: "freeRide" | "workout"; imperial: units for speed/distance/elevation.
    construct new(mode, imperial, isDemo, graphWindowSeconds) {
        _mode = mode
        _imperial = imperial
        _graphWindow = graphWindowSeconds
        _fonts = RideFonts.new()
        _layout = RideLayout.load()
        _cueUntil = 0
        _targetEntry = null

        _root = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(Application.gameWidth, Application.gameHeight), Vec2.new(0, 0))
        _root.setBackgroundColor(Vec4.new(20, 24, 30, 255))
        _root.setBorderColor(RideTheme.clear)
        _root.setZIndex(ROOT_Z)
        UIManager.addUIElement(_root)

        buildWorkoutPanel_()
        buildStatsPanel_()
        buildGradePanel_()
        buildTargetPanel_()
        buildErgPanel_()
        buildCuePanel_()
        buildBannerPanel_()
        buildGraphPanel_()
        buildStatusPanel_(isDemo)
        buildHintsPanel_()
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

    panel_(id) {
        var spec = _layout[id]
        var panel = RideTheme.panel(_root, spec.position, spec.anchor, spec.anchor, spec.size, PANEL_Z)
        panel.setBatchRoot(true)
        return panel
    }

    text_(parent, bold, size, x, y, anchor, color) {
        var font = bold ? _fonts.bold(size) : _fonts.regular(size)
        return TextField.new(parent, font, size, Vec2.new(x, y), Vec2.new(0, 0), anchor, color, TEXT_Z)
    }

    caption_(parent, text, x, y, anchor) {
        var field = text_(parent, false, 15, x, y, anchor, RideTheme.muted)
        field.text = text
        return field
    }

    tr_(key) { Localization.get(key) }

    buildWorkoutPanel_() {
        var spec = _layout["workout"]
        var width = spec.size.x
        var height = spec.size.y
        var panel = panel_("workout")
        var left = Vec2.new(0, 0)
        var right = Vec2.new(1, 0)
        if (_mode == "workout") {
            _workoutName = text_(panel, true, 22, 14, 10, left, RideTheme.text)
            _stepLine = text_(panel, false, 15, 14, 40, left, RideTheme.muted)
            _countdown = text_(panel, true, 54, 14, 58, left, RideTheme.text)
            _stepTarget = text_(panel, true, 26, width - 14, 66, right, RideTheme.text)
            _stepCadence = text_(panel, false, 15, width - 14, 98, right, RideTheme.muted)
            _nextLine = text_(panel, false, 16, 14, 130, left, RideTheme.muted)
            _workoutTime = text_(panel, false, 16, width - 14, 130, right, RideTheme.muted)
            var well = UIPanel.new(Vec2.new(14, 158), Vec2.new(0, 0), Vec2.new(width - 28, height - 170), Vec2.new(0, 0))
            well.setBackgroundColor(Vec4.new(0, 0, 0, 90))
            well.setBorderColor(RideTheme.clear)
            well.setZIndex(PANEL_Z + 1)
            panel.addChild(well)
            _profile = UIGraph.new(Vec2.new(4, 4), Vec2.new(0, 0), Vec2.new(width - 36, height - 178), Vec2.new(0, 0))
            _profile.setZIndex(PANEL_Z + 2)
            well.addChild(_profile)
        } else {
            var title = text_(panel, true, 28, 14, 10, left, RideTheme.text)
            title.text = tr_("ride.free_ride")
            caption_(panel, tr_("ride.elapsed"), 14, 54, left)
            _elapsed = text_(panel, true, 54, 14, 72, left, RideTheme.text)
            _lapCaption = caption_(panel, "LAP 1", 14, 146, left)
            _lapTime = text_(panel, true, 32, 14, 164, left, RideTheme.text)
            caption_(panel, tr_("ride.distance") + " (" + RideFormat.distanceUnit(_imperial) + ")", width / 2 + 10, 146, left)
            _distance = text_(panel, true, 32, width / 2 + 10, 164, left, RideTheme.text)
            caption_(panel, tr_("ride.lap_hint"), 14, height - 26, left)
        }
    }

    buildStatsPanel_() {
        var spec = _layout["stats"]
        var width = spec.size.x
        var panel = panel_("stats")
        var left = Vec2.new(0, 0)
        var centre = Vec2.new(0.5, 0)
        var right = Vec2.new(1, 0)
        caption_(panel, tr_("ride.power") + " (W)", width / 2, 10, centre)
        _power = text_(panel, true, 100, width / 2, 26, centre, RideTheme.text)
        caption_(panel, tr_("ride.average_power") + " (W)", width / 2, 140, centre)
        _averagePower = text_(panel, true, 34, width / 2, 158, centre, RideTheme.text)

        caption_(panel, tr_("ride.cadence") + " (rpm)", 20, 12, left)
        _cadence = text_(panel, true, 46, 20, 32, left, RideTheme.text)
        caption_(panel, tr_("ride.heart_rate") + " (bpm)", 20, 110, left)
        _heartRate = text_(panel, true, 46, 20, 130, left, RideTheme.text)

        caption_(panel, tr_("ride.speed") + " (" + RideFormat.speedUnit(_imperial) + ")", width - 20, 12, right)
        _speed = text_(panel, true, 46, width - 20, 32, right, RideTheme.text)
        caption_(panel, tr_("ride.average_speed"), width - 20, 110, right)
        _averageSpeed = text_(panel, true, 46, width - 20, 130, right, RideTheme.text)
    }

    buildGradePanel_() {
        var width = _layout["grade"].size.x
        var panel = panel_("grade")
        caption_(panel, tr_("ride.grade"), 14, 8, Vec2.new(0, 0))
        _grade = text_(panel, true, 38, 14, 26, Vec2.new(0, 0), RideTheme.text)
        caption_(panel, tr_("ride.climbed") + " (" + RideFormat.elevationUnit(_imperial) + ")", width - 14, 8, Vec2.new(1, 0))
        _climbed = text_(panel, true, 38, width - 14, 26, Vec2.new(1, 0), RideTheme.text)
    }

    buildTargetPanel_() {
        var width = _layout["target"].size.x
        var panel = panel_("target")
        _targetCaption = caption_(panel, tr_("ride.target"), 14, 8, Vec2.new(0, 0))
        _target = text_(panel, true, 38, 14, 26, Vec2.new(0, 0), RideTheme.text)
        _inTargetCaption = caption_(panel, tr_("ride.in_target"), width - 14, 8, Vec2.new(1, 0))
        _inTarget = text_(panel, true, 38, width - 14, 26, Vec2.new(1, 0), RideTheme.text)
    }

    buildErgPanel_() {
        var width = _layout["erg"].size.x
        var panel = panel_("erg")
        var buttonFont = _fonts.bold(17)
        _ergButton = RideButton.new(panel, buttonFont, "ERG", Vec2.new(12, 10), Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(128, 34), TEXT_Z)
        _ergButton.setOnClick { |sender, mousePos|
            if (_onErg != null) _onErg.call()
        }
        _ergDetail = text_(panel, false, 16, width - 14, 18, Vec2.new(1, 0), RideTheme.muted)

        _resistance = text_(panel, true, 22, 14, 60, Vec2.new(0, 0), RideTheme.text)
        _trimModeButton = RideButton.new(panel, buttonFont, "W", Vec2.new(width - 140, 54), Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(40, 34), TEXT_Z)
        _trimModeButton.setOnClick { |sender, mousePos|
            if (_onTrimMode != null) _onTrimMode.call()
        }
        var minus = RideButton.new(panel, _fonts.bold(22), "-", Vec2.new(width - 94, 54), Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(38, 34), TEXT_Z)
        minus.setOnClick { |sender, mousePos|
            if (_onResistance != null) _onResistance.call(-1)
        }
        var plus = RideButton.new(panel, _fonts.bold(22), "+", Vec2.new(width - 50, 54), Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(38, 34), TEXT_Z)
        plus.setOnClick { |sender, mousePos|
            if (_onResistance != null) _onResistance.call(1)
        }

        if (_mode == "freeRide") {
            _targetRowCaption = text_(panel, true, 22, 14, 104, Vec2.new(0, 0), RideTheme.text)
            _targetEditButton = RideButton.new(panel, buttonFont, "--", Vec2.new(width - 190, 98), Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(88, 34), TEXT_Z)
            _targetEditButton.setOnClick { |sender, mousePos|
                if (_onTargetEdit != null) _onTargetEdit.call()
            }
            _nudgeDown = RideButton.new(panel, _fonts.bold(22), "-", Vec2.new(width - 94, 98), Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(38, 34), TEXT_Z)
            _nudgeDown.setOnClick { |sender, mousePos|
                if (_onTargetNudge != null) _onTargetNudge.call(-1)
            }
            _nudgeUp = RideButton.new(panel, _fonts.bold(22), "+", Vec2.new(width - 50, 98), Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(38, 34), TEXT_Z)
            _nudgeUp.setOnClick { |sender, mousePos|
                if (_onTargetNudge != null) _onTargetNudge.call(1)
            }
        } else {
            var skip = RideButton.new(panel, buttonFont, tr_("ride.skip_step"), Vec2.new(12, 98), Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(130, 34), TEXT_Z)
            skip.setOnClick { |sender, mousePos|
                if (_onSkip != null) _onSkip.call()
            }
            var extend = RideButton.new(panel, buttonFont, tr_("ride.extend_step"), Vec2.new(150, 98), Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(width - 162, 34), TEXT_Z)
            extend.setOnClick { |sender, mousePos|
                if (_onExtend != null) _onExtend.call()
            }
        }
    }

    buildCuePanel_() {
        var spec = _layout["cue"]
        _cuePanel = panel_("cue")
        _cuePanel.setBackgroundColor(Vec4.new(20, 90, 120, 225))
        _cueText = text_(_cuePanel, true, 22, spec.size.x / 2, 10, Vec2.new(0.5, 0), RideTheme.text)
        _cuePanel.setVisible(false)
    }

    buildBannerPanel_() {
        var spec = _layout["banner"]
        _bannerPanel = panel_("banner")
        _bannerText = text_(_bannerPanel, true, 30, spec.size.x / 2, 12, Vec2.new(0.5, 0), RideTheme.text)
        _bannerPanel.setVisible(false)
        _bannerShown = false
    }

    buildGraphPanel_() {
        var spec = _layout["graph"]
        var width = spec.size.x
        var height = spec.size.y
        var panel = panel_("graph")
        var powerKey = text_(panel, true, 15, 14, 8, Vec2.new(0, 0), RideTheme.text)
        powerKey.text = tr_("ride.graph_power")
        var heartKey = text_(panel, true, 15, 160, 8, Vec2.new(0, 0), RideTheme.muted)
        heartKey.text = tr_("ride.graph_heart_rate")
        _graphWindowLabel = text_(panel, false, 15, width - 14, 8, Vec2.new(1, 0), RideTheme.muted)
        _graphWindowLabel.text = tr_("ride.graph_last") + " " + (_graphWindow / 60).round.toString + " min"
        _graph = UIGraph.new(Vec2.new(12, 30), Vec2.new(0, 0), Vec2.new(width - 24, height - 40), Vec2.new(0, 0))
        _graph.setZIndex(PANEL_Z + 1)
        panel.addChild(_graph)
    }

    buildStatusPanel_(isDemo) {
        var spec = _layout["status"]
        var panel = panel_("status")
        panel.setBackgroundColor(RideTheme.clear)
        panel.setBorderColor(RideTheme.clear)
        _dots = {}
        var x = 0
        for (entry in [["trainer", "ride.sensor_trainer"], ["heartRate", "ride.sensor_heart_rate"], ["powerMeter", "ride.sensor_power_meter"]]) {
            var dot = UIPanel.new(Vec2.new(x, 9), Vec2.new(0, 0), Vec2.new(STATUS_DOT_SIZE, STATUS_DOT_SIZE), Vec2.new(0, 0))
            dot.setBackgroundColor(RideTheme.muted)
            dot.setBorderColor(RideTheme.clear)
            dot.setZIndex(TEXT_Z)
            panel.addChild(dot)
            var label = text_(panel, false, 15, x + 16, 5, Vec2.new(0, 0), RideTheme.muted)
            label.text = tr_(entry[1])
            _dots[entry[0]] = [dot, label, null]
            x = x + 120
        }
        if (isDemo) {
            var demo = text_(panel, true, 15, x, 5, Vec2.new(0, 0), RideTheme.warn)
            demo.text = "DEMO"
        }
        var end = RideButton.new(panel, _fonts.bold(15), tr_("ride.end_ride"), Vec2.new(spec.size.x, 0), Vec2.new(0, 0), Vec2.new(1, 0), Vec2.new(96, 26), TEXT_Z)
        end.activeColor = RideTheme.bad
        end.setOnClick { |sender, mousePos|
            if (_onEnd != null) _onEnd.call()
        }
    }

    buildHintsPanel_() {
        var spec = _layout["hints"]
        var panel = panel_("hints")
        panel.setBackgroundColor(RideTheme.clear)
        panel.setBorderColor(RideTheme.clear)
        var hints = text_(panel, false, 15, spec.size.x, 5, Vec2.new(1, 0), RideTheme.muted)
        hints.text = _mode == "workout" ? tr_("ride.keys_workout") : tr_("ride.keys_free_ride")
    }

    // --- refresh -----------------------------------------------------------------------------------------------

    refresh(state, now) {
        if (_root == null || state == null) return
        var imperial = _imperial
        _power.text = RideFormat.whole(state["power"])
        _power.color = state["power"] == null ? RideTheme.muted : RideTheme.color(state["powerColor"])
        _averagePower.text = RideFormat.whole(state["averagePower"])
        _cadence.text = RideFormat.whole(state["cadence"])
        _heartRate.text = RideFormat.whole(state["heartRate"])
        _heartRate.color = state["heartRate"] == null ? RideTheme.muted : RideTheme.color(state["heartRateColor"])
        _speed.text = RideFormat.speed(state["speed"], imperial)
        _averageSpeed.text = RideFormat.speed(state["averageSpeed"], imperial)
        _grade.text = RideFormat.grade(state["grade"])
        _climbed.text = RideFormat.elevation(state["elevationGain"], imperial)

        refreshTarget_(state)
        refreshErg_(state)
        refreshStatus_(state)
        refreshBanner_(state)
        if (_mode == "workout") {
            refreshWorkout_(state)
        } else {
            _elapsed.text = RideFormat.duration(state["timer"])
            _lapCaption.text = "LAP " + state["lap"].toString
            _lapTime.text = RideFormat.duration(state["lapSeconds"])
            _distance.text = RideFormat.distance(state["distance"], imperial)
        }
        if (_cueShown && now >= _cueUntil) {
            _cueShown = false
            _cuePanel.setVisible(false)
        }
        Ride.updateGraph(_graph, _graphWindow)
    }

    refreshTarget_(state) {
        var target = state["target"]
        var caption = tr_("ride.target")
        if (state["mode"] == "workout") {
            caption = caption + (state["erg"] ? " - ERG" : " - " + tr_("ride.step"))
        } else if (state["erg"]) {
            caption = caption + " - ERG"
        }
        _targetCaption.text = caption
        _target.text = target == null ? "--" : target.round.toString + " W"
        var showScore = state["mode"] == "workout"
        _inTargetCaption.visible = showScore
        _inTarget.visible = showScore
        if (showScore) {
            var seconds = state["targetSeconds"]
            _inTarget.text = seconds > 0 ? (state["inTargetSeconds"] * 100 / seconds).round.toString + "\%" : "--"
        }
    }

    refreshErg_(state) {
        var erg = state["erg"]
        _ergButton.text = erg ? "ERG ON" : "ERG OFF"
        _ergButton.active = erg
        if (state["cadenceProtection"]) {
            _ergDetail.text = tr_("ride.cadence_protection")
            _ergDetail.color = RideTheme.warn
        } else if (state["controlMode"] == "erg") {
            _ergDetail.text = tr_("ride.holding") + " " + RideFormat.whole(state["target"]) + " W"
            _ergDetail.color = RideTheme.muted
        } else {
            _ergDetail.text = tr_("ride.simulation")
            _ergDetail.color = RideTheme.muted
        }
        if (erg) {
            var percent = state["trimMode"] == "percent"
            _resistance.text = tr_("ride.trim") + " " + (percent ? RideFormat.signedWhole(state["trimPercent"]) + "\%" : RideFormat.signedWhole(state["trimWatts"]) + " W")
            _trimModeButton.text = percent ? "\%" : "W"
        } else {
            _resistance.text = tr_("ride.grade") + " " + RideFormat.grade(state["userGrade"])
        }
        _trimModeButton.panel.setVisible(erg)
        if (_mode == "freeRide") {
            _targetRowCaption.text = tr_("ride.target")
            _targetEditButton.text = state["freeRideTarget"].round.toString + " W"
            var show = erg
            _targetRowCaption.visible = show
            _targetEditButton.panel.setVisible(show)
            _nudgeDown.panel.setVisible(show)
            _nudgeUp.panel.setVisible(show)
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
                entry[1].color = status == "none" ? Vec4.new(90, 98, 110, 255) : RideTheme.text
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
        if (text == null) {
            if (_bannerShown) {
                _bannerShown = false
                _bannerPanel.setVisible(false)
            }
            return
        }
        _bannerText.text = text
        if (!_bannerShown) {
            _bannerShown = true
            _bannerPanel.setVisible(true)
        }
    }

    refreshWorkout_(state) {
        var workout = state["workout"]
        if (workout == null) return
        _workoutComplete = workout["finished"]
        _workoutName.text = workout["name"]
        _stepLine.text = tr_("ride.step") + " " + (workout["stepIndex"] + 1).toString + "/" + workout["stepCount"].toString + "  " + workout["stepLabel"]
        _countdown.text = workout["finished"] ? tr_("ride.done") : RideFormat.duration(workout["stepRemaining"])
        if (workout["stepHasTarget"] && !workout["finished"]) {
            var stepTarget = state["stepTarget"]
            var ftp = state["ftpWatts"]
            _stepTarget.text = RideFormat.whole(stepTarget) + " W  " + (stepTarget == null ? "" : (stepTarget * 100 / ftp).round.toString + "\%")
        } else {
            _stepTarget.text = workout["finished"] ? "" : tr_("ride.free")
        }
        var cadence = workout["stepCadence"]
        _stepCadence.text = cadence == null ? "" : cadence.round.toString + " rpm"
        if (workout.containsKey("nextLabel")) {
            var nextTarget = workout["nextHasTarget"] ? " @ " + workout["nextLow"].round.toString + " W" : ""
            _nextLine.text = tr_("ride.next") + "  " + RideFormat.duration(workout["nextDuration"]) + nextTarget
        } else {
            _nextLine.text = tr_("ride.next") + "  " + tr_("ride.finish")
        }
        _workoutTime.text = RideFormat.duration(workout["planPosition"]) + " / " + RideFormat.duration(workout["totalDuration"])
        Ride.updateProfile(_profile)
    }

    showCue(message, durationSeconds, now) {
        if (_root == null) return
        _cueText.text = message
        _cueUntil = now + (durationSeconds > 0 ? durationSeconds : 8)
        if (!_cueShown) {
            _cueShown = true
            _cuePanel.setVisible(true)
        }
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
        var box = RideTheme.panel(_targetEntry, Vec2.new(0, 0), Vec2.new(0.5, 0.5), Vec2.new(0.5, 0.5), Vec2.new(420, 190), OVERLAY_Z + 1)
        box.setBackgroundColor(Vec4.new(18, 24, 32, 245))
        var caption = TextField.new(box, _fonts.regular(18), 18, Vec2.new(0, 16), Vec2.new(0.5, 0), Vec2.new(0.5, 0), RideTheme.muted, OVERLAY_Z + 4)
        caption.text = tr_("ride.set_target_title")
        _entryText = TextField.new(box, _fonts.bold(64), 64, Vec2.new(0, 44), Vec2.new(0.5, 0), Vec2.new(0.5, 0), RideTheme.text, OVERLAY_Z + 4)
        var hint = TextField.new(box, _fonts.regular(15), 15, Vec2.new(0, 150), Vec2.new(0.5, 0), Vec2.new(0.5, 0), RideTheme.muted, OVERLAY_Z + 4)
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
        _graph = null
        _profile = null
        _dots = {}
        _ergButton = null
        _trimModeButton = null
        _targetEditButton = null
        _nudgeDown = null
        _nudgeUp = null
        _cuePanel = null
        _bannerPanel = null
        _fonts.unloadAll()
    }
}
