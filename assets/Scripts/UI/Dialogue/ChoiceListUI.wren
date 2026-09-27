// UI/Dialogue/ChoiceListUI.wren
// A vertical list of dialogue-choice rows in the top-right corner, positioned directly above the
// dialogue text box - rebuilt fresh every time a node offers choices (see
// States/InteractState.wren.processDialogueResult). Owns its own focus-highlight bookkeeping;
// onFocus is called only so the State can react to a focus change (playing a sound) without this
// view needing to know about audio.
//
// Layout (all children of the parent panel handed to the constructor):
//   ┌┐  ← the choice container
//   │  1.  First choice text     │
//   │  2.  Second choice text    │
//   │  3.  Third choice text     │
//   └┘
import "ui" for UIManager, UILabel, UIPanel
import "math" for Vec2, Vec4
import "Colors" for WHITE, BLACK

var PANEL_W    = 380
var ROW_H      = 44
var PADDING    = 12
var GAP        = 6
var TEXT_BOX_H = 200   // matches DialogueBoxUI's text box height - the container sits just above it
var MARGIN_R   = 40

var CHOICE_BG_DEFAULT     = Vec4.new(20,  16,  12,  200)
var CHOICE_BG_FOCUSED     = Vec4.new(180, 140, 60,  230)
var CHOICE_BORDER_DEFAULT = Vec4.new(80,  65,  45,  200)
var CHOICE_BORDER_FOCUSED = Vec4.new(220, 180, 80,  255)

class ChoiceListUI {
    // parent: DialogueBoxUI.screenPanel. choiceTexts: already-numbered+localized display strings
    // (e.g. "1.  Yes"). font: DialogueBoxUI.font (shared with the continue prompt).
    construct new(parent, choiceTexts, font, onFocus) {
        _parent = parent
        _choicePanels = []
        _choiceLabels = []
        _focusedIndex = 0

        var totalH = choiceTexts.count * ROW_H + (choiceTexts.count - 1) * GAP + PADDING * 2
        var marginB = TEXT_BOX_H + 20   // 20px gap between the container and the text box

        // Container sits above the text box: anchored bottom-right of the parent, offset up by
        // (text box height + margin) and in from the right edge.
        _containerPanel = UIPanel.new(Vec2.new(-MARGIN_R, -marginB), Vec2.new(1, 1),
                                      Vec2.new(PANEL_W, totalH), Vec2.new(0, 0))
        _containerPanel.setAnchorPoint(Vec2.new(1, 1))
        _containerPanel.setBackgroundColor(Vec4.new(10, 8, 6, 210))
        _containerPanel.setBorderColor(Vec4.new(80, 65, 45, 180))
        parent.addChild(_containerPanel)

        var curY = PADDING
        var i = 0
        for (text in choiceTexts) {
            var panel = UIPanel.new(Vec2.new(PADDING, curY), Vec2.new(0, 0),
                                    Vec2.new(PANEL_W - PADDING * 2, ROW_H), Vec2.new(0, 0))
            panel.setAnchorPoint(Vec2.new(0, 0))
            panel.setBackgroundColor(CHOICE_BG_DEFAULT)
            panel.setBorderColor(CHOICE_BORDER_DEFAULT)
            panel.setFocusable(true)

            var capturedIndex = i
            panel.setOnFocus { |sender|
                setFocused_(capturedIndex)
                onFocus.call(capturedIndex)
            }

            _containerPanel.addChild(panel)
            _choicePanels.add(panel)

            var label = UILabel.new(Vec2.new(12, 0), Vec2.new(0, 0.5), text, 16.0)
            label.setTextColor(WHITE)
            label.setAnchorPoint(Vec2.new(0, 0.5))
            label.setFont(font)
            panel.addChild(label)
            _choiceLabels.add(label)

            curY = curY + ROW_H + GAP
            i = i + 1
        }

        // Give focus to the first row so controller/keyboard navigation starts there.
        if (_choicePanels.count > 0) {
            UIManager.setFocus(_choicePanels[0])
            setFocused_(0)
        }
    }

    // Which row is currently highlighted - read by the Controller when the player confirms.
    focusedIndex { _focusedIndex }

    // Updates the visual state of every row to reflect which is focused.
    setFocused_(index) {
        _focusedIndex = index
        var i = 0
        for (panel in _choicePanels) {
            if (i == index) {
                panel.setBackgroundColor(CHOICE_BG_FOCUSED)
                panel.setBorderColor(CHOICE_BORDER_FOCUSED)
                _choiceLabels[i].setTextColor(BLACK)
            } else {
                panel.setBackgroundColor(CHOICE_BG_DEFAULT)
                panel.setBorderColor(CHOICE_BORDER_DEFAULT)
                _choiceLabels[i].setTextColor(WHITE)
            }
            i = i + 1
        }
    }

    teardown() {
        if (_containerPanel != null) {
            _parent.removeChild(_containerPanel)
            _containerPanel = null
        }
    }
}
