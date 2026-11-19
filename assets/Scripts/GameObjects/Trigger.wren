// GameObjects/Trigger.wren
// The LDtk "Trigger" entity: an invisible area that plays a dialogue node the first time the player walks into it
// (Kira noticing something and going over to look, a line as you cross into the woods...). Fields:
//   Node          - the Dialogue/Arc1Dialogue.json node to play (once per save - World/StoryTriggers.wren)
//   RequiresFlag  - optional "<int flag> <op> <number>": it only fires while this holds. Checked again while the
//                   player stands inside, so a moment can wait for something to happen there (a fight ending)
//   ShowFromStage / HideFromStage - optional story gating, as for NPCs (World/StoryGates.wren)
// Detection is a physics sensor on the "Trigger" layer, like a pressure plate; only the player counts, not the party.
import "app" for Application
import "gameObject" for GameObject
import "gameObjectComponents" for PhysicsBody
import "physics" for BodyDefinition, PhysicsBoxShape, BodyType, CollisionLayers
import "World/StoryGates" for StoryGates
import "World/StoryTriggers" for StoryTriggers

class Trigger {
    construct new(entity, args) {
        _entity = entity
        _node = args["Node"]
        _requires = args["RequiresFlag"]
        _size = [args["Width"] != null ? args["Width"] : 32, args["Height"] != null ? args["Height"] : 32]
        _gate = StoryGates.fromArgs(args)
        _inside = false
    }

    name { _node }

    start() {
        StoryGates.register(_entity, _gate)
        if (_node == null || _node == "" || StoryTriggers.fired(_node)) return
        // The entity sits at the centre of its LDtk rect (Engine/World/Level.cpp), so the box is centred on it.
        var metres = Application.pixelsPerMeter
        var body = PhysicsBody.create(_entity, BodyDefinition.new(BodyType.STATIC_BODY),
                                      PhysicsBoxShape.new(_size[0] / 2 / metres, _size[1] / 2 / metres))
        body.isSensor = true
        body.setCollisionFilter(CollisionLayers.getLayer("Trigger"), CollisionLayers.getLayer("Actor"))
    }

    // While the player is inside and the moment is still waiting on its RequiresFlag.
    update() {
        if (_inside) tryFire_()
    }

    onDestroy() {
        StoryGates.unregister(_entity)
    }

    // Contact events go to every script, so only the player touching this one counts.
    onEvent(type, data) {
        if (type != "collisionEnter" && type != "collisionExit") return
        var other = null
        if (data["entityA"] == _entity) {
            other = data["entityB"]
        } else if (data["entityB"] == _entity) {
            other = data["entityA"]
        }
        if (other == null || !GameObject.getAllWithIdentifier("Player").contains(other)) return
        _inside = type == "collisionEnter"
        if (_inside) tryFire_()
    }

    tryFire_() {
        if (StoryTriggers.fired(_node)) {
            _inside = false
            return
        }
        if (StoryTriggers.holds(_requires)) StoryTriggers.request(_node)
    }

    isInteractable() { false }

    getEntity() { _entity }
}
