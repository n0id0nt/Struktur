// States/RideSettingsState.wren
// Ride settings: the current rider's numbers (FTP, max HR, threshold HR, weight, units - saved on the rider profile,
// so they're never stored twice) and the app-wide ride options (zone model, graph window, power source, auto-pause,
// trim mode - saved to ride_settings.json through the ride module), plus Pair devices and Change rider. A second column
// holds the procedural world's settings (on/off, quality, grade source, biome, seed, look, comfort mode, stats).
// Left/Right changes a value (hold to repeat); Accept runs an action row; every change saves immediately. Down from the
// last left row moves to the world column, Up from its first row comes back.
import "app" for Application, Time
import "input" for Input
import "math" for Vec2, Vec4
import "ui" for UIManager, UIPanel, NavigationDirection
import "localization" for Localization
import "ride" for Ride
import "States/BaseState" for BaseState
import "UI/Ride/RideTheme" for RideTheme, RideFonts, TextField, RideListRow
import "UI/Ride/RideFormat" for RideFormat
import "Rider/RiderProfiles" for RiderProfiles, Units

var ROW_SIZE = Vec2.new(580, 38)
var LEFT_COLUMN_X = 40
var RIGHT_COLUMN_X = 660
var ROW_STEP = 42
var ROWS_TOP = 96
var REPEAT_DELAY_SECONDS = 0.4
var REPEAT_INTERVAL_SECONDS = 0.08
var DIRECTION_THRESHOLD = 0.5
var GRAPH_WINDOWS = [120, 300, 600, 1200, 1800]
var AUTO_PAUSES = [0, 3, 5, 10, 20, 30]
var POWER_SOURCES = ["auto", "trainer", "powerMeter"]
var ZONE_MODELS = ["percentMax", "percentLthr"]
var QUALITIES = ["low", "medium", "high"]
var GRADE_SOURCES = ["manual", "route"]
var LOOKS = ["pixel", "pixelDither", "crt", "smooth"]
var WORLD_ROWS = ["world", "worldQuality", "worldGradeSource", "worldBiome", "worldSeed", "worldLook", "worldComfort", "worldStats"]

class RideSettingsState is BaseState {
    construct new() {
        super()
        name = "RideSettingsState"
        _stateManager = null
        _root = null
        _pending = []
    }

    enter(stateManager, params) {
        super.enter(stateManager, params)
        _stateManager = stateManager
        _pending = []
        _heldDirection = 0
        _nextRepeat = 0
        _focusedRow = null
        _rider = RiderProfiles.current
        _fonts = RideFonts.new()

        _root = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(Application.gameWidth, Application.gameHeight), Vec2.new(0, 0))
        _root.setBackgroundColor(Vec4.new(20, 24, 30, 255))
        _root.setBorderColor(RideTheme.clear)
        _root.setZIndex(1)
        UIManager.addUIElement(_root)
        var title = TextField.new(_root, _fonts.bold(38), 38, Vec2.new(0, 24), Vec2.new(0.5, 0), Vec2.new(0.5, 0), RideTheme.text, 6)
        title.text = Localization.get("ride.settings_title")
        var hint = TextField.new(_root, _fonts.regular(16), 16, Vec2.new(0, -26), Vec2.new(0.5, 1), Vec2.new(0.5, 1), RideTheme.muted, 6)
        hint.text = Localization.get("ride.settings_hint")

