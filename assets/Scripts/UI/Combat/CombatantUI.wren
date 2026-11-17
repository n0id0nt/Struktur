// UI/Combat/CombatantUI.wren
// One combatant's card: a compact UI-kit "tile" with its name and HP numbers on the top row, its HP bar, its stamina
// bar and a line for its statuses (Burn x2, Chill, Frozen, Charge...), with the kit's orange focus ring around it
// while it is the one whose chain is being picked. A defeated combatant's card dims; an unused card is hidden.
// Pure view - refresh() takes only primitives States/CombatState.wren has already read off the Combatant.
//
//   ┌──────────────────────────────┐
//   │ Name                         │
//   │ [HP ██████████████░░░░░░░]   │
//   │ [stamina ████████████░░░░]   │
//   │ Burn x2  Chill        82/100 │
//   └──────────────────────────────┘
import "ui" for UILabel
import "math" for Vec2, Vec4
import "UI/Theme/UIKit" for UIKit
import "UI/Combat/HealthBarUI" for HealthBarUI
import "UI/Combat/StaminaBarUI" for StaminaBarUI

var PAD_X = 12            // content in from the card's sides (the tile's rim is 4px art at 2x = 8px)
var NAME_Y = 7
var HP_Y = 34
var HP_HEIGHT = 15        // the kit track's 3px rim plus a 9px fill
var STAMINA_Y = 52
var BAR_HEIGHT = 12
var STATUS_Y = 66
var CARD_HEIGHT = 92
var TEXT_SIZE = 18.0
var DEAD_TINT = Vec4.new(150, 150, 150, 255)
var NO_TINT = Vec4.new(255, 255, 255, 255)
var STATUS_COLOR = Vec4.new(255, 200, 120, 255)

class CombatantUI {
    static height { CARD_HEIGHT }

    // parent: UIElement to attach to. x,y: the card's top-left, px. width: the card's width, px.
    construct new(parent, x, y, width, font) {
        _card = UIKit.frame(parent, "tile", Vec2.new(x, y), Vec2.new(0, 0), Vec2.new(width, CARD_HEIGHT), Vec2.new(0, 0), 1)
        _ring = UIKit.focusRing(_card, 4)

        _name = UILabel.new(Vec2.new(PAD_X - 5, NAME_Y), Vec2.new(0, 0), "", TEXT_SIZE)
        _name.setFont(font)
        _name.setTextColor(UIKit.text)
        _name.setBoundingBoxToText()
        _name.setZIndex(3)
        _card.addChild(_name)

        // HP numbers, right-aligned on the status row - the name gets the whole top row (5 / 2: the rich label's
        // own padding).
        _numbers = UIKit.richText(_card, "", Vec2.new(-PAD_X + 5, STATUS_Y - 2), Vec2.new(1, 0), Vec2.new(1, 0), TEXT_SIZE,
            UIKit.textMuted, 3)
        _status = UIKit.richText(_card, "", Vec2.new(PAD_X - 5, STATUS_Y - 2), Vec2.new(0, 0), Vec2.new(0, 0), TEXT_SIZE,
            STATUS_COLOR, 3)

        var barWidth = width - 2 * PAD_X
        _hp = HealthBarUI.new(_card, PAD_X, HP_Y, barWidth, HP_HEIGHT)
        _stamina = StaminaBarUI.new(_card, PAD_X, STAMINA_Y, barWidth, BAR_HEIGHT)
        _shown = {}
        _alive = null
    }

    setVisible(visible) {
        if (_shown["visible"] != visible) {
            _shown["visible"] = visible
            _card.setVisible(visible)
        }
    }

    selected=(sel) { _ring.setVisible(sel) }

    // Everything is resolved by the caller from the Combatant before this is called.
    refresh(name, alive, hp, maxHp, staminaFraction, statusText) {
        _hp.setFraction(maxHp > 0 ? hp / maxHp : 0)
        _stamina.setFraction(staminaFraction)
        // Only when they change - refresh runs every frame of the fight.
        if (_shown["name"] != name) {
            _shown["name"] = name
            _name.setText(name)
            _name.setBoundingBoxToText()
        }
        var numbers = alive ? "%(hp.ceil)/%(maxHp)" : "down"
        if (_shown["numbers"] != numbers) {
            _shown["numbers"] = numbers
            _numbers.setMarkupText(numbers)
        }
        if (_shown["status"] != statusText) {
            _shown["status"] = statusText
            _status.setMarkupText(statusText)
        }
        if (alive != _alive) {
            _alive = alive
            _card.setTint(alive ? NO_TINT : DEAD_TINT)
            _name.setTextColor(alive ? UIKit.text : UIKit.textMuted)
        }
        if (!alive) _ring.setVisible(false)
    }
}
