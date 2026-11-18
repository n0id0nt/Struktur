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
        var clampedTier = tier < Moves.minTier(id) ? Moves.minTier(id) : tier
        if (clampedTier > 3) clampedTier = 3
        return defs[id].call(clampedTier)
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

    static universal_(defs) {
        // Any orthogonal cell from tier I (the starter Step of the first arc); any of 8 with a short dodge at III.
        defs["Step"] = Fn.new { |tier|
            var move = Move.new("Step", tier, 0.6, 1).movement(0.3, tier >= 3 ? "any" : "ortho", "stay").anim("step")
            if (tier >= 3) move.dodge(0.0, 0.2)
            return move
        }
        defs["Roll"] = Fn.new { |tier|
            return Move.new("Roll", tier, 0.8, 4).movement(0.3, "any", "stay").dodge(0.0, [0, 0.3, 0.4, 0.5][tier]).anim("roll")
        }
        // The fusion's move (first arc, scene 9): a Roll under another name.
        defs["Soul Step"] = Fn.new { |tier|
            return Move.new("Soul Step", tier, 0.8, 4).movement(0.3, "any", "stay").dodge(0.0, [0, 0.3, 0.4, 0.5][tier]).anim("roll")
        }
        // Tuck in and take the hit. Also covers the ally directly behind (the bodyguard guard range).
        defs["Brace"] = Fn.new { |tier|
            return Move.new("Brace", tier, 1.1, 3).defend(0.1, 0.5, 0.8, 8, tier >= 3 ? 0.4 : 0.3).guard(-1, 0, 1).anim("guard")
        }
        defs["Shove"] = Fn.new { |tier|
            var move = Move.new("Shove", tier, 0.8, 3).attackFlat(0.4, [0, 4, 6, 8][tier], [0, 0.5, 0.55, 0.6][tier]).reach(1).anim("punch")
            if (tier >= 3) move.push(1)
            return move
        }
        defs["Back Kick"] = Fn.new { |tier|
            var cells = [[-1, 0, 1]]
            if (tier >= 2) {
                cells.add([-1, -1, 0.5])
                cells.add([-1, 1, 0.5])
            }
            var move = Move.new("Back Kick", tier, 0.9, 4).attack(0.45, 8, 0.4).around(cells).anim("punch")
            if (tier >= 3) move.push(1)
            return move
        }
    }

    // --- blades: dagger, light sword, sword, long sword, claymore --------------------

    static blades_(defs) {
        defs["Stab"] = Fn.new { |tier|
            return Move.new("Stab", tier, 0.5, 2).costFixed(tier >= 3 ? 2 : [0, 2, 3][tier]).attack(0.25, 8, 0.2).reach(1).weapons(BLADES_LIGHT).anim("thrust")
        }
        defs["Backstab"] = Fn.new { |tier|
            return Move.new("Backstab", tier, 0.8, 3).attack(0.4, 10, 0.3).reach(1).backstab(tier >= 3 ? 2.5 : 2).weapons(["dagger"]).anim("thrust")
        }
        defs["Slip Strike"] = Fn.new { |tier|
            return Move.new("Slip Strike", tier, 0.9, 5).movement(0.2, "diag", "stay").attack(0.45, 8, 0.2).reach(1).dodge(0.0, [0, 0.3, 0.35, 0.4][tier]).weapons(BLADES_LIGHT).anim("slash")
        }
        defs["Throwing Knife"] = Fn.new { |tier|
            var move = Move.new("Throwing Knife", tier, 0.7, 3).attackFlat(0.35, [0, 6, 9, 12][tier], [0, 0.15, 0.2, 0.2][tier]).first().weapons(["dagger"]).anim("shoot")
            if (tier >= 3) move.sides(0.3)
            return move
        }
        defs["Parry"] = Fn.new { |tier|
            var move = Move.new("Parry", tier, 0.6, 2).defend(0.05, 0.15, 0.25, 20, tier >= 3 ? 0.5 : 0.4).weapons(["parrydagger", "dagger", "lightsword"]).anim("guard")
            if (tier >= 2) move.react("deflect")
            return move
        }
        defs["Slash"] = Fn.new { |tier|
            var move = Move.new("Slash", tier, 1.0, 6).attack(0.5, 18, 0.3).reach(1).weapons(SWORDS).anim("slash")
            if (tier >= 3) move.sides(0.25)
            return move
        }
        defs["Lunge"] = Fn.new { |tier|
            return Move.new("Lunge", tier, 1.1, 8).movement(0.3, "forward", tier >= 3 ? "short" : "stay").attack(0.5, 20, 0.3).reach(1).dodge(0.0, tier >= 3 ? 0.4 : 0.3).weapons(["lightsword", "sword"]).anim("heavy")
        }
        defs["Riposte"] = Fn.new { |tier|
            var move = Move.new("Riposte", tier, 0.8, 5).defendFlat(0.1, 0.2, 0.4, [0, 25, 35, 45][tier], tier >= 3 ? 0.6 : 0.5).weapons(["lightsword", "sword"]).anim("guard")
            if (tier >= 2) move.react("deflect")
            return move
        }
        // Little damage, hard stagger: the cheap interrupt for heavier blades.
        defs["Pommel Strike"] = Fn.new { |tier|
            var move = Move.new("Pommel Strike", tier, 0.7, 4).attackFlat(0.35, [0, 8, 10, 12][tier], [0, 0.5, 0.6, 0.7][tier]).reach(1).weapons(HEAVY_BLADES).anim("punch")
            if (tier >= 3) move.push(1)
            return move
        }
        defs["Cross Cut"] = Fn.new { |tier|
            return Move.new("Cross Cut", tier, 1.3, 8).attack(0.7, 16, 0.35).reach(1).sides(tier >= 3 ? 0.75 : 0.5).weapons(HEAVY_BLADES).anim("slash")
        }
        defs["Spin Slash"] = Fn.new { |tier|
            return Move.new("Spin Slash", tier, 1.5, 11).attack(0.8, 20, 0.35).around8(tier >= 3 ? 0.7 : 0.6).weapons(HEAVY_BLADES).anim("spin")
        }
        defs["Overhead Cleave"] = Fn.new { |tier|
            var move = Move.new("Overhead Cleave", tier, 1.7, 14).attack(0.9, 32, 0.6).reach(1).weapons(["longsword", "claymore"]).anim("heavy")
            if (tier >= 3) move.react("steadfast")
            return move
        }
        defs["Half-sword Thrust"] = Fn.new { |tier|
            var move = Move.new("Half-sword Thrust", tier, 1.2, 9).attack(0.6, 22, 0.4).reach(1).depth(0.4).weapons(["longsword"]).anim("thrust")
            if (tier >= 3) move.react("steadfast")
            return move
        }
        defs["Hanging Guard"] = Fn.new { |tier|
            var move = Move.new("Hanging Guard", tier, 1.4, 5).defend(0.1, 0.6, 1.1, 20, 0.3).weapons(["longsword"]).anim("guard")
            if (tier >= 3) move.guardSides(0.5)
            return move
        }
        defs["Wide Sweep"] = Fn.new { |tier|
            return Move.new("Wide Sweep", tier, 1.9, 16).attack(1.0, 34, 0.6).reach(1).sides(tier >= 3 ? 1 : 0.8).weapons(["claymore"]).anim("spin")
        }
        defs["Crushing Overhead"] = Fn.new { |tier|
            var move = Move.new("Crushing Overhead", tier, 2.4, 22).attack(1.3, 48, 0.9).reach(1).sides(0.3).weapons(["claymore"]).anim("heavy")
            if (tier == 2) move.react("steadfast")
            if (tier >= 3) move.react("unflinching")
            return move
        }
        defs["Whirlwind"] = Fn.new { |tier|
            var move = Move.new("Whirlwind", tier, 2.2, 16).attackFlat(1.2, tier >= 3 ? 80 : 60, tier >= 3 ? 0.7 : 0.6).around8(0.7).weapons(["claymore"]).anim("spin")
            if (tier >= 3) move.react("steadfast")
            return move
        }
        defs["Blade Block"] = Fn.new { |tier|
            return Move.new("Blade Block", tier, 1.6, 6).defend(0.2, 0.7, 1.2, 25, tier >= 3 ? 0.5 : 0.3).weapons(["claymore"]).anim("guard")
        }
    }

    // --- shields, and sword-and-shield ------------------------------------------------

    static shields_(defs) {
        defs["Block"] = Fn.new { |tier|
            return Move.new("Block", tier, 1.2, 3).defend(0.1, 0.5, 0.9, 15, tier >= 3 ? 0.4 : 0.3).withShield().weapons(["shield"]).anim("guard")
        }
        defs["Shield Bash"] = Fn.new { |tier|
            var move = Move.new("Shield Bash", tier, 1.1, 6).defend(0.1, 0.3, 0.5, 10, 0.3).attackFlat(0.6, [0, 10, 15, 20][tier], [0, 0.6, 0.7, 0.8][tier]).reach(1).weapons(["shield"]).anim("punch")
            if (tier >= 3) move.push(1)
            return move
        }
        defs["Shield Charge"] = Fn.new { |tier|
            var move = Move.new("Shield Charge", tier, 1.2, 8).movement(0.3, "forward", "short").defend(0.0, 0.3, 0.5, 10, 0.3).attack(0.6, 12, 0.8).reach(1).weapons(["shield"]).anim("heavy")
            if (tier >= 3) move.push(1)
            return move
        }
        // Sides raised to 100% of the wall, on top of whatever the shield already covers.
        defs["Shield Wall"] = Fn.new { |tier|
            return Move.new("Shield Wall", tier, 2.0, 5).defendFlat(0.1, 0.8, 1.6, tier >= 3 ? 36 : 27, tier >= 3 ? 0.3 : 0.2).withShield().guardSides(1).weapons(["heater", "tower"]).anim("guard")
        }
        defs["Buckler Parry"] = Fn.new { |tier|
            var move = Move.new("Buckler Parry", tier, 0.6, 2).defendFlat(0.05, 0.15, 0.25, 20, tier >= 3 ? 0.6 : 0.4).withShield().weapons(["buckler"]).anim("guard")
            if (tier >= 2) move.react("deflect")
            return move
        }
        defs["Guarded Thrust"] = Fn.new { |tier|
            var move = Move.new("Guarded Thrust", tier, 1.3, 8).defend(0.1, 0.4, 0.6, 15, 0.3).attack(0.7, 16, 0.3).reach(1).weapons(["blade+shield"]).anim("thrust")
            if (tier >= 3) move.depth(0.3)
            return move
        }
        // Built on the skip rule: blocking in its guard window skips the animation toward its own impact.
        defs["Counter Cut"] = Fn.new { |tier|
            var move = Move.new("Counter Cut", tier, 1.4, 7).defend(0.1, 0.3, 0.5, 18, tier >= 2 ? 0.4 : 0.3).attack(0.8, 18, 0.3).reach(1).weapons(["blade+shield"]).anim("slash")
            if (tier >= 3) move.react("deflect")
            return move
        }
        defs["Phalanx Step"] = Fn.new { |tier|
            return Move.new("Phalanx Step", tier, 0.9, 4).movement(0.3, tier >= 3 ? "ortho" : "forward", "stay").defend(0.0, 0.3, 0.6, 15, 0.3).withShield().weapons(["blade+shield"]).anim("step")
        }
    }

    // --- spears -------------------------------------------------------------------------

    static polearms_(defs) {
        defs["Thrust"] = Fn.new { |tier|
            return Move.new("Thrust", tier, 1.2, 7).attack(0.6, 20, 0.4).reach(1).depth(tier >= 3 ? 0.75 : 0.5).weapons(["spear"]).anim("thrust")
        }
        defs["Long Thrust"] = Fn.new { |tier|
            var move = Move.new("Long Thrust", tier, 1.3, 8).attack(0.7, 18, 0.4).reach(2).weapons(["longspear", "pike"]).anim("thrust")
            if (tier >= 3) move.depth(0.5)
            return move
        }
        defs["Set Spear"] = Fn.new { |tier|
            var move = Move.new("Set Spear", tier, 1.4, 4).defend(0.1, 0.6, 1.1, 18, tier >= 3 ? 0.4 : 0.3).weapons(["spear"]).anim("guard")
            if (tier >= 2) move.react("deflect")
            return move
        }
        defs["Twin Cleave"] = Fn.new { |tier|
            return Move.new("Twin Cleave", tier, 1.3, 10).attack(0.7, 18, 0.35).around([[1, 0, 1], [-1, 0, 1]]).weapons(["longspear"]).anim("spin")
        }
        defs["Vaulting Thrust"] = Fn.new { |tier|
            var move = Move.new("Vaulting Thrust", tier, 1.2, 8).movement(0.3, "forward", "stay").dodge(0.1, 0.4).attack(0.6, 22, 0.4).reach(1).depth(0.5).weapons(["shortspear", "longspear"]).anim("heavy")
            if (tier >= 3) move.react("steadfast")
            return move
        }
    }

    // --- bows and the crossbow -----------------------------------------------------------

    static ranged_(defs) {
        defs["Quick Shot"] = Fn.new { |tier|
            return Move.new("Quick Shot", tier, 0.8, 3).costFixed(tier >= 3 ? 3 : [0, 3, 4][tier]).attackFlat(0.4, [0, 10, 15, 20][tier], [0, 0.15, 0.2, 0.2][tier]).first().weapons(["bow"]).anim("shoot")
        }
        defs["Aimed Shot"] = Fn.new { |tier|
            var move = Move.new("Aimed Shot", tier, 1.6, 9).attack(1.0, 26, 0.4).first().weapons(["bow"]).anim("shoot")
            if (tier >= 3) move.depth(0.5)
            return move
        }
        defs["Retreating Shot"] = Fn.new { |tier|
            var move = Move.new("Retreating Shot", tier, 1.0, 5).movement(0.2, "back", "stay").attackFlat(0.6, [0, 12, 18, 24][tier], [0, 0.2, 0.2, 0.25][tier]).first().weapons(["bow"]).anim("shoot")
            if (tier >= 3) move.dodge(0.0, 0.2)
            return move
        }
        defs["Volley"] = Fn.new { |tier|
            var share = tier >= 3 ? 0.75 : 0.6
            return Move.new("Volley", tier, 2.0, 10).attack(1.3, 18, 0.25).reach(2).depth(share).sides(share).weapons(["longbow", "warbow"]).anim("shoot")
        }
        defs["Point-blank Shot"] = Fn.new { |tier|
            return Move.new("Point-blank Shot", tier, 0.7, 3).attack(0.3, 12, 0.3).reach(1).weapons(["shortbow"]).anim("shoot")
        }
        // Needs a loaded bolt (Combat/BattleEngine.wren - a Bolt with none loaded falters); Reload loads one.
        defs["Bolt"] = Fn.new { |tier|
            var move = Move.new("Bolt", tier, 0.6, 5).attack(0.3, 30, 0.5).first().weapons(["crossbow"]).special("bolt").anim("shoot")
            if (tier >= 3) move.depth(0.4)
            return move
        }
        defs["Reload"] = Fn.new { |tier|
            var move = Move.new("Reload", tier, 1.2, 0).costFixed(0).weapons(["crossbow"]).special("reload").anim("guard")
            if (tier >= 2) move.dodge(0.0, 0.3)
            if (tier >= 3) move.defendFlat(0.4, 0.8, 1.1, 15, 0.2)
            return move
        }
    }

    // --- gauntlets ---------------------------------------------------------------------

    static fists_(defs) {
        defs["Jab"] = Fn.new { |tier|
            return Move.new("Jab", tier, 0.45, 2).attack(0.2, 6, 0.2).reach(1).weapons(["unarmed"]).damageAnim(0.2).anim("punch")
        }
        defs["Hook"] = Fn.new { |tier|
            return Move.new("Hook", tier, 0.8, 4).attack(0.4, 12, 0.35).reach(1).weapons(["unarmed"]).anim("punch")
        }
        defs["Uppercut"] = Fn.new { |tier|
            var move = Move.new("Uppercut", tier, 1.1, 7).attack(0.6, 20, 0.7).reach(1).weapons(["gauntlets"]).anim("heavy")
            if (tier >= 3) move.react("unflinching")
            return move
        }
        defs["Palm Strike"] = Fn.new { |tier|
            var move = Move.new("Palm Strike", tier, 0.9, 5).attack(0.45, 10, 0.5).reach(1).push(1).weapons(["gauntlets"]).anim("punch")
            if (tier >= 3) move.depth(0.5)
            return move
        }
        defs["Catch"] = Fn.new { |tier|
            var move = Move.new("Catch", tier, 0.6, 2).defendFlat(0.05, 0.15, 0.3, [0, 15, 25, 35][tier], tier >= 3 ? 0.6 : 0.5).weapons(["gauntlets"]).anim("guard")
            if (tier >= 2) move.react("deflect")
            return move
        }
        // Moves the target to the cell behind the thrower, if it is free.
        defs["Throw"] = Fn.new { |tier|
            return Move.new("Throw", tier, 1.3, 6).attackFlat(0.7, tier >= 3 ? 20 : 15, tier >= 3 ? 0.7 : 0.6).reach(1).special("throw").weapons(["unarmed"]).anim("heavy")
        }
    }

    // --- wands and staffs ----------------------------------------------------------------

    static magic_(defs) {
        defs["Spark"] = Fn.new { |tier|
            return Move.new("Spark", tier, 0.7, 2).attackFlat(0.35, [0, 8, 12, 16][tier], [0, 0.15, 0.15, 0.2][tier]).first().mana().weapons(["wand"]).anim("cast")
        }
        defs["Arcane Bolt"] = Fn.new { |tier|
            var move = Move.new("Arcane Bolt", tier, 1.4, 8).attack(0.8, 22, 0.2).first().mana().weapons(MAGIC).anim("cast")
            if (tier >= 3) move.depth(0.3)
            return move
        }
        // Heals the caster and the ally in the cell ahead.
        defs["Mend"] = Fn.new { |tier|
            var cells = [[0, 0, 1], [1, 0, 1]]
            if (tier >= 3) {
                cells.add([0, -1, 0.5])
                cells.add([0, 1, 0.5])
            }
            return Move.new("Mend", tier, 1.2, 6).heal(0.6, 14).around(cells).mana().weapons(MAGIC).anim("cast")
        }
        defs["Ward"] = Fn.new { |tier|
            var move = Move.new("Ward", tier, 1.2, 4).defend(0.1, 0.5, 0.9, 15, 0).react("absorb").weapons(MAGIC).anim("guard")
            if (tier >= 3) move.guardSides(0.5)
            return move
        }
        defs["Blink"] = Fn.new { |tier|
            return Move.new("Blink", tier, 0.7, 4).movement(0.3, "blink", "stay").dodge(tier >= 3 ? 0.0 : 0.1, tier >= 3 ? 0.4 : 0.3).weapons(["wand"]).anim("roll")
        }
        defs["Blast"] = Fn.new { |tier|
            var near = tier >= 3 ? 0.45 : 0.3
            var far = tier >= 3 ? 0.25 : 0.15
            return Move.new("Blast", tier, 1.8, 12).attack(1.0, 30, 0.4).first().depth(near).sides(near).diagonals(far).mana().weapons(["magicstaff"]).anim("cast")
        }
        defs["Mending Circle"] = Fn.new { |tier|
            var move = Move.new("Mending Circle", tier, 1.8, 12).heal(1.0, 20).around8([0, 0.6, 0.8, 1][tier]).mana().weapons(["magicstaff"]).anim("cast")
            move.stencil.add([0, 0, 1])
            return move
        }
        defs["Barrier"] = Fn.new { |tier|
            var move = Move.new("Barrier", tier, 1.8, 6).defendFlat(0.2, 0.8, 1.4, tier >= 3 ? 24 : 18, 0).react("absorb").weapons(["magicstaff"]).anim("guard")
            var share = tier >= 3 ? 1 : 0.75
            for (dz in -1..1) {
                for (side in -1..1) {
                    if (dz != 0 || side != 0) move.guard(dz, side, share)
                }
            }
            return move
        }
        // Physical: scales with Attack even on a magic staff.
        defs["Staff Strike"] = Fn.new { |tier|
            return Move.new("Staff Strike", tier, 0.8, 2).attack(0.4, 6, 0.4).reach(1).weapons(["staff"]).anim("slash")
        }
        defs["Sweep"] = Fn.new { |tier|
            var move = Move.new("Sweep", tier, 1.2, 6).attack(0.6, 12, 0.5).reach(1).sides(0.6).weapons(["quarterstaff"]).anim("spin")
            if (tier >= 3) move.push(1)
            return move
        }
        defs["Pole Vault"] = Fn.new { |tier|
            return Move.new("Pole Vault", tier, 0.8, 4).movement(0.4, "forward2", "short").dodge(0.1, tier >= 2 ? 0.5 : 0.4).weapons(["quarterstaff"]).anim("roll")
        }
    }
}
