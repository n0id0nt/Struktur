// Party/TrailFollow.wren
// The default FOLLOW STRATEGY for a party follower (GameObjects/PartyMember.wren): walk the route the member
// ahead actually took, staying `followDistance` pixels behind them along that route.
//
// A strategy is any object with this small protocol - swap or add one (a path-search follower, say) without
// touching PartyMember:
//   desiredVelocity(follower, leader) -> Vec2   velocity (m/s) the strategy wants this tick; (0,0) = rest
//   wantsToMove                                 true while the last call asked for movement (used by the
//                                               follower's stuck detector)
//   targetPosition                              the [x, y] pixel position it is currently heading for, or null
//   reset(follower)                             forget progress (called after a recovery warp)
// `follower` exposes position (Vec3, px) and speed (m/s); `leader` is the Party Member ahead (its `trail`).
//
// How it behaves:
//   - The follower holds a path-distance coordinate `s` along the leader's trail and walks to the point at `s`
//     (straight toward it - consecutive trail points are only a few pixels apart, so it hugs the route instead of
//     cutting corners), advancing `s` toward the head as it arrives.
//   - The goal is always `trail.total - followDistance`: the spot where the leader was `followDistance` px of
//     walking ago. A stationary leader records nothing, so the goal stops changing and the follower simply stops
//     once it gets there; when the leader moves again the goal slides back along the path and the follower
//     retargets on its own.
//   - Speed eases off near the goal (arrive) and rises a little when the follower is far behind (catch-up).
import "math" for Vec2

class TrailFollow {
    // followDistance: px of path to stay behind the leader. arriveRadius: px from the goal inside which it slows
    // and finally stops. waypointRadius: how close to the current waypoint counts as "reached" while catching up.
    // catchUp: speed multiplier reached when the gap to the goal is `catchUpGap` px or more.
    construct new(followDistance, arriveRadius, waypointRadius, catchUp, catchUpGap) {
        _followDistance = followDistance
        _arriveRadius = arriveRadius
        _waypointRadius = waypointRadius
        _catchUp = catchUp
        _catchUpGap = catchUpGap
        _s = null           // path-distance coordinate of the waypoint being walked toward
        _wantsMove = false
        _target = null
    }

    wantsToMove { _wantsMove }
    targetPosition { _target }

    reset(follower) {
        _s = null
        _wantsMove = false
        _target = null
    }

    desiredVelocity(follower, leader) {
        var trail = leader.trail
        var here = follower.position
        _wantsMove = false
        _target = null
        if (trail.isEmpty || here == null) {
            return Vec2.new(0, 0)
        }

        // Not enough history yet to be `followDistance` behind anyone (the leader has barely moved since the
        // follower appeared): stay put rather than creeping up on them.
        if (trail.total - trail.oldest < _followDistance) {
            return Vec2.new(0, 0)
        }

        // Where the follower should end up, as a path distance - never behind the oldest point we still have.
        var goalS = trail.total - _followDistance
        if (goalS < trail.oldest) {
            goalS = trail.oldest
        }
        if (_s == null || _s < trail.oldest) {
            _s = goalS
        }
        if (_s > goalS) {
            _s = goalS
        }

        // Reached the current waypoint? Advance toward the goal one step at a time so the follower walks the
        // route rather than aiming at the final goal directly.
        var waypoint = trail.pointAt(_s)
        var dx = waypoint[0] - here.x
        var dy = waypoint[1] - here.y
        var distance = (dx * dx + dy * dy).sqrt
        if (_s < goalS && distance <= _waypointRadius) {
            _s = _s + _waypointRadius
            if (_s > goalS) {
                _s = goalS
            }
            waypoint = trail.pointAt(_s)
            dx = waypoint[0] - here.x
            dy = waypoint[1] - here.y
            distance = (dx * dx + dy * dy).sqrt
        }
        _target = waypoint

        // At the goal itself: rest.
        var goal = trail.pointAt(goalS)
        var gx = goal[0] - here.x
        var gy = goal[1] - here.y
        var goalDistance = (gx * gx + gy * gy).sqrt
        if (_s >= goalS && goalDistance <= _arriveRadius * 0.25) {
            return Vec2.new(0, 0)
        }
        if (distance < 0.001) {
            return Vec2.new(0, 0)
        }

        // Speed: full when far, easing off inside the arrive radius, a little extra when well behind.
        var speed = follower.speed
        var gap = (trail.total - _followDistance) - _s + goalDistance
        var boost = 1 + (_catchUp - 1) * (gap / _catchUpGap > 1 ? 1 : gap / _catchUpGap)
        speed = speed * boost
        if (_s >= goalS && goalDistance < _arriveRadius) {
            speed = speed * (goalDistance / _arriveRadius)
        }

        _wantsMove = true
        return Vec2.new(dx / distance * speed, dy / distance * speed)
    }
}
