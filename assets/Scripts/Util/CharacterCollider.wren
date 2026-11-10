// Util/CharacterCollider.wren
// The collision circle every walking character uses - the player (GameObjects/Player.wren), NPCs (GameObjects/NPC.wren)
// and party followers (GameObjects/PartyMember.wren) - so they all fit the same gaps.
// Sized a little under one 16 px tile (12.8 px across at 32 px/m) so a character slips through a one-tile gap, and
// lifted so its bottom edge sits on the sprite's feet: the generated sheets put their pivot (the entity's position)
// 2-4 px above the feet, so a circle centred on the pivot would poke out below them and catch walls the feet clear.
import "physics" for PhysicsCircleShape

// Radius in metres (6.4 px).
var RADIUS = 0.2
// Centre offset from the entity's position, in metres (+y is down the screen): 3.2 px up from the pivot.
var OFFSET_Y = -0.1

class CharacterCollider {
    static radius { RADIUS }
    static offsetY { OFFSET_Y }

    // A fresh shape for PhysicsBody.create.
    static shape() { PhysicsCircleShape.new(RADIUS, 0, OFFSET_Y) }
}
