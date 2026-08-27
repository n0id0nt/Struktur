import "serialisation" for Json, MapUtil
import "fileSystem" for FileSystem

class Inventory {
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
        var convertedList = MapUtil.mapToPairs(map)
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