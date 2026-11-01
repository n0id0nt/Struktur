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
// Staying put: while standing on a pressure plate a follower can be talked to and asked to stay there (holding the
// plate down); talked to again, it follows once more (Member.staying, set by the "partyStay"/"partyFollow" dialogue
// commands - nodes "<npc>_party_stay" / "<npc>_party_follow"). Anyone behind it in the party follows the member
// ahead of it meanwhile (Party.ahead).
//
// Art: the NPC's generated sheet (GameObjects/NPC.wren's NPCData) - it walks / runs facing the way it moves (8
// directions) and idles facing that way when it stops.
import "gameObjectComponents" for PhysicsBody, Sprite, SpriteAnimation, WorldTransform, Script
import "math" for Vec2, Vec3, Vec4
import "physics" for BodyDefinition, PhysicsCircleShape, BodyType, CollisionLayers
import "app" for Time
import "debug" for Debug
import "Party" for Party
import "Party/TrailFollow" for TrailFollow
import "Party/PartySeparation" for PartySeparation
import "Party/WarpRecovery" for WarpRecovery
import "GameObjects/NPC" for NPCData
import "Dialogue/DialogueEntryPoints" for DialogueEntryPoints
import "PressurePlates" for PressurePlates
import "Sprites/SpriteSheet" for SpriteSheet

// --- tuning (pixels unless noted; speeds in m/s like Player.speed) ---
var DEFAULT_SPEED     = 3.0    // used when no player is around to match
// Path distance kept behind the member ahead, by pace:
var RUN_DISTANCE      = 30     // while running (the pace matched is faster than RUN_SPEED)
var WALK_DISTANCE     = 22     // while walking
var REST_DISTANCE     = 18     // once they've stopped...
var REST_DELAY        = 0.3    // ...for this many seconds
var ARRIVE_RADIUS     = 14     // eases off and stops inside this of the goal
var WAYPOINT_RADIUS   = 8      // how close counts as "reached" while walking the trail
var CATCH_UP          = 1.5    // speed multiplier when far behind
var CATCH_UP_GAP      = 40     // px behind the goal at which the full catch-up boost applies
var SEPARATION_RADIUS = 20     // keep-out around any other party member
var SEPARATION_PUSH   = 1.4
var LEADER_RADIUS     = 16     // keep-out around the member being followed (inside REST_DISTANCE, or it pushes back)
var LEADER_PUSH       = 2.4
var STUCK_CHECK_TIME  = 1.2    // seconds between "did we actually move?" checks while trying to move
var STUCK_MIN_MOVE    = 6      // px that must be covered per check, or the follower counts as stuck
var MAX_SEPARATION    = 420    // farther than this from the member ahead counts as separated
var RECOVERY_BEHIND   = 40     // px behind the leader (along their trail) a recovery warp puts the follower
var VELOCITY_BLEND    = 0.25   // fraction of the way to the wanted velocity covered each fixed step (smooths turns)
var STOP_SPEED        = 0.05   // m/s - a smoothed velocity this slow, with nothing wanted, snaps to a stop
var MOVING_SPEED      = 0.3    // m/s - faster than this starts the walk/run clip...
var IDLE_SPEED        = 0.15   // m/s - ...and only slower than this goes back to idle
var RUN_SPEED         = 1.8    // m/s - runs when the pace it matches is faster than this (the player is running)
var CATCH_UP_RUN      = 2.6    // m/s - or when catching up this fast
var FACING_TURN       = 0.9    // cos of the angle the heading must swing past before the facing changes (~25 deg)
var DOWNED_TINT       = Vec4.new(120, 120, 130, 255)   // a downed member (Member.downed) - greyed out until rested
var NORMAL_TINT       = Vec4.new(255, 255, 255, 255)

