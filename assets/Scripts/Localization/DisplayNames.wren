// DisplayNames.wren
// Maps the raw ids used internally as lookup keys (NPC.wren's NPCData table, Item.wren's ItemData
// table, dialogue "speaker" fields, Inventory item strings) to their localization keys - explicit
// tables rather than slugifying the id string, matching the enumerated-dict style already used by
// NPCData/ItemData. Internal ids never change; only what's displayed to the player is localized.

import "localization" for Localization

class DisplayNames {
    static npcKey(npcId) {
        var keys = {
            "Astronomer": "npc.astronomer",
            "Cook": "npc.cook",
            "Cordelia": "npc.cordelia",
            "Dreamer": "npc.dreamer",
            "Gardener": "npc.gardener",
            "Guardian": "npc.guardian",
            "Inventor": "npc.inventor",
            "Merchant": "npc.merchant",
            "Narrator": "npc.narrator",
            "Scholar": "npc.scholar"
        }
        return keys.containsKey(npcId) ? keys[npcId] : null
    }

    static itemKey(itemId) {
        var keys = {
            "Love Letter": "item.love_letter",
            "Hammer": "item.hammer",
            "Star Chart": "item.star_chart",
            "Ornate Key": "item.ornate_key",
            "Ancient Tome": "item.ancient_tome",
            "Rose": "item.rose",
            "Fresh Bread": "item.fresh_bread",
            "Tool Box": "item.tool_box",
            "Personal Diary": "item.personal_diary",
            "Telescope": "item.telescope",
            "Gold Coins": "item.gold_coins",
            "Ancient Seal": "item.ancient_seal",
            "Red Pedestal": "item.red_pedestal",
            "Green Pedestal": "item.green_pedestal",
            "Blue Pedestal": "item.blue_pedestal",
            "Yellow Pedestal": "item.yellow_pedestal",
            "Blue Crystal Key": "item.blue_crystal_key",
            "Red Crystal Key": "item.red_crystal_key",
            "Green Crystal Key": "item.green_crystal_key",
            "Yellow Crystal Key": "item.yellow_crystal_key",
            "Cabinet Key": "item.cabinet_key",
            "Dried Herbs": "item.dried_herbs",
            "Feast Loaf": "item.feast_loaf",
            "Flour Sack": "item.flour_sack",
            "Jar of Honey": "item.jar_of_honey",
            "Journal Page": "item.journal_page",
            "Merchant Ledger": "item.merchant_ledger",
            "Merchant Voucher": "item.merchant_voucher",
            "Toolbox Body": "item.toolbox_body",
            "Toolbox Handle": "item.toolbox_handle"
        }
        return keys.containsKey(itemId) ? keys[itemId] : null
    }

    // Falls back to the raw id (visibly correct-ish, never crashes) if an id has no mapping - same
    // "never blank, never crash" philosophy as LocalizationManager::GetString itself.
    static npcDisplay(npcId) {
        var key = npcKey(npcId)
        return key ? Localization.get(key) : npcId
    }

    static itemDisplay(itemId) {
        var key = itemKey(itemId)
        return key ? Localization.get(key) : itemId
    }
}
