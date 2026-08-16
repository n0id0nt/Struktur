// states/MainMenuState.wren
// Main menu state - first state when game starts
// Handles menu navigation and transitions to game

import "app" for Application
import "math" for Vec2
import "resourceManager" for Font
import "ui" for UIManager, UILabel, UIPanel, TextAlignment

import "States/BaseState" for BaseState
import "Colors" for WHITE, BLACK, BLANK, DARKGRAY, LIGHTGRAY

class MainMenuState is BaseState {
    construct new() {
        super()
        name = "MainMenuState"
        _stateManager = null
        _screenPanel = null
    }

    enter(stateManager, params) {
        super.enter(stateManager, params)
        _stateManager = stateManager

        System.print("Entering Main Menu")

        var font = Font.load("Fonts/medieval_sharp/MedievalSharp-Bold.ttf", 60)

        _screenPanel = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0),
            Vec2.new(Application.gameWidth, Application.gameHeight), Vec2.new(0, 0))
        _screenPanel.setBackgroundColor(DARKGRAY)
        _screenPanel.setBorderColor(BLANK)
        UIManager.addUIElement(_screenPanel)

        var titleLabel = UILabel.new(Vec2.new(0, 90), Vec2.new(0.5, 0), "Memory Palace", 64.0)
        titleLabel.setFont(font)
        titleLabel.setTextColor(WHITE)
        titleLabel.setAlignment(TextAlignment.CENTER)
        titleLabel.setBoundingBoxToText()
        titleLabel.setAnchorPoint(Vec2.new(0.5, 0))
        _screenPanel.addChild(titleLabel)

        var startButton = makeButton("Start Game", 0, font)
        startButton.setOnClick { |sender, mousePos| startGame() }

        var settingsButton = makeButton("Settings", 1, font)
        settingsButton.setOnClick { |sender, mousePos| openSettings() }

        var quitButton = makeButton("Quit", 2, font)
        quitButton.setOnClick { |sender, mousePos| quit() }

        UIManager.setFocus(startButton)

        font.unload()
    }

    // Builds a focusable/clickable button (panel + centered label) at the given menu slot index and parents it
    // under _screenPanel - caller is responsible for wiring setOnClick to whatever the button should do.
    makeButton(text, index, font) {
        var buttonWidth = 280
        var buttonHeight = 64
        var spacing = 24
        var startY = 320

        var button = UIPanel.new(Vec2.new(0, startY + index * (buttonHeight + spacing)), Vec2.new(0.5, 0),
            Vec2.new(buttonWidth, buttonHeight), Vec2.new(0, 0))
        button.setAnchorPoint(Vec2.new(0.5, 0))
        button.setBackgroundColor(LIGHTGRAY)
        button.setBorderColor(WHITE)
        button.setBorderWidth(2)
        button.setFocusable(true)
        _screenPanel.addChild(button)

        var label = UILabel.new(Vec2.new(0, 0), Vec2.new(0.5, 0.5), text, 26.0)
        label.setFont(font)
        label.setTextColor(BLACK)
        label.setAlignment(TextAlignment.CENTER)
        label.setBoundingBoxToText()
        label.setAnchorPoint(Vec2.new(0.5, 0.5))
        button.addChild(label)

        // Hover/focus highlight - also a handy visual check that batched background-color writes (UIPanel.
        // Render(), see UIRenderer::WriteRect) actually reach the GPU every time focus changes.
        button.setOnFocus { |sender| button.setBackgroundColor(WHITE) }
        button.setOnLoseFocus { |sender| button.setBackgroundColor(LIGHTGRAY) }

        return button
    }

    update(stateManager) {
        // Mouse clicks and keyboard/gamepad focus navigation are both handled by UIManager itself - nothing
        // needed here.
    }

    exit() {
        super.exit()

        System.print("Exiting Main Menu")

        if (_screenPanel) {
            UIManager.removeUIElement(_screenPanel)
            _screenPanel = null
        }
    }

    startGame() {
        System.print("Starting game...")
        _stateManager.changeState("GameWorld")
    }

    openSettings() {
        System.print("Settings not implemented yet")
    }

    quit() {
        System.print("Quit requested (not yet wired to the application)")
    }
}
