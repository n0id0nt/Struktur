// UI/GameOver/GameOverUI.wren
// The game-over screen's view: a dim full-screen overlay + centered title, optionally with one button under it
// (the "You fell" screen's Continue - see States/GameOverState.wren). No logic: the caller wires the button.
import "math" for Vec2, Vec4
import "resourceManager" for Font
import "ui" for UIManager, UILabel, UIPanel
import "app" for Application
import "Util/Colors" for BLANK, WHITE
import "UI/Shared/ButtonUI" for ButtonUI

class GameOverUI {
    // The finished-game screen: "Game Complete", no button.
    construct new() {
        init_("Game Complete", null)
    }

    // title: the big centered line. buttonText: label for a button under it (null for none) - see `button`.
    construct new(title, buttonText) {
        init_(title, buttonText)
    }

    init_(title, buttonText) {
        var font = Font.load("Fonts/medieval_sharp/MedievalSharp-Bold.ttf", 120)

        _screenPanel = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0),
            Vec2.new(Application.gameWidth, Application.gameHeight), Vec2.new(0, 0))
        _screenPanel.setBackgroundColor(Vec4.new(0, 0, 0, 200))
        _screenPanel.setBorderColor(BLANK)
        UIManager.addUIElement(_screenPanel)

        var label = UILabel.new(Vec2.new(-20, -20), Vec2.new(0.5, 0.5), title, 60)
        label.setAnchorPoint(Vec2.new(0.5, 0.5))
        label.setTextColor(WHITE)
        label.setFont(font)
        label.setBoundingBoxToText()
        _screenPanel.addChild(label)

        _button = null
        if (buttonText != null) {
            _button = ButtonUI.new(_screenPanel, buttonText, Vec2.new(0, 90), Vec2.new(0.5, 0.5), Vec2.new(280, 64),
                                   font, 26.0)
            _button.panel.setAnchorPoint(Vec2.new(0.5, 0))
            UIManager.setFocus(_button.panel)
        }
    }

    // The button under the title (null when built without one) - wire it with setOnClick.
    button { _button }

    teardown() {
        if (_screenPanel != null) {
            UIManager.removeUIElement(_screenPanel)
            _screenPanel = null
        }
        _button = null
    }
}
