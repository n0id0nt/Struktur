// UI/Combat/CombatantUI.wren
// The per-combatant UI stack: name + HP bar + charge bar + (optional) stamina bar + a committed-move
// readout, plus a target-select marker. Pure view - refresh() takes only primitives (alive/fraction
// numbers/strings/bools) that States/CombatState.wren has already read off the Combatant/Timeline
// model; no domain object crosses into this file.
import "ui" for UILabel
import "math" for Vec2, Vec4
import "UI/Combat/HealthBarUI" for HealthBarUI
import "UI/Combat/ChargeBarUI" for ChargeBarUI
import "UI/Combat/StaminaBarUI" for StaminaBarUI
import "Colors" for WHITE

var DIM_COLOR    = Vec4.new(140, 140, 140, 255)
var MARKER_COLOR = Vec4.new(255, 216, 96, 255)

class CombatantUI {
    // parent: UIElement to attach to. name: fixed for the fight. x,y: stack top-left, px.
    // width: bar width, px. showStamina: true for the player (and any combatant whose stamina you
    // get to see).
    construct new(parent, name, x, y, width, showStamina, font) {
        _name = label_(parent, name, x, y, 20.0, font)

        var by = y + 24
        _hp = HealthBarUI.new(parent, x, by, width, 13)
        _charge = ChargeBarUI.new(parent, x, by + 16, width, 7)
        _stamina = null
        var moveY = by + 26
        if (showStamina) {
            _stamina = StaminaBarUI.new(parent, x, by + 25, width, 5)
            moveY = by + 33
        }
        _move = label_(parent, "", x, moveY, 14.0, font)

        _marker = label_(parent, ">>", x - 26, y, 22.0, font)
        _marker.setTextColor(MARKER_COLOR)
        _marker.setVisible(false)
    }

    label_(parent, text, x, y, size, font) {
        var lbl = UILabel.new(Vec2.new(x, y), Vec2.new(0, 0), text, size)
        lbl.setFont(font)
        lbl.setTextColor(WHITE)
        lbl.setBoundingBoxToText()
        parent.addChild(lbl)
        return lbl
    }

    selected=(sel) { _marker.setVisible(sel) }

    // alive/hpFraction/chargeFraction/staminaFraction/moveText/staggered are all resolved by the
    // Controller from the Combatant/Timeline model before this is called.
    refresh(alive, hpFraction, chargeFraction, staminaFraction, moveText, staggered) {
        _hp.setFraction(hpFraction)
        _charge.setFraction(alive ? chargeFraction : 0)
        _charge.setStaggered(staggered)
        if (_stamina != null) {
            _stamina.setFraction(staminaFraction)
        }
        _name.setTextColor(alive ? WHITE : DIM_COLOR)
        _move.setText(alive ? moveText : "defeated")
        _move.setBoundingBoxToText()
        if (!alive) {
            _marker.setVisible(false)
        }
    }
}
