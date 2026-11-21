// UI/Gameplay/ScreenFade.wren
// A full-screen black fade over the world and every piece of UI: out to black through a warp door and back in on
// the other side, and in from black when you wake after falling in a fight (States/GameWorldState.wren). Its owner
// calls update every frame; it runs on real time, so it fades while game time is paused too. A top-level element,
// so it is its own UI batch and redrawing it every frame of a fade rebuilds nothing else.
import "ui" for UIManager, UIColor
import "math" for Vec2, Vec4
import "app" for Time, Application

var FADE_Z = 1000   // over everything - the HUD, banners, prompts

class ScreenFade {
    construct new() {
        _rect = UIColor.new(Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(Application.gameWidth, Application.gameHeight), Vec2.new(0, 0))
        _rect.setZIndex(FADE_Z)
        _rect.setFocusable(false)
        UIManager.addUIElement(_rect)
        _amount = 0     // 0 clear .. 1 black
        _target = 0
        _speed = 1      // amount per second
        _shown = null
        apply_()
    }

    // Fades to black over `seconds`.
    fadeOut(seconds) { fadeTo_(1, seconds) }

    // Fades back to clear over `seconds`.
    fadeIn(seconds) { fadeTo_(0, seconds) }

    // Straight to black - e.g. under the "You fell" screen, so the world never shows before the fade in.
    setBlack() {
        _amount = 1
        _target = 1
        apply_()
    }

    // All the way black (a fade out has finished).
    black { _amount >= 1 }
    // Clear and not fading.
    clear { _amount <= 0 && _target <= 0 }

    update() {
        if (_amount == _target) return
        var step = Time.unscaledDelta * _speed
        _amount = _target > _amount ? (_amount + step).min(_target) : (_amount - step).max(_target)
        apply_()
    }

    teardown() {
        if (_rect != null) UIManager.removeUIElement(_rect)
        _rect = null
    }

    fadeTo_(target, seconds) {
        _target = target
        _speed = seconds > 0 ? 1 / seconds : 1000000
    }

    apply_() {
        var alpha = (255 * _amount).round
        if (alpha != _shown) {
            _rect.setColor(Vec4.new(0, 0, 0, alpha))
            _shown = alpha
        }
        _rect.setVisible(alpha > 0)
    }
}
