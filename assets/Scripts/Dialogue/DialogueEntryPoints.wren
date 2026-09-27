// Dialogue/DialogueEntryPoints.wren
// NPC/item-name -> dialogue-node-id lookup. Extracted from what used to be States/InteractState.
// wren's embedded getEntryPoint method - pure data/logic, zero UI dependency.
import "debug" for Debug
import "Inventory" for Inventory

class DialogueEntryPoints {
    static get(interactableName) {
        // NPCs
        if (interactableName == "Scholar")    return "scholar"
        if (interactableName == "Gardener")   return "gardener"
        if (interactableName == "Cook")       return "cook"
        if (interactableName == "Merchant")   return "merchant"
        if (interactableName == "Guard")      return "guard"
        if (interactableName == "Librarian")  return "librarian"
        if (interactableName == "Astronomer") return "astronomer"
        if (interactableName == "Cordelia")   return "cordelia"
        if (interactableName == "Dreamer")    return "dreamer"
        if (interactableName == "Guardian")   return "guardian"
        if (interactableName == "Inventor")   return "inventor"

        // Immovable interactables
        if (interactableName == "Safe")                    return "safe"
        if (interactableName == "Red Pedestal Inactive")   return "red_pedestal_inactive"
        if (interactableName == "Red Pedestal Active")     return "red_pedestal_active"
        if (interactableName == "Blue Pedestal Inactive")  return "blue_pedestal_inactive"
        if (interactableName == "Blue Pedestal Active")    return "blue_pedestal_active"
        if (interactableName == "Green Pedestal Inactive") return "green_pedestal_inactive"
        if (interactableName == "Green Pedestal Active")   return "green_pedestal_active"
        if (interactableName == "Yellow Pedestal Inactive") return "yellow_pedestal_inactive"
        if (interactableName == "Yellow Pedestal Active")  return "yellow_pedestal_active"
        if (interactableName == "Entrance Door")           return "entrance_door"

        // World items (always pickuppable — no suffix logic)
        if (interactableName == "Rose")         return "rose"
        if (interactableName == "Tool Box")     return "tool_box"
        if (interactableName == "Telescope")    return "telescope"
        if (interactableName == "Ancient Seal") return "ancient_seal"

        // Returnable items — use "_return" entry if already in inventory
        var suffix = Inventory.contains(interactableName) ? "_return" : ""
        if (interactableName == "Ancient Tome") return "ancient_book" + suffix
        if (interactableName == "Love Letter")  return "love_letter" + suffix
        if (interactableName == "Hammer")       return "hammer" + suffix
        if (interactableName == "Star Chart")   return "star_chart" + suffix
        if (interactableName == "Ornate Key")   return "ornate_key" + suffix

        Debug.warning("No dialogue entry point found for '%(interactableName)'")
        return null
    }
}
