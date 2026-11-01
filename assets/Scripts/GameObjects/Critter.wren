// GameObjects/Critter.wren
// Shared base class for the wandering-critter behaviour scripts (Chicken, Chinlin). Owns the bits
// every critter needs regardless of species: the entity/name, current facing + move/velocity state,
// the per-entity FSM (see GameObjects/StateMachine.wren), the front/back/side animation-picking
// logic every sheet so far uses (see playAnimation below), and the registry of every live critter
// that steering behaviours (see GameObjects/Steering/*.wren) need to see each other.
//
// Concrete critters build their states out of the reusable mixins in GameObjects/States/
// (IdleMixin, WanderMixin, ChaseMixin) via addState(), and their continuous steering out of
// GameObjects/Steering/ (Separation, ...) via addSteering(); they do their own texture/Sprite/
// PhysicsBody/SpriteAnimation setup in start() - that part isn't shared, since the sheets, cell
// sizes, body radii and animation frame ranges are all species-specific.
//
// Wren note: a subclass cannot read a base class's fields directly - `_name` written in one class
// body and `_name` written in another are different storage slots even with the same name, even
// across inheritance (see third_party/wren/test/language/inheritance/inherited_fields_in_closure.wren
// for the language behaviour this is working around). So every field a mixin, a steering behaviour,
// or a subclass needs lives here, reached only through the methods below - never through
// `_entity`/`_facing`/etc. directly from Chicken/Chinlin or from a mixin/behaviour.
//
// Every critter belongs to a home room - the LDtk level it was placed in (Rooms.wren). It never moves out of it
// (tickStates() cancels any movement past the room's edge), wanders only to spots inside it (randomPointInHome),
// and only chases a player who is inside it (inHomeRoom - see GameObjects/States/ChaseMixin.wren).
import "math" for Vec2
import "random" for Random
import "gameObject" for GameObject
import "gameObjectComponents" for Sprite, SpriteAnimation, PhysicsBody, WorldTransform
import "renderer" for FlipBit
import "GameObjects/StateMachine" for StateMachine
import "Defeated" for Defeated
import "Rooms" for Rooms

// px in from the home room's edges that a critter is stopped at (tickStates)...
var ROOM_EDGE_MARGIN = 8
// ...and that wander spots are picked inside (randomPointInHome), so a wander never ends pressed against an edge.
var WANDER_MARGIN = 24
// Shared by every critter - see GameObjects/States/WanderMixin.wren on why one generator, not one each.
var RNG = Random.new()

class Critter {
    // Every live critter, across every species - lazily created since Wren has no static-field
    // initialiser syntax (a bare `__all` defaults to null until first assigned). Populated by
    // construct new()/onDestroy() below; queried by steering behaviours that need to see every
    // other critter without each one needing to register with every behaviour separately.
    static all {
        if (__all == null) {
            __all = []
        }
        return __all
    }

    construct new(entity, args) {
        _entity = entity
        _name = args["Name"]
        _iid = args["Iid"]
        _fsm = StateMachine.new()
        _facing = Vec2.new(0, 1)     // start facing the camera ("front")
        _moveDir = Vec2.new(0, 0)
        _velocity = Vec2.new(0, 0)
        _steeringBehaviours = []
        _combatant = null
        _homeRoom = null             // found on first use - see homeRoom
        // Killed for good in an earlier fight (Defeated.wren): the level re-created it, so it removes itself
        // (GameObject.destroy is queued, so it's gone by the end of the frame) and never joins the registry -
        // nothing can see it or fight it in the meantime. Subclasses skip their start()/fixedUpdate() while
        // `removed` is set.
        _removed = Defeated.contains(_iid)
        if (_removed) {
            GameObject.destroy(entity)
        } else {
            Critter.all.add(this)
        }
    }

    entity { _entity }
    name { _name }
    // The LDtk instance id (null for a critter spawned from script) - what Defeated.wren records a kill by.
    iid { _iid }
    // True for a critter that was already defeated and is removing itself (see the constructor).
    removed { _removed }
    // World position, for steering behaviours that need to measure distance between critters -
    // null if this entity somehow has no Transform (shouldn't happen; GameObject.create always adds
    // one), guarded the same way WorldTransform.getPosition itself guards it.
    position { WorldTransform.getPosition(_entity) }

    // The room (level index - Rooms.wren) this critter belongs to: the one it is in when first asked, which is
    // where its level placed it. null when there's no world to look rooms up in.
    homeRoom {
        if (_homeRoom == null) {
            _homeRoom = Rooms.indexAt(position)
        }
        return _homeRoom
    }

    // True if position (Vec2/Vec3) is inside this critter's home room - or if it has no home room, so a critter
    // outside any known room behaves as it always did.
    inHomeRoom(position) {
        var room = homeRoom
        return room == null || Rooms.contains(room, position, 0)
    }

    // A random spot (Vec2) inside the home room, WANDER_MARGIN in from its edges. With maxDistance (px) it is
    // also within that distance of where the critter is now, so ordinary wandering stays local; null for
    // anywhere in the room. Returns null if the critter has no home room.
    randomPointInHome(maxDistance) {
        var b = Rooms.bounds(homeRoom)
        var here = position
        if (b == null || here == null) {
            return null
        }
        var minX = b[0] + WANDER_MARGIN
        var maxX = b[0] + b[2] - WANDER_MARGIN
        var minY = b[1] + WANDER_MARGIN
        var maxY = b[1] + b[3] - WANDER_MARGIN
        if (maxDistance != null) {
            minX = minX.max(here.x - maxDistance)
            maxX = maxX.min(here.x + maxDistance)
            minY = minY.max(here.y - maxDistance)
            maxY = maxY.min(here.y + maxDistance)
        }
        // Outside its room (or a room too small for the margin): head for the room's middle instead.
        if (minX > maxX || minY > maxY) {
            return Vec2.new(b[0] + b[2] / 2, b[1] + b[3] / 2)
        }
        return Vec2.new(RNG.float(minX, maxX), RNG.float(minY, maxY))
    }

