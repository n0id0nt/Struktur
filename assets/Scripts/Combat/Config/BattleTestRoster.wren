// Combat/Config/BattleTestRoster.wren
// What the battle test (States/BattleTestState.wren) can put on either side: every battle def by its menu label,
// and how to mint a fresh Combatant from one with a chosen move set. Add a fighter here and it shows up in the
// setup menu's pickers.
import "Combat/Combatant" for Combatant
import "Combat/Config/Archetype" for Archetype
import "Combat/Config/BattleCritter" for BattleCritter
import "Combat/Config/BattlePlayer" for BattlePlayer

class BattleTestRoster {
    // [label, def] - a def is BattlePlayer (a class with static getters) or a BattleCritter; both expose name,
    // archetypeName, maxHp, maxStamina, attributes (overrides), kit and the sprite fields Battler reads.
    static fighters {
        return [
            ["You", BattlePlayer],
            ["Villager", BattleCritter.villager],
            ["Duelist", BattleCritter.duelist],
            ["Chicken", BattleCritter.chicken],
            ["Chinlin", BattleCritter.chinlin],
            ["Training Dummy", BattleCritter.trainingDummy]
        ]
    }

    // The full attribute block a def fights with (archetype baseline + its overrides).
    static attributesOf(def) { Archetype.byName(def.archetypeName).attributesWith(def.attributes) }

    // A fresh, full-health Combatant for `def` knowing `moves`, named `name` (so two Chickens can be told apart).
    static combatant(def, name, moves) {
        return Combatant.new(name, def.maxHp, def.maxStamina, BattleTestRoster.attributesOf(def), moves, null)
    }

    // One-line stat readout for the setup menu.
    static statLine(def) {
        var a = BattleTestRoster.attributesOf(def)
        return "%(def.archetypeName)  HP %(def.maxHp)  SP %(def.maxStamina)   Mig %(a["might"])  Grd %(a["guard"])  Poi %(a["poise"])  Agi %(a["agility"])  Ref %(a["reflex"])  Pre %(a["precision"])  End %(a["endurance"])  Foc %(a["focus"])"
    }
}
