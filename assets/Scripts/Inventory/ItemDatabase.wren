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

// id -> its icon (one small image per item, under assets/Items/). Used by the inventory and by the world pickup
// sprite (GameObjects/Item.wren). Items not listed here have no art yet - the inventory draws a "?" for them.
var ICONS = {
    "Gold Coins": "Items/Treasure/GoldCoin.png",
    "Lucky Coin": "Items/Treasure/SilverCoin.png",
    "Ornate Key": "Items/Treasure/GoldKey.png",
    "Iron Key": "Items/Treasure/SilverKey.png",
    "Cabinet Key": "Items/Treasure/SilverKey.png",
    "Red Crystal Key": "Items/Resource/GemRed.png",
    "Green Crystal Key": "Items/Resource/GemGreen.png",
    "Yellow Crystal Key": "Items/Resource/GemYellow.png",
    "Blue Crystal Key": "Items/Resource/GemPurple.png",
    "Love Letter": "Items/Other/Letter.png",
    "Journal Page": "Items/Other/Letter3.png",
    "Ancient Tome": "Items/Object/Book.png",
    "Personal Diary": "Items/Object/Book.png",
    "Merchant Ledger": "Items/Object/Book.png",
    "ancient_book": "Items/Object/Book.png",
    "Star Chart": "Items/Scroll/Scroll.png",
    "Hammer": "Items/Tool/Hammer.png",
    "Tool Box": "Items/Object/CrateEmpty.png",
    "Toolbox Body": "Items/Object/CrateEmpty.png",
    "Toolbox Handle": "Items/Resource/BarIron.png",
    "Jar of Honey": "Items/Food/Honey.png",
    "Dried Herbs": "Items/Food/TeaLeaf.png",
    "Flour Sack": "Items/Object/Bag.png",
    "Ancient Seal": "Items/Other/Stamp.png",
    "Merchant Voucher": "Items/Other/Stamp.png",
    "rare_gem": "Items/Resource/GemPurple.png",
    "Iron Sword": "Items/Weapons/Sword/Sprite.png",
    "Oak Staff": "Items/Weapons/MagicWand/Sprite.png"
}

// Notes and receipts all share one generic icon.
var NOTE_ICON = "Items/Other/Letter2.png"

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
    // Texture.load failing on a missing file).
    static iconPath(id) {
        if (id.endsWith(" Note") || id.endsWith(" Recipt")) {
            return NOTE_ICON
        }
        var path = ICONS[id]
        return (path != null && FileSystem.exists(path)) ? path : null
    }
}
