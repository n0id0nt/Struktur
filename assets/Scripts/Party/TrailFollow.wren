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
//   - The goal is always `trail.total - distance`: the spot where the leader was `distance` px of walking ago.
//     The distance depends on the pace: `runDistance` while the pace matched is a run (faster than `runSpeed`),
//     `walkDistance` while it's a walk, and `restDistance` once the leader has stood still for `restDelay` seconds. A stationary leader records nothing, so the goal stops changing and the follower simply stops
//     once it gets there; when the leader moves again the goal slides back along the path and the follower
//     retargets on its own.
//   - Speed eases off near the goal (arrive) and rises a little when the follower is far behind (catch-up).
//   - Resting has hysteresis: it stops within a quarter of the arrive radius, but only sets off again once the
//     leader has moved (the goal shifted) or it has been nudged a whole arrive radius away - so a small push from
//     separation steering doesn't start a stop/start shuffle.
//   - A shorter distance (stopping, or slowing from a run to a walk) applies at once and the follower closes up.
//     A longer one opens up as the leader walks away: from a standstill the goal holds still until the gap is full
//     size, so the follower waits for them rather than stepping backwards; breaking from a walk into a run it opens
//     at OPEN_RATE of the leader's pace, and the follower moves at that same fraction of its own until it's open,
//     so it keeps step with its goal rather than halting or overshooting it.
//   - The path runs on past the trail's last recorded point to where the leader is right now, so the goal slides
//     along smoothly instead of hopping a trail step at a time (which a follower keeping pace would overshoot).
//   - The arrive slow-down and the stop only apply once the leader has stopped - near a goal that's still moving
//     the follower just keeps pace.
import "math" for Vec2
import "app" for Time
import "Party/Party" for Party

var MOVING_WINDOW = 0.15
var OPEN_RATE     = 0.5    // breaking into a run from a walk, the gap opens at this fraction of the leader's pace

class TrailFollow {
    // runDistance / walkDistance: px of path to stay behind a leader moving at a run (the follower's speed - the
    // pace it matches - above runSpeed m/s) / a walk. restDistance: px to close up to once the leader has stood
    // still for restDelay seconds. arriveRadius: px from the goal inside which it slows and finally stops.
    // waypointRadius: how close to the current waypoint counts as "reached" while catching up.
    // catchUp: speed multiplier reached when the gap to the goal is `catchUpGap` px or more.
    construct new(runDistance, walkDistance, restDistance, restDelay, runSpeed, arriveRadius, waypointRadius, catchUp,
                  catchUpGap) {
        _runDistance = runDistance
        _walkDistance = walkDistance
        _restDistance = restDistance
        _restDelay = restDelay
        _runSpeed = runSpeed
        _distance = walkDistance     // the gap currently kept - easing toward the run/walk/rest distance (updateDistance_)
        _lastTotal = null            // the leader's trail.total last tick, to tell whether they're moving
        _lastMoveTime = 0
        _opening = false             // the gap is opening up while the follower walks (updateDistance_)
        _arriveRadius = arriveRadius
        _waypointRadius = waypointRadius
        _catchUp = catchUp
        _catchUpGap = catchUpGap
        _s = null           // path-distance coordinate of the waypoint being walked toward
        _wantsMove = false
        _target = null
        _restGoal = null    // [x, y] goal the follower settled at, while resting
    }

    wantsToMove { _wantsMove }
    targetPosition { _target }

    reset(follower) {
        _s = null
        _wantsMove = false
        _target = null
        _restGoal = null
        _distance = _walkDistance
        _lastTotal = null
    }

    // True while the leader is on the move: their trail grew within the last MOVING_WINDOW seconds (a walking
    // leader adds a point every few hundredths of a second).
    leaderMoving_ { Time.scaledTime - _lastMoveTime < MOVING_WINDOW }

