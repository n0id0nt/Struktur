// GameObjects/PerimeterWolf.wren
// The starving thornwolf at the village's north edge that Thomas talks you through (Dialogue/Arc1/
// "thomas_gate"). Unlike the woods' wolves it never notices you by itself: it only wanders and idles about its room.
// When Thomas says it has spotted you, the "chargePlayer" dialogue command calls charge(): it runs straight at the
// player, and catching them starts the fight (States/GameWorldState.startForcedCombat_) - the player's alone, the
// party watches (fightsLeaderAlone - States/CombatState.wren).
// Spawned by the LDtk "Critter" entity whose Enemy is PerimeterThornwolf (GameObjects/Level/Critter.wren); fights as a
// Starving Thornwolf (Combat/Config/BattleCritter.wren). Wears the Chinlin's sheet until the wolves have their own.
import "gameObjectComponents" for PhysicsBody, SpriteAnimation
import "physics" for BodyDefinition, PhysicsCircleShape, BodyType, CollisionLayers
import "GameObjects/Critter" for Critter
import "GameObjects/Chinlin" for Chinlin
import "GameObjects/States/IdleMixin" for IdleMixin
import "GameObjects/States/WanderMixin" for WanderMixin
import "GameObjects/States/ChaseMixin" for ChaseMixin
import "GameObjects/Steering/Separation" for Separation

var WANDER_SPEED = 0.8       // an unhurried prowl
var CHARGE_SPEED = 2.6       // flat out once it has seen you
// The charge never gives up while the player is in its room (ChaseMixin drops a chase out of range or out of the room).
var CHARGE_RANGE = 100000

class PerimeterWolf is Critter {
    construct new(entity, args) {
        super(entity, args)
        _charging = false
    }

    // Only once it charges does catching the player start a fight.
    aggressive { _charging }

    // The fight is the player's alone - Kira watches.
    fightsLeaderAlone() { true }

    // Thomas watched it: once it's beaten he talks to you straight away (States/GameWorldState.onCombatEnded_).
    talkAfterFight() { "Thomas" }

    start() {
        if (removed) {
            return
        }
        Chinlin.createSprite(entity)
        var body = PhysicsBody.create(entity, BodyDefinition.new(BodyType.DYNAMIC_BODY), PhysicsCircleShape.new(0.18))
        body.fixedRotation = true
        // Collide with the level (Wall bit), pass through the player and other critters (see Main.start()).
        body.setCollisionFilter(CollisionLayers.getLayer("Actor"), CollisionLayers.getLayer("Wall"))
        Chinlin.createAnimations(entity)

        _charge = ChaseMixin.new(CHARGE_SPEED, "Run", CHARGE_RANGE, CHARGE_RANGE, "wander")
        addState("wander", WanderMixin.new(WANDER_SPEED, "Run", 1.0, 4.0, "idle"))
        addState("idle", IdleMixin.new("Stand", 1.5, 3.5, "wander"))
        addState("charge", _charge)
        addSteering(Separation.new(28, 1.6))
        changeState("idle")
    }

    // Level viewer (editor): the sprite standing in place.
    static preview(entity, args) {
        Chinlin.createSprite(entity)
        Chinlin.createAnimations(entity)
        SpriteAnimation.setCurrentAnimation(entity, "StandFront")
    }

    // It has spotted the player: it runs at them until it catches them (the fight starts) or they leave its room.
    charge() {
        if (removed || _charging) return
        _charging = true
        changeState("charge")
    }

    fixedUpdate() {
        if (removed) {
            return
        }
        if (_charging) _charge.checkProximity(this, "charge")
        tickStates()
    }
}
