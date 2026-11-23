// Dialogue/Portraits.wren
// The face shown beside a speaker's lines (UI/Dialogue/DialogueBoxUI.wren setPortrait), and the expression a line is
// said with. Portraits are rendered by 3dModelsToSprites (each character's job.portrait.json - a head-and-shoulders
// close-up, installed to assets/Portraits). The rigs can't change their faces, so every portrait only has its
// default expression: a line asking for another one (the "expression" dialogue command) shows the default face with
// the expression written under it as a placeholder, until art for it exists.

// Dialogue "speaker" id -> its portrait image.
var PORTRAITS = {
    "Finn": "Portraits/finn_portrait.png",
    "Kira": "Portraits/yellow_pants_girl_portrait.png",
    "Thomas": "Portraits/goblin_viking_portrait.png",
    "Elena": "Portraits/peasant_girl_portrait.png",
    "Garrett": "Portraits/wrestler_portrait.png",
    "Child": "Portraits/pink_hair_girl_portrait.png",
    "Guide": "Portraits/claire_portrait.png",
    "Warden": "Portraits/mutant_portrait.png",
    "Player": "Portraits/explorer_portrait.png"
}

// The expression every portrait is rendered with.
var DEFAULT_EXPRESSION = "neutral"

// speaker -> the expressions it has art for besides the default (none yet - add a name here once its art exists).
var EXPRESSIONS = {}

class Portraits {
    // The portrait image for a speaker, or null (the narrator-less lines and anyone without one).
    static path(speaker) { speaker == null ? null : PORTRAITS[speaker] }

    // Whether the speaker's portrait can show `expression` (null and the default always can).
    static supports(speaker, expression) {
        if (expression == null || expression == DEFAULT_EXPRESSION) return true
        var known = EXPRESSIONS[speaker]
        return known != null && known.contains(expression)
    }

    // The expression the next line is said with - set by the "expression" dialogue command when its node is reached,
    // taken by States/InteractState.wren when the line shows (so it lasts one line).
    static expression=(value) { __pending = value }

    static takeExpression() {
        var expression = __pending
        __pending = null
        return expression
    }
}
