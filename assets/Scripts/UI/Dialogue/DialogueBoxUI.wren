// UI/Dialogue/DialogueBoxUI.wren
// The dialogue box: full-screen transparent root, a rich-text label in a bottom-centre text box, and
// a "Continue" prompt. Pure view - reveal timing is computed by States/InteractState.wren (same
// precedent as Combat's strike animation / Main Menu's rich-text reveal) and handed in via
// revealGlyphs(count); this view never touches Time itself.
import "resourceManager" for Font, Texture
import "ui" for UIManager, UILabel, UIRichLabel, UIPanel, TextWrapping
import "app" for Application
import "math" for Vec2
import "localization" for Localization
import "Colors" for BLANK, BLACK, DARKGRAY

class DialogueBoxUI {
    construct new() {
        var font = Font.load("Fonts/medieval_sharp/MedievalSharp-Bold.ttf", 30)
        // Rich-text font variants for the dialogue label itself - same medieval_sharp family/sizing
        // convention Main Menu's showcaseRichText() uses, just at the dialogue box's existing size (20).
        var regularFont    = Font.load("Fonts/medieval_sharp/MedievalSharp-Book.ttf", 20)
        var boldFont       = Font.load("Fonts/medieval_sharp/MedievalSharp-Bold.ttf", 20)
        var italicFont     = Font.load("Fonts/medieval_sharp/MedievalSharp-BookOblique.ttf", 20)
        var boldItalicFont = Font.load("Fonts/medieval_sharp/MedievalSharp-BoldOblique.ttf", 20)
        var dialoguePanelTexture = Texture.load("Tiles/DialoguePanel.png")

        // Full-screen transparent root - UI/Dialogue/ChoiceListUI.wren parents into this too.
        _screenPanel = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0),
            Vec2.new(Application.gameWidth, Application.gameHeight), Vec2.new(0, 0))
        _screenPanel.setBackgroundColor(BLANK)
        _screenPanel.setBorderColor(BLANK)
        UIManager.addUIElement(_screenPanel)

        // Dialogue text box - bottom-centre. Anchored at (0.5, 1) so the bottom edge sits at 95% of
        // screen height.
        _textBackgroundPanel = UIPanel.new(Vec2.new(0, -30), Vec2.new(0.5, 1), Vec2.new(800, 200), Vec2.new(0, 0))
        _textBackgroundPanel.setAnchorPoint(Vec2.new(0.5, 1))
        _textBackgroundPanel.setBorderColor(BLANK)
        _textBackgroundPanel.setBackgroundColor(DARKGRAY)
        _textBackgroundPanel.setBackgroundTexture(dialoguePanelTexture)
        dialoguePanelTexture.unload()
        _screenPanel.addChild(_textBackgroundPanel)

        _dialogueLabel = UIRichLabel.new(Vec2.new(40, 25), Vec2.new(0, 0), "", 20.0)
        _dialogueLabel.setTextColor(BLACK)
        _dialogueLabel.setAnchorPoint(Vec2.new(0, 0))
        _dialogueLabel.setFont(regularFont)
        _dialogueLabel.setBoldFont(boldFont)
        _dialogueLabel.setItalicFont(italicFont)
        _dialogueLabel.setBoldItalicFont(boldItalicFont)
        _dialogueLabel.setWordWrap(TextWrapping.WORD_WRAP)
        // Fill the panel minus 40px left/right margin and 60px from bottom so the continue prompt
        // has room.
        _dialogueLabel.setSize(Vec2.new(-80, -60), Vec2.new(1, 1))
        _textBackgroundPanel.addChild(_dialogueLabel)

        // UIRichLabel holds its own reference once set via setFont/setBoldFont/etc - safe to release
        // ours immediately, matching Main Menu's showcaseRichText()'s identical pattern.
        regularFont.unload()
        boldFont.unload()
        italicFont.unload()
        boldItalicFont.unload()

        // "Continue" prompt - bottom-right of text box, 20px from the right edge, 18px from the
        // bottom edge. setAnchorPoint(1,1) means the label's own bottom-right corner sits there.
        _continueLabel = UILabel.new(Vec2.new(-20, -18), Vec2.new(1, 1),
            Localization.get("menu.interact.continue_prompt"), 16.0)
        _continueLabel.setTextColor(BLACK)
        _continueLabel.setAnchorPoint(Vec2.new(1, 1))
        _continueLabel.setFont(font)
        _continueLabel.setVisible(false)
        _textBackgroundPanel.addChild(_continueLabel)

        // Kept alive (unloaded in teardown) - also used by ChoiceListUI's row labels, via `font`.
        _font = font
    }

    // The screen root - UI/Dialogue/ChoiceListUI.wren parents into this.
    screenPanel { _screenPanel }
    // The same font the continue prompt uses - ChoiceListUI's row labels use it too.
    font { _font }

    setMarkupText(text) { _dialogueLabel.setMarkupText(text) }
    glyphCount { _dialogueLabel.getGlyphCount() }
    revealGlyphs(count) { _dialogueLabel.setVisibleGlyphCount(count) }
    showContinuePrompt(visible) { _continueLabel.setVisible(visible) }

    teardown() {
        if (_screenPanel != null) {
            UIManager.removeUIElement(_screenPanel)
            _screenPanel = null
        }
        if (_font != null) {
            _font.unload()
            _font = null
        }
    }
}
