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
// The same pinned pan can also look away from the player: focusOn() pans to a fixed world point (centred on it
// exactly, even past the room's edge) and holds there; returnToPlayer() pans back to where the follow wants the
// view and lets go. States/ImpactState.wren uses these for its "look at the door opening" moment.
//
// Owned by the gameplay state (States/ExperimentState.wren): it calls moveTo() whenever the player's room
// changes, snapTo() after a teleport (respawn), and update() every frame to drive the pan.
import "gameObjectComponents" for Camera, WorldTransform
import "math" for Vec2, Vec4
import "app" for Time, Application
import "World/Rooms" for Rooms

// Seconds the pan from one room to the next takes.
var TRANSITION_TIME = 0.6

class RoomCamera {
    // cameraEntity: the entity carrying the Camera component (the player's), and the one the view follows. Room
    // bounds come from World/Rooms.wren.
    construct new(cameraEntity) {
        _cameraEntity = cameraEntity
        _roomIndex = null
        _bounds = null       // bounds of _roomIndex
        _start = null        // view centre the current pan started from
        _point = null        // view centre the pan has reached
        _target = null       // fixed point a focus pan heads for (null: the player, clamped to the room)
        _startTime = 0
        _duration = TRANSITION_TIME
        _mode = null         // null: following the player in the room; "pan": panning to the player, then letting
                             // go; "focus": panning to _target and holding there
        _snapPending = false
    }

    // The level index the camera is confined to (or panning to), null before the first room.
    roomIndex { _roomIndex }
    // True while the view is pinned by a pan or a focus rather than following the player.
    transitioning { _mode != null }
    // True once a focus pan has reached its point (and is holding there).
    focused { _mode == "focus" && progress_ >= 1 }

    // Confines to this room straight away, without a pan - for spawning or teleporting in.
    snapTo(levelIndex) {
        if (levelIndex == null) {
            return
        }
        _roomIndex = levelIndex
        _bounds = boundsOf_(levelIndex)
        _mode = null
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
        if (Camera.get(_cameraEntity) == null) {
            snapTo(levelIndex)
            return
        }
        _roomIndex = levelIndex
        _bounds = boundsOf_(levelIndex)
        beginPan_("pan", null, TRANSITION_TIME)
    }

    // Pans over duration seconds to centre the view on point (a Vec2/Vec3 in world px) and holds it there until
    // returnToPlayer().
    focusOn(point, duration) {
        if (_bounds == null || Camera.get(_cameraEntity) == null) {
            return
        }
        beginPan_("focus", Vec2.new(point.x, point.y), duration)
    }

    // Pans back over duration seconds to where the follow wants the view, then lets the follow take over again.
    returnToPlayer(duration) {
        if (_mode == null) {
            return
        }
        beginPan_("pan", null, duration)
    }

    // Starts a pan from wherever the view is now - partway through another pan included, so nothing jumps.
    beginPan_(mode, target, duration) {
        _start = _mode != null ? _point : Camera.get(_cameraEntity).viewCenter
        _point = _start
        _target = target
        _startTime = Time.scaledTime
        _duration = duration > 0 ? duration : 0.001
        _mode = mode
    }

    // 0 at the start of the current pan, 1 once it has arrived.
    progress_ { ((Time.scaledTime - _startTime) / _duration).clamp(0, 1) }

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
        if (_mode == null) {
            camera.bounds = _bounds
            if (_snapPending) {
                camera.forcePosition = true
                _snapPending = false
            }
            return
        }

        var t = progress_
        if (_mode == "pan" && t >= 1) {
            _mode = null
            camera.bounds = _bounds
            return
        }

        var halfView = halfView_(camera)
        var end = _target != null ? _target : clampToRoom_(WorldTransform.getPosition(_cameraEntity), halfView)
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
