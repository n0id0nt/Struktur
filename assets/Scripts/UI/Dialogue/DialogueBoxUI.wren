// UI/Dialogue/DialogueBoxUI.wren
// The dialogue box: full-screen transparent root, a rich-text label in a bottom-centre box in the UI kit's
// "dialogue" style, and a prompt plate sitting on the box's bottom-right rim that says what the Interact button does
// next - "[E] Continue" once a line has finished, "[E] Choose" while choices are up (UI/Dialogue/ChoiceListUI.wren
// hangs those off the box's top-right corner). top() puts a shorter box at the top-centre instead - a caption for
// something happening in the world (States/ImpactState.wren). Pure view - reveal timing is computed by
// States/InteractState.wren (same precedent as Combat's strike animation / Main Menu's rich-text reveal) and handed in
// via revealGlyphs(count); tick(now) only bobs the prompt. A line too long for the box makes it taller (upward at the
// bottom, downward at the top) rather than spilling out - the choices ride along on its top edge.
import "ui" for UIManager, UIRichLabel, UIPanel, TextWrapping
import "app" for Application
import "math" for Vec2
import "localization" for Localization
import "Util/Colors" for BLANK, BLACK
import "UI/Theme/UIKit" for UIKit

var BOX_WIDTH = 800
var BOX_HEIGHT = 200
var TOP_BOX_HEIGHT = 120
var SCREEN_MARGIN = 30        // between the box and the screen's edge
var TEXT_INSET_X = 32         // the text, in from the box's sides (the kit rim is 8px art at 3x = 24px)
var TEXT_INSET_TOP = 26
var TEXT_INSET_BOTTOM = 34    // keeps the last line clear of the prompt plate on the bottom rim
var TEXT_SIZE = 20.0
var PROMPT_INSET_RIGHT = 32   // the prompt plate's right edge, in from the box's right edge
var PROMPT_PAD_X = 14         // around the hint, inside the plate (the plate's rim is 5px art at 2x = 10px)
var PROMPT_PAD_Y = 10
var PROMPT_BOB = 2            // how far the prompt bobs while it waits, in pixels
var PROMPT_PERIOD = 1.0       // seconds per bob

class DialogueBoxUI {
    construct new() {
        init_(false)
    }

    // A shorter box at the top of the screen.
    construct top() {
        init_(true)
    }

    init_(atTop) {
        UIKit.open()
        // Kept alive (unloaded in teardown) - ChoiceListUI's row labels use it too, via `font`.
        _font = UIKit.font()

        // Full-screen transparent root.
        _screenPanel = UIPanel.new(Vec2.new(0, 0), Vec2.new(0, 0),
            Vec2.new(Application.gameWidth, Application.gameHeight), Vec2.new(0, 0))
        _screenPanel.setBackgroundColor(BLANK)
        _screenPanel.setBorderColor(BLANK)
        UIManager.addUIElement(_screenPanel)

        // The box - bottom-centre, its bottom edge SCREEN_MARGIN above the screen's (anchored at (0.5, 1)); or, at the
        // top, a shorter box with its top edge SCREEN_MARGIN below the screen's (anchored at (0.5, 0)).
        _baseHeight = atTop ? TOP_BOX_HEIGHT : BOX_HEIGHT
        if (atTop) {
            _textBox = UIKit.frame(_screenPanel, "dialogue", Vec2.new(0, SCREEN_MARGIN), Vec2.new(0.5, 0),
                Vec2.new(BOX_WIDTH, TOP_BOX_HEIGHT), Vec2.new(0, 0), 0)
            _textBox.setAnchorPoint(Vec2.new(0.5, 0))
        } else {
            _textBox = UIKit.frame(_screenPanel, "dialogue", Vec2.new(0, -SCREEN_MARGIN), Vec2.new(0.5, 1),
                Vec2.new(BOX_WIDTH, BOX_HEIGHT), Vec2.new(0, 0), 0)
            _textBox.setAnchorPoint(Vec2.new(0.5, 1))
        }

        // The single-weight game font: [i] runs get UIRichLabel's synthetic slant, [b] runs the same weight.
        // (-5 / -2: the rich label's own padding.)
        _dialogueLabel = UIRichLabel.new(Vec2.new(TEXT_INSET_X - 5, TEXT_INSET_TOP - 2), Vec2.new(0, 0), "", TEXT_SIZE)
        _dialogueLabel.setTextColor(BLACK)
        _dialogueLabel.setFont(_font)
        _dialogueLabel.setWordWrap(TextWrapping.WORD_WRAP)
        _dialogueLabel.setSize(Vec2.new(-2 * TEXT_INSET_X + 10, -TEXT_INSET_TOP - TEXT_INSET_BOTTOM), Vec2.new(1, 1))
        _dialogueLabel.setZIndex(1)
        _textBox.addChild(_dialogueLabel)

        // The prompt: a small plate centred on the box's bottom rim at the right (anchored by its right-middle), with
        // the Interact button's hint inside. Sized to the hint in tick(), since the hint changes width with the device.
        _promptPlate = UIKit.frame(_textBox, "plate", Vec2.new(-PROMPT_INSET_RIGHT, 0), Vec2.new(1, 1), Vec2.new(0, 0),
            Vec2.new(0, 0), 5)
        _promptPlate.setAnchorPoint(Vec2.new(1, 0.5))
        _promptHint = UIKit.hint(_promptPlate, "Interact", Localization.get("menu.interact.continue_prompt"),
            Vec2.new(0, 0), Vec2.new(0.5, 0.5), Vec2.new(0.5, 0.5), 6)
        _promptHint.setTextColor(BLACK)
        _promptPlate.setVisible(false)
        _promptBob = null
        _promptSize = null
        fitPrompt_()
    }

