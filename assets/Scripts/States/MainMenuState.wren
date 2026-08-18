// states/MainMenuState.wren
// Main menu state - first state when game starts
// Handles menu navigation and transitions to game

import "app" for Application, Time
import "math" for Vec2
import "resourceManager" for Font, Texture
import "ui" for UIManager, UILabel, UIPanel, UIRichLabel, IconAtlas, TextAlignment, TextWrapping

import "States/BaseState" for BaseState
import "Colors" for WHITE, BLACK, BLANK, DARKGRAY, LIGHTGRAY

// Reveal speed for the rich-text showcase below (seconds per glyph) - matches InteractState's own dialogue
// scroll pacing convention (Time.scaledTime driving a glyph count), just via UIRichLabel.setVisibleGlyphCount
// instead of substring truncation - see MainMenuState.showcaseRichText.
var RICH_TEXT_REVEAL_SPEED = 0.03

class MainMenuState is BaseState {
    construct new() {
        super()
        name = "MainMenuState"
        _stateManager = null
        _screenPanel = null
        _richLabel = null
        _richTextStartTime = 0
        _richTextGlyphCount = 0
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
        titleLabel.setZIndex(10)
        _screenPanel.addChild(titleLabel)

        showcaseRichText()

        var startButton = makeButton("Start Game", 0, font)
        startButton.setOnClick { |sender, mousePos| startGame() }

        var settingsButton = makeButton("Settings", 1, font)
        settingsButton.setOnClick { |sender, mousePos| openSettings() }

        var quitButton = makeButton("Quit", 2, font)
        quitButton.setOnClick { |sender, mousePos| quit() }

        UIManager.setFocus(startButton)

        font.unload()
    }

    // Rich-text showcase - demonstrates every UIRichLabel feature: real bold/italic/bold-italic fonts (the
    // medieval_sharp family's own Book/Bold/BookOblique/BoldOblique variants - not the synthetic shear
    // fallback, since real fonts are set for every style here), inline [color=] spans, an [icon=] pulled from
    // a real item sprite, word wrap, a Time.scaledTime-driven reveal via setVisibleGlyphCount (the same
    // typewriter effect InteractState's dialogue box gets via raw string truncation, just without ever
    // risking a cut mid-tag), and Phase 8's shader-driven [wave]/[shake]/[pulse] animated tags, including a
    // layered combination on one span.
    showcaseRichText() {
        var regularFont    = Font.load("Fonts/medieval_sharp/MedievalSharp-Book.ttf", 24)
        var boldFont       = Font.load("Fonts/medieval_sharp/MedievalSharp-Bold.ttf", 24)
        var italicFont     = Font.load("Fonts/medieval_sharp/MedievalSharp-BookOblique.ttf", 24)
        var boldItalicFont = Font.load("Fonts/medieval_sharp/MedievalSharp-BoldOblique.ttf", 24)

        var iconAtlas = IconAtlas.new()
        iconAtlas.setTexture(Texture.load("Tiles/Items/Ornate Key.png"))
        iconAtlas.addIcon("key", 0, 0, 64, 64)

        var showcasePanel = UIPanel.new(Vec2.new(0, 200), Vec2.new(0.5, 0), Vec2.new(820, 380), Vec2.new(0, 0))
        showcasePanel.setAnchorPoint(Vec2.new(0.5, 0))
        showcasePanel.setBackgroundColor(LIGHTGRAY)
        showcasePanel.setBorderColor(WHITE)
        showcasePanel.setBorderWidth(2)
        showcasePanel.setZIndex(5)
        _screenPanel.addChild(showcasePanel)

        _richLabel = UIRichLabel.new(Vec2.new(16, 12), Vec2.new(0, 0),
            "This is [b]bold[/b], [i]italic[/i], and [b][i]bold italic[/i][/b] text, all real font " +
            "variants (not synthesized). Colors work too: [color=#d9534f]red[/color], " +
            "[color=#0275d8]blue[/color], and [color=#5cb85c]green[/color]. You can even embed icons " +
            "inline, like an [icon=key] ornate key, right in the middle of a sentence that keeps going " +
            "long enough to wrap across several lines.\n" +
            "Phase 8 adds shader-driven animated tags: [wave]this text waves[/wave], " +
            "[shake]this text shakes[/shake], and [pulse]this text pulses[/pulse] - and they layer: " +
            "[shake][pulse][color=#d9534f]shaky pulsing red text[/color][/pulse][/shake].\n" +
            "v2-v4 add more: [rainbow]this text cycles hue[/rainbow], " +
            "[tornado]this text orbits[/tornado], and [fade]this text fades out toward the end[/fade].", 22.0)
        _richLabel.setFont(regularFont)
        _richLabel.setBoldFont(boldFont)
        _richLabel.setItalicFont(italicFont)
        _richLabel.setBoldItalicFont(boldItalicFont)
        _richLabel.setTextColor(BLACK)
        _richLabel.setWordWrap(TextWrapping.WORD_WRAP)
        _richLabel.setIconAtlas(iconAtlas)
        _richLabel.setSize(Vec2.new(-32, -24), Vec2.new(1, 1))
        _richLabel.setZIndex(6)
        _richLabel.setVisibleGlyphCount(0)
        showcasePanel.addChild(_richLabel)

        _richTextGlyphCount = _richLabel.getGlyphCount()
        _richTextStartTime = Time.scaledTime

        regularFont.unload()
        boldFont.unload()
        italicFont.unload()
        boldItalicFont.unload()
    }

    // Builds a focusable/clickable button (panel + centered label) at the given menu slot index and parents it
    // under _screenPanel - caller is responsible for wiring setOnClick to whatever the button should do.
    makeButton(text, index, font) {
        var buttonWidth = 280
        var buttonHeight = 64
        var spacing = 24
        var startY = 610

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
        label.setZIndex(10)
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

        // Rich-text showcase reveal (see showcaseRichText/RICH_TEXT_REVEAL_SPEED) - same elapsed-time-driven
        // glyph count InteractState's dialogue scroll uses, just driving setVisibleGlyphCount instead of
        // slicing a raw string.
        if (_richLabel && _richTextGlyphCount > 0) {
            var revealed = ((Time.scaledTime - _richTextStartTime) / RICH_TEXT_REVEAL_SPEED).floor
            if (revealed > _richTextGlyphCount) {
                revealed = _richTextGlyphCount
            }
            _richLabel.setVisibleGlyphCount(revealed)
        }
    }

    exit() {
        super.exit()

        System.print("Exiting Main Menu")

        if (_screenPanel) {
            UIManager.removeUIElement(_screenPanel)
            _screenPanel = null
        }
        _richLabel = null
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
