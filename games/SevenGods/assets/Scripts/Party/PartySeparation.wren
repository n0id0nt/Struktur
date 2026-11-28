// Party/PartySeparation.wren
// Soft steering that keeps a party follower from walking through the rest of the party. Actors pass through each
// other physically (CollisionLayers: Actor collides with Wall only - see Main.start()), so overlap avoidance is a
// velocity nudge, exactly like GameObjects/Steering/Separation.wren - but over the party members (players and
// followers) rather than the critter registry, with a stronger push around the member the follower is trailing so
// it stops short of the leader instead of bumping into them.
//
// Same protocol as every steering behaviour: steer(owner) -> Vec2 nudge (m/s) added to the owner's velocity.
// `owner` exposes position (Vec3, px) and member (its Party Member).
import "math" for Vec2
import "Party/Party" for Party

class PartySeparation {
    // radius/strength: the push on anyone closer than `radius` px (full `strength` m/s when overlapping, fading to
    // nothing at `radius`). leaderRadius/leaderStrength: the same around the member directly ahead.
    construct new(radius, strength, leaderRadius, leaderStrength) {
        _radius = radius
        _strength = strength
        _leaderRadius = leaderRadius
        _leaderStrength = leaderStrength
    }

    steer(owner) {
        var here = owner.position
        if (here == null) {
            return Vec2.new(0, 0)
        }
        var ahead = Party.ahead(owner.member)

        var push = Vec2.new(0, 0)
        for (other in Party.members) {
            if (other == owner.member) {
                continue
            }
            var there = Party.positionOf(other)
            if (there == null) {
                continue
            }
            var radius = _radius
            var strength = _strength
            if (other == ahead) {
                radius = _leaderRadius
                strength = _leaderStrength
            }
            var away = Vec2.new(here.x - there.x, here.y - there.y)
            var distance = away.length()
            if (distance > 0.001 && distance < radius) {
                push = push + (away.normalize() * ((1 - distance / radius) * strength))
            }
        }
        return push
    }
}
