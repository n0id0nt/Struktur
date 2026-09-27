// UI/GameOver/GameOverUI.wren
// The game-over screen's view: a dim full-screen overlay + centered "Game Complete" label. No
// logic at all - this screen has none today (see States/GameOverState.wren).
import "math" for Vec2, Vec4
import "resourceManager" for Font
import "ui" for UIManager, UILabel, UIPanel
import "app" for Application
import "Colors" for BLANK, WHITE

class GameOverUI {
    construct new() {
        var font = Font.load("Fonts/medieval_sharp/MedievalSharp-Bold.ttf", 120)

        _screenPanel = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0),
            Vec2.new(Application.gameWidth, Application.gameHeight), Vec2.new(0, 0))
        _screenPanel.setBackgroundColor(Vec4.new(0, 0, 0, 200))
        _screenPanel.setBorderColor(BLANK)
        UIManager.addUIElement(_screenPanel)

        var label = UILabel.new(Vec2.new(-20, -20), Vec2.new(0.5, 0.5), "Game Complete", 60)
        label.setAnchorPoint(Vec2.new(0.5, 0.5))
        label.setTextColor(WHITE)
        label.setFont(font)
        label.setBoundingBoxToText()
        _screenPanel.addChild(label)
    }

    teardown() {
        if (_screenPanel != null) {
            UIManager.removeUIElement(_screenPanel)
            _screenPanel = null
        }
    }
}
