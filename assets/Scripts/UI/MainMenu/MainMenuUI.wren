// UI/MainMenu/MainMenuUI.wren
// The main menu's view: title, a rich-text feature showcase panel, and the Start/Settings/Quit
// buttons (via UI/Shared/ButtonUI). Pure construction - button actions are wired by
// States/MainMenuState.wren via setOnClick; the rich-text reveal's elapsed-time computation
// also lives on the State (same precedent as Combat's strike animation) - this view only
// exposes revealGlyphs(count) and the total glyphCount to clamp against.
import "app" for Application
import "math" for Vec2
import "resourceManager" for Font, Texture
import "ui" for UIManager, UILabel, UIPanel, UIRichLabel, IconAtlas, TextAlignment, TextWrapping
import "localization" for Localization
import "Colors" for WHITE, BLACK, BLANK, DARKGRAY, LIGHTGRAY
import "UI/Shared/ButtonUI" for ButtonUI

class MainMenuUI {
    construct new() {
        var font = Font.load("Fonts/medieval_sharp/MedievalSharp-Bold.ttf", 60)

        _screenPanel = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0),
            Vec2.new(Application.gameWidth, Application.gameHeight), Vec2.new(0, 0))
        _screenPanel.setBackgroundColor(DARKGRAY)
        _screenPanel.setBorderColor(BLANK)
        UIManager.addUIElement(_screenPanel)

        var titleLabel = UILabel.new(Vec2.new(0, 90), Vec2.new(0.5, 0), Localization.get("menu.main.title"), 64.0)
        titleLabel.setFont(font)
        titleLabel.setTextColor(WHITE)
        titleLabel.setAlignment(TextAlignment.CENTER)
        titleLabel.setBoundingBoxToText()
        titleLabel.setAnchorPoint(Vec2.new(0.5, 0))
        titleLabel.setZIndex(10)
        _screenPanel.addChild(titleLabel)

        showcaseRichText_()

        _startButton = button_(Localization.get("menu.start_game"), 0, font)
        _settingsButton = button_(Localization.get("menu.settings"), 1, font)
        _quitButton = button_(Localization.get("menu.quit"), 2, font)

        UIManager.setFocus(_startButton.panel)

        font.unload()
    }

    // Rich-text showcase - demonstrates every UIRichLabel feature: real bold/italic/bold-italic fonts (the
    // medieval_sharp family's own Book/Bold/BookOblique/BoldOblique variants - not the synthetic shear
    // fallback, since real fonts are set for every style here), inline [color=] spans, an [icon=] pulled from
    // a real item sprite, word wrap, a reveal via setVisibleGlyphCount (driven by the Controller, see
    // revealGlyphs), and Phase 8's shader-driven [wave]/[shake]/[pulse] animated tags, including a layered
    // combination on one span.
    showcaseRichText_() {
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
        // Main menu content should keep animating (wave/shake/pulse/etc.) even while gameplay time is paused or
        // scaled - it isn't gameplay itself, so it follows real unscaled time instead of Time.scaledTime.
        _richLabel.setUseUnscaledTime(true)
        showcasePanel.addChild(_richLabel)

        regularFont.unload()
        boldFont.unload()
        italicFont.unload()
        boldItalicFont.unload()
    }

    button_(text, index, font) {
        var buttonWidth = 280
        var buttonHeight = 64
        var spacing = 24
        var startY = 610
        var b = ButtonUI.new(_screenPanel, text, Vec2.new(0, startY + index * (buttonHeight + spacing)),
                             Vec2.new(0.5, 0), Vec2.new(buttonWidth, buttonHeight), font, 26.0)
        b.panel.setAnchorPoint(Vec2.new(0.5, 0))
        return b
    }

    startButton { _startButton }
    settingsButton { _settingsButton }
    quitButton { _quitButton }

    // Total glyph count to reveal toward - read once by the Controller at start.
    richTextGlyphCount { _richLabel.getGlyphCount() }

    revealGlyphs(count) { _richLabel.setVisibleGlyphCount(count) }

    teardown() {
        if (_screenPanel != null) {
            UIManager.removeUIElement(_screenPanel)
            _screenPanel = null
        }
        _richLabel = null
    }
}
