// UI/Combat/CombatantUI.wren
// One combatant's card: a compact UI-kit "tile" with its name and committed move on the top row, then its HP bar,
// charge bar and (optional) stamina bar, with the kit's orange focus ring around it while it is selected (whose move
// is being picked, the target being chosen, or who is attacking during a parry prompt). A defeated combatant's card
// dims. Pure view - refresh() takes only primitives (alive/fraction numbers/strings/bools) that
// States/Combat/CombatSession.wren has already read off the Combatant/Timeline model; no domain object crosses into
// this file.
//
//   ┌──────────────────────────────┐
//   │ Name                  Cleave │
//   │ [HP ██████████████░░░░░░░]   │
//   │ [charge █████░░░░░░░░░░░░]   │
//   │ [stamina ████████████░░░░]   │
//   └──────────────────────────────┘
import "ui" for UILabel
import "math" for Vec2, Vec4
import "UI/Theme/UIKit" for UIKit
import "UI/Combat/HealthBarUI" for HealthBarUI
import "UI/Combat/ChargeBarUI" for ChargeBarUI
import "UI/Combat/StaminaBarUI" for StaminaBarUI

var PAD_X = 12            // content in from the card's sides (the tile's rim is 4px art at 2x = 8px)
var NAME_Y = 7
var HP_Y = 34
var HP_HEIGHT = 15        // the kit track's 3px rim plus a 9px fill
var CHARGE_Y = 52
var BAR_HEIGHT = 12       // charge and stamina: 3px rim plus a 6px fill
var STAMINA_Y = 67
var PAD_BOTTOM = 12
var TEXT_SIZE = 18.0
var DEAD_TINT = Vec4.new(150, 150, 150, 255)
var NO_TINT = Vec4.new(255, 255, 255, 255)

class CombatantUI {
    // The card's height - for laying cards out before building them.
    static height(showStamina) {
        return (showStamina ? STAMINA_Y : CHARGE_Y) + BAR_HEIGHT + PAD_BOTTOM
    }

    // parent: UIElement to attach to. name: fixed for the fight. x,y: the card's top-left, px. width: the card's
    // width, px. showStamina: true for the player (and any combatant whose stamina you get to see).
    construct new(parent, name, x, y, width, showStamina, font) {
        _card = UIKit.frame(parent, "tile", Vec2.new(x, y), Vec2.new(0, 0),
            Vec2.new(width, CombatantUI.height(showStamina)), Vec2.new(0, 0), 1)
        _ring = UIKit.focusRing(_card, 4)

        _name = UILabel.new(Vec2.new(PAD_X - 5, NAME_Y), Vec2.new(0, 0), name, TEXT_SIZE)
        _name.setFont(font)
        _name.setTextColor(UIKit.text)
        _name.setBoundingBoxToText()
        _name.setZIndex(3)
        _card.addChild(_name)

        // The move, right-aligned on the name's row - rich text sizes itself exactly, so its right edge sits on the
        // padding however long the move's name is (5 / 2: the rich label's own padding).
        _move = UIKit.richText(_card, "", Vec2.new(-PAD_X + 5, NAME_Y - 2), Vec2.new(1, 0), Vec2.new(1, 0), TEXT_SIZE,
            UIKit.textMuted, 3)
        _moveText = ""

        var barWidth = width - 2 * PAD_X
        _hp = HealthBarUI.new(_card, PAD_X, HP_Y, barWidth, HP_HEIGHT)
        _charge = ChargeBarUI.new(_card, PAD_X, CHARGE_Y, barWidth, BAR_HEIGHT)
        _stamina = showStamina ? StaminaBarUI.new(_card, PAD_X, STAMINA_Y, barWidth, BAR_HEIGHT) : null
        _alive = null
    }

    selected=(sel) { _ring.setVisible(sel) }

    // alive/hpFraction/chargeFraction/staminaFraction/moveText/staggered are all resolved by the
    // Controller from the Combatant/Timeline model before this is called.
    refresh(alive, hpFraction, chargeFraction, staminaFraction, moveText, staggered) {
        _hp.setFraction(hpFraction)
        _charge.setFraction(alive ? chargeFraction : 0)
        _charge.setStaggered(staggered)
        if (_stamina != null) {
            _stamina.setFraction(staminaFraction)
        }
        // Only when they change - refresh runs every frame of the fight.
        var text = alive ? moveText : "defeated"
        if (text != _moveText) {
            _moveText = text
            _move.setMarkupText(text)
        }
        if (alive != _alive) {
            _alive = alive
            _card.setTint(alive ? NO_TINT : DEAD_TINT)
            _name.setTextColor(alive ? UIKit.text : UIKit.textMuted)
        }
        if (!alive) {
            _ring.setVisible(false)
        }
    }
}
