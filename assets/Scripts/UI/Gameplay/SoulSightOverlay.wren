// UI/Gameplay/SoulSightOverlay.wren
// Soul Sight's screen tint (World/SoulSight.wren): a full-screen wash of cold blue over the world, under the HUD,
// that fades in when Soul Sight comes on and out when it goes off. Its owner (States/GameWorldState.wren out in the
// world, States/CombatState.wren in a fight) passes the soulSight event on (setActive) and calls update every frame
// for the fade; setShowing hides it outright while something covers the world.
import "ui" for UIManager, UIColor
import "math" for Vec2, Vec4
import "app" for Time, Application

var TINT = Vec4.new(30, 70, 170, 0)   // its colour - the alpha comes from the fade
var MAX_ALPHA = 85
var FADE_SECONDS = 0.35

class SoulSightOverlay {
    // z: its z index among the top-level UI (below the HUD's).
    construct new(active, z) {
        _rect = UIColor.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(Application.gameWidth, Application.gameHeight), Vec2.new(0, 0))
        _rect.setZIndex(z)
        _rect.setFocusable(false)
        UIManager.addUIElement(_rect)
        _target = active ? 1 : 0
        _amount = _target
        _showing = true
        apply_()
    }

    setActive(active) { _target = active ? 1 : 0 }

    setShowing(showing) {
        if (_showing == showing) return
        _showing = showing
        apply_()
    }

    update() {
        if (_amount == _target) return
        var step = Time.unscaledDelta / FADE_SECONDS
        _amount = _target > _amount ? (_amount + step).min(_target) : (_amount - step).max(_target)
        apply_()
    }

    apply_() {
        _rect.setColor(Vec4.new(TINT.x, TINT.y, TINT.z, (MAX_ALPHA * _amount).round))
        _rect.setVisible(_showing && _amount > 0)
    }

    teardown() {
        if (_rect != null) UIManager.removeUIElement(_rect)
        _rect = null
    }
}