class PartyMember {
    construct new(entity, args) {
        _entity = entity
        _name = args["Name"]
        _strategy = TrailFollow.new(RUN_DISTANCE, WALK_DISTANCE, REST_DISTANCE, REST_DELAY, RUN_SPEED,
                                    ARRIVE_RADIUS, WAYPOINT_RADIUS, CATCH_UP, CATCH_UP_GAP)
        _recovery = WarpRecovery.new(RECOVERY_BEHIND)
        _steering = [PartySeparation.new(SEPARATION_RADIUS, SEPARATION_PUSH, LEADER_RADIUS, LEADER_PUSH)]
        _checkTime = null
        _checkPosition = null
        _facing = Vec2.new(0, 1)
        _velocity = Vec2.new(0, 0)   // smoothed velocity actually applied to the body
        _moving = false              // walk/run clip showing (vs idle) - see animate_
        _tintedDowned = false
        _leader = null               // who it followed last step - a change restarts the follow strategy
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
        // Trigger: stands on pressure plates like the player does.
        body.setCollisionFilter(CollisionLayers.getLayer("Actor"),
                                CollisionLayers.getLayer("Wall") | CollisionLayers.getLayer("Trigger"))
    }

    fixedUpdate() {
        var me = member
        refreshTint_(me)
        var leader = me == null || me.staying ? null : Party.ahead(me)
        if (leader != _leader) {
            // Told to stay or to follow again, or the member ahead started/stopped staying: walk the new
            // leader's trail from scratch.
            _leader = leader
            _strategy.reset(this)
            _checkTime = null
        }
        var here = position
        if (leader == null || here == null) {
            _velocity = Vec2.new(0, 0)
            PhysicsBody.setLinearVelocity(_entity, _velocity)
            animate_(_velocity)
            return
        }

        var wanted = _strategy.desiredVelocity(this, leader)
        for (behaviour in _steering) {
            wanted = wanted + behaviour.steer(this)
        }
        // Ease toward the wanted velocity rather than jumping to it, so the follow pull and the separation push
        // (and each new trail waypoint) blend instead of flipping the direction from one step to the next.
        _velocity = _velocity + (wanted - _velocity) * VELOCITY_BLEND
        if (wanted.length() < 0.001 && _velocity.length() < STOP_SPEED) {
            _velocity = Vec2.new(0, 0)
        }
        PhysicsBody.setLinearVelocity(_entity, _velocity)

        animate_(_velocity)

        if (isStuckOrSeparated_(here, leader)) {
            Debug.info("[Party] %(_name) is stuck/separated - recovering")
            _recovery.recover(this, leader)
            _strategy.reset(this)
            _checkTime = null
        }
    }

    // Greys the follower out while its member is downed, and back to normal once rested - only on a change.
    refreshTint_(me) {
        var downed = me != null && me.downed
        if (downed == _tintedDowned) {
            return
        }
        var sprite = Sprite.get(_entity)
        if (sprite != null) {
            sprite.color = downed ? DOWNED_TINT : NORMAL_TINT
            _tintedDowned = downed
        }
    }

    // Walk / run toward where it is heading, or idle facing the last way it went. Each choice has some slack so a
    // speed or heading hovering on a threshold doesn't flicker the clip: idle/moving has separate start and stop
    // speeds, walk vs run follows the pace being matched (not the catch-up boost or arrival easing), and the
    // facing only turns once the heading has swung clearly away from it.
    animate_(velocity) {
        var speed = velocity.length()
        _moving = _moving ? speed > IDLE_SPEED : speed > MOVING_SPEED
        var clip = "Idle"
        if (_moving) {
            var heading = velocity.normalize()
            if (Vec2.dot(heading, _facing.normalize()) < FACING_TURN) {
                _facing = heading
            }
            clip = this.speed > RUN_SPEED || speed > CATCH_UP_RUN ? "Run" : "Walk"
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

    // Only talkable while on a pressure plate (to ask them to stay) or while staying (to ask them to follow) -
    // otherwise a follower right behind the player would keep taking the interact prompt.
    isInteractable() {
        var me = member
        return me != null && (me.staying || PressurePlates.isStoodOn(_entity))
    }

    requiresFacing() { true }

    getEntity() { _entity }

    // Used by States/InteractState.wren in place of the NPC's usual conversation.
    dialogueEntryPoint() {
        var base = DialogueEntryPoints.get(_name)
        if (base == null) {
            return null
        }
        var me = member
        return base + (me != null && me.staying ? "_party_follow" : "_party_stay")
    }
}
