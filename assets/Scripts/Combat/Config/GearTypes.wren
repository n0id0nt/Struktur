// Combat/Config/GearTypes.wren
// Weapon and armour TYPES from the weapon, move and armour types doc. A type decides which moves can be equipped
// (its tags - see Combat/Config/Moves.wren), how hard they hit (power), how much they stagger (staggerMul) and what
// every move costs (tax). It never changes a move's timing. Numbers are early-game quality; a specific item
// (Combat/Gear.wren - GearItem) scales power / defence by its quality and adds an element and custom effects. Tax,
// stagger multiplier, surcharge, resistance and Mana belong to the type and never scale.
class WeaponType {
    // hands: "1H", "2H" or "off" (off-hand only). scales: "attack" or "mana". tags: what its moves can ask for.
    construct new(id, name, family, hands, scales, power, tax, staggerMul, tags) {
        _id = id
        _name = name
        _family = family
        _hands = hands
        _scales = scales
        _power = power
        _tax = tax
        _staggerMul = staggerMul
        _tags = tags
        _guard = 0
        _rangeSides = 0
        _rangeBehind = 0
        _offhandOk = hands == "off"
        _note = ""
    }

    // A shield: its guard bonus (added to shield moves' defence) and the ally cells its moves cover.
    shield(guard, rangeSides, rangeBehind) {
        _guard = guard
        _rangeSides = rangeSides
        _rangeBehind = rangeBehind
        return this
    }
    // A one-handed weapon that may also sit in the off-hand (the wand).
    alsoOffhand() {
        _offhandOk = true
        return this
    }
    note(text) {
        _note = text
        return this
    }

    id { _id }
    name { _name }
    family { _family }
    hands { _hands }
    scales { _scales }
    power { _power }
    tax { _tax }
    staggerMul { _staggerMul }
    tags { _tags }
    guard { _guard }
    rangeSides { _rangeSides }
    rangeBehind { _rangeBehind }
    offhandOk { _offhandOk }
    isShield { _family == "shield" }
    twoHanded { _hands == "2H" }
    note { _note }
    // A one-handed blade for the sword-and-shield moves: dagger, light sword, arming sword or short spear.
    isBlade1H { _hands == "1H" && (_family == "dagger" || _family == "lightsword" || _family == "sword" || _id == "shortspear") }
}

class ArmourType {
    // staggerRes: share of every set-back ignored (negative = set back further). surcharge: extra stamina on Move /
    // Dodge moves only. mana: added to the wearer's Mana.
    construct new(id, name, weight, defence, tax, surcharge, staggerRes, mana) {
        _id = id
        _name = name
        _weight = weight
        _defence = defence
        _tax = tax
        _surcharge = surcharge
        _staggerRes = staggerRes
        _mana = mana
    }

    id { _id }
    name { _name }
    weight { _weight }
    defence { _defence }
    tax { _tax }
    surcharge { _surcharge }
    staggerRes { _staggerRes }
    mana { _mana }
}

