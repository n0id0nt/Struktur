// UI/Gameplay/InteractPromptUI.wren
// A floating label that hovers above whatever entity the player can currently interact with -
// GameWorldState's "Interact" prompt (ExperimentState has its own near-identical "Fight!" prompt,
// UI/Gameplay/FightPromptUI.wren - kept separate rather than merged, since the two states pick their
// target for very different reasons). Pure view: showAbove(worldPosition) does the
// world-to-screen conversion (legitimate view/presentation work) but the Controller decides *when*
// to call it and *which* entity's position to pass.
import "ui" for UIManager, UILabel
import "math" for Vec2
import "resourceManager" for Font
import "gameObjectComponents" for Camera
import "Colors" for WHITE

class InteractPromptUI {
    construct new(text) {
        var font = Font.load("Fonts/medieval_sharp/MedievalSharp-Bold.ttf", 60)
        _label = UILabel.new(Vec2.new(0, 0), Vec2.new(0, 0), text, 16.0)
        _label.setVisible(false)
        _label.setFont(font)
        _label.setTextColor(WHITE)
        _label.setAnchorPoint(Vec2.new(0.5, 0.5))
        _label.setBoundingBoxToText()
        UIManager.addUIElement(_label)
        font.unload()
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
