// States/ImpactState.wren
// A short "look at that" moment (an ImpactEvent from ImpactEvents.wren - e.g. a door opening once its pressure
// plates are all down): the camera pans slowly over to the event, it happens there with a little screen shake,
// the camera holds on it a moment, then pans back to the player. Players are held still for the duration the same
// way dialogue holds them (Player.setInteracting) - their animations keep running, they just take no input.
// If the event has a caption (ImpactEvent.message) it types out in a dialogue box at the top of the screen
// (UI/Dialogue/DialogueBoxUI.top) as the event happens, and the camera holds until it has had time to be read.
//
// A substate of States/ExperimentState.wren, entered with params:
//   "event"       - the ImpactEvent to play (its target is where the camera looks, trigger() makes it happen)
//   "roomCamera"  - the gameplay state's Camera/RoomCamera, which does the panning
//   "cameraEntity"- the entity with the Camera component, for the shake
// Clears itself once the camera is back with the player.
import "gameObject" for GameObject
import "gameObjectComponents" for Script, Camera
import "app" for Time
import "debug" for Debug
import "localization" for Localization
import "States/BaseState" for BaseState
import "UI/Dialogue/DialogueBoxUI" for DialogueBoxUI

// --- timing (seconds) ---
var PAN_IN_TIME     = 1.4    // slow pan over to the event
var IMPACT_DELAY    = 0.2    // settle on it before it happens
var HOLD_TIME       = 1.2    // watch it after it happens (at least)
var PAN_OUT_TIME    = 1.0    // pan back to the player
var CAPTION_SPEED   = 0.02   // seconds per caption character as it types out (dialogue's own pace)
var CAPTION_READ    = 1.8    // the hold lasts at least this long after the caption has finished typing
// Camera trauma added at the impact (0..1). Shake grows with trauma squared and fades over the camera's traumaTime
// (1 s); at 0.8 the view moves up to ~0.4 world px (~2 screen px at zoom 5) and tilts up to ~0.6 degrees - see
// CameraSystem::CalculateCameraShake (maxOffset 0.6 px, maxAngle 1 degree at full trauma).
var SHAKE_TRAUMA    = 0.8

class ImpactState is BaseState {
    construct new() {
        super()
        name = "ImpactState"
        _event = null
        _roomCamera = null
        _cameraEntity = null
        _phase = null
        _phaseStart = 0
        _caption = null          // the top-of-screen box, while it's up
        _captionGlyphs = 0
    }

    enter(stateManager, params) {
        super.enter(stateManager, params)
        _event = params["event"]
        _roomCamera = params["roomCamera"]
        _cameraEntity = params["cameraEntity"]
        holdPlayers_(true)
        _roomCamera.focusOn(_event.target, PAN_IN_TIME)
        setPhase_("panIn")
        Debug.info("[Impact] Playing '%(_event.key)' at %(_event.target.x), %(_event.target.y)")
    }

    update(stateManager) {
        var elapsed = Time.scaledTime - _phaseStart
        if (_phase == "panIn") {
            if (_roomCamera.focused && elapsed >= PAN_IN_TIME + IMPACT_DELAY) {
                _event.trigger()
                Camera.addCameraTrauma(_cameraEntity, SHAKE_TRAUMA)
                showCaption_()
                setPhase_("hold")
            }
        } else if (_phase == "hold") {
            if (_caption != null) {
                _caption.revealGlyphs((elapsed / CAPTION_SPEED).floor.min(_captionGlyphs))
            }
            if (elapsed >= holdTime_) {
                hideCaption_()
                _roomCamera.returnToPlayer(PAN_OUT_TIME)
                setPhase_("panOut")
            }
        } else if (_phase == "panOut") {
            if (!_roomCamera.transitioning) {
                stateManager.clearCurrentState()
            }
        }
    }

    exit() {
        super.exit()
        holdPlayers_(false)
        hideCaption_()
        // A reused singleton - drop everything from this run (see InteractState's exit()).
        _event = null
        _roomCamera = null
        _cameraEntity = null
        _phase = null
    }

    // How long to hold on the event: HOLD_TIME, or long enough to type out and read the caption.
    holdTime_ {
        if (_caption == null) {
            return HOLD_TIME
        }
        return HOLD_TIME.max(_captionGlyphs * CAPTION_SPEED + CAPTION_READ)
    }

    showCaption_() {
        if (_event.message == null) {
            return
        }
        _caption = DialogueBoxUI.top()
        _caption.setMarkupText(Localization.get(_event.message))
        _captionGlyphs = _caption.glyphCount
        _caption.revealGlyphs(0)
    }

    hideCaption_() {
        if (_caption != null) {
            _caption.teardown()
            _caption = null
        }
    }

    setPhase_(phase) {
        _phase = phase
        _phaseStart = Time.scaledTime
    }

    // Puts every player into (or back out of) its interacting mode, as dialogue does.
    holdPlayers_(held) {
        for (entity in GameObject.getAllWithIdentifier("Player")) {
            var player = Script.getInstance(entity)
            if (player != null) {
                player.setInteracting(held)
            }
        }
    }
}
