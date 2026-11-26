// UI/Ride/RideLayout.wren
// The ride HUD's layouts, from assets/Settings/RideLayout.json. A layout is a set of panels; each panel has a place
// (anchor / position / size in the 1280x720 layout space - anchor is both where on the screen it attaches and which of
// its own points sits there), a size class ("L", "M" or "S": how big its text and buttons are) and what it shows:
//
//   "widget": "tiles"    a row ("flow": "row") or column ("flow": "column") of stat tiles - "tiles": [ids...]
//                        ids: power, averagePower, cadence, heartRate, speed, averageSpeed, grade, climbed, target,
//                        inTarget, distance, elapsed, lapTime
//   "widget": "workout"  workout: name, step, countdown, step target, next step, profile (Free Ride: title, time, lap,
//                        distance)
//   "widget": "erg"      ERG badge, resistance -/+ (trim or grade), Free Ride target / workout skip and +30 s
//   "widget": "graph"    power and heart rate history
//   "widget": "status"   sensor connection, End ride
//   "widget": "hints"    keyboard keys
//   "widget": "cue"      workout text cues (shown when one is due)
//   "widget": "banner"   start / paused / complete (shown when needed)
//
// Panels that fail to parse are skipped with a warning; a layout without panels falls back to the built-in compact
// layout, so a bad edit can't leave the screen empty. Switch layouts in a ride with H, or in Ride Settings.
import "fileSystem" for FileSystem
import "serialisation" for Json
import "math" for Vec2
import "debug" for Debug

var LAYOUT_PATH = "Settings/RideLayout.json"
var WIDGETS = ["tiles", "workout", "erg", "graph", "status", "hints", "cue", "banner"]
var SIZE_CLASSES = ["L", "M", "S"]

class RidePanelSpec {
    construct new(id, widget, anchor, position, size, sizeClass, tiles, flow) {
        _id = id
        _widget = widget
        _anchor = anchor
        _position = position
        _size = size
        _sizeClass = sizeClass
        _tiles = tiles
        _flow = flow
    }
    id { _id }
    widget { _widget }
    anchor { _anchor }
    position { _position }
    size { _size }
    sizeClass { _sizeClass }
    tiles { _tiles }
    flow { _flow }
}

class RideLayout {
    // Names of the layouts in the file, in file order (the built-in compact one when the file is unusable).
    static names {
        var data = RideLayout.file_
        if (data == null) return ["compact"]
        var names = []
        for (name in data["order"]) names.add(name)
        return names
    }

    // The file's default layout name.
    static defaultName {
        var data = RideLayout.file_
        if (data == null) return "compact"
        return data["default"]
    }

    // List of RidePanelSpec for a layout name (unknown name: the default layout).
    static load(name) {
        var data = RideLayout.file_
        if (data == null) return RideLayout.builtIn_
        var layouts = data["layouts"]
        var layout = layouts.containsKey(name) ? layouts[name] : layouts[data["default"]]
        var specs = []
        if (layout is Map && layout["panels"] is List) {
            for (entry in layout["panels"]) {
                var spec = RideLayout.parse_(entry)
                if (spec == null) {
                    Debug.warning("RideLayout.json: a panel in '%(name)' is malformed - skipped")
                } else {
                    specs.add(spec)
                }
            }
        }
        if (specs.isEmpty) {
            Debug.warning("RideLayout.json: layout '%(name)' has no usable panels - using the built-in layout")
            return RideLayout.builtIn_
        }
        return specs
    }

    // Parsed file: {"default": name, "order": [names], "layouts": {name: layout}} or null.
    static file_ {
        var result = FileSystem.readString(LAYOUT_PATH)
        if (!result.success) return null
        var data = Json.parse(result.value)
        if (!(data is Map) || !(data["layouts"] is Map) || data["layouts"].count == 0) {
            Debug.warning("RideLayout.json has no layouts map - using the built-in layout")
            return null
        }
        var order = []
        if (data["order"] is List) {
            for (name in data["order"]) {
                if (name is String && data["layouts"].containsKey(name)) order.add(name)
            }
        }
        for (name in data["layouts"].keys) {
            if (!order.contains(name)) order.add(name)
        }
        var fallback = order[0]
        var defaultName = data["default"] is String && data["layouts"].containsKey(data["default"]) ? data["default"] : fallback
        var parsed = {}
        parsed["default"] = defaultName
        parsed["order"] = order
        parsed["layouts"] = data["layouts"]
        return parsed
    }

    static parse_(entry) {
        if (!(entry is Map)) return null
        var widget = entry["widget"]
        if (!(widget is String) || !WIDGETS.contains(widget)) return null
        var anchor = RideLayout.pair_(entry["anchor"])
        var position = RideLayout.pair_(entry["position"])
        var size = RideLayout.pair_(entry["size"])
        if (anchor == null || position == null || size == null || size.x <= 0 || size.y <= 0) return null
        var sizeClass = entry["sizeClass"] is String && SIZE_CLASSES.contains(entry["sizeClass"]) ? entry["sizeClass"] : "M"
        var tiles = []
        if (entry["tiles"] is List) {
            for (tile in entry["tiles"]) {
                if (tile is String) tiles.add(tile)
            }
        }
        if (widget == "tiles" && tiles.isEmpty) return null
        var flow = entry["flow"] == "column" ? "column" : "row"
        var id = entry["id"] is String ? entry["id"] : widget
        return RidePanelSpec.new(id, widget, anchor, position, size, sizeClass, tiles, flow)
    }

    static pair_(value) {
        if (!(value is List) || value.count != 2 || !(value[0] is Num) || !(value[1] is Num)) return null
        return Vec2.new(value[0], value[1])
    }

    // Used when the file is missing or unusable: the compact layout.
    static builtIn_ {
        var specs = []
        specs.add(RidePanelSpec.new("top", "tiles", Vec2.new(0.5, 0), Vec2.new(0, 8), Vec2.new(780, 74), "M",
            ["power", "cadence", "heartRate", "speed", "grade", "target"], "row"))
        specs.add(RidePanelSpec.new("workout", "workout", Vec2.new(0, 1), Vec2.new(8, -36), Vec2.new(300, 150), "S", [], "row"))
        specs.add(RidePanelSpec.new("erg", "erg", Vec2.new(1, 1), Vec2.new(-8, -36), Vec2.new(300, 118), "S", [], "row"))
        specs.add(RidePanelSpec.new("graph", "graph", Vec2.new(0.5, 1), Vec2.new(0, -36), Vec2.new(440, 96), "S", [], "row"))
        specs.add(RidePanelSpec.new("status", "status", Vec2.new(0, 1), Vec2.new(8, -6), Vec2.new(560, 24), "S", [], "row"))
        specs.add(RidePanelSpec.new("hints", "hints", Vec2.new(1, 1), Vec2.new(-8, -6), Vec2.new(680, 24), "S", [], "row"))
        specs.add(RidePanelSpec.new("cue", "cue", Vec2.new(0.5, 0), Vec2.new(0, 92), Vec2.new(560, 40), "M", [], "row"))
        specs.add(RidePanelSpec.new("banner", "banner", Vec2.new(0.5, 0.5), Vec2.new(0, -150), Vec2.new(520, 50), "M", [], "row"))
        return specs
    }
}
