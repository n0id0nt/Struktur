// UI/Combat/MoveIcons.wren
// Which icon (from the UI/Skill Icon pack, 24 x 24 tiles) stands for a move's type - on the command panel's move
// buttons (UI/Combat/MoveButtonUI.wren) and on the timeline's blocks (UI/Combat/TimelineUI.wren). Drawn at 1x, like
// the item icons in the battle's item list.

var ICON_DIR = "UI/Skill Icon/"

var ICONS = {
    "slash": ICON_DIR + "Spell/Cut.png",
    "punch": ICON_DIR + "Job & Action/Punch.png",
    "shoot": ICON_DIR + "Items & Weapon/Arrow.png",
    "throw": ICON_DIR + "Items & Weapon/Kunai.png",
    "magic": ICON_DIR + "Spell/OrbLight.png",
    "counter": ICON_DIR + "Spell/Counter.png",
    "defend": ICON_DIR + "Spell/DefenseUpgrade.png",
    "heal": ICON_DIR + "Spell/Heal.png",
    "move": ICON_DIR + "Items & Weapon/Boot.png",
    "dodge": ICON_DIR + "Spell/Mist.png",
    "soul": ICON_DIR + "Spell/Vision.png",
    "reload": ICON_DIR + "Job & Action/Repair.png",
    "item": ICON_DIR + "Job & Action/Potion.png",
    "hold": "Items/Object/Hourglass.png",
    "other": ICON_DIR + "Items & Weapon/Scroll.png"
}

class MoveIcons {
    // The icon image path for `move`.
    static pathFor(move) { ICONS[MoveIcons.kindOf(move)] }

    // The icon for a Hold (the Hold buttons have no Move to ask about).
    static hold { ICONS["hold"] }

    // Which icon a move gets: what it is first (a Hold, an item, Soul Step), then what it does - a defence that also
    // strikes is a counter; an attack is told apart by how it's delivered (a shot, a thrown blade, a punch, a spell).
    static kindOf(move) {
        if (move.isHold) return "hold"
        if (move.item != null) return "item"
        if (move.name == "Soul Step") return "soul"
        if (move.special == "reload") return "reload"
        if (move.isHeal) return "heal"
        if (move.isDefend) return move.isAttack ? "counter" : "defend"
        if (move.isAttack) {
            if (move.scaling == "mana" || move.anim == "cast") return "magic"
            if (move.anim == "shoot") {
                var weapons = move.weapons
                return (weapons != null && weapons.contains("dagger")) ? "throw" : "shoot"
            }
            if (move.anim == "punch") return "punch"
            return "slash"
        }
        if (move.isDodge) return "dodge"
        if (move.isMovement) return "move"
        return "other"
    }
}