    // Whether this species forces the player into combat by proximity alone (see Chinlin) rather
    // than merely offering it via the interact prompt (see Chicken.isInteractable()). False by
    // default - overridden as a getter, not a constructor flag, since it's a species trait, not
    // per-instance state (see ExperimentState.checkCombatTriggers() for where this is read).
    aggressive { false }

    // A critter's battle profile (move set, stats, arena sprite + animations) lives in
    // Combat/BattleCritter.wren, looked up by this critter's name when an encounter starts - not
    // here, so overworld behaviour and battle presentation stay separable.

    // This critter's battle Combatant once it has fought (null until then) - minted from its BattleCritter
    // def by States/CombatState.wren and kept here, so wounds from a fled or lost fight carry into the next
    // one. It lives as long as this entity: when the level unloads and re-creates the critter, it's fresh.
    combatant { _combatant }
    combatant=(value) { _combatant = value }

    facing { _facing }
    facing=(value) { _facing = value }
    moveDir { _moveDir }
    moveDir=(value) { _moveDir = value }

    // The velocity a state mixin *wants* this tick (see IdleMixin/WanderMixin/ChaseMixin) - steering
    // behaviours are summed on top of this, not instead of it (see tickStates()), so e.g. a critter
    // can be wandering north-east while also being nudged sideways away from a neighbour.
    velocity { _velocity }
    velocity=(value) { _velocity = value }

    // Name of the currently active state - e.g. ChaseMixin uses this to tell whether it's already
    // active before forcing a transition in, or whether it's the one currently active before forcing
    // a transition back out.
    currentStateName { _fsm.name }

    // Registers a state built from a mixin object - anything exposing enter(owner) and update(owner)
    // (see GameObjects/States/*.wren). The mixin is handed `this` (the concrete Chicken/Chinlin
    // instance) so it can drive movement/animation through the methods on this class without needing
    // to know which concrete species it's driving.
    addState(name, mixin) {
        _fsm.define(name, Fn.new { mixin.enter(this) }, Fn.new { mixin.update(this) }, null)
    }

    // Enters the given state - call once from start() after every addState(), and from a mixin (or a
    // subclass) whenever something needs to force a transition outside the normal per-state timers.
    changeState(name) { _fsm.change(name) }

    // Registers a continuous steering behaviour (see GameObjects/Steering/*.wren) - anything exposing
    // steer(owner) that returns a Vec2 nudge to add to this tick's velocity. Unlike states, every
    // registered behaviour runs every tick regardless of which state is currently active.
    addSteering(behaviour) {
        _steeringBehaviours.add(behaviour)
    }

    // Drives the FSM (which sets `velocity` via the active state's mixin), sums this tick's steering
    // nudges on top of it, and commits the result to the physics body. Call once per fixedUpdate().
    tickStates() {
        _fsm.update()

        var steering = Vec2.new(0, 0)
        for (behaviour in _steeringBehaviours) {
            steering = steering + behaviour.steer(this)
        }
        PhysicsBody.setLinearVelocity(_entity, keepInHomeRoom_(_velocity + steering))
    }

    // Cancels whatever part of velocity would carry the critter past an edge of its home room (within
    // ROOM_EDGE_MARGIN of it) - whichever state or steering push asked for it. Moving back inwards is left alone.
    keepInHomeRoom_(velocity) {
        var b = Rooms.bounds(homeRoom)
        var here = position
        if (b == null || here == null) {
            return velocity
        }
        var x = velocity.x
        var y = velocity.y
        if ((x < 0 && here.x <= b[0] + ROOM_EDGE_MARGIN) || (x > 0 && here.x >= b[0] + b[2] - ROOM_EDGE_MARGIN)) {
            x = 0
        }
        if ((y < 0 && here.y <= b[1] + ROOM_EDGE_MARGIN) || (y > 0 && here.y >= b[1] + b[3] - ROOM_EDGE_MARGIN)) {
            y = 0
        }
        return Vec2.new(x, y)
    }

    // Picks "<action>Front/Back/Side" from the current facing and mirrors the side strip when facing
    // left (Sprite.setFlipped) - every critter sheet so far only has front/back/right strips, no
    // separate left. Shared here because it only touches _entity/_facing, both owned by this class.
    playAnimation(action) {
        var sideways = _facing.x.abs > _facing.y.abs
        var suffix = "Front"
        if (sideways) {
            suffix = "Side"
            Sprite.setFlipped(_entity, _facing.x < 0 ? FlipBit.HORIZONTAL : FlipBit.NONE)
        } else if (_facing.y < 0) {
            suffix = "Back"
        }
        SpriteAnimation.setCurrentAnimation(_entity, "%(action)%(suffix)")
    }

    // Default no-op lifecycle hooks, inherited as-is by subclasses that don't need them - only
    // start() and fixedUpdate() are species-specific enough that every critter defines its own.
    update() {}
    onEvent(type, data) {}

    // Unregisters from the shared registry - if a subclass overrides onDestroy() for its own cleanup
    // it must call super.onDestroy(), or this critter will keep nudging others' Separation forever.
    onDestroy() {
        Critter.all.remove(this)
    }
}