    // Keeps _distance in step with the leader's pace: running, walking (their trail grows) or standing still.
    updateDistance_(trail, follower) {
        var now = Time.scaledTime
        if (_lastTotal == null || trail.total < _lastTotal) {
            // First tick, or the trail was cleared: start from the walking gap.
            _distance = _walkDistance
            _lastTotal = trail.total
            _lastMoveTime = now
            return
        }
        _opening = false
        var grown = trail.total - _lastTotal
        _lastTotal = trail.total
        if (grown > 0) {
            _lastMoveTime = now
        }

        var wanted = follower.speed > _runSpeed ? _runDistance : _walkDistance
        if (grown == 0 && now - _lastMoveTime >= _restDelay) {
            wanted = _restDistance
        }
        if (_distance > wanted) {
            _distance = wanted
        } else {
            // Opens up by however far they went - so from a standstill the goal holds still until the gap is full
            // size - or, if the follower is already walking, by OPEN_RATE of that (see the header).
            _opening = _restGoal == null && _distance < wanted
            _distance = _distance + (_opening ? grown * OPEN_RATE : grown)
            if (_distance > wanted) {
                _distance = wanted
            }
        }
    }

    // trail.pointAt, carried on past the head toward the leader's live position (_live, _liveGap px beyond it).
    pointAt_(trail, s) {
        if (s <= trail.total || _liveGap < 0.001) {
            return trail.pointAt(s)
        }
        var t = (s - trail.total) / _liveGap
        if (t > 1) {
            t = 1
        }
        return [trail.headX + (_live.x - trail.headX) * t, trail.headY + (_live.y - trail.headY) * t]
    }

    desiredVelocity(follower, leader) {
        var trail = leader.trail
        var here = follower.position
        _wantsMove = false
        _target = null
        if (trail.isEmpty || here == null) {
            return Vec2.new(0, 0)
        }
        updateDistance_(trail, follower)

        // The leader's live position, as one more stretch of path past the trail's head (see pointAt_).
        _live = Party.positionOf(leader)
        _liveGap = 0
        if (_live != null) {
            var hx = _live.x - trail.headX
            var hy = _live.y - trail.headY
            _liveGap = (hx * hx + hy * hy).sqrt
        }
        var headS = trail.total + _liveGap

        // Not enough history yet to be `distance` behind anyone (the leader has barely moved since the follower
        // appeared): stay put rather than creeping up on them.
        if (headS - trail.oldest < _distance) {
            return Vec2.new(0, 0)
        }

        // Where the follower should end up, as a path distance - never behind the oldest point we still have.
        var goalS = headS - _distance
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
        var waypoint = pointAt_(trail, _s)
        var dx = waypoint[0] - here.x
        var dy = waypoint[1] - here.y
        var distance = (dx * dx + dy * dy).sqrt
        if (_s < goalS && distance <= _waypointRadius) {
            _s = _s + _waypointRadius
            if (_s > goalS) {
                _s = goalS
            }
            waypoint = pointAt_(trail, _s)
            dx = waypoint[0] - here.x
            dy = waypoint[1] - here.y
            distance = (dx * dx + dy * dy).sqrt
        }
        _target = waypoint

        // At the goal itself: rest.
        var goal = pointAt_(trail, goalS)
        var gx = goal[0] - here.x
        var gy = goal[1] - here.y
        var goalDistance = (gx * gx + gy * gy).sqrt
        if (_restGoal != null) {
            var moved = (goal[0] - _restGoal[0]).abs + (goal[1] - _restGoal[1]).abs
            if (moved < 0.5 && goalDistance <= _arriveRadius) {
                return Vec2.new(0, 0)
            }
            _restGoal = null
        }
        if (!leaderMoving_ && _s >= goalS && goalDistance <= _arriveRadius * 0.25) {
            _restGoal = goal
            return Vec2.new(0, 0)
        }
        if (distance < 0.001) {
            return Vec2.new(0, 0)
        }

        // Speed: full when far, a little extra when well behind, and easing off inside the arrive radius - but only
        // once the leader has stopped: easing toward a goal that's still moving away would just settle the
        // follower a steady arrive radius further back than its distance.
        var speed = follower.speed
        if (_opening) {
            speed = speed * OPEN_RATE
        }
        var gap = goalS - _s + goalDistance
        var boost = 1 + (_catchUp - 1) * (gap / _catchUpGap > 1 ? 1 : gap / _catchUpGap)
        speed = speed * boost
        if (!leaderMoving_ && _s >= goalS && goalDistance < _arriveRadius) {
            speed = speed * (goalDistance / _arriveRadius)
        }

        _wantsMove = true
        return Vec2.new(dx / distance * speed, dy / distance * speed)
    }
}
