// Combat/Gear.wren
// What a combatant carries into a fight: a main hand, an off-hand and one armour piece (a Loadout of GearItems), and
// the two layers gear adds on top of its type - one of the seven ELEMENTS and up to two CUSTOM EFFECTS at rank I-III
// (the elements and custom effects doc). The rules that read these live in Combat/Rules.wren; this file is the data
// and the questions about it: what a move may use, what every move is taxed, which weapon x armour pairing is active.
//
//   quality 0 early / 1 mid / 2 late scales weapon power x1 / x2 / x4 and armour defence (and a shield's guard)
//   x1 / x2 / x3.5 - never the tax. An element's strength is "late" on late-quality gear and "mid" otherwise.
//   Resonance: the same custom effect on weapon and armour acts one rank higher on both (max III); both already at
//   rank III unlocks that effect's capstone instead.
import "Combat/Config/GearTypes" for GearTypes

var ELEMENTS = ["Flame", "Frost", "Storm", "Iron", "Wild", "Light", "Gold"]
var OPPOSED = {"Flame": "Frost", "Frost": "Flame", "Storm": "Iron", "Iron": "Storm", "Light": "Gold", "Gold": "Light"}
var EFFECTS = ["Riposte", "Momentum", "Opportunist", "Vanguard", "Skirmisher", "Bloodthirst", "Breaker", "Tempo",
    "Heavy", "Guardian", "Last Stand", "Precision", "Piercing", "Focus", "Sentinel", "Reprisal"]
var QUALITY_NAMES = ["Early", "Mid", "Late"]
var POWER_SCALE = [1, 2, 4]
var DEFENCE_SCALE = [1, 2, 3.5]
var RANK_NAMES = ["", "I", "II", "III"]

// weapon element -> { armour element -> the named pairing } (anything else that differs is an Echo pair).
var PAIRS = {
    "Flame": {"Frost": "Temper", "Wild": "Wildfire", "Light": "Beacon"},
    "Frost": {"Flame": "Quench", "Storm": "Cold Snap", "Iron": "Permafrost"},
    "Storm": {"Iron": "Grounded", "Wild": "Stormstep", "Gold": "Overload"},
    "Iron": {"Flame": "Forge", "Frost": "Shatterpoint", "Storm": "Magnetize"},
    "Wild": {"Storm": "Squall", "Gold": "Scatter"},
    "Light": {"Iron": "Bastion", "Wild": "Second Wind", "Gold": "Tithe"},
    "Gold": {"Frost": "Frozen Assets", "Iron": "Toll", "Light": "Indulgence"}
}

class Elements {
    static all { ELEMENTS }
    static opposed(element) { OPPOSED.containsKey(element) ? OPPOSED[element] : null }
    static effects { EFFECTS }
    static qualityNames { QUALITY_NAMES }
    static rankName(rank) { RANK_NAMES[rank] }

    // "Matched", a named synergy / paradox, "Echo", or null when either piece carries no element.
    static pairing(weaponElement, armourElement) {
        if (weaponElement == null || armourElement == null) return null
        if (weaponElement == armourElement) return "Matched"
        var row = PAIRS[weaponElement]
        if (row != null && row.containsKey(armourElement)) return row[armourElement]
        return "Echo"
    }

    // How the wearer's armour element takes an incoming element: x0.5 for its own, x1.5 for the opposed god's,
    // x1 otherwise (Wild, or no element, is neutral to everything). The same factor inverted scales status time.
    static resistance(armourElement, incoming) {
        if (armourElement == null || incoming == null) return 1
        if (armourElement == incoming) return 0.5
        if (Elements.opposed(armourElement) == incoming) return 1.5
        return 1
    }
}

class GearItem {
    // type: a WeaponType or ArmourType. quality 0-2. element: one of ELEMENTS or null. effects: [[name, rank], ...].
    construct new(type, quality, element, effects) {
        _type = type
        _quality = quality
        _element = element
        _effects = effects == null ? [] : effects
    }

    static weapon(id) { GearItem.new(GearTypes.weapon(id), 0, null, null) }
    static weapon(id, quality, element, effects) { GearItem.new(GearTypes.weapon(id), quality, element, effects) }
    static armour(id) { GearItem.new(GearTypes.armour(id), 0, null, null) }
    static armour(id, quality, element, effects) { GearItem.new(GearTypes.armour(id), quality, element, effects) }

    type { _type }
    quality { _quality }
    element { _element }
    effects { _effects }
    late { _quality >= 2 }

    power { _type.power * POWER_SCALE[_quality] }
    defence { _type.defence * DEFENCE_SCALE[_quality] }
    guard { _type.guard * DEFENCE_SCALE[_quality] }
    tax { _type.tax }

