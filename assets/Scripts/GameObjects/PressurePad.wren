// GameObjects/PressurePad.wren
// The pressure plate as placed in the level: the LDtk entity is called "PressurePad", and World::Level runs the
// script named after the entity - so this is GameObjects/PressurePlate.wren under that name. Its channel is its LDtk
// "Iid", which is what a PressurePad Door's Keys list. (Wren doesn't inherit constructors, hence the forwarding one.)
import "GameObjects/PressurePlate" for PressurePlate

class PressurePad is PressurePlate {
    construct new(entity, args) {
        super(entity, args)
    }
}
