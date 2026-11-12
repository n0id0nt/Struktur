// UI/Gameplay/InteractPromptUI.wren
// A floating label that hovers above whatever entity the player can currently interact with -
// GameWorldState's "Interact" prompt (ExperimentState has its own near-identical "Fight!" prompt,
// UI/Gameplay/FightPromptUI.wren - kept separate rather than merged, since the two states pick their
// target for very different reasons). Pure view: showAbove(worldPosition) does the
// world-to-screen conversion (legitimate view/presentation work) but the Controller decides *when*
// to call it and *which* entity's position to pass.
import "ui" for UIManager
import "math" for Vec2
import "gameObjectComponents" for Camera
import "Util/Colors" for WHITE
import "UI/Theme/UIKit" for UIKit

class InteractPromptUI {
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
