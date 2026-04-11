import "serialisation" for Json
import "fileSystem" for FileSystem

class Inventory {
    static mapToPairs(value) {
        if (value is Map) {
            var pairs = [["__type", "map"]]
            for (key in value.keys) {
                pairs.add([key, mapToPairs(value[key])])  // recurse
            }
            return pairs
        } else if (value is List) {
            return value.map { |elem| mapToPairs(elem) }.toList  // recurse into lists too
        } else {
            return value  // primitive, pass through
        }
    }

    static pairsToMap(value) {
        if (value is List) {
            // Check for the sentinel
            if (value.count > 0 && value[0] is List && value[0][0] == "__type" && value[0][1] == "map") {
                var map = {}
                for (i in 1...value.count) {
                    var pair = value[i]
                    map[pair[0]] = pairsToMap(pair[1])  // recurse on values
                }
                return map
            } else {
                // Plain list — recurse into elements
                return value.map { |elem| pairsToMap(elem) }.toList
            }
        } else {
            return value  // primitive
        }
    }

    static items { __items }

    static init() {
        __items = []
    }

    static contains(item)   { __items.contains(item) }
    static getItems()       { __items }

    static addItem(item)    { __items.add(item) }
    static removeItem(item) { __items.remove(item) }

    // --- Serialisation ---

    static serialise() {
        var map = { "items": __items }
        var convertedList = mapToPairs(map)
        var json = Json.stringify(convertedList)
        return json
    }

    // Restores state from a Map produced by Json.parse()
    static parse(json) {
        var data = Json.parse(json)
        __items = []
        var loaded = data["items"]
        if (loaded != null) {
            for (item in loaded) {
                __items.add(item)
            }
        }
    }

    // Convenience: persist straight to a file path (PhysicsFS write path)
    static save(path) {
        var data = serialise()
        var result = FileSystem.writeString(path, data)
        if (result.success) {
            FileSystem.syncSaves()
        }
    }

    // Convenience: restore straight from a file path
    static load(path) {
        var result = FileSystem.readString(path)
        if (result.success) {
            parse(result.value)
        }
    }
}