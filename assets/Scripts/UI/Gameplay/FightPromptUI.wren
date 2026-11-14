// UI/Gameplay/FightPromptUI.wren
// A floating label that hovers above a non-aggressive critter the player can challenge -
// GameWorldState's "Fight!" prompt. Structurally a twin of UI/Gameplay/InteractPromptUI.wren
// (GameWorldState's "Interact" prompt) - kept as a separate class rather than merged, since the two
// states pick their target for very different reasons (an interactable vs. a critter in combat
// range), even though the widget itself is identical.
import "ui" for UIManager
import "math" for Vec2
import "gameObjectComponents" for Camera
import "Util/Colors" for WHITE
import "UI/Theme/UIKit" for UIKit

class FightPromptUI {
    // `text` follows the Interact button's prompt - "[E] %(text)", or the pad's button when one is in use.
    construct new(text) {
        _label = UIKit.hint(null, "Interact", text, Vec2.new(0, 0), Vec2.new(0, 0), Vec2.new(0.5, 0.5), 0)
        _label.setTextColor(WHITE)
        _label.setVisible(false)
        UIManager.addUIElement(_label)
    }

    // Positions the prompt just above worldPosition (in screen space) and shows it.
    showAbove(worldPosition) {
        var screenPos = Camera.worldPosToScreenPos(worldPosition) + Vec2.new(0, -32)
        _label.setPosition(screenPos, Vec2.new(0, 0))
        _label.setVisible(true)
    }

    hide() { _label.setVisible(false) }

    teardown() {
        if (_label != null) {
            UIManager.removeUIElement(_label)
            _label = null
        }
    }
}
