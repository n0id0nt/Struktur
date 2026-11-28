// GameObjects/Steering/Separation.wren
// Boid-style separation: nudges the owner away from every other registered critter (see
// GameObjects/Critter.wren's static `all` registry) that's closer than `radius`. Registered via
// Critter.addSteering() and summed on top of whatever velocity the active state wants - see
// Critter.tickStates() for how the two combine.
//
// This exists because CollisionLayers keeps critters (and the player) from physically colliding
// with each other at all (see Main.start()) - deliberately, so nothing shoves a critter's slow
// physics body around - but that also means nothing stops them overlapping. A soft steering push
// here reads far more natural than turning physical collision back on would (a stiff Box2D response
// between differently-sized, differently-massed circles), and unlike a real collision it only ever
// separates critters from each other, never from the player.
import "math" for Vec2
import "GameObjects/Critter" for Critter

class Separation {
    // radius - how close (world pixels) another critter needs to be before it's pushed away.
    // strength - top push speed (m/s, the same unit Critter.velocity is in) when two critters are
    // right on top of each other; falls off linearly to 0 as they reach `radius` apart.
    construct new(radius, strength) {
        _radius = radius
        _strength = strength
    }

    // Returns a Vec2 nudge to add to the owner's velocity this tick - zero if nothing registered is
    // close enough to matter. Distances are measured in world pixels (Critter.position), but the
    // result only ever depends on *direction* to each neighbour, so it's fine to add straight onto a
    // metres/sec velocity - see Critter.velocity's own comment.
    steer(owner) {
        var here = owner.position
        if (!here) {
            return Vec2.new(0, 0)
        }

        var push = Vec2.new(0, 0)
        for (other in Critter.all) {
            if (other == owner) {
                continue
            }
            var there = other.position
            if (!there) {
                continue
            }
            var away = Vec2.new(here.x - there.x, here.y - there.y)
            var distance = away.length()
            if (distance > 0.001 && distance < _radius) {
                var weight = (1 - distance / _radius) * _strength
                push = push + (away.normalize() * weight)
            }
        }
        return push
    }
}
