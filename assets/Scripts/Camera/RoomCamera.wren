// Camera/RoomCamera.wren
// Keeps a camera inside the room (LDtk level) the player is in: the view's edges stop at the room's edges
// (Camera.confineToBounds / Camera.bounds - clamped in C++ by CameraSystem). A room smaller than the view on an
// axis is centred on that axis instead.
//
// When the player crosses into another room the view pans over on a single ease-in/ease-out curve, from where it
// is now to where the normal follow will want it in the new room (the player, clamped to that room - re-read every
// frame so a player who keeps walking is still framed when the pan lands). During the pan the bounds are shrunk to
// exactly one view around the eased point, so CameraSystem's clamp pins the view there and its damped follow can't
// add a second, competing motion. Once the pan ends the bounds go back to the room and the follow carries on from
// the same spot.
//
// Owned by the gameplay state (States/ExperimentState.wren): it calls moveTo() whenever the player's room
// changes, snapTo() after a teleport (respawn), and update() every frame to drive the pan.
import "gameObjectComponents" for Camera, WorldTransform
import "math" for Vec2, Vec4
import "app" for Time, Application
import "Rooms" for Rooms

// Seconds the pan from one room to the next takes.
var TRANSITION_TIME = 0.6

class RoomCamera {
    // cameraEntity: the entity carrying the Camera component (the player's), and the one the view follows. Room
    // bounds come from Rooms.wren.
    construct new(cameraEntity) {
        _cameraEntity = cameraEntity
        _roomIndex = null
        _bounds = null       // bounds of _roomIndex
        _start = null        // view centre the current pan started from
        _point = null        // view centre the pan has reached
        _startTime = 0
        _transitioning = false
        _snapPending = false
    }

    // The level index the camera is confined to (or panning to), null before the first room.
    roomIndex { _roomIndex }
    transitioning { _transitioning }

    // Confines to this room straight away, without a pan - for spawning or teleporting in.
    snapTo(levelIndex) {
        if (levelIndex == null) {
            return
        }
        _roomIndex = levelIndex
        _bounds = boundsOf_(levelIndex)
        _transitioning = false
        // Applied by update(): on spawn this runs before the player's script has created its Camera.
        _snapPending = true
        update()
    }

    // Pans the view over to this room. From partway through another pan it carries on from where the view is now,
    // so stepping back and forth across a doorway doesn't jump.
    moveTo(levelIndex) {
        if (levelIndex == null || levelIndex == _roomIndex) {
            return
        }
        if (_roomIndex == null) {
            snapTo(levelIndex)
            return
        }
        var camera = Camera.get(_cameraEntity)
        if (camera == null) {
            snapTo(levelIndex)
            return
        }
        _start = _transitioning ? _point : camera.viewCenter
        _point = _start
        _roomIndex = levelIndex
        _bounds = boundsOf_(levelIndex)
        _startTime = Time.scaledTime
        _transitioning = true
    }

    update() {
        if (_bounds == null) {
            return
        }
        // Looked up every time rather than kept: a component handle can go stale when other cameras are created
        // (CombatState's battle camera, say). It's also missing for the first frame or so, until the player's
        // script creates it - the room is applied once it turns up.
        var camera = Camera.get(_cameraEntity)
        if (camera == null) {
            return
        }
        camera.confineToBounds = true
        if (!_transitioning) {
            camera.bounds = _bounds
            if (_snapPending) {
                camera.forcePosition = true
                _snapPending = false
            }
            return
        }

        var t = ((Time.scaledTime - _startTime) / TRANSITION_TIME).clamp(0, 1)
        if (t >= 1) {
            _transitioning = false
            camera.bounds = _bounds
            return
        }

        var halfView = halfView_(camera)
        var end = clampToRoom_(WorldTransform.getPosition(_cameraEntity), halfView)
        t = t * t * (3 - 2 * t)   // smoothstep: one ease in, one ease out
        _point = Vec2.new(lerp_(_start.x, end.x, t), lerp_(_start.y, end.y, t))
        camera.bounds = Vec4.new(_point.x - halfView.x, _point.y - halfView.y, halfView.x * 2, halfView.y * 2)
    }

    // Half the view's size in world px.
    halfView_(camera) {
        var zoom = camera.zoom > 0 ? camera.zoom : 1
        return Vec2.new(Application.gameWidth / (2 * zoom), Application.gameHeight / (2 * zoom))
    }

    // Where the confined follow settles for a target at position in the current room - mirrors
    // CameraSystem::ConfineToBounds.
    clampToRoom_(position, halfView) {
        return Vec2.new(clampAxis_(position.x, _bounds.x, _bounds.z, halfView.x),
                        clampAxis_(position.y, _bounds.y, _bounds.w, halfView.y))
    }

    clampAxis_(value, min, size, half) {
        var low = min + half
        var high = min + size - half
        return low > high ? min + size / 2 : value.clamp(low, high)
    }

    lerp_(a, b, t) { a + (b - a) * t }

    // The level's world-space rect as a Vec4 (x, y, width, height).
    boundsOf_(levelIndex) {
        var bounds = Rooms.bounds(levelIndex)
        return Vec4.new(bounds[0], bounds[1], bounds[2], bounds[3])
    }
}
