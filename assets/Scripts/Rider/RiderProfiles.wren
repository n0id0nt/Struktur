// Rider/RiderProfiles.wren
// The rider profiles: a name, a weight, a height and the player's preferred units (metric or imperial). Kept in
// riders.json in the save folder. Weight and height are always stored in metric (kg, cm) and only converted for
// display and editing, so switching units never drifts the stored values.
//
//   RiderProfiles.load()                  read riders.json (missing file = no riders yet)
//   RiderProfiles.profiles                list of profile maps, in creation order
//   RiderProfiles.create(profile) / update(profile) / remove(id), then save()
//   RiderProfiles.select(id) / current    the rider who is riding (remembered as "last ridden")
//
// A profile map: {"id": Num, "name": String, "weightKg": Num, "heightCm": Num, "units": "metric" | "imperial"}.
import "fileSystem" for FileSystem
import "serialisation" for Json, MapUtil
import "debug" for Debug

var FILE_NAME = "riders.json"
var MAX_PROFILES = 6
var MAX_NAME_LENGTH = 16

var KG_PER_POUND = 0.45359237
var CM_PER_INCH = 2.54

var MIN_WEIGHT_KG = 30
var MAX_WEIGHT_KG = 200
var MIN_HEIGHT_CM = 120
var MAX_HEIGHT_CM = 220
var METRIC_WEIGHT_STEP_KG = 0.5
var METRIC_HEIGHT_STEP_CM = 1

class RiderProfiles {
    static maxProfiles { MAX_PROFILES }
    static maxNameLength { MAX_NAME_LENGTH }

    static load() {
        __profiles = []
        __selectedId = null
        __nextId = 1
        var result = FileSystem.readString(FILE_NAME)
        if (!result.success) return
        var data = Json.parse(result.value)
        if (!(data is Map) || !(data["profiles"] is List)) {
            Debug.warning("riders.json is malformed - starting with no riders")
            return
        }
        for (entry in data["profiles"]) {
            if (entry is Map && entry["id"] is Num && entry["name"] is String) {
                __profiles.add(RiderProfiles.sanitised_(entry))
            }
        }
        __nextId = data["nextId"] is Num ? data["nextId"] : 1
        for (profile in __profiles) {
            if (profile["id"] >= __nextId) __nextId = profile["id"] + 1
        }
        __selectedId = RiderProfiles.find(data["selectedId"]) != null ? data["selectedId"] : null
    }

    static save() {
        var data = {}
        data["version"] = 1
        data["nextId"] = __nextId
        data["selectedId"] = __selectedId
        data["profiles"] = RiderProfiles.profiles
        // The engine's Json.stringify takes maps as MapUtil pairs (Json.parse gives real maps back), like every save.
        var result = FileSystem.writeString(FILE_NAME, Json.stringify(MapUtil.mapToPairs(data)))
        if (result.success) {
            FileSystem.syncSaves()
        } else {
            Debug.error("Could not write " + FILE_NAME)
        }
    }

    static profiles {
        if (__profiles == null) RiderProfiles.load()
        return __profiles
    }

    static isFull { RiderProfiles.profiles.count >= MAX_PROFILES }

    static find(id) {
        for (profile in RiderProfiles.profiles) {
            if (profile["id"] == id) return profile
        }
        return null
    }

    // A fresh, unsaved profile with sensible defaults (the editor fills in the rest).
    static newProfile() {
        var profile = {}
        profile["id"] = null
        profile["name"] = ""
        profile["weightKg"] = 75
        profile["heightCm"] = 175
        profile["units"] = "metric"
        return profile
    }

    // Adds a profile (its id is assigned here) and returns that id. Call save() after.
    static create(profile) {
        var stored = RiderProfiles.sanitised_(profile)
        stored["id"] = __nextId
        __nextId = __nextId + 1
        RiderProfiles.profiles.add(stored)
        return stored["id"]
    }

    static update(profile) {
        var list = RiderProfiles.profiles
        for (i in 0...list.count) {
            if (list[i]["id"] == profile["id"]) list[i] = RiderProfiles.sanitised_(profile)
        }
    }

    static remove(id) {
        var list = RiderProfiles.profiles
        var index = list.count - 1
        while (index >= 0) {
            if (list[index]["id"] == id) list.removeAt(index)
            index = index - 1
        }
        if (__selectedId == id) __selectedId = null
    }

    static select(id) {
        if (RiderProfiles.find(id) != null) __selectedId = id
    }

    static selectedId { __selectedId }
    // The rider who is riding, or null before one has been chosen.
    static current { __selectedId == null ? null : RiderProfiles.find(__selectedId) }

    // A copy with every field present, clamped and the name trimmed - what gets stored.
    static sanitised_(profile) {
        var stored = {}
        stored["id"] = profile["id"]
        var name = profile["name"] is String ? profile["name"].trim() : ""
        if (name.count > MAX_NAME_LENGTH) name = name[0...MAX_NAME_LENGTH]
        stored["name"] = name
        stored["weightKg"] = Units.clampWeightKg(profile["weightKg"] is Num ? profile["weightKg"] : 75)
        stored["heightCm"] = Units.clampHeightCm(profile["heightCm"] is Num ? profile["heightCm"] : 175)
        stored["units"] = profile["units"] == "imperial" ? "imperial" : "metric"
        return stored
    }
}

// Metric/imperial conversion, stepping and display. Inputs and outputs of the step functions are metric; the
// step itself happens in the profile's own units (0.5 kg / 1 lb, 1 cm / 1 inch) so every value the player can
// reach displays cleanly.
class Units {
    static clampWeightKg(kg) { kg.clamp(MIN_WEIGHT_KG, MAX_WEIGHT_KG) }
    static clampHeightCm(cm) { cm.clamp(MIN_HEIGHT_CM, MAX_HEIGHT_CM) }

    static poundsFromKg(kg) { (kg / KG_PER_POUND).round }
    static totalInchesFromCm(cm) { (cm / CM_PER_INCH).round }

    // direction: -1 or 1
    static stepWeightKg(kg, units, direction) {
        if (units == "imperial") {
            return Units.clampWeightKg((Units.poundsFromKg(kg) + direction) * KG_PER_POUND)
        }
        var steps = (kg / METRIC_WEIGHT_STEP_KG).round + direction
        return Units.clampWeightKg(steps * METRIC_WEIGHT_STEP_KG)
    }

    static stepHeightCm(cm, units, direction) {
        if (units == "imperial") {
            return Units.clampHeightCm((Units.totalInchesFromCm(cm) + direction) * CM_PER_INCH)
        }
        return Units.clampHeightCm(cm.round + direction * METRIC_HEIGHT_STEP_CM)
    }

    // "72.5 kg" / "160 lb"
    static formatWeight(kg, units) {
        if (units == "imperial") return Units.poundsFromKg(kg).toString + " lb"
        var halves = (kg / METRIC_WEIGHT_STEP_KG).round
        var whole = (halves / 2).floor
        return halves % 2 == 0 ? whole.toString + " kg" : whole.toString + ".5 kg"
    }

    // "178 cm" / "5' 10\""
    static formatHeight(cm, units) {
        if (units == "imperial") {
            var inches = Units.totalInchesFromCm(cm)
            return (inches / 12).floor.toString + "' " + (inches % 12).toString + "\""
        }
        return cm.round.toString + " cm"
    }

}
