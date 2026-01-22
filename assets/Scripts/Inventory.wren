class Inventory {
    static items { __items }
    
    static init() {
        __items = []
    }
    
    // Check if the inventory contains an item.
    static contains(item) {
        return __items.contains(item)
    }
    
    // Get a list of all items in inventory.
    static getItems() {
        return __items
    }
    
    // Add an item to the inventory.
    static addItem(item) {
        __items.add(item)
    }
    
    // Remove an item from the inventory.
    static removeItem(item) {
        __items.remove(item)
    }
}