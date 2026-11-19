// World/SoulVeil.wren
// The "SoulVeil" LDtk tile layer: wall tiles painted over a way only Soul Sight shows (World/SoulSight.wren) - a hidden
// doorway (GameObjects/Door.wren, DoorType "Hidden"), a secret alcove. The tiles draw as normal, so the way looks
// like solid wall; while Soul Sight is on they're hidden and the way shows through. The layer only draws - the Hidden
// door underneath still blocks the way until it's opened.
//
// Setting it up in LDtk: a Tiles layer named exactly SoulVeil, in any level that needs one, listed above Entities in
// the layer panel (so it draws over everything in the doorway) and painted with the wall tiles the way should pass for.
// A level without one is fine.
//
// Each loaded level's tile layer is an entity named after the layer (Engine/World/Level.cpp), and the renderer skips
// inactive entities - so hiding the veil is switching those entities off. States/GameWorldState.wren calls show on
// the soulSight event, and refresh whenever it streams a level in, so a newly loaded veil matches.
import "gameObject" for GameObject
import "World/SoulSight" for SoulSight

var LAYER_NAME = "SoulVeil"

class SoulVeil {
    static layerName { LAYER_NAME }

    // Every loaded level's veil drawn (true) or hidden (false).
    static show(visible) {
        for (entity in GameObject.getAllWithIdentifier(LAYER_NAME)) {
            if (visible) {
                GameObject.setActive(entity)
            } else {
                GameObject.setInactive(entity)
            }
        }
    }

    // In step with Soul Sight: hidden while it's on.
    static refresh() { SoulVeil.show(!SoulSight.active) }
}
