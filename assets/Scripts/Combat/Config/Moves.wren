// Combat/Config/Moves.wren
// The move catalog of the animation-timed battle system - every human move from the weapon, move and armour types
// doc, by weapon family, each in up to three tiers (Combat/Move.wren scales damage / heal / defence / stagger / cost
// by tier; the builders below add what a tier gains: a wider stencil, a bigger skip, a hit-reaction override...).
// Times never differ between tiers.
//
//   Moves.get("Slash", 2)     a fresh tier II Slash           Moves.byName("Slash II")   the same, from a saved name
//   Moves.hold(0.8)           a Hold of that length           Moves.ids                  every id, in menu order
//
// `weapons` tags gate a move by what is held (Combat/Gear.wren - Loadout.itemFor): a family ("sword", "bow"...), a
// subtype ("pike", "buckler"...), "shield" (any shield), "blade+shield" (a one-handed blade with a shield) or
// "unarmed" (bare hands or gauntlets). A move with no tags is universal.
//
// anim: the role of the clip the move plays (Combat/Config/SheetAnims.wren maps roles to a sheet's clips).
import "Combat/Move" for Move

var HOLD_LENGTHS = [0.2, 0.4, 0.6, 0.8, 1.2, 2.0]

// Moves that only exist from tier II up (multi-direction and team-protecting moves are mid-game unlocks).
var MIN_TIER = {
    "Spin Slash": 2, "Whirlwind": 2, "Shield Wall": 2, "Twin Cleave": 2, "Vaulting Thrust": 2, "Volley": 2,
    "Throw": 2, "Barrier": 2, "Blink": 2
}

var BLADES_LIGHT = ["dagger", "lightsword"]
var SWORDS = ["lightsword", "sword", "longsword"]
var HEAVY_BLADES = ["sword", "longsword", "claymore"]
var MAGIC = ["wand", "magicstaff"]

class Moves {
    static holdLengths { HOLD_LENGTHS }

    static hold(seconds) { Move.hold(seconds) }

    static minTier(id) { MIN_TIER.containsKey(id) ? MIN_TIER[id] : 1 }

    // A fresh move `id` at `tier` (raised to the move's lowest tier), or null if there is no such move.
    static get(id, tier) {
        var defs = Moves.defs_
        if (!defs.containsKey(id)) return null
        var t = tier < Moves.minTier(id) ? Moves.minTier(id) : tier
        if (t > 3) t = 3
        return defs[id].call(t)
    }

    static get(id) { Moves.get(id, 1) }

    // "Slash", "Slash II", "Slash III" -> the move (how saved move names come back - Combat/MoveBook.wren).
    static byName(name) {
        if (name == null) return null
        if (name.endsWith(" III")) return Moves.get(name[0...(name.count - 4)], 3)
        if (name.endsWith(" II")) return Moves.get(name[0...(name.count - 3)], 2)
        return Moves.get(name, 1)
    }

    // Every move id, grouped as in the types doc - the battle test's pickers cycle through this.
    static ids {
        if (__ids == null) {
            __ids = ["Step", "Roll", "Brace", "Shove", "Back Kick", "Soul Step",
                "Stab", "Backstab", "Slip Strike", "Throwing Knife", "Parry",
                "Slash", "Lunge", "Riposte", "Pommel Strike", "Cross Cut", "Spin Slash",
                "Overhead Cleave", "Half-sword Thrust", "Hanging Guard",
                "Wide Sweep", "Crushing Overhead", "Whirlwind", "Blade Block",
                "Block", "Shield Bash", "Shield Charge", "Shield Wall", "Buckler Parry",
                "Guarded Thrust", "Counter Cut", "Phalanx Step",
                "Thrust", "Long Thrust", "Set Spear", "Twin Cleave", "Vaulting Thrust",
                "Quick Shot", "Aimed Shot", "Retreating Shot", "Volley", "Point-blank Shot",
                "Bolt", "Reload",
                "Jab", "Hook", "Uppercut", "Palm Strike", "Catch", "Throw",
                "Spark", "Arcane Bolt", "Mend", "Ward", "Blink",
                "Blast", "Mending Circle", "Barrier", "Staff Strike", "Sweep", "Pole Vault"]
        }
        return __ids
    }

