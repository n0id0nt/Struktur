// Combat/Archetype.wren
// Phase 5 of the Interrupt Combat Roadmap - the three fighting styles the Rock-Paper-Scissors
// triangle is built on. An archetype is just a display name + HP/stamina profile + move kit
// (Combat/Moves.wren); a Combatant is built from one. The player picks one at the start of a fight;
// a critter carries one (see GameObjects/Critter.combatArchetype).
//
//   Speed / Rogue    - 2u moves, low damage, high stamina efficiency. Interrupt specialist.
//   Power / Warrior  - 4-8u moves, devastating damage + huge stagger, stamina-hungry, big HP pool.
//   Control / Mage   - 3-4u moves, a heal and a heavy timeline debuff. Support / disruptor.
import "Combat/Moves" for Moves

class Archetype {
    construct new(name, maxHp, maxStamina, kit) {
        _name = name
        _maxHp = maxHp
        _maxStamina = maxStamina
        _kit = kit
    }

    name { _name }
    maxHp { _maxHp }
    maxStamina { _maxStamina }
    kit { _kit }
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

    static speed   { Archetype.new("Rogue", 95, 95, Moves.speedKit) }
    static power   { Archetype.new("Warrior", 170, 55, Moves.powerKit) }
    static control { Archetype.new("Mage", 115, 80, Moves.controlKit) }

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