class GearTypes {
    // Every weapon / shield type, in the doc's order. Off-hand-only types come up in off-hand pickers only.
    static weapons {
        if (__weapons == null) {
            __weapons = [
                WeaponType.new("knife", "Knife", "dagger", "1H", "attack", 3, 0, 0.7, ["dagger"]),
                WeaponType.new("stiletto", "Stiletto", "dagger", "1H", "attack", 4, 0, 0.6, ["dagger"]).note("Backstab x2.5"),
                WeaponType.new("parrydagger", "Parrying Dagger", "dagger", "off", "attack", 2, 0, 0.6, ["parrydagger"]).note("Unlocks Parry"),
                WeaponType.new("rapier", "Rapier", "lightsword", "1H", "attack", 5, 1, 0.75, ["lightsword"]),
                WeaponType.new("sabre", "Sabre", "lightsword", "1H", "attack", 6, 1, 0.85, ["lightsword"]),
                WeaponType.new("armingsword", "Arming Sword", "sword", "1H", "attack", 7, 3, 1.0, ["sword"]).note("The baseline weapon"),
                WeaponType.new("longsword", "Long Sword", "longsword", "2H", "attack", 10, 4, 1.15, ["longsword"]),
                WeaponType.new("claymore", "Claymore", "claymore", "2H", "attack", 15, 7, 1.4, ["claymore"]).note("Side cells x1.15"),
                WeaponType.new("buckler", "Buckler", "shield", "off", "attack", 2, 1, 1.0, ["shield", "buckler"]).shield(6, 0, 0),
                WeaponType.new("heater", "Heater Shield", "shield", "off", "attack", 4, 2, 1.0, ["shield", "heater"]).shield(12, 0.5, 0),
                WeaponType.new("tower", "Tower Shield", "shield", "off", "attack", 6, 4, 1.0, ["shield", "tower"]).shield(20, 0.75, 1),
                WeaponType.new("shortspear", "Short Spear", "spear", "1H", "attack", 6, 2, 0.9, ["spear", "shortspear"]),
                WeaponType.new("spear", "Spear", "spear", "2H", "attack", 9, 3, 1.0, ["spear", "longspear"]).note("Depth cells x1.25"),
                WeaponType.new("pike", "Pike", "spear", "2H", "attack", 11, 4, 1.0, ["spear", "pike"]).note("Reach 1 at half"),
                WeaponType.new("shortbow", "Short Bow", "bow", "2H", "attack", 5, 1, 0.8, ["bow", "shortbow"]),
                WeaponType.new("longbow", "Longbow", "bow", "2H", "attack", 9, 3, 1.0, ["bow", "longbow"]),
                WeaponType.new("warbow", "Warbow", "bow", "2H", "attack", 14, 6, 1.3, ["bow", "warbow"]).note("Aimed Shot pierces"),
                WeaponType.new("crossbow", "Crossbow", "crossbow", "2H", "attack", 12, 3, 1.2, ["crossbow"]).note("Must be loaded"),
                WeaponType.new("handwraps", "Hand Wraps", "gauntlets", "1H", "attack", 2, 0, 1.0, ["gauntlets", "unarmed"]),
                WeaponType.new("gauntlets", "Gauntlets", "gauntlets", "1H", "attack", 4, 1, 1.2, ["gauntlets", "unarmed"]),
                WeaponType.new("cestus", "Cestus", "gauntlets", "1H", "attack", 6, 2, 1.3, ["gauntlets", "unarmed"]),
                WeaponType.new("wand", "Wand", "wand", "1H", "mana", 4, 1, 0.6, ["wand"]).alsoOffhand(),
                WeaponType.new("rod", "Rod", "wand", "1H", "mana", 6, 2, 0.8, ["wand"]),
                WeaponType.new("magicstaff", "Magic Staff", "staff", "2H", "mana", 8, 3, 0.8, ["staff", "magicstaff"]),
                WeaponType.new("quarterstaff", "Quarterstaff", "staff", "2H", "attack", 6, 2, 1.1, ["staff", "quarterstaff"])
            ]
        }
        return __weapons
    }

    static armours {
        if (__armours == null) {
            __armours = [
                ArmourType.new("clothes", "Clothes", "None", 2, 0, 0, 0, 0),
                ArmourType.new("robes", "Robes", "Light", 3, 0, 0, -0.10, 8),
                ArmourType.new("martialgarb", "Martial Garb", "Light", 5, 0, -1, 0.10, 0),
                ArmourType.new("battlerobes", "Battle Robes", "Light", 7, 1, 0, 0, 4),
                ArmourType.new("padded", "Padded", "Light", 6, 1, 0, 0.10, 0),
                ArmourType.new("leather", "Leather", "Light", 8, 1, 0, 0.05, 0),
                ArmourType.new("hide", "Hide", "Medium", 10, 2, 0, 0.20, -2),
                ArmourType.new("brigandine", "Brigandine", "Medium", 12, 2, 1, 0.15, -2),
                ArmourType.new("chainmail", "Chain Mail", "Medium", 16, 3, 1, 0.20, -5),
                ArmourType.new("scalemail", "Scale Mail", "Heavy", 20, 4, 1, 0.25, -8),
                ArmourType.new("halfplate", "Half Plate", "Heavy", 24, 5, 2, 0.35, -10),
                ArmourType.new("fullplate", "Full Plate", "Heavy", 30, 7, 3, 0.50, -15)
            ]
        }
        return __armours
    }

    static weapon(id) {
        for (weapon in GearTypes.weapons) {
            if (weapon.id == id) return weapon
        }
        return null
    }

    static armour(id) {
        for (armour in GearTypes.armours) {
            if (armour.id == id) return armour
        }
        return null
    }

    // What can go in the main hand / the off-hand - the battle test's pickers.
    static mainHands { GearTypes.weapons.where { |weapon| weapon.hands != "off" }.toList }
    static offHands { GearTypes.weapons.where { |weapon| weapon.offhandOk }.toList }
}