    static defs_ {
        if (__defs == null) {
            __defs = {}
            Moves.universal_(__defs)
            Moves.blades_(__defs)
            Moves.shields_(__defs)
            Moves.polearms_(__defs)
            Moves.ranged_(__defs)
            Moves.fists_(__defs)
            Moves.magic_(__defs)
        }
        return __defs
    }

    // --- universal ---------------------------------------------------------------------

    static universal_(d) {
        // Any orthogonal cell from tier I (the starter Step of the first arc); any of 8 with a short dodge at III.
        d["Step"] = Fn.new { |t|
            var m = Move.new("Step", t, 0.6, 1).movement(0.3, t >= 3 ? "any" : "ortho", "stay").anim("step")
            if (t >= 3) m.dodge(0.0, 0.2)
            return m
        }
        d["Roll"] = Fn.new { |t|
            return Move.new("Roll", t, 0.8, 4).movement(0.3, "any", "stay").dodge(0.0, [0, 0.3, 0.4, 0.5][t]).anim("roll")
        }
        // The fusion's move (first arc, scene 9): a Roll under another name.
        d["Soul Step"] = Fn.new { |t|
            return Move.new("Soul Step", t, 0.8, 4).movement(0.3, "any", "stay").dodge(0.0, [0, 0.3, 0.4, 0.5][t]).anim("roll")
        }
        // Tuck in and take the hit. Also covers the ally directly behind (the bodyguard guard range).
        d["Brace"] = Fn.new { |t|
            return Move.new("Brace", t, 1.1, 3).defend(0.1, 0.5, 0.8, 8, t >= 3 ? 0.4 : 0.3).guard(-1, 0, 1).anim("guard")
        }
        d["Shove"] = Fn.new { |t|
            var m = Move.new("Shove", t, 0.8, 3).attackFlat(0.4, [0, 4, 6, 8][t], [0, 0.5, 0.55, 0.6][t]).reach(1).anim("punch")
            if (t >= 3) m.push(1)
            return m
        }
        d["Back Kick"] = Fn.new { |t|
            var cells = [[-1, 0, 1]]
            if (t >= 2) {
                cells.add([-1, -1, 0.5])
                cells.add([-1, 1, 0.5])
            }
            var m = Move.new("Back Kick", t, 0.9, 4).attack(0.45, 8, 0.4).around(cells).anim("punch")
            if (t >= 3) m.push(1)
            return m
        }
    }

    // --- blades: dagger, light sword, sword, long sword, claymore --------------------

