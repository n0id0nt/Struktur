// Combat/Config/BattleItems.wren
// Using an item is a move like any other (battle system doc): it has its own animation length, its effect lands only
// at its effect time, it pays the weapon and armour tax, and the item is consumed at that effect time - so if the
// user is defeated or it is interrupted before then, the item isn't lost.
//
//   Health Potion   1.3 s, effect 1.0 s, Self            heal 30% max HP
//   Smelling Salts  1.0 s, effect 0.7 s, Self            clears statuses
//   Fire Flask      1.7 s, effect 1.1 s, First; sides 30%   20 damage + Burn
//
// The consumables the game hands out so far are all food and herbs: each is a self-heal of a share of max HP.
// forItem(id) returns the move for an inventory item (null = it can't be used in a fight).
import "Combat/Move" for Move

// item id -> % of max HP it restores.
var HEALING = {
    "Healing Herb": 30,
    "Dried Herbs": 20,
    "Bread Roll": 15,
    "Fresh Bread": 15,
    "Feast Loaf": 30,
    "Jar of Honey": 20,
    "Special Ration": 40
}

class BattleItems {
    static forItem(id) {
        if (HEALING.containsKey(id)) return BattleItems.heal_(id, HEALING[id])
        if (id == "Health Potion") return BattleItems.heal_(id, 30)
        if (id == "Smelling Salts") {
            return Move.new(id, 1, 1.0, 0).self().at(0.7).special("salts").item(id).anim("guard")
        }
        if (id == "Fire Flask") {
            return Move.new(id, 1, 1.7, 0).attackFlat(1.1, 20, 0.2).first().sides(0.3).special("flask").item(id).anim("shoot")
        }
        return null
    }

    static heal_(id, percent) {
        return Move.new(id, 1, 1.3, 0).healFlat(1.0, percent).self().special("potion").item(id).anim("guard")
    }

    // One line for the command panel: what the item does.
    static describe(id) {
        if (HEALING.containsKey(id)) return "Heals %(HEALING[id]) pct of max HP at 1.0s"
        if (id == "Health Potion") return "Heals 30 pct of max HP at 1.0s"
        if (id == "Smelling Salts") return "Clears statuses at 0.7s"
        if (id == "Fire Flask") return "20 damage and Burn to the first enemy in the lane at 1.1s"
        return ""
    }

    // What the battle test hands every fighter, so items can be tried without an inventory.
    static testItems { ["Health Potion", "Smelling Salts", "Fire Flask"] }
}
