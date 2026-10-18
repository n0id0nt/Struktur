// Party/WarpRecovery.wren
// The default RECOVERY strategy for a party follower (GameObjects/PartyMember.wren): when the follower is stuck or
// has fallen too far from the member it trails, put it back on the leader's trail. Deliberately the dumbest thing
// that always works.
//
// EXTENSION POINT: a recovery strategy is any object with
//   recover(follower, leader) -> Bool   try to get the follower back with the party; true if it did something
//                                       (PartyMember resets its follow strategy afterwards either way)
// so a smarter one - path search around a closed door, waiting for it to open, then walking - can replace this
// (or run before it and fall back to a warp) without touching PartyMember or TrailFollow.
import "gameObjectComponents" for WorldTransform
import "math" for Vec3

class WarpRecovery {
    // behind: px behind the leader (along their trail) to put the follower.
    construct new(behind) {
        _behind = behind
    }

    recover(follower, leader) {
        if (leader.trail.isEmpty) {
            return false
        }
        var spot = leader.trail.pointBehind(_behind)
        WorldTransform.setPosition(follower.entity, Vec3.new(spot[0], spot[1], 0))
        return true
    }
}
