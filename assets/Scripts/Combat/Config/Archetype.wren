// Combat/Config/Archetype.wren
// Phase 5 of the Interrupt Combat Roadmap - the three fighting styles the Rock-Paper-Scissors
// triangle is built on. An archetype is just a display name + HP/stamina profile + move kit
// (Combat/Config/Moves.wren) + a baseline attribute block (Combat/Stats.wren - 10 is average, a def
// overrides single entries via attributesWith); a Combatant is built from one. The player picks one at the start of a fight;
// a critter carries one (see GameObjects/Critter.combatArchetype).
//
//   Speed / Rogue    - 2u moves, low damage, high stamina efficiency. Interrupt specialist.
//   Power / Warrior  - 4-8u moves, devastating damage + huge stagger, stamina-hungry, big HP pool.
//   Control / Mage   - 3-4u moves, a heal and a heavy timeline debuff. Support / disruptor.
import "Combat/Config/Moves" for Moves

class Archetype {
    construct new(name, maxHp, maxStamina, kit, attributes) {
        _name = name
        _maxHp = maxHp
        _maxStamina = maxStamina
        _kit = kit
        _attributes = attributes
    }

    name { _name }
    maxHp { _maxHp }
    maxStamina { _maxStamina }
    kit { _kit }
    attributes { _attributes }

    // This archetype's baseline with `overrides` ({ name: value }, or null) laid over it - a fresh map, so a
    // Combatant's block can change (levelling, gear) without touching the archetype.
    attributesWith(overrides) {
        var out = {}
        for (key in _attributes.keys) {
            out[key] = _attributes[key]
        }
        if (overrides != null) {
            for (key in overrides.keys) {
                out[key] = overrides[key]
            }
        }
        return out
    }
    // The mechanical label (what a critter's combatArchetype returns and byName matches on), as
    // opposed to `name`, which is the flavour class.
    style {
        if (_name == "Warrior") {
            return "Power"
        }
        if (_name == "Mage") {
            return "Control"
        }
        return "Speed"
    }

    // Quick hands: parries well, lands precise cuts, light on guard and poise.
    static speed {
        return Archetype.new("Rogue", 95, 95, Moves.speedKit, {"might": 8, "guard": 7, "poise": 7, "agility": 14, "reflex": 14, "precision": 12, "endurance": 11, "focus": 8})
    }

    // Heavy: hits and absorbs hard, but slow to react - a poor parrier whose blows are easy to read.
    static power {
        return Archetype.new("Warrior", 170, 55, Moves.powerKit, {"might": 15, "guard": 13, "poise": 15, "agility": 7, "reflex": 8, "precision": 9, "endurance": 9, "focus": 7})
    }

    // Measured: precise (hard to parry) and the strongest healer.
    static control {
        return Archetype.new("Mage", 115, 80, Moves.controlKit, {"might": 9, "guard": 9, "poise": 9, "agility": 10, "reflex": 11, "precision": 14, "endurance": 10, "focus": 15})
    }

    static list { [Archetype.speed, Archetype.power, Archetype.control] }

    static byName(name) {
        if (name == "Power") {
            return Archetype.power
        }
        if (name == "Control") {
            return Archetype.control
        }
        return Archetype.speed
    }
}