        _leftIds = ["rider", "ftp", "maxHr", "thresholdHr", "weight", "units", "zones", "graph", "powerSource", "autoPause", "trimMode", "pair", "back"]
        _rowIds = _leftIds + WORLD_ROWS
        _rows = {}
        for (i in 0..._rowIds.count) {
            var id = _rowIds[i]
            var inLeft = i < _leftIds.count
            var x = inLeft ? LEFT_COLUMN_X : RIGHT_COLUMN_X
            var y = ROWS_TOP + (inLeft ? i : i - _leftIds.count) * ROW_STEP
            var row = RideListRow.new(_root, _fonts, Localization.get("ride.setting_" + id), "", Vec2.new(x, y), ROW_SIZE, 4)
            row.setOnFocused(Fn.new { _focusedRow = id })
            row.setOnClick { |sender, mousePos| _pending.add(["activate", id]) }
            _rows[id] = row
        }
        var worldTitle = TextField.new(_root, _fonts.bold(18), 18, Vec2.new(RIGHT_COLUMN_X, ROWS_TOP - 30), Vec2.new(0, 0), Vec2.new(0, 0), RideTheme.accent, 6)
        worldTitle.text = Localization.get("ride.world_title")
        // One chain of rows: the left column, then the world column.
        for (i in 0..._rowIds.count) {
            var panel = _rows[_rowIds[i]].panel
            panel.setNavigationNeighbor(NavigationDirection.UP, i > 0 ? _rows[_rowIds[i - 1]].panel : panel)
            panel.setNavigationNeighbor(NavigationDirection.DOWN, i < _rowIds.count - 1 ? _rows[_rowIds[i + 1]].panel : panel)
            // Left/Right change the value - never move focus.
            panel.setNavigationNeighbor(NavigationDirection.LEFT, panel)
            panel.setNavigationNeighbor(NavigationDirection.RIGHT, panel)
        }
        refresh_()
        UIManager.setFocus(_rows[_rider == null ? "rider" : "ftp"].panel)
    }

    update(stateManager) {
        if (_root == null) return
        var actions = _pending
        _pending = []
        for (action in actions) {
            activate_(action[1])
            if (_root == null) return
        }
        if (Input.isInputJustReleased("UICancel")) {
            _stateManager.changeState("MainMenu")
            return
        }
        // Left / Right with hold-to-repeat.
        var horizontal = Input.getInputAxis2("UIDir").x
        var direction = horizontal > DIRECTION_THRESHOLD ? 1 : (horizontal < -DIRECTION_THRESHOLD ? -1 : 0)
        if (direction == 0 || _focusedRow == null) {
            _heldDirection = 0
            return
        }
        var now = Time.unscaledTime
        if (direction != _heldDirection) {
            _heldDirection = direction
            _nextRepeat = now + REPEAT_DELAY_SECONDS
            change_(_focusedRow, direction)
        } else if (now >= _nextRepeat) {
            _nextRepeat = now + REPEAT_INTERVAL_SECONDS
            change_(_focusedRow, direction)
        }
    }

    exit() {
        super.exit()
        if (_root != null) {
            UIManager.removeUIElement(_root)
            _root = null
        }
        _rows = {}
        _pending = []
        if (_fonts != null) _fonts.unloadAll()
    }

    activate_(id) {
        if (id == "pair") {
            _stateManager.changeState("DevicePairing", {"returnTo": "RideSettings"})
        } else if (id == "rider") {
            _stateManager.changeState("RiderSelect")
        } else if (id == "back") {
            _stateManager.changeState("MainMenu")
        } else if (id == "worldSeed") {
            // Accept: a fresh random seed (Left/Right step it by one).
            Ride.setSetting("worldSeed", (Time.unscaledTime * 7919).floor % 1000000 + 1)
            Ride.saveSettings()
            refresh_()
        } else {
            change_(id, 1)
        }
    }

    // Cycles a list value by `direction`.
    cycle_(list, current, direction) {
        var index = list.indexOf(current)
        if (index < 0) index = 0
        return list[(index + direction) % list.count]
    }

    change_(id, direction) {
        var settings = Ride.settings()
        var riderChanged = false
        if (_rider != null) {
            if (id == "ftp") {
                _rider["ftpWatts"] = RiderProfiles.stepFtp(_rider["ftpWatts"], direction)
                riderChanged = true
            } else if (id == "maxHr") {
                _rider["maxHeartRateBpm"] = RiderProfiles.stepMaxHeartRate(_rider["maxHeartRateBpm"], direction)
                riderChanged = true
            } else if (id == "thresholdHr") {
                _rider["thresholdHeartRateBpm"] = RiderProfiles.stepThresholdHeartRate(_rider["thresholdHeartRateBpm"], direction)
                riderChanged = true
            } else if (id == "weight") {
                _rider["weightKg"] = Units.stepWeightKg(_rider["weightKg"], _rider["units"], direction)
                riderChanged = true
            } else if (id == "units") {
                _rider["units"] = _rider["units"] == "metric" ? "imperial" : "metric"
                riderChanged = true
            }
        }
        if (id == "zones") {
            Ride.setSetting("heartRateZoneModel", cycle_(ZONE_MODELS, settings["heartRateZoneModel"], direction))
        } else if (id == "graph") {
            Ride.setSetting("graphWindowSeconds", cycle_(GRAPH_WINDOWS, settings["graphWindowSeconds"], direction))
        } else if (id == "powerSource") {
            Ride.setSetting("powerSource", cycle_(POWER_SOURCES, settings["powerSource"], direction))
        } else if (id == "autoPause") {
            Ride.setSetting("autoPauseSeconds", cycle_(AUTO_PAUSES, settings["autoPauseSeconds"], direction))
        } else if (id == "trimMode") {
            Ride.setSetting("trimMode", settings["trimMode"] == "watts" ? "percent" : "watts")
        } else if (id == "world") {
            Ride.setSetting("worldEnabled", !settings["worldEnabled"])
        } else if (id == "worldQuality") {
            Ride.setSetting("worldQuality", cycle_(QUALITIES, settings["worldQuality"], direction))
        } else if (id == "worldGradeSource") {
            Ride.setSetting("worldGradeSource", cycle_(GRADE_SOURCES, settings["worldGradeSource"], direction))
        } else if (id == "worldBiome") {
            Ride.setSetting("worldBiome", cycle_(["auto"] + Ride.biomes(), settings["worldBiome"], direction))
        } else if (id == "worldSeed") {
            var seed = settings["worldSeed"] + direction
            Ride.setSetting("worldSeed", seed < 0 ? 0 : seed)
        } else if (id == "worldLook") {
            applyLook_(cycle_(LOOKS, lookOf_(settings), direction))
        } else if (id == "worldComfort") {
            Ride.setSetting("worldComfortMode", !settings["worldComfortMode"])
        } else if (id == "worldStats") {
            Ride.setSetting("worldDebugOverlay", !settings["worldDebugOverlay"])
        }
        if (riderChanged) {
            RiderProfiles.update(_rider)
            RiderProfiles.save()
            _rider = RiderProfiles.current
        } else {
            Ride.saveSettings()
        }
        refresh_()
    }

    // The retro-look preset the world flags match, or "custom".
    lookOf_(settings) {
        var nearest = settings["worldNearestUpscale"]
        var banded = settings["worldBandedFog"]
        var flat = settings["worldFlatShading"]
        var dither = settings["worldDither"]
        var scan = settings["worldScanlines"]
        if (!nearest && !banded && !flat && !dither && !scan) return "smooth"
        if (nearest && banded && flat && !dither && !scan) return "pixel"
        if (nearest && banded && flat && dither && !scan) return "pixelDither"
        if (nearest && banded && flat && !dither && scan) return "crt"
        return "custom"
    }

    applyLook_(look) {
        var pixel = look != "smooth"
        Ride.setSetting("worldNearestUpscale", pixel)
        Ride.setSetting("worldBandedFog", pixel)
        Ride.setSetting("worldFlatShading", pixel)
        Ride.setSetting("worldDither", look == "pixelDither")
        Ride.setSetting("worldScanlines", look == "crt")
    }

    onOff_(value) { Localization.get(value ? "ride.on" : "ride.off") }

    refresh_() {
        var settings = Ride.settings()
        var none = Localization.get("ride.setting_no_rider")
        var rider = _rider
        _rows["rider"].value = rider == null ? none : rider["name"]
        _rows["ftp"].value = rider == null ? "--" : rider["ftpWatts"].toString + " W"
        _rows["maxHr"].value = rider == null ? "--" : rider["maxHeartRateBpm"].toString + " bpm"
        _rows["thresholdHr"].value = rider == null ? "--" : rider["thresholdHeartRateBpm"].toString + " bpm"
        _rows["weight"].value = rider == null ? "--" : Units.formatWeight(rider["weightKg"], rider["units"])
        _rows["units"].value = rider == null ? "--" : Localization.get("menu.rider.units_" + rider["units"])
        _rows["zones"].value = Localization.get("ride.zones_" + settings["heartRateZoneModel"])
        _rows["graph"].value = (settings["graphWindowSeconds"] / 60).round.toString + " min"
        _rows["powerSource"].value = Localization.get("ride.power_source_" + settings["powerSource"])
        var autoPause = settings["autoPauseSeconds"]
        _rows["autoPause"].value = autoPause <= 0 ? Localization.get("ride.off") : autoPause.round.toString + " s"
        _rows["trimMode"].value = Localization.get("ride.trim_mode_" + settings["trimMode"])
        _rows["pair"].value = ">"
        _rows["back"].value = ""
        _rows["world"].value = onOff_(settings["worldEnabled"])
        _rows["worldQuality"].value = Localization.get("ride.world_quality_" + settings["worldQuality"])
        _rows["worldGradeSource"].value = Localization.get("ride.world_grade_" + settings["worldGradeSource"])
        var biome = settings["worldBiome"]
        _rows["worldBiome"].value = biome == "auto" ? Localization.get("ride.world_biome_auto") : biome
        _rows["worldSeed"].value = settings["worldSeed"].round.toString
        _rows["worldLook"].value = Localization.get("ride.world_look_" + lookOf_(settings))
        _rows["worldComfort"].value = onOff_(settings["worldComfortMode"])
        _rows["worldStats"].value = onOff_(settings["worldDebugOverlay"])
    }
}