    static blades_(d) {
        d["Stab"] = Fn.new { |t|
            return Move.new("Stab", t, 0.5, 2).costFixed(t >= 3 ? 2 : [0, 2, 3][t]).attack(0.25, 8, 0.2).reach(1).weapons(BLADES_LIGHT).anim("thrust")
        }
        d["Backstab"] = Fn.new { |t|
            return Move.new("Backstab", t, 0.8, 3).attack(0.4, 10, 0.3).reach(1).backstab(t >= 3 ? 2.5 : 2).weapons(["dagger"]).anim("thrust")
        }
        d["Slip Strike"] = Fn.new { |t|
            return Move.new("Slip Strike", t, 0.9, 5).movement(0.2, "diag", "stay").attack(0.45, 8, 0.2).reach(1).dodge(0.0, [0, 0.3, 0.35, 0.4][t]).weapons(BLADES_LIGHT).anim("slash")
        }
        d["Throwing Knife"] = Fn.new { |t|
            var m = Move.new("Throwing Knife", t, 0.7, 3).attackFlat(0.35, [0, 6, 9, 12][t], [0, 0.15, 0.2, 0.2][t]).first().weapons(["dagger"]).anim("shoot")
            if (t >= 3) m.sides(0.3)
            return m
        }
        d["Parry"] = Fn.new { |t|
            var m = Move.new("Parry", t, 0.6, 2).defend(0.05, 0.15, 0.25, 20, t >= 3 ? 0.5 : 0.4).weapons(["parrydagger", "dagger", "lightsword"]).anim("guard")
            if (t >= 2) m.react("deflect")
            return m
        }
        d["Slash"] = Fn.new { |t|
            var m = Move.new("Slash", t, 1.0, 6).attack(0.5, 18, 0.3).reach(1).weapons(SWORDS).anim("slash")
            if (t >= 3) m.sides(0.25)
            return m
        }
        d["Lunge"] = Fn.new { |t|
            return Move.new("Lunge", t, 1.1, 8).movement(0.3, "forward", t >= 3 ? "short" : "stay").attack(0.5, 20, 0.3).reach(1).dodge(0.0, t >= 3 ? 0.4 : 0.3).weapons(["lightsword", "sword"]).anim("heavy")
        }
        d["Riposte"] = Fn.new { |t|
            var m = Move.new("Riposte", t, 0.8, 5).defendFlat(0.1, 0.2, 0.4, [0, 25, 35, 45][t], t >= 3 ? 0.6 : 0.5).weapons(["lightsword", "sword"]).anim("guard")
            if (t >= 2) m.react("deflect")
            return m
        }
        // Little damage, hard stagger: the cheap interrupt for heavier blades.
        d["Pommel Strike"] = Fn.new { |t|
            var m = Move.new("Pommel Strike", t, 0.7, 4).attackFlat(0.35, [0, 8, 10, 12][t], [0, 0.5, 0.6, 0.7][t]).reach(1).weapons(HEAVY_BLADES).anim("punch")
            if (t >= 3) m.push(1)
            return m
        }
        d["Cross Cut"] = Fn.new { |t|
            return Move.new("Cross Cut", t, 1.3, 8).attack(0.7, 16, 0.35).reach(1).sides(t >= 3 ? 0.75 : 0.5).weapons(HEAVY_BLADES).anim("slash")
        }
        d["Spin Slash"] = Fn.new { |t|
            return Move.new("Spin Slash", t, 1.5, 11).attack(0.8, 20, 0.35).around8(t >= 3 ? 0.7 : 0.6).weapons(HEAVY_BLADES).anim("spin")
        }
        d["Overhead Cleave"] = Fn.new { |t|
            var m = Move.new("Overhead Cleave", t, 1.7, 14).attack(0.9, 32, 0.6).reach(1).weapons(["longsword", "claymore"]).anim("heavy")
            if (t >= 3) m.react("steadfast")
            return m
        }
        d["Half-sword Thrust"] = Fn.new { |t|
            var m = Move.new("Half-sword Thrust", t, 1.2, 9).attack(0.6, 22, 0.4).reach(1).depth(0.4).weapons(["longsword"]).anim("thrust")
            if (t >= 3) m.react("steadfast")
            return m
        }
        d["Hanging Guard"] = Fn.new { |t|
            var m = Move.new("Hanging Guard", t, 1.4, 5).defend(0.1, 0.6, 1.1, 20, 0.3).weapons(["longsword"]).anim("guard")
            if (t >= 3) m.guardSides(0.5)
            return m
        }
        d["Wide Sweep"] = Fn.new { |t|
            return Move.new("Wide Sweep", t, 1.9, 16).attack(1.0, 34, 0.6).reach(1).sides(t >= 3 ? 1 : 0.8).weapons(["claymore"]).anim("spin")
        }
        d["Crushing Overhead"] = Fn.new { |t|
            var m = Move.new("Crushing Overhead", t, 2.4, 22).attack(1.3, 48, 0.9).reach(1).sides(0.3).weapons(["claymore"]).anim("heavy")
            if (t == 2) m.react("steadfast")
            if (t >= 3) m.react("unflinching")
            return m
        }
        d["Whirlwind"] = Fn.new { |t|
            var m = Move.new("Whirlwind", t, 2.2, 16).attackFlat(1.2, t >= 3 ? 80 : 60, t >= 3 ? 0.7 : 0.6).around8(0.7).weapons(["claymore"]).anim("spin")
            if (t >= 3) m.react("steadfast")
            return m
        }
        d["Blade Block"] = Fn.new { |t|
            return Move.new("Blade Block", t, 1.6, 6).defend(0.2, 0.7, 1.2, 25, t >= 3 ? 0.5 : 0.3).weapons(["claymore"]).anim("guard")
        }
    }

    // --- shields, and sword-and-shield ------------------------------------------------

