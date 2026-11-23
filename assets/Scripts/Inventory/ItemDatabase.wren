// Inventory/ItemDatabase.wren
// What every item id *is*: its category and, for weapons and armour, the attribute bonuses it carries. The
// Inventory (Inventory/Inventory.wren) only stores ids and counts - anything about an item itself is looked up
// here. Item ids are the same raw strings dialogue, the world (GameObjects/Item.wren) and the save file use.
//
// Categories decide stacking: consumables share one stack with a count; key items (quest items - the map, the
// horn, keys, letters) are one of a kind - carrying one already, getting another changes nothing - unless the item
// is marked "collect" (Soul Fragments: several are gathered); weapons and armour never stack - every one is its own
// inventory entry. The inventory's Items tab shows them in four sections in ItemCategory.all's order.
import "fileSystem" for FileSystem

class ItemCategory {
    static KEY { "key" }
    static MATERIAL { "material" }   // old saves / unlisted crafting bits - shown with the key items
    static CONSUMABLE { "consumable" }
    static WEAPON { "weapon" }
    static ARMOR { "armor" }

    // Display order - the Items tab's sections.
    static all { [ItemCategory.KEY, ItemCategory.CONSUMABLE, ItemCategory.WEAPON, ItemCategory.ARMOR] }

    static stacks(category) { category != ItemCategory.WEAPON && category != ItemCategory.ARMOR }

    // The localization key for the category's display name (en.json "inventory.category.*").
    static labelKey(category) { "inventory.category.%(category)" }
}

// id -> { "category": one of ItemCategory's, "bonuses": { attribute: amount } (weapons / armour only - the
// Combat/Stats.wren attribute names; shown in the inventory, not applied to anyone yet),
// "weaponType": the Combat/Config/GearTypes.wren weapon id it fights as (weapons - what Inventory/Equipment.wren
// equips, which decides the moves that work) }.
// Anything not listed is a key item - the quest items picked up through dialogue (keys, letters, the map, the
// toolbox parts...) all land there. "collect": true lets a key item gather a count instead of being one of a kind.
// "opensTab": pressing it in the inventory opens that tab (the map item opens the Map tab).
var ITEMS = {
    "Fresh Bread": {"category": "consumable"},
    "Feast Loaf": {"category": "consumable"},
    "Jar of Honey": {"category": "consumable"},
    "Dried Herbs": {"category": "consumable"},

    // Placeholder gear so the Weapons / Armor tabs have something to show - nothing in the world gives these yet.
    "Iron Sword": {"category": "weapon", "weaponType": "armingsword", "bonuses": {"attack": 2}},
    "Oak Staff": {"category": "weapon", "weaponType": "quarterstaff", "bonuses": {"mana": 3}},
    "Leather Vest": {"category": "armor", "bonuses": {"defence": 8}},
    "Iron Helm": {"category": "armor", "bonuses": {"defence": 2}},

    // Arc 1 (Dialogue/Arc1/). Everything else it hands out (keys, the horn, records...) is a key item.
    "Healing Herb": {"category": "consumable"},
    // Thomas's, before and after a sparring lesson (drunk straight away - the "drinkPotion" dialogue command).
    "Healing Potion": {"category": "consumable"},
    "Bread Roll": {"category": "consumable"},
    "Special Ration": {"category": "consumable"},
    "Hunter's Bow": {"category": "weapon", "weaponType": "shortbow", "bonuses": {"attack": 2}},
    // Finn's practice sword: never in the inventory itself - Inventory/Equipment.wren always offers it.
    "Practice Sword": {"category": "weapon", "weaponType": "armingsword"},
    "Leather Armor": {"category": "armor", "bonuses": {"defence": 8}},
    // Finn's old blade, after the sparring match (scene 11) - stamped with the anvil-and-mountain mark, and warm.
    "Finn's Blade": {"category": "weapon", "weaponType": "armingsword", "bonuses": {"attack": 4}},
    "Old Map": {"category": "key", "opensTab": "map"},
    "Soul Fragment": {"category": "key", "collect": true}
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
    "Practice Sword": "Items/Weapons/Sword/Sprite.png",
    "Oak Staff": "Items/Weapons/MagicWand/Sprite.png",
    // Arc 1
    "Healing Herb": "Items/Food/TeaLeaf.png",
    "Healing Potion": "Items/Potion/LifePot.png",
    "Bread Roll": "Items/Food/Onigiri.png",
    "Special Ration": "Items/Food/Meat.png",
    "Village Key": "Items/Treasure/SilverKey.png",
    "Hunter's Bow": "Items/Weapons/Bow/Sprite.png",
    "Finn's Blade": "Items/Weapons/Sword/Sprite.png",
    "Old Map": "Items/Scroll/Scroll.png",
    "Fragment Map": "Items/Scroll/Scroll.png",
    "Soul Fragment": "Items/Resource/GemPurple.png",
    "Guardian's Key": "Items/Treasure/GoldKey.png",
    "Ancient Knowledge Codex": "Items/Object/Book.png",
    "Ancient Records": "Items/Object/Book.png",
    "Communication Crystal": "Items/Resource/GemGreen.png"
}

// Notes and receipts all share one generic icon.
var NOTE_ICON = "Items/Other/Letter2.png"

class ItemDatabase {
    static category(id) {
        var def = ITEMS[id]
        return def == null ? ItemCategory.KEY : def["category"]
    }

    static stacks(id) { ItemCategory.stacks(ItemDatabase.category(id)) }

    // A key item you can only hold one of (see "collect").
    static unique(id) {
        var category = ItemDatabase.category(id)
        if (category != ItemCategory.KEY) return false
        var def = ITEMS[id]
        return def == null || def["collect"] != true
    }

    // The inventory tab pressing this item opens ("map"), or null.
    static opensTab(id) {
        var def = ITEMS[id]
        return def == null ? null : def["opensTab"]
    }

    // The weapon type id (Combat/Config/GearTypes.wren) a weapon item fights as, or null.
    static weaponType(id) {
        var def = ITEMS[id]
        return def == null ? null : def["weaponType"]
    }

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
