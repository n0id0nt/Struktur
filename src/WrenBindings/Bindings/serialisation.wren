// AUTO-GENERATED FILE - DO NOT EDIT
// Generated from C++ bindings
// Module: serialisation

class Json {
    // Takes a json file as a string and returns a wren map or list
    foreign static parse(arg0)
    // Takes a wren map or list and returns a json file as a string
    foreign static stringify(arg0)
}

class MapUtil {
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
                // Plain list - recurse into elements
                return value.map { |elem| pairsToMap(elem) }.toList
            }
        } else {
            return value  // primitive
        }
    }
    static toString(value) {
        return toStringIndented(value, 0)
    }
    static toStringIndented(value, depth) {
        var indent = "  " * depth
        var innerIndent = "  " * (depth + 1)
        var result = ""
        if (value == null) {
            result = "%(indent)null"
        } else if (value is Map) {
            result = "%(indent){\n"
            for (entry in value) {
                if (entry.value is Map || entry.value is List) {
                    result = result + "%(innerIndent)%(entry.key):\n"
                    result = result + toStringIndented(entry.value, depth + 1) + "\n"
                } else {
                    result = result + "%(innerIndent)%(entry.key): %(entry.value)\n"
                }
            }
            result = result + "%(indent)}"
        } else if (value is List) {
            result = "%(indent)[\n"
            for (item in value) {
                if (item is Map || item is List) {
                    result = result + toStringIndented(item, depth + 1) + "\n"
                } else {
                    result = result + "%(innerIndent)%(item)\n"
                }
            }
            result = result + "%(indent)]"
        } else {
            result = "%(indent)%(value)"
        }
        return result
    }
}