    static shields_(d) {
        d["Block"] = Fn.new { |t|
            return Move.new("Block", t, 1.2, 3).defend(0.1, 0.5, 0.9, 15, t >= 3 ? 0.4 : 0.3).withShield().weapons(["shield"]).anim("guard")
        }
        d["Shield Bash"] = Fn.new { |t|
            var m = Move.new("Shield Bash", t, 1.1, 6).defend(0.1, 0.3, 0.5, 10, 0.3).attackFlat(0.6, [0, 10, 15, 20][t], [0, 0.6, 0.7, 0.8][t]).reach(1).weapons(["shield"]).anim("punch")
            if (t >= 3) m.push(1)
            return m
        }
        d["Shield Charge"] = Fn.new { |t|
            var m = Move.new("Shield Charge", t, 1.2, 8).movement(0.3, "forward", "short").defend(0.0, 0.3, 0.5, 10, 0.3).attack(0.6, 12, 0.8).reach(1).weapons(["shield"]).anim("heavy")
            if (t >= 3) m.push(1)
            return m
        }
        // Sides raised to 100% of the wall, on top of whatever the shield already covers.
        d["Shield Wall"] = Fn.new { |t|
            return Move.new("Shield Wall", t, 2.0, 5).defendFlat(0.1, 0.8, 1.6, t >= 3 ? 36 : 27, t >= 3 ? 0.3 : 0.2).withShield().guardSides(1).weapons(["heater", "tower"]).anim("guard")
        }
        d["Buckler Parry"] = Fn.new { |t|
            var m = Move.new("Buckler Parry", t, 0.6, 2).defendFlat(0.05, 0.15, 0.25, 20, t >= 3 ? 0.6 : 0.4).withShield().weapons(["buckler"]).anim("guard")
            if (t >= 2) m.react("deflect")
            return m
        }
        d["Guarded Thrust"] = Fn.new { |t|
            var m = Move.new("Guarded Thrust", t, 1.3, 8).defend(0.1, 0.4, 0.6, 15, 0.3).attack(0.7, 16, 0.3).reach(1).weapons(["blade+shield"]).anim("thrust")
            if (t >= 3) m.depth(0.3)
            return m
        }
        // Built on the skip rule: blocking in its guard window skips the animation toward its own impact.
        d["Counter Cut"] = Fn.new { |t|
            var m = Move.new("Counter Cut", t, 1.4, 7).defend(0.1, 0.3, 0.5, 18, t >= 2 ? 0.4 : 0.3).attack(0.8, 18, 0.3).reach(1).weapons(["blade+shield"]).anim("slash")
            if (t >= 3) m.react("deflect")
            return m
        }
        d["Phalanx Step"] = Fn.new { |t|
            return Move.new("Phalanx Step", t, 0.9, 4).movement(0.3, t >= 3 ? "ortho" : "forward", "stay").defend(0.0, 0.3, 0.6, 15, 0.3).withShield().weapons(["blade+shield"]).anim("step")
        }
    }

    // --- spears -------------------------------------------------------------------------

    static polearms_(d) {
        d["Thrust"] = Fn.new { |t|
            return Move.new("Thrust", t, 1.2, 7).attack(0.6, 20, 0.4).reach(1).depth(t >= 3 ? 0.75 : 0.5).weapons(["spear"]).anim("thrust")
        }
        d["Long Thrust"] = Fn.new { |t|
            var m = Move.new("Long Thrust", t, 1.3, 8).attack(0.7, 18, 0.4).reach(2).weapons(["longspear", "pike"]).anim("thrust")
            if (t >= 3) m.depth(0.5)
            return m
        }
        d["Set Spear"] = Fn.new { |t|
            var m = Move.new("Set Spear", t, 1.4, 4).defend(0.1, 0.6, 1.1, 18, t >= 3 ? 0.4 : 0.3).weapons(["spear"]).anim("guard")
            if (t >= 2) m.react("deflect")
            return m
        }
        d["Twin Cleave"] = Fn.new { |t|
            return Move.new("Twin Cleave", t, 1.3, 10).attack(0.7, 18, 0.35).around([[1, 0, 1], [-1, 0, 1]]).weapons(["longspear"]).anim("spin")
        }
        d["Vaulting Thrust"] = Fn.new { |t|
            var m = Move.new("Vaulting Thrust", t, 1.2, 8).movement(0.3, "forward", "stay").dodge(0.1, 0.4).attack(0.6, 22, 0.4).reach(1).depth(0.5).weapons(["shortspear", "longspear"]).anim("heavy")
            if (t >= 3) m.react("steadfast")
            return m
        }
    }

