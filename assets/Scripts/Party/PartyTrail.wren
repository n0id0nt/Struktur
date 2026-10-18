// Party/PartyTrail.wren
// A breadcrumb path of where one party member has recently been, used by whoever follows them (see
// Party/TrailFollow.wren). A point is only added once the member has moved more than `minStep` pixels from the
// last one - so a stationary leader adds nothing and the trail (and so any follower's target) stops changing -
// and every point stores its cumulative path distance, so a position can be looked up by "how far along the
// path" (`pointAt`) rather than by index. That distance coordinate only ever grows, which is what lets a
// follower hold a stable "I am heading for path distance s" while old points are trimmed off the tail.
class PartyTrail {
    // minStep: pixels the member must move before a new point is recorded.
    // maxLength: pixels of path kept behind the head (anything older is dropped).
    construct new(minStep, maxLength) {
        _minStep = minStep
        _maxLength = maxLength
        _points = []   // each [x, y, pathDistance], oldest first, newest (the head) last
        _total = 0     // path distance of the head
    }

    isEmpty { _points.count == 0 }

    // Path distance of the newest point - grows as the member moves, never shrinks.
    total { _total }

    // Path distance of the oldest point still kept.
    oldest { _points.count == 0 ? 0 : _points[0][2] }

    headX { _points[_points.count - 1][0] }
    headY { _points[_points.count - 1][1] }

    clear() {
        _points = []
        _total = 0
    }

    // Records the member's current position (call every fixed step).
    record(x, y) {
        if (_points.count == 0) {
            _points.add([x, y, _total])
            return
        }
        var head = _points[_points.count - 1]
        var dx = x - head[0]
        var dy = y - head[1]
        var step = (dx * dx + dy * dy).sqrt
        if (step < _minStep) {
            return
        }
        _total = _total + step
        _points.add([x, y, _total])

        // Drop points that are further than maxLength behind the head, but always keep two so there is
        // still a segment to interpolate along.
        while (_points.count > 2 && _total - _points[1][2] > _maxLength) {
            _points.removeAt(0)
        }
    }

    // The [x, y] position on the path at path distance `s` (clamped to the oldest point / the head).
    pointAt(s) {
        if (_points.count == 0) {
            return null
        }
        if (s <= _points[0][2]) {
            return [_points[0][0], _points[0][1]]
        }
        if (s >= _total) {
            return [headX, headY]
        }
        var i = _points.count - 1
        while (i > 0 && _points[i - 1][2] > s) {
            i = i - 1
        }
        var a = _points[i - 1]
        var b = _points[i]
        var span = b[2] - a[2]
        var t = span > 0 ? (s - a[2]) / span : 0
        return [a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t]
    }

    // The [x, y] position `distance` pixels back along the path from the head.
    pointBehind(distance) {
        return pointAt(_total - distance)
    }
}
