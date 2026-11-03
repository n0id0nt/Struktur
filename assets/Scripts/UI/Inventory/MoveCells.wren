// UI/Inventory/MoveCells.wren
// How a move's numbers read in the inventory's move tables (UI/Inventory/PartyPage.wren's moves panel and
// UI/Inventory/MovesPage.wren): each is a [text, colour] pair, "-" in muted grey where a move has none. Stamina is a
// cost, or a restore (Dodge) in green; heals are green too. `move` is a view-model map from
// States/InventoryState.moveEntry_: { "time", "damage", "staminaCost", "staminaRestore", "heal" }.
import "UI/Inventory/InventoryTheme" for InventoryTheme

class MoveCells {
    static time(move) {
        return ["%(move["time"])", InventoryTheme.text]
    }

    static damage(move) {
        if (move["damage"] > 0) {
            return ["%(move["damage"])", InventoryTheme.text]
        }
        return ["-", InventoryTheme.textMuted]
    }

    static stamina(move) {
        if (move["staminaRestore"] > 0) {
            return ["+%(move["staminaRestore"])", InventoryTheme.good]
        }
        if (move["staminaCost"] > 0) {
            return ["-%(move["staminaCost"])", InventoryTheme.text]
        }
        return ["-", InventoryTheme.textMuted]
    }

    static heal(move) {
        if (move["heal"] > 0) {
            return ["+%(move["heal"])", InventoryTheme.good]
        }
        return ["-", InventoryTheme.textMuted]
    }

    // Writes one of the pairs above into a label, re-fitting its box to the new text.
    static apply(label, cell) {
        label.setText(cell[0])
        label.setBoundingBoxToText()
        label.setTextColor(cell[1])
    }
}
