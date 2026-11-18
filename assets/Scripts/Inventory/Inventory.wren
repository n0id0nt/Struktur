// Inventory/Inventory.wren
// What the party is carrying: a list of ItemStacks plus their gold. Consumables share one stack per item with a
// count, key items are one of a kind; weapons and armour never stack - each one is its own stack of 1 (which category an item is
// in comes from Inventory/ItemDatabase.wren). Stacks keep pickup order. Saved to inventory.sav.
import "serialisation" for Json, MapUtil
import "fileSystem" for FileSystem
import "Inventory/ItemDatabase" for ItemDatabase

class ItemStack {
    construct new(id, count) {
        _id = id
        _count = count
    }

    id { _id }
    count { _count }
    count=(value) { _count = value }

    category { ItemDatabase.category(_id) }
    stacks { ItemDatabase.stacks(_id) }
}

class Inventory {
    static init() {
        __stacks = []
        __gold = 0
    }

    static stacks { __stacks }

    // The stacks in `category`, in pickup order.
    static stacksIn(category) { __stacks.where { |stack| stack.category == category }.toList }

    static contains(id) { Inventory.count(id) > 0 }

    // How many of `id` are carried, across every stack (several for unstackable gear).
    static count(id) {
        var total = 0
        for (stack in __stacks) {
            if (stack.id == id) {
                total = total + stack.count
            }
        }
        return total
    }

    static addItem(id) { Inventory.addItem(id, 1) }

    static addItem(id, amount) {
        // Key items are one of a kind (Inventory/ItemDatabase.wren) - holding one already, another adds nothing.
        if (ItemDatabase.unique(id)) {
            if (Inventory.contains(id)) return
            amount = 1
        }
        if (ItemDatabase.stacks(id)) {
            for (stack in __stacks) {
                if (stack.id == id) {
                    stack.count = stack.count + amount
                    return
                }
            }
            __stacks.add(ItemStack.new(id, amount))
        } else {
            for (i in 0...amount) {
                __stacks.add(ItemStack.new(id, 1))
            }
        }
    }

    static removeItem(id) { Inventory.removeItem(id, 1) }

    // Takes up to `amount` of `id`, newest stacks first; a stack that empties is dropped.
    static removeItem(id, amount) {
        var i = __stacks.count - 1
        while (i >= 0 && amount > 0) {
            var stack = __stacks[i]
            if (stack.id == id) {
                var taken = stack.count < amount ? stack.count : amount
                stack.count = stack.count - taken
                amount = amount - taken
                if (stack.count == 0) {
                    __stacks.removeAt(i)
                }
            }
            i = i - 1
        }
    }

    // --- Currency ---

    static gold { __gold }

    static addGold(amount) { __gold = __gold + amount }

    // Pays `amount` if there is enough; returns whether it did.
    static spendGold(amount) {
        if (__gold < amount) {
            return false
        }
        __gold = __gold - amount
        return true
    }

    // --- Serialisation ---

    // Parallel id / count lists (like Party/Party.wren's save) plus the gold.
    static serialise() {
        var ids = []
        var counts = []
        for (stack in __stacks) {
            ids.add(stack.id)
            counts.add(stack.count)
        }
        var map = { "ids": ids, "counts": counts, "gold": __gold }
        var convertedList = MapUtil.mapToPairs(map)
        var json = Json.stringify(convertedList)
        return json
    }

    // Restores state from a string produced by serialise(). Saves from before stacks existed only have an
    // "items" list of names - each name is one item.
    static parse(json) {
        var data = Json.parse(json)
        __stacks = []
        __gold = 0
        if (data == null) {
            return
        }
        var ids = data["ids"]
        var counts = data["counts"]
        if (ids != null) {
            for (i in 0...ids.count) {
                var amount = (counts != null && i < counts.count) ? counts[i] : 1
                Inventory.addItem(ids[i], amount)
            }
        }
        var legacy = data["items"]
        if (legacy != null) {
            for (id in legacy) {
                Inventory.addItem(id)
            }
        }
        if (data["gold"] != null) {
            __gold = data["gold"]
        }
    }

    // Convenience: persist straight to a file path (PhysicsFS write path)
    static save(path) {
        var data = serialise()
        var result = FileSystem.writeString(path, data)
        if (result.success) {
            FileSystem.syncSaves()
        }
    }

    // Convenience: restore straight from a file path
    static load(path) {
        var result = FileSystem.readString(path)
        if (result.success) {
            parse(result.value)
        }
    }
}
