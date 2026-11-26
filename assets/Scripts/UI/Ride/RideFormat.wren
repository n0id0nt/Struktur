// UI/Ride/RideFormat.wren
// Number formatting for the ride screens. Values arrive in SI units (m/s, m) and are shown in the rider's units.
var KMH_PER_MPS = 3.6
var MPH_PER_MPS = 2.2369363
var FEET_PER_METER = 3.2808399
var METERS_PER_MILE = 1609.344

class RideFormat {
    // 75 -> "1:15", 3725 -> "1:02:05"
    static duration(seconds) {
        if (seconds == null) return "--:--"
        var total = seconds.floor
        if (total < 0) total = 0
        var hours = (total / 3600).floor
        var minutes = ((total % 3600) / 60).floor
        var secs = total % 60
        var secsText = secs < 10 ? "0" + secs.toString : secs.toString
        if (hours > 0) {
            var minutesText = minutes < 10 ? "0" + minutes.toString : minutes.toString
            return hours.toString + ":" + minutesText + ":" + secsText
        }
        return minutes.toString + ":" + secsText
    }

    static whole(value) { value == null ? "--" : value.round.toString }

    static oneDecimal(value) {
        if (value == null) return "--"
        var tenths = (value * 10).round
        var sign = tenths < 0 ? "-" : ""
        tenths = tenths.abs
        return sign + (tenths / 10).floor.toString + "." + (tenths % 10).toString
    }

    static speed(metersPerSecond, imperial) {
        return RideFormat.oneDecimal(metersPerSecond * (imperial ? MPH_PER_MPS : KMH_PER_MPS))
    }
    static speedUnit(imperial) { imperial ? "mph" : "km/h" }

    static distance(meters, imperial) {
        return RideFormat.oneDecimal(imperial ? meters / METERS_PER_MILE : meters / 1000)
    }
    static distanceUnit(imperial) { imperial ? "mi" : "km" }

    static elevation(meters, imperial) { (imperial ? meters * FEET_PER_METER : meters).round.toString }
    static elevationUnit(imperial) { imperial ? "ft" : "m" }

    static grade(percent) { RideFormat.oneDecimal(percent) + "\%" }

    static signedWhole(value) { value >= 0 ? "+" + value.round.toString : value.round.toString }
}
