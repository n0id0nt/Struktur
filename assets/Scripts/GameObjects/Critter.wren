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
import "math" for Vec2
import "gameObjectComponents" for Sprite, SpriteAnimation, PhysicsBody, WorldTransform
import "renderer" for FlipBit
import "GameObjects/StateMachine" for StateMachine

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
        _fsm = StateMachine.new()
        _facing = Vec2.new(0, 1)     // start facing the camera ("front")
        _moveDir = Vec2.new(0, 0)
        _velocity = Vec2.new(0, 0)
        _steeringBehaviours = []
        Critter.all.add(this)
    }

    entity { _entity }
    name { _name }
    // World position, for steering behaviours that need to measure distance between critters -
    // null if this entity somehow has no Transform (shouldn't happen; GameObject.create always adds
    // one), guarded the same way WorldTransform.getPosition itself guards it.
    position { WorldTransform.getPosition(_entity) }

    // Whether this species forces the player into combat by proximity alone (see Chinlin) rather
    // than merely offering it via the interact prompt (see Chicken.isInteractable()). False by
    // default - overridden as a getter, not a constructor flag, since it's a species trait, not
    // per-instance state (see ExperimentState.checkCombatTriggers() for where this is read).
    aggressive { false }

    // A critter's battle profile (move set, stats, arena sprite + animations) lives in
    // Combat/BattleCritter.wren, looked up by this critter's name when an encounter starts - not
    // here, so overworld behaviour and battle presentation stay separable.

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
        PhysicsBody.setLinearVelocity(_entity, _velocity + steering)
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
