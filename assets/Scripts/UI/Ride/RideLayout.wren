// UI/Ride/RideLayout.wren
// Loads the ride screen's panel layout from assets/Settings/RideLayout.json (anchor / position / size per panel, in the
// 1280x720 layout space). Any panel missing or malformed in the file falls back to the built-in default below, so a bad
// edit can move a panel but never break the screen.
import "fileSystem" for FileSystem
import "serialisation" for Json
import "math" for Vec2
import "debug" for Debug

var LAYOUT_PATH = "Settings/RideLayout.json"

class RidePanelSpec {
    construct new(anchor, position, size) {
        _anchor = anchor
        _position = position
        _size = size
    }
    anchor { _anchor }
    position { _position }
    size { _size }
}

class RideLayout {
    static defaults_ {
        var specs = {}
        specs["workout"] = RidePanelSpec.new(Vec2.new(0, 0), Vec2.new(16, 16), Vec2.new(330, 262))
        specs["stats"] = RidePanelSpec.new(Vec2.new(0.5, 0), Vec2.new(0, 16), Vec2.new(540, 210))
        specs["grade"] = RidePanelSpec.new(Vec2.new(1, 0), Vec2.new(-16, 16), Vec2.new(300, 76))
        specs["target"] = RidePanelSpec.new(Vec2.new(1, 0), Vec2.new(-16, 100), Vec2.new(300, 76))
        specs["erg"] = RidePanelSpec.new(Vec2.new(1, 0), Vec2.new(-16, 184), Vec2.new(300, 142))
        specs["cue"] = RidePanelSpec.new(Vec2.new(0.5, 0), Vec2.new(0, 236), Vec2.new(620, 44))
        specs["banner"] = RidePanelSpec.new(Vec2.new(0.5, 0.5), Vec2.new(0, -40), Vec2.new(560, 70))
        specs["graph"] = RidePanelSpec.new(Vec2.new(0.5, 1), Vec2.new(0, -46), Vec2.new(1248, 300))
        specs["status"] = RidePanelSpec.new(Vec2.new(0, 1), Vec2.new(16, -10), Vec2.new(520, 28))
        specs["hints"] = RidePanelSpec.new(Vec2.new(1, 1), Vec2.new(-16, -10), Vec2.new(700, 28))
        return specs
    }

    // Map of panel id -> RidePanelSpec.
    static load() {
        var specs = RideLayout.defaults_
        var result = FileSystem.readString(LAYOUT_PATH)
        if (!result.success) return specs
        var data = Json.parse(result.value)
        if (!(data is Map) || !(data["panels"] is Map)) {
            Debug.warning("RideLayout.json has no panels map - using the default layout")
            return specs
        }
        for (id in data["panels"].keys) {
            var spec = RideLayout.parse_(data["panels"][id])
            if (spec == null) {
                Debug.warning("RideLayout.json: panel '" + id + "' is malformed - keeping its default")
            } else {
                specs[id] = spec
            }
        }
        return specs
    }

    static parse_(entry) {
        if (!(entry is Map)) return null
        var anchor = RideLayout.pair_(entry["anchor"])
        var position = RideLayout.pair_(entry["position"])
        var size = RideLayout.pair_(entry["size"])
        if (anchor == null || position == null || size == null || size.x <= 0 || size.y <= 0) return null
        return RidePanelSpec.new(anchor, position, size)
    }

    static pair_(value) {
        if (!(value is List) || value.count != 2 || !(value[0] is Num) || !(value[1] is Num)) return null
        return Vec2.new(value[0], value[1])
    }
}
