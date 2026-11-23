// Util/CharacterCollider.wren
// The collision circle every walking character uses - the player (GameObjects/Player.wren), NPCs (GameObjects/NPC.wren)
// and party followers (GameObjects/PartyMember.wren) - so they all fit the same gaps.
// Sized a little under one 16 px tile (12.8 px across at 32 px/m) so a character slips through a one-tile gap.
//
// A character's origin (its entity's position) is the centre of this circle, so a distance from a character is the
// same whichever way you measure it (a warp door's arrival - GameObjects/DoorWorp.wren). The circle sits with its
// bottom edge on the sprite's feet: the generated sheets put their pivot 2-4 px above the feet, so the overworld sheets
// are drawn with their pivot raised by ORIGIN_LIFT_PX (Sprites/SpriteSheet.wren) - the origin, and this circle, sit that
// far above the sheet's own pivot. The shadow (Util/Shadow.wren) is drawn that far below the origin, under the sprite.
import "physics" for PhysicsCircleShape

// Radius in metres (6.4 px).
var RADIUS = 0.2
// How far above a sheet's pivot a character's origin (the circle's centre) is, in px (+y is down the screen) - in px,
// not metres, as it moves sprite art (the editor's level viewer draws sheets at another px/m).
var ORIGIN_LIFT_PX = 3.2
// How far below the origin the feet - the bottom of the sprite, and of the circle - are, in px: the radius at the
// game's 32 px/m. Characters draw in order of this line, not of their origin (Sprites/SpriteSheet.wren).
var FEET_BELOW_ORIGIN_PX = 6.4

class CharacterCollider {
    static radius { RADIUS }
    // The circle's offset from the origin, in metres - none: the origin is its centre.
    static offsetY { 0 }
    // How far the sheet's pivot is raised, and how far below the origin the sheet's pivot (and shadow) are, in px.
    static originLiftPx { ORIGIN_LIFT_PX }
    // How far below the origin the feet are, in px - the y characters are sorted by on the entities layer.
    static feetBelowOriginPx { FEET_BELOW_ORIGIN_PX }

    // A fresh shape for PhysicsBody.create, centred on the entity.
    static shape() { PhysicsCircleShape.new(RADIUS, 0, 0) }
}