    // This item's own rank of a custom effect (0 = it doesn't carry it).
    rank(effect) {
        for (e in _effects) {
            if (e[0] == effect) return e[1]
        }
        return 0
    }

    // "Mid Flame Arming Sword (Momentum II)".
    label {
        var parts = []
        if (_quality > 0) parts.add(QUALITY_NAMES[_quality])
        if (_element != null) parts.add(_element)
        parts.add(_type.name)
        var text = parts.join(" ")
        if (_effects.count > 0) {
            text = text + " (" + _effects.map { |e| "%(e[0]) %(RANK_NAMES[e[1]])" }.join(", ") + ")"
        }
        return text
    }
}

class Loadout {
    // main / off: GearItems over WeaponTypes (or null - bare hands / nothing). armour: a GearItem over an ArmourType
    // (or null). A two-handed main weapon leaves no off-hand.
    construct new(main, off, armour) {
        _main = main
        _off = (main != null && main.type.twoHanded) ? null : off
        _armour = armour
    }

    static bare { Loadout.new(null, null, null) }

    main { _main }
    off { _off }
    armour { _armour }
    shield { (_off != null && _off.type.isShield) ? _off : null }

    // --- what a move may use ---------------------------------------------------------------

    // Whether `move` can be used with what is held, and the item whose power / stagger / element it uses (main
    // hand first): [allowed, item or null].
    itemFor(move) {
        var tags = move.weapons
        if (tags == null) {
            // Universal: damage uses gauntlet power if worn, otherwise nothing.
            var fists = (_main != null && _main.type.family == "gauntlets") ? _main : null
            return [true, fists]
        }
        for (tag in tags) {
            if (tag == "blade+shield") {
                if (_main != null && _main.type.isBlade1H && shield != null) return [true, _main]
            } else if (tag == "unarmed") {
                if (_main == null) return [true, null]
                if (_main.type.family == "gauntlets") return [true, _main]
            } else {
                if (_main != null && _main.type.tags.contains(tag)) return [true, _main]
                if (_off != null && _off.type.tags.contains(tag)) return [true, _off]
            }
        }
        return [false, null]
    }

    allows(move) { itemFor(move)[0] }

    // --- taxes and passive numbers ---------------------------------------------------------

    // Stamina added to every move except Hold. Heavy adds 20% to the piece that carries it (its capstone lifts it).
    tax {
        var weapons = (_main != null ? _main.tax : 0) + (_off != null ? _off.tax : 0)
        var armour = _armour != null ? _armour.tax : 0
        if (weaponRank("Heavy") > 0 && !capstone("Heavy")) weapons = weapons * 1.2
        if (armourRank("Heavy") > 0 && !capstone("Heavy")) armour = armour * 1.2
        return [weapons, armour]
    }

    armourDefence { _armour != null ? _armour.defence : 0 }
    staggerRes { _armour != null ? _armour.type.staggerRes : 0 }
    surcharge { _armour != null ? _armour.type.surcharge : 0 }
    manaBonus { _armour != null ? _armour.type.mana : 0 }

    // --- elements ------------------------------------------------------------------------

    weaponElement { _main != null ? _main.element : null }
    armourElement { _armour != null ? _armour.element : null }
    pairing { Elements.pairing(weaponElement, armourElement) }
    matched(element) { weaponElement == element && armourElement == element }
    weaponLate { _main != null && _main.late }
    armourLate { _armour != null && _armour.late }

    // --- custom effects ------------------------------------------------------------------

    rawWeaponRank_(effect) {
        var a = _main != null ? _main.rank(effect) : 0
        var b = _off != null ? _off.rank(effect) : 0
        return a > b ? a : b
    }
    rawArmourRank_(effect) { _armour != null ? _armour.rank(effect) : 0 }

    // The weapon / armour version's rank after Resonance (0 = not carried).
    weaponRank(effect) {
        var w = rawWeaponRank_(effect)
        if (w == 0) return 0
        return (rawArmourRank_(effect) > 0 && w < 3) ? w + 1 : w
    }
    armourRank(effect) {
        var a = rawArmourRank_(effect)
        if (a == 0) return 0
        return (rawWeaponRank_(effect) > 0 && a < 3) ? a + 1 : a
    }
    // Both versions carried at rank III: the effect's capstone.
    capstone(effect) { rawWeaponRank_(effect) == 3 && rawArmourRank_(effect) == 3 }

    // "Arming Sword + Heater Shield / Chain Mail".
    label {
        var hands = _main != null ? _main.label : "Unarmed"
        if (_off != null) hands = hands + " + " + _off.label
        return hands + " / " + (_armour != null ? _armour.label : "No armour")
    }
}