    // --- bows and the crossbow -----------------------------------------------------------

    static ranged_(d) {
        d["Quick Shot"] = Fn.new { |t|
            return Move.new("Quick Shot", t, 0.8, 3).costFixed(t >= 3 ? 3 : [0, 3, 4][t]).attackFlat(0.4, [0, 10, 15, 20][t], [0, 0.15, 0.2, 0.2][t]).first().weapons(["bow"]).anim("shoot")
        }
        d["Aimed Shot"] = Fn.new { |t|
            var m = Move.new("Aimed Shot", t, 1.6, 9).attack(1.0, 26, 0.4).first().weapons(["bow"]).anim("shoot")
            if (t >= 3) m.depth(0.5)
            return m
        }
        d["Retreating Shot"] = Fn.new { |t|
            var m = Move.new("Retreating Shot", t, 1.0, 5).movement(0.2, "back", "stay").attackFlat(0.6, [0, 12, 18, 24][t], [0, 0.2, 0.2, 0.25][t]).first().weapons(["bow"]).anim("shoot")
            if (t >= 3) m.dodge(0.0, 0.2)
            return m
        }
        d["Volley"] = Fn.new { |t|
            var share = t >= 3 ? 0.75 : 0.6
            return Move.new("Volley", t, 2.0, 10).attack(1.3, 18, 0.25).reach(2).depth(share).sides(share).weapons(["longbow", "warbow"]).anim("shoot")
        }
        d["Point-blank Shot"] = Fn.new { |t|
            return Move.new("Point-blank Shot", t, 0.7, 3).attack(0.3, 12, 0.3).reach(1).weapons(["shortbow"]).anim("shoot")
        }
        // Needs a loaded bolt (Combat/BattleEngine.wren - a Bolt with none loaded falters); Reload loads one.
        d["Bolt"] = Fn.new { |t|
            var m = Move.new("Bolt", t, 0.6, 5).attack(0.3, 30, 0.5).first().weapons(["crossbow"]).special("bolt").anim("shoot")
            if (t >= 3) m.depth(0.4)
            return m
        }
        d["Reload"] = Fn.new { |t|
            var m = Move.new("Reload", t, 1.2, 0).costFixed(0).weapons(["crossbow"]).special("reload").anim("guard")
            if (t >= 2) m.dodge(0.0, 0.3)
            if (t >= 3) m.defendFlat(0.4, 0.8, 1.1, 15, 0.2)
            return m
        }
    }

    // --- gauntlets ---------------------------------------------------------------------

    static fists_(d) {
        d["Jab"] = Fn.new { |t|
            return Move.new("Jab", t, 0.45, 2).attack(0.2, 6, 0.2).reach(1).weapons(["unarmed"]).damageAnim(0.2).anim("punch")
        }
        d["Hook"] = Fn.new { |t|
            return Move.new("Hook", t, 0.8, 4).attack(0.4, 12, 0.35).reach(1).weapons(["unarmed"]).anim("punch")
        }
        d["Uppercut"] = Fn.new { |t|
            var m = Move.new("Uppercut", t, 1.1, 7).attack(0.6, 20, 0.7).reach(1).weapons(["gauntlets"]).anim("heavy")
            if (t >= 3) m.react("unflinching")
            return m
        }
        d["Palm Strike"] = Fn.new { |t|
            var m = Move.new("Palm Strike", t, 0.9, 5).attack(0.45, 10, 0.5).reach(1).push(1).weapons(["gauntlets"]).anim("punch")
            if (t >= 3) m.depth(0.5)
            return m
        }
        d["Catch"] = Fn.new { |t|
            var m = Move.new("Catch", t, 0.6, 2).defendFlat(0.05, 0.15, 0.3, [0, 15, 25, 35][t], t >= 3 ? 0.6 : 0.5).weapons(["gauntlets"]).anim("guard")
            if (t >= 2) m.react("deflect")
            return m
        }
        // Moves the target to the cell behind the thrower, if it is free.
        d["Throw"] = Fn.new { |t|
            return Move.new("Throw", t, 1.3, 6).attackFlat(0.7, t >= 3 ? 20 : 15, t >= 3 ? 0.7 : 0.6).reach(1).special("throw").weapons(["unarmed"]).anim("heavy")
        }
    }

