// Inventory/ItemDatabase.wren
// What every item id *is*: its category and, for weapons and armour, the attribute bonuses it carries. The
// Inventory (Inventory/Inventory.wren) only stores ids and counts - anything about an item itself is looked up
// here. Item ids are the same raw strings dialogue, the world (GameObjects/Item.wren) and the save file use.
//
// Categories decide stacking: materials and consumables share one stack with a count, weapons and armour never
// stack - every one is its own inventory entry.
import "fileSystem" for FileSystem

class ItemCategory {
    static MATERIAL { "material" }
    static CONSUMABLE { "consumable" }
    static WEAPON { "weapon" }
    static ARMOR { "armor" }

    // Display order (also the order the Items tab lists consumables before materials).
    static all { [ItemCategory.CONSUMABLE, ItemCategory.MATERIAL, ItemCategory.WEAPON, ItemCategory.ARMOR] }

    static stacks(category) { category == ItemCategory.MATERIAL || category == ItemCategory.CONSUMABLE }

    // The localization key for the category's display name (en.json "inventory.category.*").
    static labelKey(category) { "inventory.category.%(category)" }
}

// id -> { "category": one of ItemCategory's, "bonuses": { attribute: amount } (weapons / armour only - the
// Combat/Stats.wren attribute names; shown in the inventory, not applied to anyone until there is equipping) }.
// Anything not listed is a material - the quest items picked up through dialogue (keys, letters, the toolbox
// parts...) all land there.
var ITEMS = {
    "Fresh Bread": {"category": "consumable"},
    "Feast Loaf": {"category": "consumable"},
    "Jar of Honey": {"category": "consumable"},
    "Dried Herbs": {"category": "consumable"},

    // Placeholder gear so the Weapons / Armor tabs have something to show - nothing in the world gives these yet.
    "Iron Sword": {"category": "weapon", "bonuses": {"might": 2, "precision": 1}},
    "Oak Staff": {"category": "weapon", "bonuses": {"focus": 3}},
    "Leather Vest": {"category": "armor", "bonuses": {"guard": 2}},
    "Iron Helm": {"category": "armor", "bonuses": {"guard": 1, "poise": 2}}
}

var NO_BONUSES = {}

class ItemDatabase {
    static category(id) {
        var def = ITEMS[id]
        return def == null ? ItemCategory.MATERIAL : def["category"]
    }

    static stacks(id) { ItemCategory.stacks(ItemDatabase.category(id)) }

    static bonuses(id) {
        var def = ITEMS[id]
        if (def == null || def["bonuses"] == null) {
            return NO_BONUSES
        }
        return def["bonuses"]
    }

    // The icon texture path, or null when the item has no art yet (the inventory draws a "?" instead, rather than
    // Texture.load failing on a missing file). Notes and receipts share one generic icon.
    static iconPath(id) {
        if (id.endsWith(" Note") || id.endsWith(" Recipt")) {
            return "Tiles/Items/Recipt.png"
        }
        var path = "Tiles/Items/%(id).png"
        return FileSystem.exists(path) ? path : null
    }
}