    // The screen root.
    screenPanel { _screenPanel }
    // The box itself - UI/Dialogue/ChoiceListUI.wren hangs the choices off its top-right corner.
    textBox { _textBox }
    // The game font - ChoiceListUI's row labels use it too.
    font { _font }

    setMarkupText(text) {
        _dialogueLabel.setMarkupText(text)
        fitToText_()
    }
    glyphCount { _dialogueLabel.getGlyphCount() }
    revealGlyphs(count) { _dialogueLabel.setVisibleGlyphCount(count) }

    // "[E] Continue" on the bottom rim - the line is done and Interact moves on.
    showContinuePrompt(visible) {
        showPrompt_(visible ? "menu.interact.continue_prompt" : null)
    }

    // "[E] Choose" on the bottom rim - choices are up and Interact picks the highlighted one.
    showChoosePrompt(visible) {
        showPrompt_(visible ? "menu.interact.choose_prompt" : null)
    }

    // Bobs the prompt and keeps its plate fitted to the hint - call every frame with Time.unscaledTime.
    tick(now) {
        if (!_promptPlate.isVisible) {
            return
        }
        fitPrompt_()
        var bob = (((now / PROMPT_PERIOD) * 2 * Num.pi).sin * PROMPT_BOB).round
        if (bob != _promptBob) {
            _promptBob = bob
            _promptPlate.setPosition(Vec2.new(-PROMPT_INSET_RIGHT, bob), Vec2.new(1, 1))
        }
    }

    teardown() {
        if (_screenPanel != null) {
            UIManager.removeUIElement(_screenPanel)
            _screenPanel = null
            UIKit.close()
        }
        if (_font != null) {
            _font.unload()
            _font = null
        }
    }

    // Tall enough for the whole line (never shorter than its base height), measured once per line - before the
    // reveal starts, so the box doesn't grow as the text types out.
    fitToText_() {
        var needed = (_dialogueLabel.getContentHeight() + TEXT_INSET_TOP + TEXT_INSET_BOTTOM).ceil
        _textBox.setSize(Vec2.new(BOX_WIDTH, needed.max(_baseHeight)), Vec2.new(0, 0))
    }

    // key: the localization key of the prompt's text, or null to hide the prompt.
    showPrompt_(key) {
        if (key == null) {
            _promptPlate.setVisible(false)
            return
        }
        _promptHint.setLabel(Localization.get(key))
        fitPrompt_()
        _promptPlate.setVisible(true)
    }

    // The plate wraps the hint - re-sized only when the hint's size changes (a device switch, a new label).
    fitPrompt_() {
        var size = _promptHint.getSize()
        var plate = Vec2.new(size.x + 2 * PROMPT_PAD_X, size.y + 2 * PROMPT_PAD_Y)
        if (_promptSize == null || _promptSize.x != plate.x || _promptSize.y != plate.y) {
            _promptSize = plate
            _promptPlate.setSize(plate, Vec2.new(0, 0))
        }
    }
}