    // --- wands and staffs ----------------------------------------------------------------

    static magic_(d) {
        d["Spark"] = Fn.new { |t|
            return Move.new("Spark", t, 0.7, 2).attackFlat(0.35, [0, 8, 12, 16][t], [0, 0.15, 0.15, 0.2][t]).first().mana().weapons(["wand"]).anim("cast")
        }
        d["Arcane Bolt"] = Fn.new { |t|
            var m = Move.new("Arcane Bolt", t, 1.4, 8).attack(0.8, 22, 0.2).first().mana().weapons(MAGIC).anim("cast")
            if (t >= 3) m.depth(0.3)
            return m
        }
        // Heals the caster and the ally in the cell ahead.
        d["Mend"] = Fn.new { |t|
            var cells = [[0, 0, 1], [1, 0, 1]]
            if (t >= 3) {
                cells.add([0, -1, 0.5])
                cells.add([0, 1, 0.5])
            }
            return Move.new("Mend", t, 1.2, 6).heal(0.6, 14).around(cells).mana().weapons(MAGIC).anim("cast")
        }
        d["Ward"] = Fn.new { |t|
            var m = Move.new("Ward", t, 1.2, 4).defend(0.1, 0.5, 0.9, 15, 0).react("absorb").weapons(MAGIC).anim("guard")
            if (t >= 3) m.guardSides(0.5)
            return m
        }
        d["Blink"] = Fn.new { |t|
            return Move.new("Blink", t, 0.7, 4).movement(0.3, "blink", "stay").dodge(t >= 3 ? 0.0 : 0.1, t >= 3 ? 0.4 : 0.3).weapons(["wand"]).anim("roll")
        }
        d["Blast"] = Fn.new { |t|
            var near = t >= 3 ? 0.45 : 0.3
            var far = t >= 3 ? 0.25 : 0.15
            return Move.new("Blast", t, 1.8, 12).attack(1.0, 30, 0.4).first().depth(near).sides(near).diagonals(far).mana().weapons(["magicstaff"]).anim("cast")
        }
        d["Mending Circle"] = Fn.new { |t|
            var m = Move.new("Mending Circle", t, 1.8, 12).heal(1.0, 20).around8([0, 0.6, 0.8, 1][t]).mana().weapons(["magicstaff"]).anim("cast")
            m.stencil.add([0, 0, 1])
            return m
        }
        d["Barrier"] = Fn.new { |t|
            var m = Move.new("Barrier", t, 1.8, 6).defendFlat(0.2, 0.8, 1.4, t >= 3 ? 24 : 18, 0).react("absorb").weapons(["magicstaff"]).anim("guard")
            var share = t >= 3 ? 1 : 0.75
            for (dz in -1..1) {
                for (s in -1..1) {
                    if (dz != 0 || s != 0) m.guard(dz, s, share)
                }
            }
            return m
        }
        // Physical: scales with Attack even on a magic staff.
        d["Staff Strike"] = Fn.new { |t|
            return Move.new("Staff Strike", t, 0.8, 2).attack(0.4, 6, 0.4).reach(1).weapons(["staff"]).anim("slash")
        }
        d["Sweep"] = Fn.new { |t|
            var m = Move.new("Sweep", t, 1.2, 6).attack(0.6, 12, 0.5).reach(1).sides(0.6).weapons(["quarterstaff"]).anim("spin")
            if (t >= 3) m.push(1)
            return m
        }
        d["Pole Vault"] = Fn.new { |t|
            return Move.new("Pole Vault", t, 0.8, 4).movement(0.4, "forward2", "short").dodge(0.1, t >= 2 ? 0.5 : 0.4).weapons(["quarterstaff"]).anim("roll")
        }
    }
}
