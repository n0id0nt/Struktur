// Combat/Stats.wren
// The numeric core of a battle participant: HP and stamina (Phase 4 of the Interrupt Combat
// Roadmap), plus the attribute block every contest in Combat/Arbiter.wren reads. Later phases hang
// resistances and status timers off this same bag, so systems that only care about "how hurt / how
// tired / how quick is this thing" never need to know about Combatant, moves, or the timeline.
var EXHAUSTION_THRESHOLD = 0.2   // below this fraction of max stamina, moves take +1 time unit

// The attribute block. 10 is "average"; the archetype supplies a baseline and a def overrides single
// entries (Combat/Config/Archetype.attributesWith). Which ones the Arbiter actually reads so far:
//   reflex    - parry quality (vs the attacker's precision)
//   precision - beating the defender's parry
// Declared for the upcoming checks (see Combat/Arbiter.wren header): might (damage / stagger dealt),
// guard (damage taken), poise (stagger taken), agility (charge rate / initiative / flee), endurance
// (stamina regen / exhaustion), focus (heal strength).
var ATTRIBUTE_NAMES = ["might", "guard", "poise", "agility", "reflex", "precision", "endurance", "focus"]
var DEFAULT_ATTRIBUTE = 10

class Stats {
    // attributes: { name: value } for any of ATTRIBUTE_NAMES (null or missing entries -> DEFAULT_ATTRIBUTE).
    construct new(maxHp, maxStamina, attributes) {
        _maxHp = maxHp
        _hp = maxHp
        _maxStamina = maxStamina
        _stamina = maxStamina
        _attributes = {}
        for (name in ATTRIBUTE_NAMES) {
            _attributes[name] = (attributes != null && attributes.containsKey(name)) ? attributes[name] : DEFAULT_ATTRIBUTE
        }
    }

    // --- attributes ---------------------------------------------------------------------

    attr(name) { _attributes[name] }
    // For levelling / gear / buffs later - the block persists with the Combatant between fights.
    setAttr(name, value) { _attributes[name] = value }

    might { _attributes["might"] }
    guard { _attributes["guard"] }
    poise { _attributes["poise"] }
    agility { _attributes["agility"] }
    reflex { _attributes["reflex"] }
    precision { _attributes["precision"] }
    endurance { _attributes["endurance"] }
    focus { _attributes["focus"] }

    // --- pools -----------------------------------------------------------------------------

    hp { _hp }
    maxHp { _maxHp }
    alive { _hp > 0 }
    fraction { _maxHp > 0 ? _hp / _maxHp : 0 }

    stamina { _stamina }
    maxStamina { _maxStamina }
    staminaFraction { _maxStamina > 0 ? _stamina / _maxStamina : 0 }
    // The Disruption/Timeline exhaustion penalty gate (see Timeline.commit).
    exhausted { staminaFraction < EXHAUSTION_THRESHOLD }

    takeDamage(amount) {
        _hp = _hp - amount
        if (_hp < 0) {
            _hp = 0
        }
    }

    heal(amount) {
        _hp = _hp + amount
        if (_hp > _maxHp) {
            _hp = _maxHp
        }
    }

    spendStamina(amount) {
        _stamina = _stamina - amount
        if (_stamina < 0) {
            _stamina = 0
        }
    }

    // Continuous regen (Timeline.tick feeds this) and Guard's one-shot restore both land here.
    gainStamina(amount) {
        _stamina = _stamina + amount
        if (_stamina > _maxStamina) {
            _stamina = _maxStamina
        }
    }
}
