// Combat/CombatantView.wren
// The per-combatant UI stack: name + HP bar + charge bar + (optional) stamina bar + a committed-move
// readout, plus a target-select marker. Phase 5 needs one of these per body on both sides, so this
// bundles what CombatState used to spell out as a dozen loose fields.
import "ui" for UILabel
import "math" for Vec2, Vec4
import "Combat/HealthBar" for HealthBar
import "Combat/ChargeBar" for ChargeBar
import "Combat/StaminaBar" for StaminaBar
import "Colors" for WHITE

var DIM_COLOR    = Vec4.new(140, 140, 140, 255)
var MARKER_COLOR = Vec4.new(255, 216, 96, 255)

class CombatantView {
    // parent: UIElement to attach to. x,y: stack top-left, px. width: bar width, px.
    // showStamina: true for the player (and any combatant whose stamina you get to see).
    construct new(parent, combatant, x, y, width, showStamina, font) {
        _combatant = combatant

        _name = label_(parent, combatant.name, x, y, 20.0, font)

        var by = y + 24
        _hp = HealthBar.new(parent, x, by, width, 13)
        _charge = ChargeBar.new(parent, x, by + 16, width, 7)
        _stamina = null
        var moveY = by + 26
        if (showStamina) {
            _stamina = StaminaBar.new(parent, x, by + 25, width, 5)
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

    combatant { _combatant }

    selected=(sel) { _marker.setVisible(sel) }

    // chargeFraction / moveText come from the Timeline; staggered from CombatState's stagger list.
    refresh(chargeFraction, moveText, staggered) {
        var dead = !_combatant.alive
        _hp.setFraction(_combatant.stats.fraction)
        _charge.setFraction(dead ? 0 : chargeFraction)
        _charge.setStaggered(staggered)
        if (_stamina != null) {
            _stamina.setFraction(_combatant.stats.staminaFraction)
        }
        _name.setTextColor(dead ? DIM_COLOR : WHITE)
        _move.setText(dead ? "defeated" : moveText)
        _move.setBoundingBoxToText()
        if (dead) {
            _marker.setVisible(false)
        }
    }
}
