// GameObjects/PartyMember.wren
// The world-side body of an NPC who has joined the party: a dynamic Actor-layer body that follows the member
// ahead of it in the party list (Party.wren) and otherwise stands about. Spawned/removed by
// States/ExperimentState.syncPartyFollowers() whenever the party changes - recruiting an NPC only edits the
// party list.
//
// Movement is three small pluggable pieces so the AI can grow without rewriting this script:
//   follow strategy   (Party/TrailFollow.wren)     walks the leader's trail, stays right behind, rests when they rest
//   steering          (Party/PartySeparation.wren) soft push so followers don't walk through the party
//   recovery strategy (Party/WarpRecovery.wren)    run when the follower is stuck or separated from the party -
//                                                  the place to plug in path search (e.g. around a moving door)
// See each file for the protocol it expects.
//
// Art: the NPC's generated sheet (GameObjects/NPC.wren's NPCData) - it walks / runs facing the way it moves (8
// directions) and idles facing that way when it stops.
import "gameObjectComponents" for PhysicsBody, SpriteAnimation, WorldTransform, Script
import "math" for Vec2, Vec3
import "physics" for BodyDefinition, PhysicsCircleShape, BodyType, CollisionLayers
import "app" for Time
import "debug" for Debug
import "Party" for Party
import "Party/TrailFollow" for TrailFollow
import "Party/PartySeparation" for PartySeparation
import "Party/WarpRecovery" for WarpRecovery
import "GameObjects/NPC" for NPCData
import "Sprites/SpriteSheet" for SpriteSheet

// --- tuning (pixels unless noted; speeds in m/s like Player.speed) ---
var DEFAULT_SPEED     = 3.0    // used when no player is around to match
var FOLLOW_DISTANCE   = 30     // path distance kept behind the member ahead
var ARRIVE_RADIUS     = 14     // eases off and stops inside this of the goal
var WAYPOINT_RADIUS   = 8      // how close counts as "reached" while walking the trail
var CATCH_UP          = 1.5    // speed multiplier when far behind
var CATCH_UP_GAP      = 120    // gap at which the full catch-up boost applies
var SEPARATION_RADIUS = 20     // keep-out around any other party member
var SEPARATION_PUSH   = 1.4
var LEADER_RADIUS     = 24     // keep-out around the member being followed
var LEADER_PUSH       = 2.4
var STUCK_CHECK_TIME  = 1.2    // seconds between "did we actually move?" checks while trying to move
var STUCK_MIN_MOVE    = 6      // px that must be covered per check, or the follower counts as stuck
var MAX_SEPARATION    = 420    // farther than this from the member ahead counts as separated
var RECOVERY_BEHIND   = 40     // px behind the leader (along their trail) a recovery warp puts the follower
var MOVING_SPEED      = 0.3    // m/s - slower than this plays the idle clip
var RUN_SPEED         = 1.8    // m/s - faster than this runs, otherwise walks

class PartyMember {
    construct new(entity, args) {
        _entity = entity
        _name = args["Name"]
        _strategy = TrailFollow.new(FOLLOW_DISTANCE, ARRIVE_RADIUS, WAYPOINT_RADIUS, CATCH_UP, CATCH_UP_GAP)
        _recovery = WarpRecovery.new(RECOVERY_BEHIND)
        _steering = [PartySeparation.new(SEPARATION_RADIUS, SEPARATION_PUSH, LEADER_RADIUS, LEADER_PUSH)]
        _checkTime = null
        _checkPosition = null
        _facing = Vec2.new(0, 1)
    }

    name { _name }
    entity { _entity }
    position { WorldTransform.getPosition(_entity) }

    // This follower's slot in the party list (null if it has been removed from the party).
    member { Party.member(_name) }

    // Speed to move at (m/s) - matches the first player's, so a follower keeps pace with whoever it trails.
    speed {
        for (m in Party.members) {
            if (m.isPlayer && m.entity != null) {
                var script = Script.getInstance(m.entity)
                if (script != null) {
                    return script.speed
                }
            }
        }
        return DEFAULT_SPEED
    }

    // Swap the follow strategy (anything with the protocol in Party/TrailFollow.wren).
    strategy=(value) { _strategy = value }
    // Swap the stuck/separated recovery strategy (protocol in Party/WarpRecovery.wren).
    recovery=(value) { _recovery = value }
    // Add a steering behaviour (protocol in Party/PartySeparation.wren).
    addSteering(behaviour) { _steering.add(behaviour) }

    start() {
        var data = NPCData.forName(_name)
        Debug.assertMsg(data != null, "No sprite data for party member %(_name)")

        data.createSprite(_entity)

        var body = PhysicsBody.create(_entity, BodyDefinition.new(BodyType.DYNAMIC_BODY), PhysicsCircleShape.new(0.2))
        body.fixedRotation = true
        // Collide with the level (Wall bit), pass through other actors - separation is steering (see Main.start()).
        body.setCollisionFilter(CollisionLayers.getLayer("Actor"), CollisionLayers.getLayer("Wall"))
    }

    fixedUpdate() {
        var me = member
        var leader = me == null ? null : Party.ahead(me)
        var here = position
        if (leader == null || here == null) {
            PhysicsBody.setLinearVelocity(_entity, Vec2.new(0, 0))
            animate_(Vec2.new(0, 0))
            return
        }

        var velocity = _strategy.desiredVelocity(this, leader)
        for (behaviour in _steering) {
            velocity = velocity + behaviour.steer(this)
        }
        PhysicsBody.setLinearVelocity(_entity, velocity)

        animate_(velocity)

        if (isStuckOrSeparated_(here, leader)) {
            Debug.info("[Party] %(_name) is stuck/separated - recovering")
            _recovery.recover(this, leader)
            _strategy.reset(this)
            _checkTime = null
        }
    }

    // Walk / run toward where it is heading, or idle facing the last way it went.
    animate_(velocity) {
        var speed = velocity.length()
        var clip = "Idle"
        if (speed > MOVING_SPEED) {
            _facing = velocity
            clip = speed > RUN_SPEED ? "Run" : "Walk"
        }
        SpriteAnimation.setCurrentAnimation(_entity, SpriteSheet.clipName(_facing, clip))
    }

    // True when the follower keeps trying to move but isn't getting anywhere (wedged on level geometry,
    // say), or has simply fallen too far from the member it trails.
    isStuckOrSeparated_(here, leader) {
        var leaderPosition = Party.positionOf(leader)
        if (leaderPosition != null && Vec3.distance(here, leaderPosition) > MAX_SEPARATION) {
            return true
        }
        if (!_strategy.wantsToMove) {
            _checkTime = null
            return false
        }
        var now = Time.scaledTime
        if (_checkTime == null) {
            _checkTime = now
            _checkPosition = here
            return false
        }
        if (now - _checkTime < STUCK_CHECK_TIME) {
            return false
        }
        var moved = Vec3.distance(here, _checkPosition)
        _checkTime = now
        _checkPosition = here
        return moved < STUCK_MIN_MOVE
    }

    update() {}
    onDestroy() {}
    onEvent(type, data) {}
}
